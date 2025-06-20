# Parses output from remarklint.
# Requires powershell 6 or better.
(& "$Env:UserProfile\node_modules\.bin\remark.cmd" @args |
    Tee-object -Variable 'String' | Out-Null) 2>&1 | ForEach-Object -begin {
    echo "<?xml version=""1.0"" encoding=""utf-8"" ?>"
    echo "<checkstyle version=""4.3"">"
} -process {
    # 1) Remove the annoying quote and colouring around line numbers.
    #    I'm assuming that the same goes for everything it decides to quote.
    $line = $_ -replace
        ("\e" + '\[36m`(.+?)`' + "\e" + '\[39m'), '$1'

    # 2) Now split on <esc>[\d+m
    $columns = $line.trim() -split '\s*' + "`e" + '\[[0-9]+?m' + '\s*'

    # 0: Line
    # 1: Severity
    # 2: Filename - this one comes first (fortunately) and is blank elsewhere.
    # 3: Message
    # 4: Rule, Tool

    # There are a couple of other message types (a separator between a warning
    # and a related info message, and also the summary at the end).
    if ($columns.Length -ne 5)
    {
        return
    }


    if ($columns[2] -ne "")
    {
        echo "    <file name=""$($columns[2])"">"
        return
    }

    # The line number is generally of the form line:col but it can also be
    # line:col-line:col or completely blank.
    if ($columns[0] -eq '')
    {
        $line_num, $column = 1, 1
    }
    else
    {
        $line_num, $column = ($columns[0] -split '-')[0] -split ':'
    }

    # This consists of the remark-lint rule and the name remark-lint with
    # a lot of white space in the middle...
    $rule, $tool = $columns[4] -split '\s+'
    # Add the rule to the message for now.
    $message = $columns[3] + ' (' + $rule + ')'
    # If the message has &s and "s in it, we need to replace them
    $message = $message.Replace("&", "&amp;").Replace('"', "&quot;")

    echo "        <error line=""$line_num"" column=""$column"" severity=""$($columns[1])""  message=""$message"" source=""$tool.$rule""/>"
} -end {
    echo "    </file>"
    echo "</checkstyle>"
}
