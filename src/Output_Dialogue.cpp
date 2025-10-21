#include "Output_Dialogue.h"

#include "Plugin/Plugin.h"

#include "Checkstyle_Parser.h"
#include "Clipboard.h"
#include "Encoding.h"
#include "Error_Info.h"
#include "Linter.h"
#include "Menu_Entry.h"
#include "Settings.h"
#include "System_Error.h"

#include "notepad++/menuCmdID.h"

#include "resource.h"

#include <CommCtrl.h>
#include <intsafe.h>

#include <cstddef>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace Linter
{

namespace
{
/** A slightly more explicit version of reinterpret_cast which
 * requires both types to be specified, and disables the cast warning.
 */
template <typename Target_Type, typename Orig_Type>
Target_Type cast_to(Orig_Type val) noexcept
{
#pragma warning(suppress : 26490)
    return reinterpret_cast<Target_Type>(val);
}

/** A similar cast to wrap const_cast without the warnings.
 * The windows APIs leave a lot to be desired in the area of const correctness.
 */
template <typename Orig_Type>
Orig_Type windows_const_cast(
    std::remove_pointer_t<Orig_Type> const *val
) noexcept
{
#pragma warning(suppress : 26492)
    return const_cast<Orig_Type>(val);
}

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
    tab_definitions_({
        TabDefinition{L"Lint Errors",   IDC_LIST_LINTS,  Lint_Error,   *this},
        TabDefinition{L"System Errors", IDC_LIST_OUTPUT, System_Error, *this}
}),
    current_tab_(&tab_definitions_.at(0)),
    settings_(plugin.settings()),
    sort_callback_(
        [this](
            LPARAM row1_index, LPARAM row2_index,
            Report_View::Data_Column column
        ) { return this->sort_call_function(row1_index, row2_index, column); }
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

Output_Dialogue::~Output_Dialogue()
{
}

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
    std::vector<Error_Info> errs = {err};
    add_errors(Tab::System_Error, errs);
}

void Output_Dialogue::add_lint_errors(std::vector<Error_Info> const &errs)
{
    add_errors(Tab::Lint_Error, errs);
}

void Output_Dialogue::select_next_lint() noexcept
{
    select_lint(1);
}

void Output_Dialogue::select_previous_lint() noexcept
{
    select_lint(-1);
}

Output_Dialogue::Message_Return Output_Dialogue::on_dialogue_message(
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
    auto const notify_header = cast_to<NMHDR const *, LPARAM>(lParam);
    switch (notify_header->code)
    {
        case LVN_KEYDOWN:
            if (notify_header->idFrom == current_tab_->list_view_id)
            {
                NMLVKEYDOWN const *pnkd =
                    cast_to<LPNMLVKEYDOWN, LPARAM>(lParam);
                if (pnkd->wVKey == 'A'
                    && (::GetKeyState(VK_CONTROL) & 0x8000U) != 0)
                {
                    current_report_view_->select_all();
                    return TRUE;
                }
                else if (pnkd->wVKey == 'C'
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
                auto const column_click =
                    cast_to<NMLISTVIEW const *, LPARAM>(lParam);
                current_report_view_->sort_by_column(
                    column_click->iSubItem, sort_callback_
                );
                return TRUE;
            }
            break;

        case NM_DBLCLK:
            if (notify_header->idFrom == current_tab_->list_view_id)
            {
                auto const item_activate =
                    cast_to<NMITEMACTIVATE const *, LPARAM>(lParam);
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
                    cast_to<NMLVCUSTOMDRAW *, LPARAM>(lParam)
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
        case CDDS_PREPAINT:
            return CDRF_NOTIFYITEMDRAW;

        case CDDS_ITEMPREPAINT:
#pragma warning(suppress : 26472)
            current_item_ = static_cast<int>(custom_draw->nmcd.dwItemSpec);
            return CDRF_NOTIFYSUBITEMDRAW;

        case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
            if (custom_draw->iSubItem == Column_Message)
            {
                // FIXME: Shouldn't we be using the current list view here?
                LVITEM const item{.mask = LVIF_PARAM, .iItem = current_item_};
                ListView_GetItem(custom_draw->nmcd.hdr.hwndFrom, &item);

#pragma warning(suppress : 26472)
                if (static_cast<std::size_t>(item.lParam)
                    >= current_tab_->errors.size())
                {
                    // For reasons I don't entirely understand, windows paints
                    // an entry for a line that doesn't exist. So don't do
                    // anything for that.
                    break;
                }

                // Now we colour the text according to the severity level.
                auto const &lint_error = current_tab_->errors[item.lParam];
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
    RECT const rc = getClientRect();

    ::MoveWindow(
        tab_bar_, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE
    );

    TabCtrl_AdjustRect(tab_bar_, FALSE, &rc);
    for (auto const &tab : tab_definitions_)
    {
        tab.report_view.set_window_position(tab_bar_, rc);
    }
}

void Output_Dialogue::update_displayed_counts()
{
    for (auto const &tab : tab_definitions_)
    {
        std::wstring strTabName;
        int const count = tab.report_view.num_items();
        if (count > 0)
        {
            std::wstringstream stream;
            stream << tab.tab_name << L" (" << count << L")";
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
    auto row = static_cast<int>(tab_def.errors.size());

    // FIXME Should I add the expected size to the list_view
    for (auto const &lint : lints)
    {
        tab_def.errors.push_back(lint);

        report_view.add_row(row, Report_View::Row_Data{.user_data = row});

        report_view.set_item_text_with_autosize(
            row, Column_Message, lint.message_
        );

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

    // Also allow user to sort differently and remember?
    if (&tab_def == current_tab_)
    {
        report_view.sort_by_column(Column_Line, sort_callback_);
        InvalidateRect();
    }
}

void Output_Dialogue::select_lint(int n) noexcept
{
    int const count = current_report_view_->num_items();
    if (count == 0)
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

void Output_Dialogue::show_selected_lint(List_View::Data_Row selected_item)
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
            plugin()->send_to_editor(SCI_STYLESETUNDERLINE, style, true);

            append_text_with_style("Command:", style);
            // This should not throw if the command line is valid UTF16.
            // Which it is.
            append_text(
                "\n\n" + Encoding::convert(lint_error.command_) + "\n\n"
            );
            append_text_with_style("Return code:", style);
            append_text(" " + std::to_string(lint_error.result_) + "\n\n");
            append_text_with_style("Output:", style);
            append_text("\n\n");
            line = static_cast<int>(plugin()->send_to_editor(
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

int Output_Dialogue::sort_call_function(
    LPARAM row1_index, LPARAM row2_index, Report_View::Data_Column column
)
{
    auto const &errs = current_tab_->errors;
    switch (column)
    {
        case Column_Line:
        {
            int res = errs[row1_index].line_ - errs[row2_index].line_;
            if (res == 0)
            {
                res = errs[row1_index].column_ - errs[row2_index].column_;
            }
            return res;
        }

        case Column_Position:
        {
            int res = errs[row1_index].column_ - errs[row2_index].column_;
            if (res == 0)
            {
                res = errs[row1_index].line_ - errs[row2_index].line_;
            }
            return res;
        }

        case Column_Tool:
            return errs[row1_index].tool_.compare(errs[row2_index].tool_);

        case Column_Message:
            return errs[row1_index].message_.compare(errs[row2_index].message_);

        default:
            // Not possible!
            return 0;
    }
}

Output_Dialogue::TabDefinition::TabDefinition(
    wchar_t const *name, UINT id, Tab tab, Output_Dialogue const &parent
) :
    tab_name(name),
    list_view_id(id),
    tab(tab),
    report_view(parent.GetDlgItem(id))
{
    typedef Report_View::Column_Data Column_Data;

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
