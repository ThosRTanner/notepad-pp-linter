"""gen_npp_release_info - Generate version info details for nppPluginList.

Usage:
    gen_npp_release_info.py (-h | --help)
    gen_npp_release_info.py --info <INFO>

Options:
    -h --help       Show this help message and exit.
    --info <INFO>   A JSON file containing the necessary fields for the
                    nppPluginList files.

This dumps the INFO json file and details from the specified label to
STDOUT, and that is then used to update the nppPluginList repository via
the version_updated workflow.

The INFO file should contain the fields required by the nppPluginList files:

author
description
display-name
folder-name
homepage

and optionally

npp-compatible-versions

Other information is loaded from the GIBHUB_OBJECT environment variable
which is presumed to have been set by the workflow.
"""

import json
import os
import subprocess
import sys

from docopt import docopt
from ghapi.all import GhApi
import github3
import smart_open

def read_info_file(info_file: str) -> dict:
    """Read the info file and return it as a dictionary."""
    with open(info_file, "r") as f:
        info = json.load(f)
    return info

def read_github_environment_variable() -> dict[str, str]:
    """Return the GITHUB_OBJECT environment variable as a dictionary."""
    return json.loads(os.environ["GITHUB_OBJECT"])

def process_command_line_arguments():
    """Process command line arguments and return them as a dictionary."""
    args = docopt(__doc__, default_help=False, options_first=True)
    if args["--help"]:
        print(__doc__)
        sys.exit(1)
    return args

def main():
    """Main function to generate info for nppPluginList."""
    args = process_command_line_arguments()

    github_object = read_github_environment_variable()

    owner, repo_name = github_object["repository"].split("/")
    release_tag = github_object["event"]["release"]["tag_name"]

    gh3 = github3.login(token=github_object["token"])
    repo = gh3.repository(owner, repo_name)
    release = repo.release_from_tag(release_tag)

    info = read_info_file(args["--info"])
    info["files"] = {}
    info["release"] = release_tag

    # Unfortunately we need to use both github3 and ghapi to get all the
    # information we need, as github3 doesn't support actions yet.
    gh_api = GhApi(token=github_object["token"], owner=owner, repo=repo_name)

    # A note: The data is stored compressed in github, but the sha256 is based
    # on the uncompressed data, so if we are going to validate the download is
    # correct, we need to decompress it before we can calculate the sha256 and
    # compare it to the value in the release assets. HOWEVER, when we store the
    # file again, we need to store it compressed, as that's what notepad++
    # expects. This results in the sha256 changing.
    #
    # So I'm not validating it...

    # note: this doesn't appear to get intercepted by nekos act, so need to
    # patch the parameter.
    for artifact in gh_api.actions.list_workflow_run_artifacts(
           github_object["run_id"]
    )["artifacts"]:
        with smart_open.open(
            artifact["archive_download_url"],
            "rb",
            transport_params={
                "headers": {
                    "Authorization": f'Bearer {github_object["token"]}'
                }
            }
        ) as compressed_data:
            asset_data = compressed_data.read()

        release.upload_asset(
            "application/zip", f'{artifact["name"]}.zip', asset_data
        )        

    notes = """
# Plugin Checksums

| File | SHA-256 |
| - | - |
"""

    for asset in release.assets():
        arch = asset.name[11:-4].lower()
        info["files"]["x86" if arch == "win32" else arch] = {
            "id": asset.digest[7:],
            "repository": asset.browser_download_url,
        }
        notes += f"| {asset.name} | {asset.digest[7:].upper()} |\n"

    release.edit(body=notes[1:])

    # Finally, print the info to STDOUT, which will be used to update the
    # nppPluginList repository.
    print(f"info={json.dumps(info)}")

if __name__ == "__main__":
    main()
