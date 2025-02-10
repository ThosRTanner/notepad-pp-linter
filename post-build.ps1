try {
	$shell = New-Object -ComObject "Shell.Application"

	$source_dir = "$($args[0])"
	$dll = "$($source_dir)linter++.dll"
	$pdb = "$($source_dir)linter++.pdb"
	echo $dll
	echo $pdb

	$dest_dir = "C:\Program Files\Notepad++\plugins\Linter++"
	$dest_folder = $shell.NameSpace($dest_dir)
	# I'd like to do these with 1 command but powershell doesn't
	# seem to work well if you try to get exotic with filters.
	$dest_folder.CopyHere($dll, 16)
	$dest_folder.CopyHere($pdb, 16)
	# Note - given i've cleaned out the build output directory, I could copy everything now?
}
catch {
	echo "An error occurred at $_.ScriptStackTrace"
	echo $_
	exit 1
}
