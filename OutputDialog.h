#pragma once

#include "DockingDlgInterface.h"
#include "XmlParser.h"

#include <array>
#include <string>
#include <vector>

struct NppData;

namespace Linter
{
    /** This maintains an output dialogue for the linter plugin.
     *
     * The dialogue consists of:
     * 1 tab - status (number of linters found for the current file, fatal errors, etc)
     * 1 tab for each linter 
     */
    class OutputDialog : public DockingDlgInterface
    {
      public:
        OutputDialog(NppData const &, HANDLE module, int dlg_num);

        OutputDialog(OutputDialog const &) = delete;
        OutputDialog(OutputDialog &&) = delete;
        OutputDialog &operator=(OutputDialog const &) = delete;
        OutputDialog &operator=(OutputDialog &&) = delete;

        ~OutputDialog();

        void display(bool toShow = true) const noexcept override;

        /** Clears all linting information */
        void clear_lint_info();

        /** Add an error to the system error list */
        void add_system_error(XmlParser::Error const &);

        /** Add a list of lint errors to the lint error list */
        void add_lint_errors(std::vector<XmlParser::Error> const &lints);

        /*
        void select_next_lint();
        void select_previous_lint();
        *.
        */
      protected:
        INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

        void on_toolbar_cmd(UINT message); //If we actually implement any...
        //void OnToolbarDropDown(LPNMTOOLBAR lpnmtb);

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

        HWND dialogue_;
        std::array<HWND, Num_Tabs> list_views_;
        Tab current_tab_;
        HWND current_list_view_;

        struct TabDefinition
        {
            wchar_t const *tab_name_;
            UINT list_view_id_;
        };
        static std::array<TabDefinition, Num_Tabs> const tab_definitions_;

        std::array<std::vector<XmlParser::Error>, Num_Tabs> errors_;

        /** Initialise the output window */
        void initialise_dialogue() noexcept;

        /** Initialise the specified tab */
        void initialise_tab(Tab tab) noexcept;

        /** Window resize */
        void resize() noexcept;

        /** Selected tab has been changed. Display new one */
        void selected_tab_changed() noexcept;

        /** Update the counts in the tab bar */
        void update_displayed_counts();

        /** Add list of errors to the appropriate tab */
        void add_errors(Tab tab, std::vector<XmlParser::Error> const &lints);

        //void get_name_from_cmd(UINT resID, LPTSTR tip, UINT count);

        /** Move to the line/column of the displayed error */
        void show_selected_lint(int selected_item) noexcept;

        /** Copy selected messages to clipboard */
        void copy_to_clipboard();

        /** Structure needed to map from sort call parameter to C++ */
        struct Sort_Call_Info
        {
            OutputDialog *dialogue;
            Tab tab;
        };

        /** This defines the sorting for the list view */
        int sort_selected_list(Tab tab, LPARAM row1_index, LPARAM row2_index) noexcept;

        /** This is what is actually called from the ListView_Sort method */
        static int sort_call_function(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) noexcept;
    };

}    // namespace Linter
