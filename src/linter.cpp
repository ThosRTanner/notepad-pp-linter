#include "Linter.h"

#include "Output_Dialogue.h"
#include "Settings.h"
#include "XmlDecodeException.h"
#include "XmlParser.h"
#include "encoding.h"
#include "file.h"

#include "Plugin/Callback_Context.h"    // IWYU pragma: keep

#include "notepad++/Notepad_plus_msgs.h"
#include "notepad++/PluginInterface.h"
#include "notepad++/Scintilla.h"

#include <Shlwapi.h>
#include <combaseapi.h>
#include <comutil.h>
#include <handleapi.h>
#include <process.h>
#include <threadpoollegacyapiset.h>

#include <exception>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#pragma comment(lib, "msxml6.lib")
#pragma comment(lib, "shlwapi.lib")

namespace
{

Linter::Output_Dialogue *output_dialogue;

NppData nppData;

LRESULT SendApp(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0) noexcept
{
    return SendMessage(nppData._nppHandle, Msg, wParam, lParam);
}

LRESULT SendApp(UINT Msg, WPARAM wParam, void const *lParam) noexcept
{
#pragma warning(suppress : 26490)
    return SendApp(Msg, wParam, reinterpret_cast<LPARAM>(lParam));
}

HWND getScintillaWindow() noexcept
{
    LRESULT const view = SendApp(NPPM_GETCURRENTVIEW);
    return view == 0 ? nppData._scintillaMainHandle
                     : nppData._scintillaSecondHandle;
}

LRESULT SendEditor(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0) noexcept
{
    return SendMessage(getScintillaWindow(), Msg, wParam, lParam);
}

LRESULT SendEditor(UINT Msg, WPARAM wParam, void const *lParam) noexcept
{
#pragma warning(suppress : 26490)
    return SendEditor(Msg, wParam, reinterpret_cast<LPARAM>(lParam));
}

std::string getDocumentText()
{
    LRESULT const lengthDoc = SendEditor(SCI_GETLENGTH);
    auto buff{std::make_unique_for_overwrite<char[]>(lengthDoc + 1)};
    SendEditor(SCI_GETTEXT, lengthDoc, buff.get());
    return std::string(buff.get(), lengthDoc);
}

std::string getLineText(int line)
{
    LRESULT const length = SendEditor(SCI_LINELENGTH, line);
    auto buff{std::make_unique_for_overwrite<char[]>(length + 1)};
    SendEditor(SCI_GETLINE, line, buff.get());
    return std::string(buff.get(), length);
}

LRESULT getPositionForLine(int line) noexcept
{
    return SendEditor(SCI_POSITIONFROMLINE, line);
}

std::vector<XmlParser::Error> errors;
std::map<LRESULT, std::wstring> errorText;
Linter::Settings *settings;

static int constexpr Error_Indicator = INDIC_CONTAINER + 2;

std::wstring GetFilePart(unsigned int part)
{
    auto buff{std::make_unique_for_overwrite<wchar_t[]>(MAX_PATH + 1)};
    SendApp(part, MAX_PATH, buff.get());
    return buff.get();
}

void showTooltip(std::wstring message = std::wstring())
{
    const LRESULT position = SendEditor(SCI_GETCURRENTPOS);

    HWND main = GetParent(getScintillaWindow());
    HWND childHandle =
        FindWindowEx(main, nullptr, L"msctls_statusbar32", nullptr);

    auto const error = errorText.find(position);
    if (error != errorText.end())
    {
#pragma warning(suppress : 26490)
        ::SendMessage(
            childHandle,
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
            childHandle,
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
                childHandle,
                WM_SETTEXT,
                0,
                reinterpret_cast<LPARAM>(message.c_str())
            );
        }
    }
}

void handle_exception(std::exception const &exc, int line = 0, int col = 0)
{
    std::string const str(exc.what());
    std::wstring const wstr{str.begin(), str.end()};
    output_dialogue->add_system_error(
        XmlParser::Error{line, col, wstr, L"linter"}
    );
    showTooltip(L"Linter: " + wstr);
}

void apply_linters()
{
    errors.clear();
    output_dialogue->clear_lint_info();

    settings->refresh();
    if (settings->empty())
    {
        throw std::runtime_error("Empty linters.xml");
    }

    std::vector<std::pair<std::wstring, bool>> commands;
    bool needs_file = false;
    auto const extension = GetFilePart(NPPM_GETEXTPART);
    for (auto const &linter : *settings)
    {
        if (extension == linter.extension_)
        {
            commands.emplace_back(linter.command_, linter.use_stdin_);
            needs_file |= ! linter.use_stdin_;
        }
    }

    if (commands.empty())
    {
        return;
    }

    std::string const text = getDocumentText();

    std::wstring const full_path{GetFilePart(NPPM_GETFULLCURRENTPATH)};
    ::Linter::File file{
        GetFilePart(NPPM_GETFILENAME), GetFilePart(NPPM_GETCURRENTDIRECTORY)
    };
    if (needs_file)
    {
        file.write(text);
    }

    for (auto const &command : commands)
    {
        // std::string xml =
        // File::exec(L"C:\\Users\\deadem\\AppData\\Roaming\\npm\\jscs.cmd
        // --reporter=checkstyle ", file);
        try
        {
            auto output =
                file.exec(command.first, command.second ? &text : nullptr);
            std::vector<XmlParser::Error> parseError;
            if (output.first.empty() && not output.second.empty())
            {
                throw std::runtime_error(output.second);
            }
            parseError = XmlParser::getErrors(output.first);
            errors.insert(errors.end(), parseError.begin(), parseError.end());
            if (output_dialogue)
            {
                output_dialogue->add_lint_errors(parseError);
            }
        }
        catch (std::exception const &e)
        {
            handle_exception(e);
        }
    }
}

}    // namespace

DEFINE_PLUGIN_MENU_CALLBACKS(Linter::Linter);

namespace Linter
{

Linter::Linter(NppData const &data) :
    Super(data, get_plugin_name()),
    config_file_(get_config_dir() + L"\\linter.xml"),
    settings_(std::make_unique<Settings>(config_file_)),
    output_dialogue_(
        std::make_unique<Output_Dialogue>(Menu_Entry_Show_Results, *this)
    ),
    timer_queue_(::CreateTimerQueue())
{
    // FIXME Crock
    settings = settings_.get();

    // FIXME crock
    nppData = data;

    // FIXME Crock
    output_dialogue = output_dialogue_.get();
}

Linter::~Linter()
{
    std::ignore = ::DeleteTimerQueueEx(timer_queue_, nullptr);
}

wchar_t const *Linter::get_plugin_name() noexcept
{
    return L"Linter";
}

std::vector<FuncItem> &Linter::on_get_menu_entries()
{
#define MAKE_CALLBACK(entry, text, method, ...) \
    PLUGIN_MENU_MAKE_CALLBACK(Linter, entry, text, method, __VA_ARGS__)

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
            if (get_document_path(notification->nmhdr.idFrom) == config_file_)
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
            showTooltip();
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
    send_to_notepad(NPPM_DOOPEN, 0, config_file_.c_str());
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
    errorText.clear();
    if (! errors.empty())
    {
        setup_error_indicator();
    }

    for (XmlParser::Error const &error : errors)
    {
        auto position = getPositionForLine(error.line_ - 1);
        position += Encoding::utfOffset(
            getLineText(error.line_ - 1), error.column_ - 1
        );
        errorText[position] = error.message_;
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
    LRESULT const oldid = SendEditor(SCI_GETINDICATORCURRENT);
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

    if (settings->alpha() != -1 || settings->color() != -1)
    {
        // Magic happens. This isn't documented
        send_to_editor(SCI_INDICSETSTYLE, Error_Indicator, INDIC_ROUNDBOX);

        if (settings->alpha() != -1)
        {
            send_to_editor(
                SCI_INDICSETALPHA, Error_Indicator, settings->alpha()
            );
        }

        if (settings->color() != -1)
        {
            send_to_editor(
                SCI_INDICSETFORE, Error_Indicator, settings->color()
            );
        }
    }
}

#pragma warning(suppress : 26429)
void Linter::
    relint_timer_callback(void *self, BOOLEAN /*TimerOrWaitFired*/) noexcept
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
        _beginthreadex(nullptr, 0, &async_lint_thread, this, 0, &thread_id)
    );
    file_changed_ = false;
}

#pragma warning(suppress : 26429)
unsigned int Linter::async_lint_thread(void *self) noexcept
{
    return static_cast<Linter *>(self)->run_linter();
}

unsigned int Linter::run_linter() noexcept
{
    std::ignore = ::CoInitialize(nullptr);
    try
    {
        try
        {
            apply_linters();
        }
        catch (::Linter::XmlDecodeException const &e)
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
        return FALSE;
    }
    ::CoUninitialize();
    return 0;
}

}    // namespace Linter
