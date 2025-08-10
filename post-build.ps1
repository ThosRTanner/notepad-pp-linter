try {
    $shell = New-Object -ComObject "Shell.Application"

    $source_dir = "$($args[0])"
    $source_folder = $shell.NameSpace($source_dir)

    $platform = "$($args[1])"
    if ($platform -eq "x64") {
        $dest_dir = "C:\Program Files\Notepad++\plugins\Linter++"
    }
    elseif ($platform -eq "Win32") {
        $dest_dir = "C:\Portables\npp.8.8.1.portable\plugins\Linter++"
    }
    else {
        echo "Unknown platform: $platform"
        exit 1
    }
    $dest_folder = $shell.NameSpace($dest_dir)
    $dest_folder.CopyHere($source_folder.Items(), 16)
}
catch {
    echo "An error occurred at $_.ScriptStackTrace"
    echo $_
    exit 1
}
