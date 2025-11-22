#pragma once

#include "Error_Info.h"
#include "Report_View.h"

#include "Plugin/Docking_Dialogue_Interface.h"

#include <intsafe.h>
#include <minwindef.h>
#include <windef.h>

#include <array>
#include <string_view>
#include <vector>

// IWYU says you need commtrl.h for this, but it's a forward reference.
struct tagNMLVCUSTOMDRAW;
using NMLVCUSTOMDRAW = tagNMLVCUSTOMDRAW;

namespace Linter
{

class Linter;
class Settings;

enum class Menu_Entry : int;

/** This maintains an output dialogue for the linter plugin.
 *
 * The dialogue consists of:
 * 1 tab - status (number of linters found for the current file, fatal errors,
 * etc) 1 tab for each linter
 */
class Output_Dialogue : protected Docking_Dialogue_Interface
{
    using Super = Docking_Dialogue_Interface;

  public:
    Output_Dialogue(Menu_Entry, Linter const &);

    Output_Dialogue(Output_Dialogue const &) = delete;
    Output_Dialogue(Output_Dialogue &&) = delete;
    Output_Dialogue &operator=(Output_Dialogue const &) = delete;
    Output_Dialogue &operator=(Output_Dialogue &&) = delete;

    ~Output_Dialogue() override;

    void display() noexcept;

    /** Clears all linting information */
    void clear_lint_info();

    /** Add an error to the system error list */
    void add_system_error(Error_Info const &);

    /** Add a list of lint errors to the lint error list */
    void add_lint_errors(std::vector<Error_Info> const &);

    /** Selects the next lint message */
    void select_next_lint();

    /** Selects the next previous message */
    void select_previous_lint();

  private:
    enum Tab
    {
        Lint_Error,
        System_Error
    };

    enum
    {
        Num_Tabs = 2
    };

    struct TabDefinition
    {
        TabDefinition(
            wchar_t const *name, UINT view_id, Tab tab_id,
            Output_Dialogue const &parent
        );

        // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
        wchar_t const *tab_name;
        UINT list_view_id;
        Tab tab;
        Report_View report_view;
        std::vector<Error_Info> errors;
        // NOLINTEND(misc-non-private-member-variables-in-classes)
    };

    Message_Return on_dialogue_message(
        UINT message, WPARAM wParam, LPARAM lParam
    ) override;

    /** Initialise the output window */
    void initialise_dialogue() noexcept;

    /** Process WM_COMMAND message */
    Message_Return process_dlg_command(WPARAM wParam);

    /** Process WM_CONTEXTMENU message */
    Message_Return process_dlg_context_menu(LPARAM lParam) noexcept;

    /** Process WM_NOTIFY message */
    Message_Return process_dlg_notify(LPARAM lParam);

    /** Process NM_CUSTOMDRAW notification */
    Message_Return process_custom_draw(NMLVCUSTOMDRAW *) noexcept;

    /** Selected tab has been changed. Display new one */
    void selected_tab_changed() noexcept;

    /** Window has changed size, position, layer */
    void window_pos_changed() noexcept;

    /** Update the counts in the tab bar */
    void update_displayed_counts();

    /** Add list of errors to the appropriate tab */
    void add_errors(Tab tab, std::vector<Error_Info> const &lints);

    /** Skip to the n-th lint forward or backward */
    void select_lint(int n);

    /** Add text to end of buffer */
    void append_text(std::string_view text) const noexcept;

    /** Add underlined text to end of buffer */
    void append_text_with_style(
        std::string_view text, int style
    ) const noexcept;

    /** Move to the line/column of the displayed error */
    void show_selected_lint(Report_View::Data_Row selected_item);

    /** Copy selected messages to clipboard */
    void copy_to_clipboard();

    /** This is called from the ListView_Sort method */
    Report_View::Sort_Callback sort_call_function;

    HWND tab_bar_;

    Report_View *current_report_view_{nullptr};

    std::array<TabDefinition, Num_Tabs> tab_definitions_;

    TabDefinition *current_tab_;

    Settings const *settings_;

    // Sorting callback function pointer.
    Report_View::Sort_Callback_Function sort_callback_;
};

}    // namespace Linter
