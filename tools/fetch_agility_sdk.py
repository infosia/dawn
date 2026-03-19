#!/usr/bin/python3
#
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
Downloads and extracts the D3D12 Agility SDK NuGet package for use in Dawn.

Usage:
    python3 tools/fetch_agility_sdk.py [--version <version>] [--output <path>] [--force]

The script downloads a specific, versioned NuGet package and places it into a
directory whose name is "AgilitySDK" under third_party/.
gclient invokes this script via a hook in //DEPS, and the per-version stamp
file inside the output directory ensures we re-download only when the
expected version changes.
"""

import argparse
import os
import shutil
import sys
import tempfile
import urllib.request
import zipfile

package_id = "Microsoft.Direct3D.D3D12"
DEFAULT_OUTPUT_BASE = os.path.join("third_party", "AgilitySDK")


def download_and_extract(force, version, output_path):
    """Download the Agility SDK NuGet package and extract it."""
    stamp = os.path.join(output_path, f"{version}.stamp")

    if os.path.exists(stamp) and not force:
        print(f"Agility SDK {version} already present at {output_path} "
              f"(use --force to re-download)")
        return

    nuget_url = (f"https://www.nuget.org/api/v2/package/"
                 f"{package_id}/{version}")
    print(f"Downloading {package_id} v{version} from {nuget_url} ...")

    def _progress(block_num, block_size, total_size):
        downloaded = block_num * block_size
        if total_size > 0:
            pct = min(downloaded * 100 // total_size, 100)
            print(
                f"\r  {downloaded // 1024}KB / {total_size // 1024}KB ({pct}%)",
                end="",
                flush=True)
        else:
            print(f"\r  {downloaded // 1024}KB downloaded",
                  end="",
                  flush=True)

    with tempfile.NamedTemporaryFile(suffix=".nupkg", delete=False) as tmp:
        tmp_path = tmp.name
    try:
        urllib.request.urlretrieve(nuget_url, tmp_path,
                                  reporthook=_progress)
        print()

        if os.path.exists(output_path):
            shutil.rmtree(output_path)

        print(f"Extracting to {output_path} ...")
        os.makedirs(output_path, exist_ok=True)
        with zipfile.ZipFile(tmp_path, "r") as zip_ref:
            zip_ref.extractall(output_path)
    finally:
        os.unlink(tmp_path)

    # Write stamp file so we don't re-download next time
    with open(stamp, "w") as f:
        pass

    print("Done.")


def main():
    parser = argparse.ArgumentParser(
        description="Download the D3D12 Agility SDK for Dawn.")
    parser.add_argument(
        "--version",
        required=True,
        help="Agility SDK version to download (e.g. '1.619.2').")
    parser.add_argument("--output",
                        default="",
                        help=("Output directory.  Defaults to "
                              f"'{DEFAULT_OUTPUT_BASE}'."))
    parser.add_argument("--force",
                        action="store_true",
                        help="Force re-download even if already present")
    parser.add_argument(
        "--stamp",
        default="",
        help="Path to write a stamp file on success (used by GN)")
    args = parser.parse_args()

    version = args.version

    output = args.output or DEFAULT_OUTPUT_BASE

    if os.path.isabs(output):
        output_path = output
    else:
        output_path = os.path.join(os.getcwd(), output)

    download_and_extract(args.force, version, output_path)

    # Write GN stamp file if requested
    if args.stamp:
        os.makedirs(os.path.dirname(args.stamp) or ".", exist_ok=True)
        with open(args.stamp, "w") as f:
            f.write(version)

    return 0


if __name__ == "__main__":
    sys.exit(main())
