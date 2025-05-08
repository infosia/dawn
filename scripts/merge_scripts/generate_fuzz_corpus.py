#!/usr/bin/env python3
#
# Copyright 2025 The Dawn & Tint Authors
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

"""Merge script to generate fuzz corpora for use in Clusterfuzz.

Note that this "merge" script does not actually merge any data - all Dawn
results are handled by ResultDB.
"""

import argparse
import json
import os
import sys
from typing import List

DAWN_ROOT = os.path.realpath(os.path.join(os.path.dirname(__file__), '..', '..'))
TESTING_MERGE_SCRIPTS = os.path.join(DAWN_ROOT, 'testing', 'merge_scripts')
sys.path.insert(0, TESTING_MERGE_SCRIPTS)

try:
    import merge_api
except ImportError as e:
    raise RuntimeError(
        'Unable to import merge_api - are you running in a Chromium checkout? '
        'This merge script only supports standalone Dawn checkouts.') from e


def find_testcases(summary_json: str) -> List[str]:
    with open(summary_json, encoding='utf-8') as infile:
        summary = json.load(f)
    print(summary)
    return []


def generate_and_upload_fuzz_corpus(summary_json: str) -> None:
    test_cases = find_testcases(summary_json)


def main(cmdline_args: List[str]) -> None:
    parser = merge_api.ArgumentParser()
    args = parser.parse_args(cmdline_args)
    generate_and_upload_fuzz_corpus(args.summary_json)


if __name__ == '__main__':
    main(sys.argv[1:])
