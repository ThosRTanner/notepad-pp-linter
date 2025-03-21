#include "Menu_Entry.h"

#include <unordered_map>
#include <utility>

namespace Linter
{

std::wstring get_menu_string(Menu_Entry entry)
{
    static std::unordered_map<Menu_Entry, std::wstring> const strings{
        {Menu_Entry_Edit_Config,        L"Edit config"          },
        {Menu_Entry_Show_Results,       L"Show linter results"  },
        {Menu_Entry_Show_Next_Lint,     L"Show next message"    },
        {Menu_Entry_Show_Previous_Lint, L"Show previous message"}
    };
    return strings.at(entry);
}

Menu_Entry get_menu_entry_from_element(std::wstring const &element_name)
{
    static std::unordered_map<std::wstring, Menu_Entry> const strings{
        {L"edit",     Menu_Entry_Edit_Config       },
        {L"show",     Menu_Entry_Show_Results      },
        {L"next",     Menu_Entry_Show_Next_Lint    },
        {L"previous", Menu_Entry_Show_Previous_Lint}
    };
    return strings.at(element_name);
}

}    // namespace Linter
