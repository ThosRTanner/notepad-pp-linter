#include "Linter.h"

#include "Checkstyle_Parser.h"
#include "Error_Info.h"
#include "File_Linter.h"
#include "Menu_Entry.h"
#include "Output_Dialogue.h"
#include "Settings.h"
#include "XML_Decode_Error.h"
#include "encoding.h"

#include "Plugin/Callback_Context.h"    // IWYU pragma: keep
// IWYU requires Plugin/Min_Win_Defs.h because it doesn't understand
// inheritance.

#include "notepad++/Notepad_plus_msgs.h"
#include "notepad++/PluginInterface.h"
#include "notepad++/Scintilla.h"
#include "notepad++/menuCmdID.h"

#include <combaseapi.h>
#include <comutil.h>
#include <handleapi.h>
#include <objbase.h>
#include <process.h>
#include <synchapi.h>
#include <threadpoollegacyapiset.h>

#include <codecvt>
#include <exception>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#pragma comment(lib, "msxml6.lib")
#pragma comment(lib, "shlwapi.lib")

static int constexpr Error_Indicator = INDIC_CONTAINER + 2;

DEFINE_PLUGIN_MENU_CALLBACKS(Linter::Linter);

namespace Linter
{

namespace
{
class Save_Selected_Indicator
{
  public:
    explicit Save_Selected_Indicator(Plugin &plugin) noexcept :
        plugin_(plugin),
        old_id_(plugin_.send_to_editor(SCI_GETINDICATORCURRENT))
    {
        plugin_.send_to_editor(SCI_SETINDICATORCURRENT, Error_Indicator);
    }

    Save_Selected_Indicator(Save_Selected_Indicator const &) = delete;

    Save_Selected_Indicator(Save_Selected_Indicator const &&) = delete;

    Save_Selected_Indicator &operator=(Save_Selected_Indicator const &) =
        delete;

    Save_Selected_Indicator &operator=(Save_Selected_Indicator const &&) =
        delete;

    ~Save_Selected_Indicator()
    {
        plugin_.send_to_editor(SCI_SETINDICATORCURRENT, old_id_);
    }

  private:
    Plugin const &plugin_;
    LRESULT old_id_;
};

}    // namespace

Linter::Linter(NppData const &data) :
    Super(data, get_plugin_name()),
    settings_(std::make_unique<Settings>(*this)),
    output_dialogue_(
        std::make_unique<Output_Dialogue>(Menu_Entry_Show_Results, *this)
    ),
    timer_queue_(::CreateTimerQueue())
{
}

Linter::~Linter()
{
    std::ignore = ::DeleteTimerQueueEx(timer_queue_, nullptr);
}

wchar_t const *Linter::get_plugin_name() noexcept
{
    return L"Linter++";
}

std::vector<FuncItem> &Linter::on_get_menu_entries()
{
#define MAKE_CALLBACK(entry, method)       \
    PLUGIN_MENU_MAKE_CALLBACK(             \
        Linter,                            \
        entry,                             \
        get_menu_string(entry),            \
        method,                            \
        false,                             \
        settings_->get_shortcut_key(entry) \
    )

    static std::vector<FuncItem> res = {
        MAKE_CALLBACK(Menu_Entry_Edit_Config, edit_config),
        MAKE_CALLBACK(Menu_Entry_Show_Results, show_results),
        MAKE_CALLBACK(Menu_Entry_Show_Previous_Lint, select_previous_lint),
        MAKE_CALLBACK(Menu_Entry_Show_Next_Lint, select_next_lint)
    };
    return res;
}

void Linter::on_notification(SCNotification const *notification)
{
    if (bg_linter_thread_handle_ != nullptr)
    {
        if (::WaitForSingleObject(bg_linter_thread_handle_, 0) == WAIT_OBJECT_0)
        {
            ::CloseHandle(bg_linter_thread_handle_);
            bg_linter_thread_handle_ = nullptr;
            highlight_errors();
            relint_current_file();
        }
    }

    // Don't do anything until notepad++ tells us it's ready.
    if (notification->nmhdr.code != NPPN_READY && not notepad_is_ready_)
    {
        return;
    }

    constexpr int Modification_Flags = SC_MOD_DELETETEXT | SC_MOD_INSERTTEXT;
    switch (notification->nmhdr.code)
    {
        case NPPN_READY:
            // FIXME need to add this, but the parameter doesn't appear
            // to be available yet.
            /*
            send_to_notepad(
                NPPM_ADDSCNMODIFIEDFLAGS,
                0,
                Modification_Flags
            );
            */
            notepad_is_ready_ = true;
            mark_file_changed();
            break;

        case NPPN_BUFFERACTIVATED:
            mark_file_changed();
            break;

        case NPPN_FILESAVED:
            if (get_document_path(notification->nmhdr.idFrom)
                == settings_->settings_file())
            {
                mark_file_changed();
            }
            break;

        case SCN_MODIFIED:
            // Sadly even with the above call in NPPN_READY, we can't rely on
            // only getting the notifications we asked for.
            if ((notification->modificationType & Modification_Flags) != 0)
            {
                mark_file_changed();
            }
            break;

        case SCN_UPDATEUI:
            show_tooltip();
            break;

        case NPPN_SHUTDOWN:
        case SCN_PAINTED:
        case SCN_FOCUSIN:
        case SCN_FOCUSOUT:
        default:
            break;
    }
}

void Linter::edit_config() noexcept
{
    send_to_notepad(NPPM_DOOPEN, 0, settings_->settings_file().c_str());
}

void Linter::show_results() noexcept
{
    output_dialogue_->display();
}

void Linter::select_next_lint() noexcept
{
    output_dialogue_->select_next_lint();
}

void Linter::select_previous_lint() noexcept
{
    output_dialogue_->select_previous_lint();
}

void Linter::mark_file_changed() noexcept
{
    file_changed_ = true;
    relint_current_file();
}

void Linter::relint_current_file() noexcept
{
    if (file_changed_)
    {
        std::ignore =
            ::DeleteTimerQueueTimer(timer_queue_, relint_timer_, nullptr);
        ::CreateTimerQueueTimer(
            &relint_timer_, timer_queue_, relint_timer_callback, this, 300, 0, 0
        );
    }
}

void Linter::highlight_errors()
{
    clear_error_highlights();
    errors_by_position_.clear();
    if (not errors_.empty())
    {
        setup_error_indicator();
    }

    for (Error_Info const &error : errors_)
    {
        auto position = send_to_editor(SCI_POSITIONFROMLINE, error.line_ - 1);
        position += Encoding::utfOffset(
            get_line_text(error.line_ - 1), error.column_ - 1
        );
        errors_by_position_[position] = error.message_;
        highlight_error_at(
            position, settings_->get_message_colour(error.severity_)
        );
    }
}

void Linter::highlight_error_at(LRESULT position, uint32_t col) noexcept
{
    Save_Selected_Indicator indicator(*this);
    if (settings_->indicator().colour_as_message())
    {
        send_to_editor(SCI_SETINDICATORVALUE, SC_INDICVALUEBIT | col);
    }
    send_to_editor(SCI_INDICATORFILLRANGE, position, 1);
}

void Linter::clear_error_highlights() noexcept
{
    {
        Save_Selected_Indicator indicator(*this);
        send_to_editor(
            SCI_INDICATORCLEARRANGE, 0, send_to_editor(SCI_GETLENGTH)
        );
    }
    send_to_editor(SCI_ANNOTATIONCLEARALL);
}

void Linter::setup_error_indicator() noexcept
{
#pragma warning(suppress : 26447)
    static std::unordered_map<Indicator::Property, int> const cmd_map = {
        {Indicator::Style,           SCI_INDICSETSTYLE       },
        {Indicator::Colour,          SCI_INDICSETFORE        },
        {Indicator::Dynamic_Colour,  SCI_INDICSETFLAGS       },
        {Indicator::Opacity,         SCI_INDICSETALPHA       },
        {Indicator::Outline_Opacity, SCI_INDICSETOUTLINEALPHA},
        {Indicator::Draw_Under,      SCI_INDICSETUNDER       },
        {Indicator::Stroke_Width,    SCI_INDICSETSTROKEWIDTH },
        {Indicator::Hover_Style,     SCI_INDICSETHOVERSTYLE  },
        {Indicator::Hover_Colour,    SCI_INDICSETHOVERFORE   },
    };

    for (auto &[command, value] : settings_->indicator().properties())
    {
#pragma warning(suppress : 26447)
        send_to_editor(cmd_map.at(command), Error_Indicator, value);
    }
}

#pragma warning(suppress : 26429)
void Linter::relint_timer_callback(
    void *self, BOOLEAN /*TimerOrWaitFired*/
) noexcept
{
    static_cast<Linter *>(self)->start_async_timer();
}

void Linter::start_async_timer() noexcept
{
    if (bg_linter_thread_handle_ != nullptr)
    {
        // The thread is doing something...
        return;
    }
    unsigned thread_id{0};
#pragma warning(suppress : 26490)
    bg_linter_thread_handle_ = reinterpret_cast<HANDLE>(
        _beginthreadex(nullptr, 0, &run_linter_thread, this, 0, &thread_id)
    );
    file_changed_ = false;
}

#pragma warning(suppress : 26429)
unsigned int Linter::run_linter_thread(void *self) noexcept
{
    struct Thread_Wrapper
    {
        Thread_Wrapper() noexcept
        {
            std::ignore = ::CoInitialize(nullptr);
        }

        Thread_Wrapper(Thread_Wrapper const &) = delete;
        Thread_Wrapper(Thread_Wrapper &&) = delete;
        Thread_Wrapper &operator=(Thread_Wrapper const &) = delete;
        Thread_Wrapper &operator=(Thread_Wrapper &&) = delete;

        ~Thread_Wrapper()
        {
            ::CoUninitialize();
        }

    } wrapper;
    return static_cast<Linter *>(self)->run_linter();
}

unsigned int Linter::run_linter() noexcept
{
    try
    {
        try
        {
            apply_linters();
        }
        catch (XML_Decode_Error const &e)
        {
            std::string exc{e.what()};
            std::wstring wstr{exc.begin(), exc.end()};
            output_dialogue_->add_system_error(
                {.message_ = wstr,
                 .mode_ = Error_Info::Bad_Linter_XML,
                 .line_ = e.line(),
                 .column_ = e.column()}
            );
            show_tooltip(wstr);
        }
        catch (std::exception const &e)
        {
            std::string const exc(e.what());
            std::wstring wstr{exc.begin(), exc.end()};
            output_dialogue_->add_system_error(
                {.message_ = wstr, .mode_ = Error_Info::Exception}
            );
            show_tooltip(wstr);
        }
    }
    catch (std::exception const &e)
    {
        try
        {
            message_box(
                static_cast<wchar_t *>(static_cast<_bstr_t>(e.what())),
                MB_OK | MB_ICONERROR
            );
        }
        catch (std::exception const &)
        {
            message_box(
                L"Caught exception but cannot get reason", MB_OK | MB_ICONERROR
            );
        }
    }
    return 0;
}

void Linter::apply_linters()
{
    errors_.clear();
    output_dialogue_->clear_lint_info();

    settings_->refresh();

    std::vector<Settings::Linter::Command> commands;
    bool needs_file = false;
    auto const full_path = get_document_path();

    auto const extension = full_path.extension();
    for (auto const &linter : settings_->linters())
    {
        if (linter.extension == extension)
        {
            commands.emplace_back(linter.command);
            if (not linter.command.use_stdin)
            {
                needs_file = true;
            }
        }
    }

    if (commands.empty())
    {
        return;
    }

    auto const text = get_document_text();

    File_Linter file{
        full_path,
        this->get_module_path().parent_path(),
        this->get_plugin_config_dir(),
        text
    };

    for (auto const &command : commands)
    {
        try
        {
            // Try and work out what to do here:
            auto const [cmdline, result, output, errout] =
                file.run_linter(command);
            if (output.empty() && not errout.empty())
            {
                // Program terminated with error.
                output_dialogue_->add_system_error(
                    {.message_ = std::wstring(errout.begin(), errout.end()),
                     .tool_ = command.program.stem(),
                     .command_ = cmdline,
                     .stdout_ = output,
                     .stderr_ = errout,
                     .mode_ = Error_Info::Stderr_Found,
                     .result_ = result}
                );
                continue;
            }
            try
            {
                std::vector<Error_Info> const detected_errors{
                    Checkstyle_Parser::get_errors(output)
                };
                errors_.insert(
                    errors_.end(),
                    detected_errors.begin(),
                    detected_errors.end()
                );
                output_dialogue_->add_lint_errors(detected_errors);
                if (not errout.empty())
                {
                    output_dialogue_->add_system_error(
                        {.message_ = std::wstring(errout.begin(), errout.end()),
                         .severity_ = L"warning",
                         .tool_ = command.program.stem(),
                         .command_ = cmdline,
                         .stdout_ = output,
                         .stderr_ = errout,
                         .mode_ = Error_Info::Stderr_Found}
                    );
                }
            }
            catch (XML_Decode_Error const &e)
            {
                std::string exc{e.what()};
                output_dialogue_->add_system_error(
                    {.message_ = std::wstring(exc.begin(), exc.end()),
                     .tool_ = command.program.stem(),
                     .command_ = cmdline,
                     .stdout_ = output,
                     .stderr_ = errout,
                     .mode_ = Error_Info::Bad_Output,
                     .line_ = e.line(),
                     .column_ = e.column()}
                );
            }
        }
        catch (std::exception const &e)
        {
            // Really bad things happened. we don't have anything much here we
            // can log
            std::string exc{e.what()};
            output_dialogue_->add_system_error(
                {.message_ = std::wstring(exc.begin(), exc.end()),
                 .tool_ = command.program.stem(),
                 .mode_ = Error_Info::Exception}
            );
        }
    }
}

void Linter::show_tooltip()
{
    show_tooltip(L"");
}

void Linter::show_tooltip(std::wstring message)
{
    const LRESULT position = send_to_editor(SCI_GETCURRENTPOS);

    auto npp_statusbar = FindWindowEx(
        get_notepad_window(), nullptr, L"msctls_statusbar32", nullptr
    );

    auto const error = errors_by_position_.find(position);
    if (error != errors_by_position_.end())
    {
#pragma warning(suppress : 26490)
        ::SendMessage(
            npp_statusbar,
            WM_SETTEXT,
            0,
            reinterpret_cast<LPARAM>(
                (std::wstring(L" - ") + error->second).c_str()
            )
        );
    }
    else
    {
        wchar_t const title[256] = {0};
#pragma warning(suppress : 26490)
        ::SendMessage(
            npp_statusbar,
            WM_GETTEXT,
            sizeof(title) / sizeof(title[0]) - 1,
            reinterpret_cast<LPARAM>(title)
        );

        std::wstring str(&title[0]);
        if (message.empty() && str.find(L" - ") == 0)
        {
            message = L" - ";
        }

        if (not message.empty())
        {
#pragma warning(suppress : 26490)
            ::SendMessage(
                npp_statusbar,
                WM_SETTEXT,
                0,
                reinterpret_cast<LPARAM>(message.c_str())
            );
        }
    }
}

}    // namespace Linter
