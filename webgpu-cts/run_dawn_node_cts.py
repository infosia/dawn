#!/usr/bin/env python3
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

"""Runs the WebGPU CTS via NodeJS."""

import argparse
import contextlib
import functools
import glob
import logging
import os
import subprocess
import sys

THIS_DIR = os.path.dirname(__file__)
DAWN_ROOT = os.path.realpath(os.path.join(THIS_DIR, '..'))
TOOLS_DIR = os.path.join(DAWN_ROOT, 'tools')
NODE_DIR = os.path.join(DAWN_ROOT, 'third_party', 'node')
CTS_DIR = os.path.join(DAWN_ROOT, 'third_party', 'webgpu-cts')


@functools.cache
def get_node_path() -> str:
    paths = glob.glob(os.path.join('*', 'bin', 'node'), root_dir=NODE_DIR)
    if not paths:
        raise RuntimeError(
            f'Unable to find the node binary under {NODE_DIR}. Is the '
            f'dawn_node gclient variable set?')
    if len(paths) > 1:
        raise RuntimeError(
            f'Found multiple node binaries when one is expected. Found: '
            f'{", ".join(paths)}')
    return os.path.join(NODE_DIR, paths[0])


@functools.cache
def get_npm_command() -> list[str]:
    paths = glob.glob(os.path.join('*', 'lib', 'node_modules', 'npm', 'bin', 'npm-cli.js'), root_dir=NODE_DIR)
    if not paths:
        raise RuntimeError(
            f'Unable to find the npm-cli.js file under {NODE_DIR}. Is the '
            f'dawn_node gclient variable set?')
    if len(paths) > 1:
        raise RuntimeError(
            f'Found multiple npm-cli.js files when one is expected. Found: '
            f'{", ".join(paths)}')
    cmd = [
        get_node_path(),
        os.path.join(NODE_DIR, paths[0]),
    ]
    return cmd


def add_node_to_path() -> None:
    node_install_dir = os.path.dirname(get_node_path())
    new_path = os.pathsep.join([node_install_dir, os.environ['PATH']])
    os.environ['PATH'] = new_path


def install_npm_deps_in_current_dir() -> None:
    logging.info('Running "npm ci" in %s', os.getcwd())
    cmd = get_npm_command() + [
        'ci',
    ]
    subprocess.run(cmd, check=True)


def run_node_cts(output_directory: str, args_to_forward: list[str]) -> None:
    logging.info('Running CTS via node in %s', os.getcwd())
    cmd = [
        sys.executable,
        os.path.join(TOOLS_DIR, 'run.py'),
        'run-cts',
        '-bin',
        output_directory,
    ] + args_to_forward
    subprocess.run(cmd, check=True)


def main() -> None:
    logging.getLogger().setLevel(logging.INFO)
    parser = argparse.ArgumentParser('Runs the WebGPU CTS via NodeJS')
    parser.add_argument(
        # We manually forward this argument so that we can use os.getcwd()
        # to work easily on the bots. Passing in "." will not work since
        # we change directories before invoking the underlying runner.
        '--output-directory',
        default=os.getcwd(),
        help='Output directory to use. Passed to the underlying runner as -bin.')
    parser.add_argument(
        '--isolated-script-test-output',
        help='Currently unused, needed for bot support.')
    parser.add_argument(
        '--isolated-script-test-perf-output',
        help='Currently unused, needed for bot support.')
    args, unknown_args = parser.parse_known_args()

    with contextlib.chdir(CTS_DIR):
        add_node_to_path()
        install_npm_deps_in_current_dir()
        run_node_cts(args.output_directory, unknown_args)


if __name__ == '__main__':
    main()
