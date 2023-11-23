#pragma once

#include "DockingDlgInterface.h"
#include "XmlParser.h"

#include <array>
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
        OutputDialog(HANDLE module, HWND npp, int dlg_num);

        OutputDialog(OutputDialog const &) = delete;
        OutputDialog(OutputDialog &&) = delete;
        OutputDialog &operator=(OutputDialog const &) = delete;
        OutputDialog &operator=(OutputDialog &&) = delete;

        ~OutputDialog();

        void display() noexcept override;

        /** Clears all linting information */
        void clear_lint_info();

        /** Add an error to the system error list */
        void add_system_error(XmlParser::Error const &);

        /** Add a list of lint errors to the lint error list */
        void add_lint_errors(std::vector<XmlParser::Error> const &lints);

        /** Selects the next lint message */
        void select_next_lint() noexcept;

        /** Selects the next previous message */
        void select_previous_lint() noexcept;

      protected:
        void resize() noexcept override;

        INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

      private:
        HWND tab_bar_;
        HWND current_list_view_;

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
            TabDefinition(wchar_t const *name, UINT id, Tab tab, DockingDlgInterface const & parent);

            wchar_t const *tab_name;
            UINT list_view_id;
            Tab tab;
            HWND list_view;
            std::vector<XmlParser::Error> errors;
        };

        std::array<TabDefinition, Num_Tabs> tab_definitions_;

        TabDefinition *current_tab_;

        /** Initialise the output window */
        void initialise_dialogue() noexcept;

        /** Initialise the specified tab */
        void initialise_tab(TabDefinition & tab) noexcept;

        /** Selected tab has been changed. Display new one */
        void selected_tab_changed() noexcept;

        /** Update the counts in the tab bar */
        void update_displayed_counts();

        /** Add list of errors to the appropriate tab */
        void add_errors(Tab tab, std::vector<XmlParser::Error> const &lints);

        /** Skip to the n-th lint forward or backward */
        void select_lint(int n) noexcept;

        /** Move to the line/column of the displayed error */
        void show_selected_lint(int selected_item) noexcept;

        /** Copy selected messages to clipboard */
        void copy_to_clipboard();

        /** This is called from the ListView_Sort method */
        static int __stdcall sort_call_function(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) noexcept;
    };

}    // namespace Linter
