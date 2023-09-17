#pragma once
#include "Notepad/DockingFeature/DockingDlgInterface.h"

struct NppData;

namespace Linter
{

    class OutputDialog : public DockingDlgInterface
    {
      public:
        OutputDialog(NppData const &, HANDLE module);
        ~OutputDialog();

        void display(bool toShow) const override;

        /*
        HICON GetTabIcon();
        */
        void clear_all_lints();
        //void AddLints(const std::wstring &strFilePath, const std::list<JSLintReportItem> &lints);
        void select_next_lint();
        void select_previous_lint();

      protected:
        INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

        void on_toolbar_cmd(UINT message);
        //void OnToolbarDropDown(LPNMTOOLBAR lpnmtb);

      private:
        NppData const &npp_data_;
        //HICON tab_icon_;
        HWND tab_window_;

        static const int NUM_TABS = 4;
        HWND tab_views_[NUM_TABS];

        struct TabDefinition
        {
            LPCTSTR tab_name_;
            UINT list_view_id_;
            enum
            {
                SYSTEM_ERROR,
                LINT_ERROR,
                LINT_WARNING,
                LINT_INFO
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

        void initialise_tab();
        void initialise_list_view(int tab);
        void resize();
        void selected_tab_changed();
        void get_name_from_cmd(UINT resID, LPTSTR tip, UINT count);
        void show_selected_lint(int i);
        void copy_to_clipboard();
    };

}    // namespace Linter
