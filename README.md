# Notepad++ Linter

This is a fork of the 'linter' plugin for notepad++ from deadem which provides realtime code checks of a file with any checkstyle-compatible linter: jshint, eslint, jscs, phpcs, csslint etc.

![](/img/1.jpg?raw=true)

## Installation

- See https://npp-user-manual.org/docs/plugins/
- Use the plugin manager. No, seriously.
- If you must install manually, run notepad++ in administrator mode and
  - Go to Settings -> Import -> import plugin(s)...
  - This pops up a filer window. Find where you put linter++.dll and 'open' it.
  - You should get a popup telling you to restart notepad++. If you don't, you probably forgot to run in admin mode.
  - Restart notepad++ in normal mode.
- Go to Plugins -> Linter++ -> Edit config.
  -	This will give you a blank configuration file. Edit to taste.

## Changes from the linter plugin.

1. It has a docking window similar to that provided by the jslint plugin, which displays a list of all the detected errors in the file (by default sorted by line), and the tool which detected the issue.
1. The window will also display (in a separate tab) any messages resulting from failures to execute checker programs.
1. It is no longer necessary to restart notepad++ after changing the configuration.

## Config example

```xml
<?xml version="1.0" encoding="utf-8" ?>
<NotepadPlus>
  <linter extension=".js" command="C:\Users\deadem\AppData\Roaming\npm\jscs.cmd --reporter=checkstyle"/>
  <linter extension=".js" command="C:\Users\deadem\AppData\Roaming\npm\jshint.cmd --reporter=checkstyle"/>
  <linter extension=".js" command="C:\Users\deadem\AppData\Roaming\npm\eslint.cmd --format checkstyle"/>
  <linter extension=".js" command="&quot;C:\Path with spaces\somelint.cmd&quot; --format checkstyle"/>
  <linter extension=".php" command="C:\Path_to\phpcs --report=checkstyle"/>
</NotepadPlus>
```

Optional attribute `stdin`="1" can be used to lint from stdin instead of temp file. i.e: 
```xml
  <linter stdin="1" extension=".js" command="C:\Users\deadem\AppData\Roaming\npm\eslint.cmd --stdin --format checkstyle"/>
```

To handle spaces in names, you should use the &quot; quote character, as follows:

```xml
  <linter extension=".none" command="&quot;C:\a command with spaces\thing&quot; --stuff" />
```

Optional parameter 

You can change default colors by an optional "style" tag. "color" attribute is a RGB hex color value, "alpha" value can range from 0 (completely transparent) to 255 (no transparency).

```xml
<NotepadPlus>
  <style color="0000FF" alpha="100" />
...
</NotepadPlus>
```
