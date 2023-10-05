#pragma once

#include "XmlParser.h"
#include "Notepad/DockingFeature/DockingDlgInterface.h"

#include <array>
#include <string>
#include <vector>
#include <unordered_map>

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
        //FIXME Why aren't tab_definitions_ and list_views_ merged?
        enum
        {
            NUM_TABS = 2
        };

        NppData const &npp_data_;
        HWND tab_window_;
        std::array<HWND, NUM_TABS> list_views_;

        struct TabDefinition
        {
            LPCTSTR tab_name_;
            UINT list_view_id_;
            enum Tab
            {
                SYSTEM_ERROR,
                LINT_ERROR,
            } type_;
        };
        static std::array<TabDefinition, NUM_TABS> tab_definitions_;

        std::array<std::vector<XmlParser::Error>, NUM_TABS> errors_;
        //std::unordered_map<std::wstring, std::vector<XmlParser::Error>> errors_;

        std::wstring ini_file_;

        void initialise_tab();
        void initialise_list_view(int tab);
        void resize();
        void selected_tab_changed();
        void update_displayed_counts();

        void add_errors(TabDefinition::Tab type, std::vector<XmlParser::Error> const &lints);


        //void get_name_from_cmd(UINT resID, LPTSTR tip, UINT count);
        void show_selected_lint(int selected_item);
        void copy_to_clipboard();

        /** Get the current scintilla window */
        HWND OutputDialog::GetCurrentScintillaWindow() const;
    };

}    // namespace Linter
