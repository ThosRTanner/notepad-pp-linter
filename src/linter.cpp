#include "stdafx.h"

#include "linter.h"

#include "OutputDialog.h"
#include "Plugin/Callback_Context.h"    // IWYU pragma: keep
#include "Settings.h"
#include "XmlDecodeException.h"
#include "XmlParser.h"
#include "encoding.h"
#include "file.h"
#include "plugin_main.h"

#include "notepad++/Notepad_plus_msgs.h"

#include <process.h>

#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace
{

bool isChanged = true;

HANDLE timer(0);
HANDLE threadHandle(0);

std::vector<XmlParser::Error> errors;
std::map<LRESULT, std::wstring> errorText;
std::unique_ptr<Linter::Settings> settings;

void ClearErrors() noexcept
{
    HideErrors();
    SendEditor(SCI_ANNOTATIONCLEARALL);
}

void InitErrors() noexcept
{
    // FIXME Make this configurable, because it is just strange.
    SendEditor(
        SCI_INDICSETSTYLE, SCE_SQUIGGLE_UNDERLINE_RED, INDIC_BOX
    );    // INDIC_SQUIGGLE);
    SendEditor(
        SCI_INDICSETFORE, SCE_SQUIGGLE_UNDERLINE_RED, 0x0000ff
    );    // Red (Reversed RGB)

    if (settings->alpha() != -1 || settings->color() != -1)
    {
        SendEditor(
            SCI_INDICSETSTYLE, SCE_SQUIGGLE_UNDERLINE_RED, INDIC_ROUNDBOX
        );

        if (settings->alpha() != -1)
        {
            SendEditor(
                SCI_INDICSETALPHA, SCE_SQUIGGLE_UNDERLINE_RED, settings->alpha()
            );
        }

        if (settings->color() != -1)
        {
            SendEditor(
                SCI_INDICSETFORE, SCE_SQUIGGLE_UNDERLINE_RED, settings->color()
            );
        }
    }
}

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
        SendMessage(
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
        SendMessage(
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
            SendMessage(
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
    if (output_dialogue)
    {
        output_dialogue->add_system_error(
            XmlParser::Error{line, col, wstr, L"linter"}
        );
    }
    showTooltip(L"Linter: " + wstr);
}

void apply_linters()
{
    errors.clear();
    if (output_dialogue)
    {
        output_dialogue->clear_lint_info();
    }

    settings->refresh();
    if (settings->empty())
    {
        throw std::runtime_error("Empty linters.xml");
    }

    std::vector<std::pair<std::wstring, bool>> commands;
    bool useStdin = true;
    auto const extension = GetFilePart(NPPM_GETEXTPART);
    for (auto const &linter : *settings)
    {
        if (extension == linter.m_extension)
        {
            commands.emplace_back(linter.m_command, linter.m_useStdin);
            useStdin = useStdin && linter.m_useStdin;
        }
    }

    if (commands.empty())
    {
        return;
    }

    std::string const text = getDocumentText();

    std::wstring const full_path{GetFilePart(NPPM_GETFULLCURRENTPATH)};
    File file{
        GetFilePart(NPPM_GETFILENAME), GetFilePart(NPPM_GETCURRENTDIRECTORY)
    };
    if (! useStdin)
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
            if (output.first == "" && output.second != "")
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

unsigned int __stdcall AsyncCheck(void *)
{
    std::ignore = CoInitialize(nullptr);
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
    return 0;
}

void DrawBoxes()
{
    ClearErrors();
    errorText.clear();
    if (! errors.empty())
    {
        InitErrors();
    }

    for (XmlParser::Error const &error : errors)
    {
        auto position = getPositionForLine(error.m_line - 1);
        position += Encoding::utfOffset(
            getLineText(error.m_line - 1), error.m_column - 1
        );
        errorText[position] = error.m_message;
        ShowError(position);
    }
}

void CALLBACK
RunThread(PVOID /*lpParam*/, BOOLEAN /*TimerOrWaitFired*/) noexcept
{
    if (threadHandle == 0)
    {
        unsigned threadID(0);
#pragma warning(suppress : 26490)
        threadHandle = reinterpret_cast<HANDLE>(
            _beginthreadex(nullptr, 0, &AsyncCheck, nullptr, 0, &threadID)
        );
        isChanged = false;
    }
}

void Check() noexcept
{
    if (isChanged)
    {
        std::ignore = DeleteTimerQueueTimer(timers, timer, nullptr);
        CreateTimerQueueTimer(&timer, timers, RunThread, nullptr, 300, 0, 0);
    }
}

void Changed() noexcept
{
    isChanged = true;
    Check();
}

void initLinters()
{
    // FIXME put into class? NB note this is currently ony done aftrer ready which seems
    // a bit pointless.
    //    settings = std::make_unique<Linter::Settings>(getIniFileName());
}
}    // namespace

DEFINE_PLUGIN_MENU_CALLBACKS(Linter::Linter);

namespace Linter
{

// FIXME should this be a member?
HANDLE timers{nullptr};

Linter::Linter(NppData const &data) :
    Super(data, get_plugin_name()),
    config_file_(get_config_dir() + L"\\linter.xml")
{
    timers = ::CreateTimerQueue();
    //  output_dialogue =
    //  std::make_unique<Linter::OutputDialog>(module_handle,
    //  nppData._nppHandle, Menu_Entry_Show_Results);

    //FIXME This should be a member?
    settings = std::make_unique<::Linter::Settings>(config_file_);
}

Linter::~Linter() = default;

wchar_t const *Linter::get_plugin_name() noexcept
{
    return L"Linter";
}

std::vector<FuncItem> &Linter::on_get_menu_entries()
{
#define MAKE_CALLBACK(entry, text, method, ...) \
    PLUGIN_MENU_MAKE_CALLBACK(Linter, entry, text, method, __VA_ARGS__)

    static ShortcutKey prev_key{
        ._isCtrl = true, ._isShift = true, ._key = VK_F7
    };

    static ShortcutKey next_key{
        ._isCtrl = true, ._isShift = true, ._key = VK_F8
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
    static bool isReady = false;

    if (threadHandle)
    {
        if (::WaitForSingleObject(threadHandle, 0) == WAIT_OBJECT_0)
        {
            CloseHandle(threadHandle);
            DrawBoxes();
            threadHandle = 0;
            Check();
        }
    }

    // Don't do anything until notepad++ tells us it's ready.
    //FIXME Why?
    if (! isReady && notification->nmhdr.code != NPPN_READY)
    {
        return;
    }

    switch (notification->nmhdr.code)
    {
        case NPPN_READY:
            initLinters();
            isReady = true;
            Changed();
            break;

        case NPPN_SHUTDOWN:
            commandMenuCleanUp();
            break;

        case NPPN_BUFFERACTIVATED:
            Changed();
            break;

        case NPPN_FILESAVED:
            if (get_document_path(notification->nmhdr.idFrom) == config_file_)
            {
                Changed();
            }
            break;

        case SCN_MODIFIED:
            //**case NPPM_GLOBAL_MODIFIED:
            if ((notification->modificationType
                 & (SC_MOD_DELETETEXT | SC_MOD_INSERTTEXT))
                != 0)
            {
                Changed();
            }
            break;

        case SCN_UPDATEUI:
            showTooltip();
            break;

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
    if (output_dialogue)
    {
        output_dialogue->display();
    }
    else
    {
        message_box(
            L"Unable to show lint errors due to startup issue",
            MB_OK | MB_ICONERROR
        );
    }
}

void Linter::select_next_lint() noexcept
{
    // FIXME move selection anyway
    if (output_dialogue)
    {
        output_dialogue->select_next_lint();
    }
    else
    {
        message_box(
            L"Unable to show lint errors due to startup issue",
            MB_OK | MB_ICONERROR
        );
    }
}

void Linter::select_previous_lint() noexcept
{
    if (output_dialogue)
    {
        output_dialogue->select_previous_lint();
    }
    else
    {
        message_box(
            L"Unable to show lint errors due to startup issue",
            MB_OK | MB_ICONERROR
        );
    }
}

}    // namespace Linter
