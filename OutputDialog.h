#pragma once

#include "XmlParser.h"
#include "Notepad/DockingFeature/DockingDlgInterface.h"

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
        ~OutputDialog();

        void display(bool toShow = true) const override;

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
            System_Error,
            Lint_Error,
        };

        enum
        {
            Num_Tabs = 2
        };

        NppData const &npp_data_;
        HWND dialogue_;
        std::array<HWND, Num_Tabs> list_views_;

        struct TabDefinition
        {
            wchar_t const *tab_name_;
            UINT list_view_id_;
        };
        static std::array<TabDefinition, Num_Tabs> const tab_definitions_;

        std::array<std::vector<XmlParser::Error>, Num_Tabs> errors_;

        std::wstring ini_file_;

        /** Initialise the output window */
        void initialise_dialogue();

        /** Initialise the specified tab */
        void initialise_tab(Tab tab);

        /** Window resize */
        void resize();

        /** Selected tab has been changed. Display new one */
        void selected_tab_changed();

        /** Update the counts in the tab bar */
        void update_displayed_counts();

        /** Add list of errors to the appropriate tab */
        void add_errors(Tab tab, std::vector<XmlParser::Error> const &lints);

        //void get_name_from_cmd(UINT resID, LPTSTR tip, UINT count);

        /** Move to the line/column of the displayed error */
        void show_selected_lint(int selected_item);
        //describe
        void copy_to_clipboard();

        /** Get the current scintilla window */
        HWND GetCurrentScintillaWindow() const;
    };

}    // namespace Linter
