#include "stdafx.h"
#include "linter.h"

#include "encoding.h"
#include "file.h"
#include "plugin.h"
#include "OutputDialog.h"
#include "Settings.h"
#include "SystemError.h"
#include "XmlDecodeException.h"
#include "XmlParser.h"

#include <CommCtrl.h>

#include <map>
#include <memory>
#include <vector>

namespace
{

    bool isChanged = true;

    HANDLE timer(0);
    HANDLE threadHandle(0);

    std::vector<XmlParser::Error> errors;
    std::map<LRESULT, std::wstring> errorText;
    std::unique_ptr<Linter::Settings> settings;

    void ClearErrors()
    {
        HideErrors();
        SendEditor(SCI_ANNOTATIONCLEARALL);
    }

    void InitErrors()
    {
        //FIXME Make this configurable, because it is just strange.
        SendEditor(SCI_INDICSETSTYLE, SCE_SQUIGGLE_UNDERLINE_RED, INDIC_BOX);    // INDIC_SQUIGGLE);
        SendEditor(SCI_INDICSETFORE, SCE_SQUIGGLE_UNDERLINE_RED, 0x0000ff);      //Red (Reversed RGB)

        if (settings->alpha() != -1 || settings->color() != -1)
        {
            SendEditor(SCI_INDICSETSTYLE, SCE_SQUIGGLE_UNDERLINE_RED, INDIC_ROUNDBOX);

            if (settings->alpha() != -1)
            {
                SendEditor(SCI_INDICSETALPHA, SCE_SQUIGGLE_UNDERLINE_RED, settings->alpha());
            }

            if (settings->color() != -1)
            {
                SendEditor(SCI_INDICSETFORE, SCE_SQUIGGLE_UNDERLINE_RED, settings->color());
            }
        }
    }

    std::wstring GetFilePart(unsigned int part)
    {
#if __cplusplus >= 202002L
        auto buff{std::make_unique_for_overwrite<wchar_t[]>(MAX_PATH + 1)};
#else
        std::unique_ptr<wchar_t[]> buff{new wchar_t[MAX_PATH + 1]};
#endif
        SendApp(part, MAX_PATH, reinterpret_cast<LPARAM>(buff.get()));
        return buff.get();
    }

    void showTooltip(std::wstring message = std::wstring())
    {
        const LRESULT position = SendEditor(SCI_GETCURRENTPOS);

        HWND main = GetParent(getScintillaWindow());
        HWND childHandle = FindWindowEx(main, nullptr, L"msctls_statusbar32", nullptr);

        auto error = errorText.find(position);
        if (error != errorText.end())
        {
            SendMessage(childHandle, WM_SETTEXT, 0, reinterpret_cast<LPARAM>((std::wstring(L" - ") + error->second).c_str()));
        }
        else
        {
            wchar_t title[256] = {0};
            SendMessage(childHandle, WM_GETTEXT, sizeof(title) / sizeof(title[0]) - 1, reinterpret_cast<LPARAM>(title));

            std::wstring str(title);
            if (message.empty() && str.find(L" - ") == 0)
            {
                message = L" - ";
            }

            if (!message.empty())
            {
                SendMessage(childHandle, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(message.c_str()));
            }
        }
    }

    void handle_exception(std::exception const &exc, int line = 0, int col = 0)
    {
        std::string const str(exc.what());
        std::wstring const wstr{str.begin(), str.end()};
        output_dialogue->add_system_error(XmlParser::Error{line, col, wstr, L"linter"});
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

        std::string const &text = getDocumentText();

        std::wstring const full_path{GetFilePart(NPPM_GETFULLCURRENTPATH)};
        File file{GetFilePart(NPPM_GETFILENAME), GetFilePart(NPPM_GETCURRENTDIRECTORY)};
        if (!useStdin)
        {
            file.write(text);
        }

        for (const auto &command : commands)
        {
            //std::string xml = File::exec(L"C:\\Users\\deadem\\AppData\\Roaming\\npm\\jscs.cmd --reporter=checkstyle ", file);
            try
            {
                nonstd::optional<std::string> str;
                if (command.second)
                {
                    str = text;
                }
                std::string xml = file.exec(command.first, str);
                std::vector<XmlParser::Error> parseError = XmlParser::getErrors(xml);
                errors.insert(errors.end(), parseError.begin(), parseError.end());
                output_dialogue->add_lint_errors(parseError);
            }
            catch (const std::exception &e)
            {
                handle_exception(e);
            }
        }
    }

    unsigned int __stdcall AsyncCheck(void *)
    {
        (void)CoInitialize(nullptr);
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

        for (const XmlParser::Error &error : errors)
        {
            auto position = getPositionForLine(error.m_line - 1);
            position += Encoding::utfOffset(getLineText(error.m_line - 1), error.m_column - 1);
            errorText[position] = error.m_message;
            ShowError(position);
        }
    }

    void CALLBACK RunThread(PVOID /*lpParam*/, BOOLEAN /*TimerOrWaitFired*/)
    {
        if (threadHandle == 0)
        {
            unsigned threadID(0);
            threadHandle = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, &AsyncCheck, nullptr, 0, &threadID));
            isChanged = false;
        }
    }

    void Check()
    {
        if (isChanged)
        {
            (void)DeleteTimerQueueTimer(timers, timer, nullptr);
            CreateTimerQueueTimer(&timer, timers, static_cast<WAITORTIMERCALLBACK>(RunThread), nullptr, 300, 0, 0);
        }
    }

    void Changed()
    {
        isChanged = true;
        Check();
    }

    void initLinters()
    {
        settings.reset(new Linter::Settings(getIniFileName()));
    }
}    // namespace

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
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

    //Don't do anything until notepad++ tells us it's ready.
    if (!isReady && notifyCode->nmhdr.code != NPPN_READY)
    {
        return;
    }

    switch (notifyCode->nmhdr.code)
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
            //get which file changed
            {
                auto buffer = notifyCode->nmhdr.idFrom;
                std::size_t len = SendApp(NPPM_GETFULLPATHFROMBUFFERID, buffer, reinterpret_cast<LPARAM>(nullptr));
                if (len != -1)
                {
#if __cplusplus >= 202002L
                    auto buff{std::make_unique_for_overwrite<wchar_t[]>(len + 1)};
#else
                    std::unique_ptr<wchar_t[]> buff{new wchar_t[len + 1]};
#endif
                    SendApp(NPPM_GETFULLPATHFROMBUFFERID, buffer, reinterpret_cast<LPARAM>(buff.get()));
                    if (std::wstring(buff.get(), len) == getIniFileName())
                    {
                        Changed();
                    }
                }
            }
            break;

        case SCN_MODIFIED:
            if ((notifyCode->modificationType & (SC_MOD_DELETETEXT | SC_MOD_INSERTTEXT)) != 0)
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
            break;
    }
}
