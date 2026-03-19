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
directory whose name encodes the version (e.g. agility_sdk_1.619.2).
gclient invokes this script via a hook in //DEPS, and the per-version stamp
file inside the output directory ensures we re-download only when the
expected version changes.

The SDK version constant used by Dawn at compile time lives in C++ code
(`kAgilitySDKVersion` in src/dawn/native/d3d12/AgilitySDK.h) and must be kept
in sync with the version pinned in //DEPS and BUILD.gn.
"""

import argparse
import io
import os
import sys
import urllib.request
import zipfile

package_id = "Microsoft.Direct3D.D3D12"
DEFAULT_VERSION = "1.619.2"
DEFAULT_OUTPUT_BASE = os.path.join("third_party", "agility_sdk")


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

    try:
        with urllib.request.urlopen(nuget_url, timeout=120) as response:
            if response.status != 200:
                print(f"ERROR: HTTP {response.status} when downloading "
                      f"{nuget_url}")
                sys.exit(1)
            total = response.headers.get("Content-Length")
            total = int(total) if total else None
            chunks = []
            downloaded = 0
            while True:
                chunk = response.read(64 * 1024)
                if not chunk:
                    break
                chunks.append(chunk)
                downloaded += len(chunk)
                if total:
                    pct = downloaded * 100 // total
                    print(
                        f"\r  {downloaded // 1024}KB / {total // 1024}KB ({pct}%)",
                        end="",
                        flush=True)
                else:
                    print(f"\r  {downloaded // 1024}KB downloaded",
                          end="",
                          flush=True)
            print()
            package_bytes = io.BytesIO(b"".join(chunks))
    except Exception as e:
        print(f"ERROR: Failed to download: {e}")
        sys.exit(1)

    print(f"Extracting to {output_path} ...")
    os.makedirs(output_path, exist_ok=True)
    with zipfile.ZipFile(package_bytes, "r") as zip_ref:
        zip_ref.extractall(output_path)

    # Write stamp file so we don't re-download next time
    with open(stamp, "w") as f:
        pass

    print("Done.")


def main():
    parser = argparse.ArgumentParser(
        description="Download the D3D12 Agility SDK for Dawn.")
    parser.add_argument(
        "--version",
        default=DEFAULT_VERSION,
        help=f"Agility SDK version to download (default: {DEFAULT_VERSION})")
    parser.add_argument(
        "--output",
        default="",
        help=(
            "Output directory.  Defaults to "
            f"'{DEFAULT_OUTPUT_BASE}_<version>' so that bumping the version "
            "creates a new sibling directory and gclient picks up the change."
        ))
    parser.add_argument("--force",
                        action="store_true",
                        help="Force re-download even if already present")
    parser.add_argument(
        "--stamp",
        default="",
        help="Path to write a stamp file on success (used by GN)")
    args = parser.parse_args()

    version = args.version

    output = args.output or f"{DEFAULT_OUTPUT_BASE}_{version}"

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
