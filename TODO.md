TODO

1)
* don't have 2 copies of lint list.
* if window size changes, should scroll selected  line back into view.
* don't clear everything and redisplay. it looks messy
  * https://learn.microsoft.com/en-us/windows/win32/gdi/wm-setredraw
* unclear why but sometimes adjusting column titles seems to needs a window redisplay. check jshint code.
  * especially you lose the scrollbar when double clicking on tool

2)
* configuration window
* use cmd=command args=string-with-{param}
  * depending on how i code this, might need to support escaping.
    * {{} -> { and {} -> {} might be enough although \{ might be clearer
  * {param} param may be
    * target - path to target file. if not supplied, input will be sent to stdin
    * dir - directory of original file
    * file - original file
    * filename - original file name ()
* support multiple extensions with same command

3)
* Use xml parser, not dom parser

4)
* configurable squiggle/box
*

5)
* Capture error output from command. May be of use when get null xml output.
* Documentation:
"If the anonymous write pipe handle has been closed and ReadFile
attempts to read using the corresponding anonymous read pipe handle, the
function returns FALSE and GetLastError returns ERROR_BROKEN_PIPE."
* possible use 'waitformultipleobjects'?


PRs todo:
1 - split out Settings class and reload config file.
2 - fix 'run twice on change' bug.
3 - pass copy of command line string before passing to createprocess
