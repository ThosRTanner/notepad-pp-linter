# Change log

## 1.0.1

Added the ability to display error details. If you double click an entry in the system errors tab, either:

1. If the error was in parsing linter++.xml, it will be opened and the cursor set to the posiion of the error. Fixes #26.
1. otherwise, a new notepad buffer will be created containing the command that was run and any output or error output. Fixes #50

This probably fixes a few issues where the extension could hang or decide it had received no output, as I completely rewrote the code that received output from the linter.

Fixed an issue where the first file was taking a long time to process (see #13).

## 1.0.0

Initial release
