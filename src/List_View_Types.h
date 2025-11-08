#pragma once

// Contains common types for List_View and related classes
namespace Linter::List_View_Types
{

// Data_Column is used to identify the actual column in the list view.
// not the position on screen.
typedef int Data_Column;

// Sorting direction for columns
enum class Sort_Direction    // NOLINT(performance-enum-size
{
    None,
    Ascending,
    Descending
};

}    // namespace Linter::List_View_Types
