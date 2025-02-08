# Things to do before release

1. don't have 2 copies of lint list - actually there's 3 now
   * `Linter::errors_`
   * `Linter::errors_by_position`
   * `Output_Dialogue::Tab_Definition`
1. if window size changes, should scroll selected line back into view.
1. don't clear everything and redisplay. it looks messy
   * <https://learn.microsoft.com/en-us/windows/win32/gdi/wm-setredraw>
1. unclear why but sometimes adjusting column titles seems to needs a window
   redisplay. check jshint code.
   * especially you lose the scrollbar when double clicking on tool
1. configuration window
   * use cmd=command args=string-with-{param}
   * depending on how i code this, might need to support escaping.
     * {{} -> { and {} -> } might be enough although \{ and \} might be clearer,
       though that would require \\ as well.
   * {param} param may be
     * target - path to target file. if not supplied, input will be sent to
       stdin
     * dir - directory of original file
     * filepath - full path to original file
     * filename - original file name (i.e. excluding the path but not the
       extension)
   * support multiple extensions with same command
   * support environment variables
     * <https://learn.microsoft.com/en-us/windows/win32/api/processenv/nf-processenv-expandenvironmentstringsa>
1. Use xml parser, not dom parser
1. configurable squiggle/box
1. Capture error output from command. May be of use when get null xml output.
   * Documentation: "If the anonymous write pipe handle has been closed and
     ReadFile attempts to read using the corresponding anonymous read pipe
     handle, the function returns FALSE and GetLastError returns
     ERROR_BROKEN_PIPE."
   * possibly use `WaitForMultipleObjects`?
1. Replace all filesystem stuff with `std::filesystem::path`
1. configurable shortcut keys
1. Can take a long time to deal with frst file
   1. There's also several times when it decides it might need to run a linter
      because of the python script output...
1. There's some general raciness in that if you make a change then switch to
   another window, as the linting is running in the background, you might get 2
   updates to display - one for the previous, then one for the current. very
   theoretically if the previous was large and the current was small you might
   get the current overwritten with the previous.
1. File.cpp is a really bad name.
