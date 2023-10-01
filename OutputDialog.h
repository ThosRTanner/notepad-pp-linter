#pragma once

#include "XmlParser.h"
#include "Notepad/DockingFeature/DockingDlgInterface.h"

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

        /** Add a system error to the list */
        void add_system_error(std::wstring const &);
        void add_lint_errors(std::wstring const &file, std::vector<XmlParser::Error> const &lints);

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
        NppData const &npp_data_;
        //HICON tab_icon_;
        HWND tab_window_;

        static const int NUM_TABS = 2;
        HWND list_views_[NUM_TABS];

        struct TabDefinition
        {
            LPCTSTR tab_name_;
            UINT list_view_id_;
            enum
            {
                SYSTEM_ERROR,
                LINT_ERROR,
            } type_;
        };
        static TabDefinition tab_definitions_[NUM_TABS];

        /*
        struct FileLint
        {
            FileLint(const std::wstring &strFilePath, const JSLintReportItem &lint) : strFilePath(strFilePath), lint(lint)
            {
            }
            std::wstring strFilePath;
            JSLintReportItem lint;
        };
        std::vector<FileLint> m_fileLints[NUM_TABS];
        */

        // Collection of lint errors by generating program. Not sure whether this is useful,
        //given you're linting the same file. Maybe it should just be system errors/linter errors.
        //This would require to report program, line, col and string. currently we only have
        //the entire program which would make this look VERY long.
        //Also it'd make the list above need to be dynamic.
        std::unordered_map<std::wstring, std::vector<XmlParser::Error>> errors_;

        void initialise_tab();
        void initialise_list_view(int tab);
        void resize();
        void selected_tab_changed();
        void update_displayed_counts();

        void get_name_from_cmd(UINT resID, LPTSTR tip, UINT count);
        void show_selected_lint(int i);
        void copy_to_clipboard();
    };

}    // namespace Linter
