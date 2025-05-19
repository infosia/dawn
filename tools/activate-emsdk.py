#!/usr/bin/env python3

# Copyright 2025 Google LLC
#
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import json
import os
import subprocess
import sys
import sysconfig
import textwrap

EMSDK_ROOT = os.path.join('third_party', 'emsdk')

EMSDK_PATH = os.path.join(EMSDK_ROOT, 'emsdk.py')
EMSDK_CONFIG_PATH = os.path.join(EMSDK_ROOT, '.emscripten')

# This is only used by --get-emsdk-version, but load it here anyway
# just to make sure it doesn't break.
with open(os.path.join(EMSDK_ROOT, 'emscripten-releases-tags.json')) as f:
    emsdk_version = json.load(f)['aliases']['latest']


# Run subprocess with stderr forwarded to stdout. Raises an execption if the subprocess fails.
def run_or_exit(name, args):
    try:
        subprocess.run(args,
                       check=True,
                       stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as ex:
        print(name, 'failed:')
        sys.stdout.buffer.write(ex.stdout)
        sys.exit(1)


def main(args):
    parser = argparse.ArgumentParser()
    parser.add_argument('--get-emsdk-version',
                        help='Print emsdk version and exit.',
                        action='store_true')
    options = parser.parse_args()

    if options.get_emsdk_version:
        print(emsdk_version)
        return

    # Install and activate the default dependencies for emsdk. 'latest' is a
    # static alias in emsdk for the toolchain corresponding to this emsdk
    # release, so we can use it stably:
    # https://github.com/emscripten-core/emsdk/blob/main/emscripten-releases-tags.json#L3
    run_or_exit('emsdk install',
                [sys.executable, EMSDK_PATH, 'install', 'latest'])
    run_or_exit('emsdk activate',
                [sys.executable, EMSDK_PATH, 'activate', 'latest'])

    # If we ever need to override the Python, Node, and LLVM binaries used by Emscripten, do that
    # here. See https://dawn-review.googlesource.com/c/dawn/+/238814/4..5 which removed support.


if __name__ == '__main__':
    main(sys.argv)
