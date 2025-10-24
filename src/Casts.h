#pragma once

/** This file contains some casts which are for making casts between
 * Windows types less warning-prone.
 *
 * Each cast takes the source type as an extra template parameter.
 */

namespace Linter
{

/** reinterpret_cast. Should probably be renamed... */
template <typename Target_Type, typename Orig_Type>
Target_Type cast_to(Orig_Type val) noexcept
{
#pragma warning(suppress : 26490)
    return reinterpret_cast<Target_Type>(val);
}

template <typename Target_Type, typename Orig_Type>
Target_Type windows_static_cast(Orig_Type val) noexcept
{
#pragma warning(suppress : 26472)
    return static_cast<Target_Type>(val);
}

template <typename Orig_Type>
Orig_Type windows_const_cast(
    std::remove_pointer_t<Orig_Type> const *val
) noexcept
{
#pragma warning(suppress : 26492)
    return const_cast<Orig_Type>(val);
}

}    // namespace Linter
