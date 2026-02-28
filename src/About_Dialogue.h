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

#pragma once
#include "Plugin/Modal_Dialogue_Interface.h"

// These don't actually provide anything useful, but IWYU can't currently work
// out that functions declared as override don't need to have a separate header

#include <windows.h>    // IWYU pragma: keep

#include <minwindef.h>

// Forward references
class Plugin;

namespace Linter
{

class About_Dialogue : public Modal_Dialogue_Interface
{
    using Super = Modal_Dialogue_Interface;

  public:
    explicit About_Dialogue(Plugin const &);

    About_Dialogue(About_Dialogue const &) = delete;
    About_Dialogue &operator=(About_Dialogue const &) = delete;

    About_Dialogue(About_Dialogue &&) = delete;
    About_Dialogue &operator=(About_Dialogue &&) = delete;

    ~About_Dialogue() override;

  private:
    Message_Return on_dialogue_message(
        UINT message, WPARAM wParam, LPARAM lParam
    ) noexcept override;
};

}    // namespace Linter
