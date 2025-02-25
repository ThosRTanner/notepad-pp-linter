#include "Linter.h"

#include "Checkstyle_Parser.h"
#include "File_Holder.h"
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

#include <combaseapi.h>
#include <comutil.h>
#include <handleapi.h>
#include <objbase.h>
#include <process.h>
#include <synchapi.h>
#include <threadpoollegacyapiset.h>

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
#define MAKE_CALLBACK(entry, text, method, ...) \
    PLUGIN_MENU_MAKE_CALLBACK(Linter, entry, text, method, __VA_ARGS__)

    // Note: These are ctrl-shift in jslint
    // Put in settings
    static ShortcutKey prev_key{
        ._isAlt = true, ._isShift = true, ._key = VK_F7
    };

    static ShortcutKey next_key{
        ._isAlt = true, ._isShift = true, ._key = VK_F8
    };

    static std::vector<FuncItem> res = {
        MAKE_CALLBACK(Menu_Entry_Edit_Config, L"Edit config", edit_config),
        MAKE_CALLBACK(
            Menu_Entry_Show_Results, L"Show linter results", show_results
        ),
        MAKE_CALLBACK(
            Menu_Entry_Show_Previous_Lint,
            L"Show previous lint",
            select_previous_lint,
            false,
            &prev_key
        ),
        MAKE_CALLBACK(
            Menu_Entry_Show_Next_Lint,
            L"Show next lint",
            select_next_lint,
            false,
            &next_key
        )
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

    switch (notification->nmhdr.code)
    {
        case NPPN_READY:
            notepad_is_ready_ = true;
            mark_file_changed();
            break;

        case NPPN_BUFFERACTIVATED:
            mark_file_changed();
            break;

        case NPPN_FILESAVED:
            if (get_document_path(notification->nmhdr.idFrom) == settings_->settings_file())
            {
                mark_file_changed();
            }
            break;

        case SCN_MODIFIED:
            //**case NPPM_GLOBAL_MODIFIED:
            if ((notification->modificationType
                 & (SC_MOD_DELETETEXT | SC_MOD_INSERTTEXT))
                != 0)
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
    if (! errors_.empty())
    {
        setup_error_indicator();
    }

    for (Checkstyle_Parser::Error const &error : errors_)
    {
        auto position = send_to_editor(SCI_POSITIONFROMLINE, error.line_ - 1);
        position += Encoding::utfOffset(
            get_line_text(error.line_ - 1), error.column_ - 1
        );
        errors_by_position_[position] = error.message_;
        highlight_error_at(position);
    }
}

void Linter::highlight_error_at(LRESULT position) noexcept
{
    update_error_indicators(position, position + 1, true);
}

void Linter::clear_error_highlights() noexcept
{
    update_error_indicators(0, send_to_editor(SCI_GETLENGTH), false);
    send_to_editor(SCI_ANNOTATIONCLEARALL);
}

void Linter::update_error_indicators(
    LRESULT start, LRESULT end, bool on
) noexcept
{
    LRESULT const oldid = send_to_editor(SCI_GETINDICATORCURRENT);
    send_to_editor(SCI_SETINDICATORCURRENT, Error_Indicator);
    send_to_editor(
        on ? SCI_INDICATORFILLRANGE : SCI_INDICATORCLEARRANGE,
        start,
        end - start
    );
    send_to_editor(SCI_SETINDICATORCURRENT, oldid);
}

void Linter::setup_error_indicator() noexcept
{
    // FIXME Why do we have to do this every time we want to use the indicator?
    // FIXME Make all this configurable, because it is just strange.
    send_to_editor(SCI_INDICSETSTYLE, Error_Indicator, INDIC_BOX);
    // ^ could use INDIC_SQUIGGLE instead of INDIC_BOX
    send_to_editor(SCI_INDICSETFORE, Error_Indicator, 0x0000ff);
    // ^ Red (Reversed RGB)

    if (settings_->fill_alpha() != -1 || settings_->fg_colour() != -1)
    {
        // Magic happens. This isn't documented
        send_to_editor(SCI_INDICSETSTYLE, Error_Indicator, INDIC_ROUNDBOX);

        if (settings_->fill_alpha() != -1)
        {
            send_to_editor(
                SCI_INDICSETALPHA, Error_Indicator, settings_->fill_alpha()
            );
        }

        if (settings_->fg_colour() != -1)
        {
            send_to_editor(
                SCI_INDICSETFORE, Error_Indicator, settings_->fg_colour()
            );
        }
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
        catch (::Linter::XML_Decode_Error const &e)
        {
            handle_exception(e, e.line(), e.column());
        }
        catch (std::exception const &e)
        {
            handle_exception(e);
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
    if (settings_->empty())
    {
        throw std::runtime_error("Empty linters.xml");
    }

    std::vector<Settings::Linter::Command> commands;
    bool needs_file = false;
    auto const full_path = get_document_path();

    auto const extension = full_path.extension();
    for (auto const &linter : *settings_)
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

    File_Holder file{full_path};
    if (needs_file)
    {
        file.write(text);
    }

    for (auto const &command : commands)
    {
        // std::string xml =
        // File_Holder::exec(L"C:\\Users\\deadem\\AppData\\Roaming\\npm\\jscs.cmd
        // --reporter=checkstyle ", file);
        try
        {
            auto output = file.exec(command, text);
            std::vector<Checkstyle_Parser::Error> parseError;
            if (output.first.empty() && not output.second.empty())
            {
                throw std::runtime_error(output.second);
            }
            parseError = Checkstyle_Parser::get_errors(output.first);
            errors_.insert(errors_.end(), parseError.begin(), parseError.end());
            output_dialogue_->add_lint_errors(parseError);
        }
        catch (std::exception const &e)
        {
            handle_exception(e);
        }
    }
}

void Linter::handle_exception(std::exception const &exc, int line, int col)
{
    std::string const str(exc.what());
    std::wstring const wstr{str.begin(), str.end()};
    output_dialogue_->add_system_error(
        Checkstyle_Parser::Error{line, col, wstr, get_plugin_name()}
    );
    show_tooltip(get_plugin_name() + wstr);
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

        if (! message.empty())
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
