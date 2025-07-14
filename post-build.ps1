try {
    $shell = New-Object -ComObject "Shell.Application"

    $source_dir = "$($args[0])"
    $source_folder = $shell.NameSpace($source_dir)

    $dest_dir = "C:\Program Files\Notepad++\plugins\Linter++"
    $dest_folder = $shell.NameSpace($dest_dir)
    $dest_folder.CopyHere($source_folder.Items(), 16)
}
catch {
    echo "An error occurred at $_.ScriptStackTrace"
    echo $_
    exit 1
}
