#pragma once
#include "Plugin/Plugin.h"

#include <memory>

namespace Linter
{

//Forward refs
class Output_Dialogue;

class Linter : public Plugin
{
    typedef Plugin Super;

  public:
    Linter(NppData const &);

    ~Linter();

    Linter(Linter const &) = delete;
    Linter(Linter &&) = delete;
    Linter &operator=(Linter const &) = delete;
    Linter &operator=(Linter &&) = delete;

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

    //Mark the current file changed and relint if necessary
    void mark_file_changed() noexcept;

    //Schedule lint of current file if necessary
    void relint_current_file() noexcept;

    // xml file
    std::wstring const config_file_;

    //Messages dockable box
    std::unique_ptr<Output_Dialogue> output_dialogue_;

    //Set once notepad is fully initialised
    bool notepad_is_ready_ = false;
};

}    // namespace Linter
