// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "About_Dialogue.h"

#include "resource.h"

#include <CommCtrl.h>
#include <shellapi.h>
// Need windows.h for winuser.h to compile
#include <windows.h>    // IWYU pragma: keep
#include <winuser.h>    // For WM_INITDIALOG
// And this is IWYU not being able to understand 'override'
#include <minwindef.h>

#include <optional>

#pragma comment(lib, "Shell32")

namespace Linter
{

About_Dialogue::About_Dialogue(Plugin const &plugin) : Super(plugin)
{
    create_modal_dialogue(IDD_ABOUT_DIALOG);
}

About_Dialogue::~About_Dialogue() = default;

About_Dialogue::Message_Return About_Dialogue::on_dialogue_message(
    UINT message, WPARAM, LPARAM lParam
) noexcept
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            centre_dialogue();
            break;
        }

        case WM_NOTIFY:
        {
            auto const *nmh = windows_cast_to<NMHDR const *, LPARAM>(lParam);
            switch (nmh->code)
            {
                case NM_CLICK:
                case NM_RETURN:
                {
                    if (nmh->idFrom == IDC_ABOUT_HOMEPAGE_LINK)
                    {
                        auto const *lnkHdr = windows_cast_to<NMLINK const *, LPARAM>(lParam);
                        auto const &item = lnkHdr->item;
                        ShellExecute(
                            window(),
                            nullptr,
                            &item.szUrl[0],
                            nullptr,
                            nullptr,
                            SW_SHOWNORMAL
                        );
                    }
                    break;
                }

                default:
                    break;
            }
        }

        default:
            break;
    }
    return std::nullopt;
}

}    // namespace Linter
