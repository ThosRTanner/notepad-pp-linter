#pragma once

#include "List_View_Types.h"

#include <minwindef.h>    //for LPARAM
#include <windef.h>       //for HWND, RECT, tagPOINT, tagRECT

#include <functional>
#if __cplusplus >= 202302L
#include <generator>
#endif
#include <optional>
#include <string>

namespace Linter
{

/** The idea of this is to wrap up a windows 'List View' control.
 *
 * This is only used for a report style list view, so the name is somewhat open
 * to question.
 */
class List_View
{
  public:
    // Data_Row is used to identify the actual row in the list view,
    // not the position on screen.
    using Data_Row = int;

    using Data_Column = typename List_View_Types::Data_Column;
    using Sort_Direction = typename List_View_Types::Sort_Direction;

    /** Data for defining a column.
     *
     * This is converted into an LVCOLUMN object. This is for creation only; you
     * will need to use a different structure for reading column data.
     */
    struct Column_Data
    {
        enum class Justification
        {
            Left,
            Right,
            Center
        };
        std::optional<Justification> justification;
        // Consolidate image options?
        // bitmap on right only makes sense with images
        // IMAGE is an image id
        // header_has_images means the header has images
        //   this doesn't seem to make sense and at least
        //   one source says it has no effect
        // has_images means the column rows have images
        bool bitmap_on_right{false};    // Only makes sense with images
        bool header_has_images{false};
        bool fixed_width{false};
        bool no_dpi_scale{false};
        bool fixed_aspect_ratio{false};
        bool split_button{false};
        std::optional<int> width;
        std::optional<std::wstring> text;
        std::optional<int> subitem;    // FIXME unclear how this works
        std::optional<int> image;
        std::optional<int> order;    // FIXME unclear how this works
        std::optional<int> min_width;
        std::optional<int> default_width;    // FIXME unclear how this works
        // This is read-only so shouldn't be settable.
        // std::optional<int> ideal_width;
    };

    /** This is a typesafe(ish) wrapper round the LVITEM structure.
     *
     * The mask field is built up according to which optional fields
     * are provided in row_data.
     *
     * This is only suitable for inserting or updating a single row.
     */
    struct Row_Data
    {
        int sub_item{0};
        struct State
        {
            std::optional<bool> activating;
            std::optional<bool> cut;
            std::optional<bool> drop_hilited;
            std::optional<bool> focused;
            std::optional<bool> selected;
            std::optional<int> overlay_image;    // must be 1 to 15
            std::optional<int> state_image;      // must be 1 to 15
        };
        std::optional<State> state;

        // technically text can be LPSTR_TEXTCALLBACK (- 1) to indicate
        // callback
        std::optional<std::wstring> text;

        // If image is I_IMAGECALLBACK (-1) , it means callback. be careful
        // when using it.
        std::optional<int> image;

        std::optional<LPARAM> user_data;
        std::optional<int> indent;
        std::optional<int> group_id;
        std::optional<unsigned int> columns;
        unsigned int *columns_array{nullptr};
        std::optional<int *> columns_format;

        /**
         * LVIF_DI_SETITEM The operating system should store the
         * requested list item information and not ask for it again.
         * This flag is used only with the LVN_GETDISPINFO notification
         * code.
         *
         * LVIF_NORECOMPUTE The control will not generate
         * LVN_GETDISPINFO to retrieve text information if it receives
         * an LVM_GETITEM message. Instead, the pszText member will
         * contain LPSTR_TEXTCALLBACK.
         */

        /**
         * Column format
         * LVCFMT_LINE_BREAK Move to the top of the next list of columns
         * LVCFMT_FILL Fill the remainder of the tile area. Might have a title.
         * LVCFMT_WRAP This sub-item can be wrapped.
         * LVCFMT_NO_TITLE This sub-item doesn't have an title.
         * LVCFMT_TILE_PLACEMENTMASK (LVCFMT_LINE_BREAK | LVCFMT_FILL)
         */
    };

    explicit List_View(HWND);

    virtual ~List_View() noexcept;

    // Prevent copying
    List_View(List_View const &) = delete;
    List_View &operator=(List_View const &) = delete;

    // Allow moving
    List_View(List_View &&) noexcept;
    List_View &operator=(List_View &&) noexcept;

    /** Add a column to the list view
     *
     * Returns: the index of the new column
     */
    int add_column(Column_Data const &) const noexcept;

    /** Add a row to the list view */
    void add_row(Data_Row, Row_Data const &) const;

    /** Ensure that there is space for at least total_rows rows in the list view
     */
    void ensure_rows(int total_rows) const noexcept;

    /** Set the text for an item */
    void set_item_text(
        Data_Row row, Data_Column col, std::wstring const &message
    ) const noexcept;

    /** Clear all items from view */
    void clear() const noexcept;

    /** Select all items */
    void select_all() const noexcept;

    /** Move selection up or down n rows, wrapping around
     *
     * Don't call this if there are no items!
     */
    Data_Row move_selection(int rows) const noexcept;

    /** Get the number of selected items */
    int num_selected_items() const noexcept;

    /** Returns true if we have a line selected */
    bool has_selected_line() const noexcept
    {
        // Note that focused and selected are not the same thing. Specifically,
        // the focussed item might not be selected.
        return get_first_selected_item() != -1;
    }

    /** Total number of items (rows) */
    int get_num_rows() const noexcept;

    /** Get the index of the first selected item */
    Data_Row get_first_selected_index() const noexcept;

    /** Get the index for a given item in the list control */
    Data_Row get_index(int item) const noexcept;

    /** Generator for selected item indices */
#if __cplusplus >= 202302L
    std::generator<List_View::Data_Row> selected_items() const noexcept;
#else
    // Fallback for pre-C++23 - returns a vector
    std::vector<List_View::Data_Row> selected_items() const noexcept;
#endif

    /** Show the list view */
    void show() const noexcept;

    /** Hide the list view */
    void hide() const noexcept;

    /** Callback function type for sorting
     *
     * Note: This *should* be const noexcept, but std::function doesn't support
     * that. With c++26 we could use std::copyable_function.
     */
#ifdef __cpp_lib_copyable_function
    using Sort_Callback = int(LPARAM, LPARAM, Data_Column) const noexcept;
    using Sort_Callback_Function = std::copyable_function<Sort_Callback>;
#else
    using Sort_Callback = int(LPARAM, LPARAM, Data_Column);
    using Sort_Callback_Function = std::function<Sort_Callback>;
#endif

    /** Sort_Direction displayed list by specified column.
     *
     * Callback function gets called with the lparams of the two items to
     * compare, and the column being sorted by.
     */
    void sort_by_column(
        Data_Column, Sort_Callback_Function, Sort_Direction
    ) const noexcept;

    /** Get the coordinates of a position in the list view */
    void get_screen_coordinates(POINT *point) const noexcept;

    /** Adjust position and size of list. */
    void set_window_position(HWND prev_win, RECT const &rect) const noexcept;

  protected:
    /** Get the handle to the list view */
    HWND handle() const noexcept
    {
        return handle_;
    }

  private:
    HWND handle_;

    /** Get the first selected item number, or -1 if none */
    int get_first_selected_item() const noexcept;
};

}    // namespace Linter
