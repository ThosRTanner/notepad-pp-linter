#pragma once

// iwyu wants forward references for FuncItem, NppData, SCNotification because
// it doesn't understand inheritance
#include "Plugin/Plugin.h"

//#include "Checkstyle_Parser.h"
#include "Error_Info.h"

#include <minwindef.h>
#include <winnt.h>

#include <cstdint>  // For uint32_t
#include <map>
#include <memory>
#include <string>
#include <vector>

struct FuncItem;
struct NppData;
struct SCNotification;

namespace Linter
{

// Forward refs
class Output_Dialogue;
class Settings;

class Linter : public Plugin
{
    using Super = Plugin;

  public:
    explicit Linter(NppData const &);

    ~Linter() override;

    Linter(Linter const &) = delete;
    Linter(Linter &&) = delete;
    Linter &operator=(Linter const &) = delete;
    Linter &operator=(Linter &&) = delete;

    /** Return the plugin name */
    static wchar_t const *get_plugin_name() noexcept;

    Settings const *settings() const noexcept
    {
        return settings_.get();
    }

  private:
    std::vector<FuncItem> &on_get_menu_entries() override;

    /** Process scintilla notifications (from beNotified) */
    void on_notification(SCNotification const *) override;

    // Menu functions
    void edit_config() noexcept;
    void show_results() noexcept;
    void select_next_lint() noexcept;
    void select_previous_lint() noexcept;
    void toggle_enable() noexcept;

    // Mark the current file changed and relint if necessary
    void mark_file_changed() noexcept;

    // Schedule lint of current file if necessary
    void relint_current_file() noexcept;

    void highlight_errors();

    void highlight_error_at(LRESULT pos, uint32_t col) noexcept;

    void clear_error_highlights() noexcept;

    void setup_error_indicator();

    static void __stdcall relint_timer_callback(void *, BOOLEAN) noexcept;

    void start_async_timer() noexcept;

    // Wrapper to call run_linter from ::beginthread
    static unsigned int __stdcall run_linter_thread(void *) noexcept;

    // Called by thread that runs apply_linters and handles exceptions.
    unsigned int run_linter() noexcept;

    // Apply all the applicable linters to the current buffer.
    void apply_linters();

    // Shows tooltip in notepad++ window.
    void show_tooltip();

    // Ditto with optional message
    void show_tooltip(std::wstring message);

    // Settings
    std::unique_ptr<Settings> settings_;

    // Messages dockable box
    std::unique_ptr<Output_Dialogue> output_dialogue_;

    // Windows timer queue
    HANDLE timer_queue_;

    // Timer for relinting
    HANDLE relint_timer_{nullptr};

    // Background thread that spawns linters and collects results.
    HANDLE bg_linter_thread_handle_{nullptr};

    // Set once notepad is fully initialised
    bool notepad_is_ready_{false};

    // Set if current file (buffer) has changed
    bool file_changed_{true};

    // List of errors picked up in latest lint(s)
    std::vector<Error_Info> errors_;

    // Same but by position in window
    std::map<LRESULT, std::wstring> errors_by_position_;

    // Whether the linter is enabled
    bool enabled_;

    // FIXME - abstract this into the plugin class
    std::vector<FuncItem> menu_entries_;
};

}    // namespace Linter
