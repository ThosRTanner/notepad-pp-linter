# Notepad++ Linter++

This is a fork of the 'linter' plugin for notepad++ from deadem which provides realtime code checks of a file with any checkstyle-compatible linter: jshint, eslint, jscs, phpcs, csslint etc.

![Document window](/img/1.jpg?raw=true)

![Dockable error window](/img/2.png?raw=true)

## Installation

- See <https://npp-user-manual.org/docs/plugins/>
- Use the plugin manager. No, seriously.
- If you must install manually, run notepad++ in administrator mode and
  - Go to Settings -> Import -> import plugin(s)...
  - This pops up a filer window. Find where you put linter++.dll and 'open' it.
  - You should get a popup telling you to restart notepad++. If you don't, you probably forgot to run in admin mode.
  - Restart notepad++ in normal mode.
- Go to Plugins -> Linter++ -> Edit config.
  - This will give you a blank configuration file. Edit to taste.

## Changes from the linter plugin

1. It has a docking window similar to that provided by the jslint plugin, which displays a list of all the detected errors in the file (by default sorted by line), and the tool which detected the issue.
1. The window will also display (in a separate tab) any messages resulting from failures to execute checker programs.
1. It is no longer necessary to restart notepad++ after changing the configuration.
1. The linter configuration file has changed considerably, and it now gets validated against an xsd file.

## Config example

This is a fairly simple configuration file that has 3 different linters for javascript programs (and modules), and one program to lint cascading stylesheets.

```xml
<?xml version="1.0" encoding="utf-8" ?>
<LinterPP>
  <linters>
    <linter>  
      <extensions> 
       <extension>.js</extension>
       <extension>.jsm</extension>
      </extensions>
      <commands>
        <command>
          <program>%AppData%\npm\eslint.cmd</program>
          <args>--resolve-plugins-relative-to="%AppData%\npm" --format checkstyle %%</args>
        </command>
        <command>
          <program>%AppData%\npm\jscs.cmd</program>
          <args>--reporter=checkstyle %%</args>
        </command>
        <command>
          <program>%AppData%\npm\jshint.cmd</program>
          <args>--reporter=checkstyle %%</args>
        </command>
      </commands>
    </linter>
    <linter>
      <extensions>
        <extension>.css</extension>
      </extensions>
      <commands>
        <command>
          <program>%AppData%\npm\csslint.cmd</program>
          <args>--format=checkstyle-xml</args>
        </command>
      </commands>
    </linter>
  </linters>
</LinterPP>
```

As you can see you can use windows environment variables in your command line. There are also some pseudo environment variables provided by linter++:

- %LINTER_TARGET% - temporary linter file
- %TARGET% - original file (e.g. `c:\users\me\fred.js`)
- %TARGET_DIR% - directory of original file (e.g. `c:\users\me`)
- %TARGET_EXT% - extension of original file (e.g. `.js`)
- %TARGET_FILENAME% - filename of original file (e.g. `fred.js`)

You will need to ensure that spaces are properly quoted in the `<args>` elements, (putting doublequotes round %...% strings is a good practice). This is not necessary in the `<program>` elements.

The `<args>` element will accept `%%` as the last two characters as a shortcut for `"%LINTER_TARGET%"`.

If you don't specify `%LINTER_TARGET%` or `%%` in your `<args>` element, it is assumed that your linter process its input from `stdin`.

### Optional parameters

By default, error messages will be shown in the docked dialogue window as red, warning messages as orange, and any other messages are shown in cyan (I'd be interested to find out if anyone gets this colour and under what circumstances).

You can change the colours by adding a `<messages>` section to the `<LinterPP>` section, like this:

```xml
  <messages>
    <default> <red>0</red> <green>255</green> <blue>255</blue> </default>
    <error> <red>255</red> <green>0</green> <blue>0</blue> </error>
    <warning> <red>127</red> <green>127</green> <blue>0</blue> </warning>
  </messages>
```

These are all optional, though if you do include a `<messages>` section, you must include at least one colour setting.
