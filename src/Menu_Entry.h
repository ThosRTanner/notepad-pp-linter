#pragma once

#include <string>

namespace Linter
{

enum Menu_Entry
{
    Menu_Entry_Edit_Config,
    Menu_Entry_Show_Results,
    Menu_Entry_Show_Next_Lint,
    Menu_Entry_Show_Previous_Lint
};

/** Get the display string for the menu entry */
std::wstring get_menu_string(Menu_Entry);

/** Get the menu entry from xml element name */
Menu_Entry get_menu_entry_from_element(std::wstring const &element_name);

}    // namespace Linter
