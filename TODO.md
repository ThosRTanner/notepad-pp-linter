# Things to do before release

1. don't have 2 copies of lint list - actually there's 3 now
   * `Linter::errors_`
   * `Linter::errors_by_position`
   * `Output_Dialogue::Tab_Definition`
1. if window size changes, should scroll selected line back into view.
1. don't clear everything and redisplay. it looks messy
   * <https://learn.microsoft.com/en-us/windows/win32/gdi/wm-setredraw>
1. unclear why but sometimes adjusting column titles seems to needs a window redisplay. check jshint code.
   * especially you lose the scrollbar when double clicking on tool
1. configuration window
   * use cmd=command args=string-with-{param}
   * depending on how i code this, might need to support escaping.
     * {{} -> { and {} -> } might be enough although \{ and \} might be clearer, though that would require \\ as well.
   * {param} param may be
     * target - path to target file. if not supplied, input will be sent to stdin
     * dir - directory of original file
     * filepath - full path to original file
     * filename - original file name (i.e. excluding the path but not the extension)
   * support multiple extensions with same command
1. Use xml parser, not dom parser
1. configurable squiggle/box
1. Capture error output from command. May be of use when get null xml output.
   * Documentation:
   "If the anonymous write pipe handle has been closed and ReadFile
    attempts to read using the corresponding anonymous read pipe handle, the
    function returns FALSE and GetLastError returns ERROR_BROKEN_PIPE."
   * possibly use `WaitForMultipleObjects`?
1. Replace all filesystem stuff with `std::filesystem::path`
1. configurable shortcut keys
