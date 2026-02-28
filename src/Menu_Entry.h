#pragma once

#include <string>

namespace Linter
{

enum class Menu_Entry : int
{
    Edit_Config,
    Separator_1,
    Show_Results,
    Show_Next_Lint,
    Show_Previous_Lint,
    Separator_2,
    Toggle_Enabled,
    Separator_3,
    About,
    Help
};

/** Get the display string for the menu entry */
std::wstring get_menu_string(Menu_Entry);

/** Get the menu entry from xml element name */
Menu_Entry get_menu_entry_from_element(std::wstring const &element_name);

}    // namespace Linter
