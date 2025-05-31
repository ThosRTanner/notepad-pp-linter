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
1. There are menu entries which allow you to see the next or previous message. You don't need the dialogue window open to use these.
1. The window will also display (in a separate tab) any messages resulting from failures to execute checker programs.
1. It is no longer necessary to restart notepad++ after changing the configuration (unless you change the shortcut keys).
1. The linter configuration file has changed considerably, and it now gets validated against an xsd file.

## Configuration

You can adjust your configration by editting the file `Linter++.xml` in `%AppData%\plugins\config\Linter++`.

There are 4 sections to the configuration file. You do need to supply a list of linters if you want to actually lint something, but the rest of the configuration is optional.

A note: Please don't supply empty sections, as this will become an error in the future. Just leave them out entirely.

### Indicator

You can control all the various settings for a scintilla indicator here. Your xml will look something like this:

```xml
<indicator>
  <colour>
    <shade>
      <red>255</red>
      <green>0</green>
      <blue>0</blue>
    </shade>
  </colour>
  <style>roundbox</style>
  <fill_opacity>30</fill_opacity>
  <outline_opacity>255</outline_opacity>
  <draw_under/>
  <hover>
    <style>box</style>
    <colour> <red>20</red> <green>20</green> <blue>250</blue> </colour>
  </hover>
</indicator>
```

If you use `<as_message/>` instead of `<shade>...</shade>` in the top level `<colour>...</colour>` block, then the colour of the indicator will match the colour of the message in the message window. This sadly isn't available for the hover colour.

It's best to read the scintilla documentation and play around a little bit to determine what works best for you.

### Messages

By default, error messages will be shown in the docked dialogue window as red, warning messages as orange, and any other messages are shown in cyan (I'd be interested to find out if anyone gets this colour and under what circumstances).

You can change the colours by adding a `<messages>` section, like this:

```xml
<messages>
  <default> <red>0</red> <green>255</green> <blue>255</blue> </default>
  <error> <red>255</red> <green>0</green> <blue>0</blue> </error>
  <warning> <red>127</red> <green>127</green> <blue>0</blue> </warning>
</messages>
```

You don't need to specify all of the entries here, just the ones where you want to override the default.

### Shortcuts

You can supply shortcuts for one or more of the menu entries, like this:

```xml
<shortcuts>
  <edit><alt/><ctrl/><key>F5</key></edit>
  <show><shift/><ctrl/><key>F6</key></show>
  <previous><ctrl/><key>F7</key></previous>
  <next>><key>F8</key></next>
</shortcuts>
```

There may be some shortcut keys which you'd like to use. Please let me know and I'll try to add them.

Notes:

1. Changing the shortcuts will not take effect till next time you start notepad++
1. notepad++ determines which shortcuts get which keypresses, so take care not to clash with shortcuts from other plugins.

### Linters

A linter definition consists of a list of extensions to match, and a list of program to run (specified as command line and arguments).

For instance, you may wish to use eslint, jscs and jshint on all files with .js or .jsm extensions and run css lint on all css file. Your `linters` section would end up looking something like this:

```xml
<linters>
  <linter>
    <extensions>
     <extension>.js</extension>
     <extension>.jsm</extension>
    </extensions>
    <commands>
      <command>
        <program>%AppData%\npm\eslint.cmd</program>
        <args>-c "%USERPROFILE%"\repositories\github\inforss\inforss.eslintrc.yaml --resolve-plugins-relative-to="%AppData%"\npm --format checkstyle %%</args>
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
```

As you can see you can use windows environment variables in your command line. There are also some pseudo environment variables provided by linter++:

- %LINTER_TARGET% - temporary linter file
- %TARGET% - original file (e.g. `c:\users\me\fred.js`)
- %TARGET_DIR% - directory of original file (e.g. `c:\users\me`)
- %TARGET_EXT% - extension of original file (e.g. `.js`)
- %TARGET_FILENAME% - filename of original file (e.g. `fred.js`)

You will need to ensure that spaces are properly quoted in the `<args>` elements, (putting doublequotes round %...% strings is a good practice). This is not necessary in the `<program>` elements.

The `<args>` element will accept `%%` as the last two characters as a shortcut for `"%LINTER_TARGET%"`.

If you don't specify `%LINTER_TARGET%` or `%%` in your `<args>` element, it is assumed that your linter process its input from `stdin` (as is the case for the csslint command in the example above).

### Example

Putting all those together, we get this:

```xml
<?xml version="1.0" encoding="utf-8" ?>
<LinterPP>
  <indicator>
    <colour>
      <shade>
        <red>255</red>
        <green>0</green>
        <blue>0</blue>
      </shade>
    </colour>
    <style>roundbox</style>
    <fill_opacity>30</fill_opacity>
    <outline_opacity>255</outline_opacity>
    <draw_under/>
  </indicator>

  <messages>
    <default> <red>0</red> <green>255</green> <blue>255</blue> </default>
    <error> <red>255</red> <green>0</green> <blue>0</blue> </error>
    <warning> <red>127</red> <green>127</green> <blue>0</blue> </warning>
  </messages>

  <shortcuts>
    <edit><alt/><ctrl/><key>F5</key></edit>
    <show><shift/><ctrl/><key>F6</key></show>
    <previous><ctrl/><key>F7</key></previous>
    <next>><key>F8</key></next>
  </shortcuts>

  <linters>
    <linter>
      <extensions>
       <extension>.js</extension>
       <extension>.jsm</extension>
      </extensions>
      <commands>
        <command>
          <program>%AppData%\npm\eslint.cmd</program>
          <args>-c "%USERPROFILE%"\repositories\github\inforss\inforss.eslintrc.yaml --resolve-plugins-relative-to="%AppData%"\npm --format checkstyle %%</args>
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
