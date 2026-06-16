#!/usr/bin/env vpython3

# Copyright 2026 The Dawn & Tint Authors
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""
Script to package up llvm-dev artifacts required by Mesa and optionally upload them to GCS.
This script is intended to be run on Linux only.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import urllib.request
import urllib.error

# Configurable GCS bucket for llvm-dev artifacts
GCS_BUCKET = "chromium-browser-clang/llvm-dev/linux"

# Output filename for the packaged LLVM dev tarball
OUTPUT_FILENAME = "llvm-dev.tar.gz"


def check_os():
    """Ensure the script is running on Linux."""
    if sys.platform != "linux" and not sys.platform.startswith("linux"):
        print("Error: This script is only supported on Linux.",
              file=sys.stderr)
        sys.exit(1)


def get_local_llvm_version(llvm_config_path="llvm-config", is_system=True):
    """Retrieve and sanitize the LLVM version via llvm-config."""
    try:
        output = subprocess.check_output([llvm_config_path, "--version"],
                                         text=True).strip()
    except (subprocess.SubprocessError, FileNotFoundError):
        if is_system:
            print("Error: 'llvm-config' not found on the local machine.",
                  file=sys.stderr)
            print("Please install the llvm-dev package first:",
                  file=sys.stderr)
            print("  sudo apt-get install llvm-dev", file=sys.stderr)
            print("And make sure Mesa builds correctly before proceeding.",
                  file=sys.stderr)
        else:
            print(
                f"Error: Failed to execute {llvm_config_path}. It might not be a valid LLVM pre-built tarball or not built for this architecture.",
                file=sys.stderr)
        sys.exit(1)

    # Clean the version string to extract X.Y.Z
    match = re.search(r"(\d+\.\d+\.\d+)", output)
    if not match:
        print(f"Error: Could not parse LLVM version from output: '{output}'",
              file=sys.stderr)
        sys.exit(1)

    version = match.group(1)
    if is_system:
        print(f"Detected system LLVM version: {version}")
    else:
        print(f"Verified package LLVM version: {version}")
    return version


def download_llvm_release(version, download_dir):
    """Download pre-built LLVM release tarball from GitHub."""
    base_url = f"https://github.com/llvm/llvm-project/releases/download/llvmorg-{version}"
    download_path = os.path.join(download_dir, "llvm-upstream.tar.xz")

    # Generate list of candidate URLs
    urls = [
        f"{base_url}/LLVM-{version}-Linux-X64.tar.xz",
        f"{base_url}/llvm-{version}-Linux-X64.tar.xz",
    ]

    for url in urls:
        print(f"Trying to download: {url}")
        try:
            # Setting a User-Agent to avoid issues with some servers rejecting python-urllib
            req = urllib.request.Request(
                url,
                headers={
                    "User-Agent": "Mozilla/5.0 (Dawn-Tint-LLVM-Packager)"
                })
            with urllib.request.urlopen(req) as response, open(
                    download_path, 'wb') as out_file:
                shutil.copyfileobj(response, out_file)
            print("Successfully downloaded upstream LLVM release.")
            return download_path
        except urllib.error.HTTPError as e:
            if e.code == 404:
                print(f"URL not found (404), trying next fallback...")
            else:
                print(f"HTTP error: {e.code} {e.reason}")
        except urllib.error.URLError as e:
            print(f"Connection error: {e.reason}")

    print(
        f"Error: Failed to find or download pre-built LLVM {version} from GitHub releases.",
        file=sys.stderr)
    sys.exit(1)


def repackage_llvm(upstream_tar_path, output_path, expected_version=None):
    """Extract upstream tarball and repackage only include/, lib/, and bin/llvm-config."""
    with tempfile.TemporaryDirectory() as extract_dir:
        print("Extracting upstream LLVM release...")
        # Since python's tarfile is slow on large .tar.xz and system tar is fast,
        # try to use system 'tar' first, falling back to python's tarfile.
        try:
            subprocess.check_call(
                ["tar", "-xf", upstream_tar_path, "-C", extract_dir])
        except (subprocess.SubprocessError, FileNotFoundError):
            import tarfile
            with tarfile.open(upstream_tar_path, "r:xz") as tar:
                tar.extractall(path=extract_dir)

        # The extracted archive typically contains a single subdirectory
        subdirs = [
            os.path.join(extract_dir, d) for d in os.listdir(extract_dir)
            if os.path.isdir(os.path.join(extract_dir, d))
        ]
        if len(subdirs) != 1:
            print(
                "Error: Extracted archive has an unexpected directory structure.",
                file=sys.stderr)
            print(
                f"Expected exactly 1 root subdirectory, but found {len(subdirs)} subdirectories:",
                file=sys.stderr)
            for d in os.listdir(extract_dir):
                print(f"  - {d}", file=sys.stderr)
            print(
                "This layout change might mean the LLVM release packaging structure has changed.",
                file=sys.stderr)
            sys.exit(1)

        llvm_root = subdirs[0]

        # Verify expected components exist
        for item in ["bin", "include", "lib"]:
            if not os.path.exists(os.path.join(llvm_root, item)):
                print(f"Error: Extracted LLVM is missing '{item}' directory.",
                      file=sys.stderr)
                sys.exit(1)

        # Verify bin/llvm-config exists and extract its version to confirm it is actually an LLVM tarball
        llvm_config_path = os.path.join(llvm_root, "bin", "llvm-config")
        if not os.path.exists(llvm_config_path):
            print(
                "Error: Extracted LLVM is missing 'bin/llvm-config'. It might not be a valid LLVM pre-built tarball.",
                file=sys.stderr)
            sys.exit(1)

        pkg_version = get_local_llvm_version(llvm_config_path, is_system=False)
        if expected_version and pkg_version != expected_version:
            print(
                f"Error: Version mismatch! Expected LLVM version '{expected_version}' but package contains version '{pkg_version}'.",
                file=sys.stderr)
            sys.exit(1)

        print("Filtering and staging repackaged LLVM artifacts...")
        # Create staging dir for clean structure
        staging_dir = os.path.join(extract_dir, "staging")
        os.makedirs(staging_dir)

        # Stage include and lib entirely
        shutil.copytree(os.path.join(llvm_root, "include"),
                        os.path.join(staging_dir, "include"))
        shutil.copytree(os.path.join(llvm_root, "lib"),
                        os.path.join(staging_dir, "lib"))

        # Stage only bin/llvm-config
        os.makedirs(os.path.join(staging_dir, "bin"))
        shutil.copy2(os.path.join(llvm_root, "bin", "llvm-config"),
                     os.path.join(staging_dir, "bin", "llvm-config"))

        print(f"Creating filtered archive '{OUTPUT_FILENAME}'...")
        # Pack staging_dir to output_path using system tar (or tarfile)
        try:
            # Files need to be stored with relative paths inside bin/, include/, lib/
            subprocess.check_call([
                "tar", "-czf", output_path, "-C", staging_dir, "bin",
                "include", "lib"
            ])
        except (subprocess.SubprocessError, FileNotFoundError):
            import tarfile
            with tarfile.open(output_path, "w:gz") as tar:
                tar.add(os.path.join(staging_dir, "bin"), arcname="bin")
                tar.add(os.path.join(staging_dir, "include"),
                        arcname="include")
                tar.add(os.path.join(staging_dir, "lib"), arcname="lib")

        print("Repackaging complete.")


def upload_to_gcs(archive_path):
    """Upload repackaged archive to GCS using upload_to_google_storage.py from depot_tools."""
    print(f"Uploading {archive_path} to GCS bucket: gs://{GCS_BUCKET}")
    script_dir = os.path.dirname(os.path.abspath(__file__))
    dawn_root = os.path.dirname(script_dir)
    upload_script = os.path.join(dawn_root, "third_party", "depot_tools",
                                 "upload_to_google_storage.py")

    if not os.path.exists(upload_script):
        print(
            f"Error: {upload_script} not found. Ensure depot_tools is initialized.",
            file=sys.stderr)
        sys.exit(1)

    cmd = ["vpython3", upload_script, "-b", GCS_BUCKET, archive_path]
    print(f"Running command: {' '.join(cmd)}")
    subprocess.check_call(cmd)
    print("Upload completed successfully.")


def validate_repackaged_archive(archive_path):
    """Verify that a repackaged LLVM archive has the correct layout and can run."""
    if not os.path.exists(archive_path):
        print(f"Error: Package archive does not exist: {archive_path}",
              file=sys.stderr)
        sys.exit(1)

    with tempfile.TemporaryDirectory() as extract_dir:
        print(f"Validating local package: {archive_path}")
        try:
            subprocess.check_call(
                ["tar", "-xf", archive_path, "-C", extract_dir])
        except (subprocess.SubprocessError, FileNotFoundError):
            import tarfile
            with tarfile.open(archive_path, "r:gz") as tar:
                tar.extractall(path=extract_dir)

        # Verify expected root-level components exist directly (no single subdir wrapper)
        for item in ["bin", "include", "lib"]:
            if not os.path.exists(os.path.join(extract_dir, item)):
                print(
                    f"Error: Package is invalid. Missing root-level '{item}' directory.",
                    file=sys.stderr)
                sys.exit(1)

        llvm_config_path = os.path.join(extract_dir, "bin", "llvm-config")
        if not os.path.exists(llvm_config_path):
            print("Error: Package is invalid. Missing 'bin/llvm-config'.",
                  file=sys.stderr)
            sys.exit(1)

        get_local_llvm_version(llvm_config_path, is_system=False)
        print("Package validation succeeded.")


def main():
    parser = argparse.ArgumentParser(
        description="Package and optionally upload llvm-dev artifacts.")
    parser.add_argument(
        "--upload",
        action="store_true",
        help=
        "Upload the generated package to GCS. Default is dry-run mode (local packaging only)."
    )
    parser.add_argument(
        "--local-tarball",
        type=str,
        help=
        "Path to a local LLVM tarball (.tar.xz). Bypasses downloading from GitHub."
    )
    parser.add_argument(
        "--fetch-only",
        action="store_true",
        help=
        "Download the upstream LLVM source tarball to the current repository directory and exit without repackaging."
    )
    parser.add_argument(
        "--upload-local-package",
        type=str,
        help=
        "Path to an already repackaged 'llvm-dev.tar.gz' to validate and upload directly to GCS."
    )
    args = parser.parse_args()

    check_os()

    script_dir = os.path.dirname(os.path.abspath(__file__))
    dawn_root = os.path.dirname(script_dir)
    final_output_path = os.path.join(dawn_root, OUTPUT_FILENAME)

    if args.upload_local_package:
        if args.local_tarball or args.fetch_only or args.upload:
            print(
                "Error: --upload-local-package is mutually exclusive with --local-tarball, --fetch-only, and --upload.",
                file=sys.stderr)
            sys.exit(1)
        validate_repackaged_archive(args.upload_local_package)
        upload_to_gcs(args.upload_local_package)
        print("\n[UPLOAD COMPLETE]")
        print("To use this newly uploaded version in the build, move the ")
        print("generated .sha1 pointer file to its expected location:")
        print(
            f"   mv {args.upload_local_package}.sha1 third_party/llvm-dev/llvm-dev.tar.gz.sha1"
        )
        sys.exit(0)

    if args.fetch_only:
        if args.local_tarball:
            print(
                "Error: --fetch-only and --local-tarball are mutually exclusive.",
                file=sys.stderr)
            sys.exit(1)
        version = get_local_llvm_version()
        print(f"Downloading upstream LLVM release to {dawn_root}...")
        download_llvm_release(version, dawn_root)
        print(
            f"\nSuccessfully fetched upstream tarball to: {os.path.join(dawn_root, 'llvm-upstream.tar.xz')}"
        )
        sys.exit(0)

    if args.local_tarball:
        if not os.path.exists(args.local_tarball):
            print(
                f"Error: Local tarball path does not exist: {args.local_tarball}",
                file=sys.stderr)
            sys.exit(1)
        print(f"Using local LLVM tarball: {args.local_tarball}")
        repackage_llvm(args.local_tarball, final_output_path)
    else:
        version = get_local_llvm_version()
        with tempfile.TemporaryDirectory() as tmp_dir:
            upstream_tar = download_llvm_release(version, tmp_dir)
            repackage_llvm(upstream_tar,
                           final_output_path,
                           expected_version=version)

    print("\n------------------------------------------------------------")
    print(f"Package created successfully: {final_output_path}")
    print("------------------------------------------------------------")

    if args.upload:
        upload_to_gcs(final_output_path)
        print("\n[UPLOAD COMPLETE]")
        print("To use this newly uploaded version in the build, move the ")
        print("generated .sha1 pointer file to its expected location:")
        print(
            f"   mv {final_output_path}.sha1 third_party/llvm-dev/llvm-dev.tar.gz.sha1"
        )
    else:
        print("\n[DRY RUN MODE]")
        print("The package was built locally but NOT uploaded to GCS.")
        print(
            "To test the compilation with this package, manually extract it:")
        print(f"  mkdir -p third_party/llvm-dev")
        print(f"  tar -xzf {OUTPUT_FILENAME} -C third_party/llvm-dev/")
        print("Then run 'gn gen' and compile the Mesa fuzzers.")


if __name__ == "__main__":
    main()
