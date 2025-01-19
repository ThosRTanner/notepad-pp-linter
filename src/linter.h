#pragma once
#include "Plugin/Plugin.h"

namespace Linter
{

class Linter : public Plugin
{
    typedef Plugin Super;

  public:
    Linter(NppData const &);

    ~Linter();

    /** Return the plugin name */
    static wchar_t const *get_plugin_name() noexcept;

    enum Menu_Entries
    {
        Menu_Entry_Edit_Config,
        Menu_Entry_Show_Results,
        Menu_Entry_Show_Next_Lint,
        Menu_Entry_Show_Previous_Lint
    };

  private:
    std::vector<FuncItem> &on_get_menu_entries() override;

    /** Process scintilla notifications (from beNotified) */
    virtual void on_notification(SCNotification const *) override;

    // Menu functions
    void edit_config() noexcept;
    void show_results() noexcept;
    void select_next_lint() noexcept;
    void select_previous_lint() noexcept;

    // xml file
    std::wstring const config_file_;
};

}    // namespace Linter
