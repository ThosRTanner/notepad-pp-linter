# Parses output from markdownlint-cli2.
$filename = Split-Path $args[-1] -Leaf

(& "$Env:UserProfile\node_modules\.bin\markdownlint-cli2.cmd" @args |
    Tee-object -Variable 'String' | Out-Null) 2>&1 | ForEach-Object -begin {
    echo "<?xml version=""1.0"" encoding=""utf-8"" ?>"
    echo "<checkstyle version=""4.3"">"
    echo "    <file name=""$filename"">"
} -process {
    # These lines are easy to parse.
    # filename:l[:c] MDnnn/rule message
    $line = $_.ToString().Substring($filename.Length + 1)

    # Split on the 1st 2 spaces
    $location, $rule, $message = $line.Split(" ", 3)

    $line_num, $column = $location -split ":"
    if ($column -eq $None) {
        $column = 0
    }

    # Add the rule to the end for now
    $message = $message + " (" + $rule + ")"

    # If the message has &s and "s in it, we need to replace them
    $message = $message.Replace("&", "&amp;").Replace('"', "&quot;")

    echo "        <error line=""$line_num"" column=""$column"" severity=""warning"" message=""$message"" source=""markdownlint""/>"
} -end {
    echo "    </file>"
    echo "</checkstyle>"
}
