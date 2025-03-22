param (
    [string]$output_dir
)

$output_file = "${output_dir}version.h"

# See if we actually need to update version.h
$intended_ver = "#define GIT_VERSION_STRING `"$(git describe --always --dirty)`""
$current_ver = Get-Content -Path "$output_file" -Tail 1

if ($current_ver -eq $intended_ver)
{
    echo "same version $intended_ver"
    exit
}

# OK, let's work out how we are going to set up the version header.

# We're going to use Semantic versioning (see https://semver.org/)

# This means (roughly) that all versions are of the form

# x.y.z
# x.y.z+<build>
# x.y.z-<prerelease>
# x.y.z-<prerelease>+<build>

# We basically ignore the prerelease and build parts of the tag, and go back to
# the previous x.y.z tag for the major, minor and patch versions.
#
# We then use that and the number of commits since that tag for the 4th fields
# in the dll version header.

# This is the official semver regex, but I've broken it down to make it
# easier to read and adjusted the captures for powershell.
$ver_digits = '0|[1-9]\d*'
$pre_rel_match = "$ver_digits|\d*[a-zA-Z-][0-9a-zA-Z-]*"
$semver_regex = '^' + "(?<major>$ver_digits)\.(?<minor>$ver_digits)\.(?<patch>$ver_digits)" +
    # pre-release
    "(?:-(?<prerelease>(?:$pre_rel_match)(?:\.(?:$pre_rel_match))*))?" +
    # build meta
    "(?:\+(?<buildmetadata>[0-9a-zA-Z-]+(?:\.[0-9a-zA-Z-]+)*))?" +
    '$'

# Find a tag to base ourselves on.
# To find the release, list all the tags, and find the latest one that
# is a non-pre-release tag
# If we can't find an appropriate tag, give up, because technically 0.0.0 is
# a valid release so this could be tagged. Alternatively 
$found = $false
foreach ($tag in git tag -l --sort=-version:refname)
{
    if ($tag -match $semver_regex -and $Matches["prerelease"] -eq $null)
    {
        $found = $true
        break
    }
}

$commits = git rev-list --count HEAD

if ($found)
{
    $major = $Matches['major']
    $minor = $Matches['minor']
    $patch = $Matches['patch']
    $commits = $commits - $(git rev-list --count $tag)
}
else
{
    # This doesn't have a tag. Behave as though we'd been created at 0.0.0
    # and just count the commits in our tree.
    $tag = "0.0.0"
    $major = 0
    $minor = 0
    $patch = 0
}

# For now...
$lines = @(
    "#pragma once",
    "#define GIT_VER_MAJOR $major",
    "#define GIT_VER_MINOR $minor",
    "#define GIT_VER_PATCH $patch",
    "#define GIT_VER_EXTENDED $commits",
    $intended_ver
)

$lines | Out-File -FilePath $output_file
