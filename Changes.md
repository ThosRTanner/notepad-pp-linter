# Change log

## 1.0.1

Added the ability to display error details. If you double click an entry in the system errors tab, either:

1. If the error was in parsing linter++.xml, it will be opened and the cursor set to the position of the error. Fixes #26.
1. otherwise, a new notepad buffer will be created containing the command that was run and any output or error output. Fixes #50

This probably fixes a few issues where the extension could hang or decide it had received no output, as I completely rewrote the code that received output from the linter.

Fixed an issue where the first file was taking a long time to process (see #13). It can sometimes still take a long time, but it's less long now.

Adds support for creating your own environment variables (with the `<variables>` XML tag) and also added two new environment variables:
- LINTER_PLUGIN_DIR - Directory where linter++.dll is installed
- LINTER_CONFIG_DIR - Directory where your linter++.xml is installed.

I've added a couple of powershell scripts into the plugin directory which can be used for linting markdown files and powershell scripts.

## 1.0.0

Initial release
