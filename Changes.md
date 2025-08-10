# Change log

## 1.0.1

1. Added the ability to display error details. If you double click an entry in the system errors tab, either:

   1. If the error was in parsing linter++.xml, it will be opened and the cursor set to the position of the error. Fixes #26.

   1. otherwise, a new notepad buffer will be created containing the command that
   was run and any output or error output. Fixes #50.

1. Fixed a few issues where the extension could hang or decide it had received no output (I completely rewrote the code that received output from the linter...)

1. Fixed an issue where the first file was taking a long time to process (see #13). It can sometimes still take a long time, but it's less long now.

1. Added support for creating your own environment variables (with the `<variables>` XML tag) and also added two new environment variables:

   - LINTER_PLUGIN_DIR - Directory where linter++.dll is installed
   - LINTER_CONFIG_DIR - Directory where your linter++.xml is installed.

1. Added some powershell scripts into the plugin directory which can be used for linting markdown files and powershell scripts.

1. Changed the parsing slightly to ensure that the content of `<args>` tags is parsed as a token (whitespace is compressed and trimmed). I originally intended it to be like that, but microsft XML treats xs:token as the same as xs:string.

1. Added an "enabled" menu entry in response to #63. If the menu entry is toggled off, linting will not take place until it is toggled back on.

   1. By default, when notepad is started, the the menu entry will be ticked. You can change this by adding a `<disabled/>` tag to the `<misc>` section in linter++.xml.
   1. You can toggle the state by clicking the menu entry. Note this state does not persist over restart.
   1. A shortcut key is configurable by adding an `<enabled><key></key></enabled>` entry in the `<shortcuts>` section in linter++.xml.

1. Fixed crash in 32 bit implementation (#61).

## 1.0.0

Initial release
