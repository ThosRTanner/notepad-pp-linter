#include "Output_Dialogue.h"

//#include "Checkstyle_Parser.h"
#include "Clipboard.h"
#include "Encoding.h"
#include "Error_Info.h"
#include "Linter.h"
//#include "Menu_Entry.h"
#include "Report_View.h"
#include "Settings.h"
//#include "System_Error.h"

#include "Plugin/Casts.h"
#include "Plugin/Plugin.h"

#include "notepad++/menuCmdID.h"
#include "notepad++/Notepad_plus_msgs.h"
#include "notepad++/Scintilla.h"

#include "resource.h"

#include <CommCtrl.h>
#include <intsafe.h>
#include <winuser.h>    // For tagNMHDR, AppendMenu
//#include <cstddef>
#include <cstdio>    // For snprintf
#include <filesystem>
#if __cplusplus >= 202302L
#include <generator>
#endif
#include <optional>     // For optional, nullopt
#include <sstream>
#include <string>
//#include <type_traits>
#include <utility>
#include <vector>

namespace Linter
{

namespace
{

/** Columns in the error list */
enum List_Column
{
    Column_Line,
    Column_Position,
    Column_Tool,
    Column_Message
};

/** Context menu items
 * Ensure they don't clash with anything in the resource file.
 */
enum Context_Menu_Entry
{
    Context_Copy_Lints = 1500,
    Context_Show_Source_Line,
    Context_Select_All
};

}    // namespace

Output_Dialogue::Output_Dialogue(Menu_Entry menu_entry, Linter const &plugin) :
    Super(IDD_OUTPUT, plugin),
    tab_bar_(GetDlgItem(IDC_TABBAR)),
    tab_definitions_{
        {
         {L"Lint Errors", IDC_LIST_LINTS, Lint_Error, *this},
         {L"System Errors", IDC_LIST_OUTPUT, System_Error, *this},
         }
},
    current_tab_(&tab_definitions_.at(0)),
    settings_(plugin.settings()),
    sort_callback_(
#ifndef __cpp_lib_copyable_function
#pragma warning(suppress : 26447)
#endif
        [this](
            LPARAM row1_index, LPARAM row2_index,
            Report_View::Data_Column column
        ) noexcept
        { return this->sort_call_function(row1_index, row2_index, column); }
    )

{
    initialise_dialogue();
    selected_tab_changed();

    // Possibly one should free the icon up, but I don't see the dialogue memory
    // being freed up anywhere.
    register_dialogue(
        static_cast<int>(menu_entry),
        Position::Dock_Bottom,
        static_cast<HICON>(::LoadImage(
            plugin.module(),
            MAKEINTRESOURCE(IDI_ICON1),
            IMAGE_ICON,
            0,
            0,
            LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT
        ))
    );
}

Output_Dialogue::~Output_Dialogue() = default;

void Output_Dialogue::display() noexcept
{
    Super::display();
    ::SetFocus(tab_bar_);
}

void Output_Dialogue::clear_lint_info()
{
    for (auto &tab : tab_definitions_)
    {
        tab.report_view.clear();
        tab.errors.clear();
    }

    update_displayed_counts();
}

void Output_Dialogue::add_system_error(Error_Info const &err)
{
    std::vector<Error_Info> const errs = {err};
    add_errors(Tab::System_Error, errs);
}

void Output_Dialogue::add_lint_errors(std::vector<Error_Info> const &errs)
{
    add_errors(Tab::Lint_Error, errs);
}

void Output_Dialogue::select_next_lint()
{
    select_lint(1);
}

void Output_Dialogue::select_previous_lint()
{
    select_lint(-1);
}

Output_Dialogue::Message_Return Output_Dialogue::on_dialogue_message(
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    UINT message, WPARAM wParam, LPARAM lParam
)
{
    switch (message)
    {
        case WM_COMMAND:
            return process_dlg_command(wParam);

        case WM_CONTEXTMENU:
            return process_dlg_context_menu(lParam);

        case WM_NOTIFY:
            return process_dlg_notify(lParam);

        case WM_WINDOWPOSCHANGED:
            // This must return as if it was unhandled
            window_pos_changed();
            return std::nullopt;

        default:
            break;
    }

    return std::nullopt;
}

void Output_Dialogue::initialise_dialogue() noexcept
{
    TCITEM tie{.mask = TCIF_TEXT | TCIF_IMAGE, .iImage = -1};

    for (auto const &tab : tab_definitions_)
    {
        tie.pszText = windows_const_cast<wchar_t *>(tab.tab_name);
        TabCtrl_InsertItem(tab_bar_, tab.tab, &tie);
    }
}

Output_Dialogue::Message_Return Output_Dialogue::process_dlg_command(
    WPARAM wParam
)
{
    switch (LOWORD(wParam))
    {
        case Context_Copy_Lints:
            copy_to_clipboard();
            return TRUE;

        case Context_Show_Source_Line:
        {
            auto const item = current_report_view_->get_first_selected_index();
            if (item != -1)
            {
                show_selected_lint(item);
            }
            return TRUE;
        }

        case Context_Select_All:
            current_report_view_->select_all();
            return TRUE;

        default:
            break;
    }
    return std::nullopt;
}

Output_Dialogue::Message_Return Output_Dialogue::process_dlg_context_menu(
    LPARAM lParam
) noexcept
{
    // Right click in docked window.
    // build context menu
    HMENU menu = ::CreatePopupMenu();

    int const numSelected = current_report_view_->num_selected_items();

    if (numSelected >= 1)
    {
        if (current_report_view_->has_selected_line())
        {
            AppendMenu(menu, MF_ENABLED, Context_Show_Source_Line, L"Show");
        }
    }

    if (GetMenuItemCount(menu) > 0)
    {
        AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
    }

    if (numSelected > 0)
    {
        AppendMenu(menu, MF_ENABLED, Context_Copy_Lints, L"Copy");
    }

    AppendMenu(menu, MF_ENABLED, Context_Select_All, L"Select All");

    // determine context menu position
    POINT point{.x = LOWORD(lParam), .y = HIWORD(lParam)};
    if (point.x == 65535 || point.y == 65535)
    {
        point.x = 0;
        point.y = 0;
        current_report_view_->get_screen_coordinates(&point);
    }

    // show context menu
    TrackPopupMenu(menu, 0, point.x, point.y, 0, window(), nullptr);
    return TRUE;
}

Output_Dialogue::Message_Return Output_Dialogue::process_dlg_notify(
    LPARAM lParam
)
{
    auto const *const notify_header =
        windows_cast_to<NMHDR const *, LPARAM>(lParam);
    switch (notify_header->code)
    {
        case LVN_KEYDOWN:
            if (notify_header->idFrom == current_tab_->list_view_id)
            {
                NMLVKEYDOWN const *pnkd =
                    windows_cast_to<LPNMLVKEYDOWN, LPARAM>(lParam);

                if (pnkd->wVKey == 'A'
                    && (::GetKeyState(VK_CONTROL) & 0x8000U) != 0)
                {
                    current_report_view_->select_all();
                    return TRUE;
                }

                if (pnkd->wVKey == 'C'
                    && (::GetKeyState(VK_CONTROL) & 0x8000U) != 0)
                {
                    copy_to_clipboard();
                    return TRUE;
                }
            }
            break;

        case LVN_COLUMNCLICK:
            if (notify_header->idFrom == current_tab_->list_view_id)
            {
                auto const *const column_click =
                    windows_cast_to<NMLISTVIEW const *, LPARAM>(lParam);
                // ENHANCMENT Should check if we've clicked shift key here and
                // add this column to the sort order rather than replacing it.
                // Using the shift key multiple times should cycle between
                // ascending sort, descending sort, and removal from the sort.
                //
                // Arguably multi-column sorting is only applicable to report
                // views, at least at this sort of level.
                current_report_view_->sort_by_column(
                    column_click->iSubItem, sort_callback_
                );
                return TRUE;
            }
            break;

        case NM_DBLCLK:
            if (notify_header->idFrom == current_tab_->list_view_id)
            {
                auto const *const item_activate =
                    windows_cast_to<NMITEMACTIVATE const *, LPARAM>(lParam);
                int const selected_item = item_activate->iItem;
                if (selected_item != -1)
                {
                    show_selected_lint(
                        current_report_view_->get_index(selected_item)
                    );
                }
                return TRUE;
            }
            break;

        case NM_CUSTOMDRAW:
            if (notify_header->idFrom == current_tab_->list_view_id)
            {
                return process_custom_draw(
                    windows_cast_to<NMLVCUSTOMDRAW *, LPARAM>(lParam)
                );
            }
            break;

        case TCN_SELCHANGE:
            if (notify_header->idFrom == IDC_TABBAR)
            {
                selected_tab_changed();
                return TRUE;
            }
            break;

        default:
            break;
    }
    return std::nullopt;
}

Output_Dialogue::Message_Return Output_Dialogue::process_custom_draw(
    NMLVCUSTOMDRAW *custom_draw
) noexcept
{
    switch (custom_draw->nmcd.dwDrawStage)
    {
        case CDDS_PREPAINT:                // NOLINT(bugprone-branch-clone)
            return CDRF_NOTIFYITEMDRAW;    // This and the value below are the
                                           // same!

        case CDDS_ITEMPREPAINT:
            return CDRF_NOTIFYSUBITEMDRAW;

        case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
            if (custom_draw->iSubItem == Column_Message)
            {
                Report_View::Data_Row const row = current_report_view_->get_index(
                    windows_static_cast<int, DWORD_PTR>(
                        custom_draw->nmcd.dwItemSpec
                    )
                );
                if (static_cast<std::size_t>(row)
                    >= current_tab_->errors.size())
                {
                    // For reasons I don't entirely understand, windows paints
                    // an entry for a line that doesn't exist. So don't do
                    // anything for that.
                    break;
                }

                // Now we colour the text according to the severity level.
                auto const &lint_error = current_tab_->errors[row];
                custom_draw->clrText =
                    settings_->get_message_colour(lint_error.severity_);
                // Tell Windows to paint the control itself.
                return CDRF_DODEFAULT;
            }
            break;

        default:
            break;
    }
    return std::nullopt;
}

void Output_Dialogue::selected_tab_changed() noexcept
{
    current_tab_ = &tab_definitions_[TabCtrl_GetCurSel(tab_bar_)];
    current_report_view_ = &current_tab_->report_view;
    for (auto const &tab : tab_definitions_)
    {
        if (&tab == current_tab_)
        {
            tab.report_view.show();
        }
        else
        {
            tab.report_view.hide();
        }
    }
}

void Output_Dialogue::window_pos_changed() noexcept
{
    RECT const rect = getClientRect();

    ::MoveWindow(
        tab_bar_,
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top,
        TRUE
    );

    TabCtrl_AdjustRect(tab_bar_, FALSE, &rect);
    for (auto const &tab : tab_definitions_)
    {
        tab.report_view.set_window_position(tab_bar_, rect);
    }
}

void Output_Dialogue::update_displayed_counts()
{
    for (auto const &tab : tab_definitions_)
    {
        std::wstring strTabName;
        int const rows = tab.report_view.get_num_rows();
        if (rows > 0)
        {
            std::wstringstream stream;
            stream << tab.tab_name << L" (" << rows << L")";
            strTabName = stream.str();
        }
        else
        {
            strTabName = tab.tab_name;
        }

        TCITEM const tie{
            .mask = TCIF_TEXT,
            .pszText = windows_const_cast<wchar_t *>(strTabName.c_str())
        };
        TabCtrl_SetItem(tab_bar_, tab.tab, &tie);
    }
}

void Output_Dialogue::add_errors(Tab tab, std::vector<Error_Info> const &lints)
{
    auto &tab_def = tab_definitions_[tab];
    auto const &report_view = tab_def.report_view;
    auto row = windows_static_cast<int, size_t>(tab_def.errors.size());

    report_view.ensure_rows(
        row + windows_static_cast<int, size_t>(lints.size())
    );

    for (auto const &lint : lints)
    {
        tab_def.errors.push_back(lint);

        report_view.add_row(row, Report_View::Row_Data{.user_data = row});

        report_view.set_item_text(row, Column_Message, lint.message_);

        report_view.set_item_text(row, Column_Tool, lint.tool_);

        report_view.set_item_text(
            row, Column_Line, std::to_wstring(lint.line_)
        );

        report_view.set_item_text(
            row, Column_Position, std::to_wstring(lint.column_)
        );

        row += 1;
    }

    update_displayed_counts();

    if (&tab_def == current_tab_)
    {
        report_view.autosize_columns();
        report_view.sort_by_column(sort_callback_);
        InvalidateRect();
    }
}

void Output_Dialogue::select_lint(int n)
{
    int const rows = current_report_view_->get_num_rows();
    if (rows == 0)
    {
        // no lints, set focus to editor
        ::SetFocus(plugin()->get_scintilla_window());
        return;
    }

    auto const row = current_report_view_->move_selection(n);

    // And select the error location in the editor window
    show_selected_lint(row);
}

void Output_Dialogue::append_text(std::string_view text) const noexcept
{
    plugin()->send_to_editor(SCI_APPENDTEXT, text.length(), text.data());
}

void Output_Dialogue::append_text_with_style(
    std::string_view text, int style
) const noexcept
{
    auto const old_pos = plugin()->send_to_editor(SCI_GETTEXTLENGTH);
    append_text(text);
    plugin()->send_to_editor(SCI_STARTSTYLING, old_pos);
    plugin()->send_to_editor(SCI_SETSTYLING, text.length(), style);
}

void Output_Dialogue::show_selected_lint(
    Report_View::Data_Row selected_item
)
{
    Error_Info const &lint_error = current_tab_->errors[selected_item];

    int line = std::max(lint_error.line_ - 1, 0);
    int const column = std::max(lint_error.column_ - 1, 0);

    /* We only need to do this if we need to pop up linter.xml. The
     * following isn't ideal */
    if (current_tab_->tab == Tab::System_Error)
    {
        if (lint_error.mode_ == Error_Info::Bad_Linter_XML)
        {
            plugin()->send_to_notepad(
                NPPM_DOOPEN, 0, settings_->settings_file().c_str()
            );
        }
        else
        {
            plugin()->send_to_notepad(NPPM_MENUCOMMAND, 0, IDM_FILE_NEW);
            plugin()->send_to_editor(SCI_SETILEXER, 0, nullptr);
            constexpr int style = STYLE_LASTPREDEFINED + 1;
            plugin()->send_to_editor(SCI_STYLESETUNDERLINE, style, TRUE);

            append_text_with_style("Command:", style);
            // This should not throw if the command line is valid UTF16.
            // Which it is.
            append_text(
                "\n\n" + Encoding::convert(lint_error.command_) + "\n\n"
            );
            append_text_with_style("Return code: ", style);
            char buff[20];    // NOLINT(cppcoreguidelines-init-variables)
            std::ignore = std::snprintf(
                &buff[0], sizeof(buff), "%lu", lint_error.result_
            );
            append_text(&buff[0]);
            append_text("\n\n");
            append_text_with_style("Output:", style);
            append_text("\n\n");
            line = windows_static_cast<int, LRESULT>(plugin()->send_to_editor(
                SCI_LINEFROMPOSITION,
                plugin()->send_to_editor(SCI_GETTEXTLENGTH)
            ));
            append_text(lint_error.stdout_ + "\n\n");

            append_text_with_style("Error:", style);
            append_text("\n\n" + lint_error.stderr_);
        }
    }

    plugin()->send_to_editor(SCI_GOTOLINE, line);

    // since there is no SCI_GOTOCOLUMN, we move to the right until ...
    for (;;)
    {
        plugin()->send_to_editor(SCI_CHARRIGHT);
        LRESULT const curPos = plugin()->send_to_editor(SCI_GETCURRENTPOS);
        LRESULT const curLine =
            plugin()->send_to_editor(SCI_LINEFROMPOSITION, curPos);
        if (curLine > line)
        {
            // ... current line is greater than desired line or ...
            plugin()->send_to_editor(SCI_CHARLEFT);
            break;
        }

        LRESULT const curCol = plugin()->send_to_editor(SCI_GETCOLUMN, curPos);
        if (curCol > column)
        {
            // ... current column is greater than desired column or ...
            plugin()->send_to_editor(SCI_CHARLEFT);
            break;
        }

        if (curCol == column)
        {
            // ... we reached desired column.
            break;
        }
    }

    InvalidateRect();
}

void Output_Dialogue::copy_to_clipboard()
{
    std::wstringstream stream;

    bool first = true;
    for (auto row : current_report_view_->selected_items())
    {
        auto const &lint_error = current_tab_->errors[row];

        if (first)
        {
            first = false;
        }
        else
        {
            stream << L"\r\n";
        }

        stream << L"Line " << lint_error.line_ << L", column "
               << lint_error.column_ << L": " << L"\r\n\t"
               << lint_error.message_ << L"\r\n";
    }

    std::wstring const str = stream.str();
    if (str.empty())
    {
        return;
    }

    Clipboard clipboard{window()};

    clipboard.empty();
    clipboard.copy(str);
}

#ifndef __cpp_lib_copyable_function
#pragma warning(suppress : 26440)
#endif
int Output_Dialogue::sort_call_function(
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    LPARAM row1_index, LPARAM row2_index, Report_View::Data_Column column
)
#ifdef __cpp_lib_copyable_function
    const noexcept
#endif
{
    auto const &errs = current_tab_->errors;
    auto const &row1 = errs[row1_index];
    auto const &row2 = errs[row2_index];
    int res = 0;

    switch (column)
    {
        case Column_Position:
            res = row1.column_ - row2.column_;
            break;

        case Column_Tool:
            res = row1.tool_.compare(row2.tool_);
            break;

        case Column_Message:
            res = row1.message_.compare(row2.message_);
            break;

        case Column_Line:
        default:
            // Nothing to do here.
            break;
    }

    // If we still don't know, sort by column and line.
    if (res == 0)
    {
        res = row1.line_ - row2.line_;
        if (res == 0)
        {
            res = row1.column_ - row2.column_;
        }
    }
    return res;
}

Output_Dialogue::TabDefinition::TabDefinition(
    wchar_t const *name, UINT view_id, Tab tab_id, Output_Dialogue const &parent
) :
    tab_name(name),
    list_view_id(view_id),
    tab(tab_id),
    report_view(parent.GetDlgItem(windows_static_cast<int, UINT>(view_id)))
{
    using Column_Data = Report_View::Column_Data;

    // Note: The first column is implicitly left justified so the 'right' here
    // is a little optimistic.
    report_view.add_column(
        Column_Line,
        {.justification = Column_Data::Justification::Right,
         .width = 50,
         .text = L"Line"}
    );
    report_view.add_column(
        Column_Position,
        {.justification = Column_Data::Justification::Right,
         .width = 50,
         .text = L"Col"}
    );
    report_view.add_column(
        Column_Tool,
        {.justification = Column_Data::Justification::Left,
         .width = 100,
         .text = L"Tool"}
    );
    report_view.add_column(
        Column_Message,
        {.justification = Column_Data::Justification::Left,
         .width = 100,
         .text = L"Reason",
         .subitem = Column_Message}
    );
}

}    // namespace Linter
