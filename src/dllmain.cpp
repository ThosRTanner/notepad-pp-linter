// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "Linter.h"
using Npp_Plugin = Linter::Linter;

#include "notepad++/PluginInterface.h"

#include <memory>

namespace
{

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::unique_ptr<Npp_Plugin> plugin;

}    // namespace

extern "C" __declspec(dllexport) wchar_t const *getName()
{
    return Npp_Plugin::get_plugin_name();
}

extern "C" __declspec(dllexport) void setInfo(NppData data)
{
    plugin = std::make_unique<Npp_Plugin>(data);
}
