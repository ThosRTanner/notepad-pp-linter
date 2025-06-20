# Turn output from ScriptAnalyzer into checkstyle xml
# Requires Invoke-ScriptAnalyzer to be installed in powershell.
$ScriptName = ""
Invoke-ScriptAnalyzer @args | ForEach-Object -begin {
    echo "<?xml version=""1.0"" encoding=""utf-8"" ?>"
    echo "<checkstyle version=""4.3"">"
} -process {
    if ($ScriptName -eq "")
    {
        $ScriptName = $_.ScriptName;
        echo "    <file name=""$ScriptName"">"
    }
    $severity = $_.Severity.toString().tolower()
    if ($severity -eq "parseerror") {
        $severity = "error"
    }

    # If the message has &s and "s in it, we need to replace them
    $message = $_.Message.Replace("&", "&amp;").Replace('"', "&quot;")

    echo "        <error line=""$($_.Line)"" column=""1"" severity=""$severity"" message=""$message)"" source=""pslint.$($_.Rulename)""/>"
} -end {
    if ($ScriptName -ne "")
    {
        echo "    </file>"
    }
    echo "</checkstyle>"
}
