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
""" Script for generating the various Tint fuzzer corpora.

The basic functionality is to collect all .wgsl files under a given directory
and copy them to a given directory, flattening their file names by replacing
path separators with underscores. If the output directory already exists, it
will be deleted and re-created. Files ending with ".expected.wgsl" are
skipped.

Additional flags can be used for providing tooling for converting this corpus
of .wgsl  files into Tint IR fuzzer protobufs (.tirb) or for minimizing the
corpora.

usage: generate_tint_corpus.py [-h]
                               [--wgsl_fuzzer WGSL_FUZZER]
                               [--ir_as IR_AS]
                               [--ir_fuzzer IR_FUZZER]
                               [-v, --verbose]
                               [--debug]
                               input_dir output_dir

Generates Tint fuzzer corpora from provided test files, using the provided
tools.

positional arguments:
  input_dir             Directory containing source WGSL (.wgsl) files to be
                        turned into corpora, non-WGSL and '*.expected.wgsl`
                        files will be ignored
  output_dir            Output directory that the results should be placed in.
                        The base WGSL fuzzer corpus is always created, in
                        '<output_dir>/wgsl'. Other corpora locations will be
                        specified in their tool specific flags. If a directory
                        already exists, it will be overwritten.

options:
  -h, --help            show this help message and exit
  --wgsl_fuzzer WGSL_FUZZER
                        Instance of tint_wgsl_fuzzer to use for minimization, if
                        present a minimized corpus will be generated in
                        '<output_dir>/wgsl_min'.
                        (This can take over an hour to run.)
  --ir_as IR_AS         Instance of ir_fuzz_as to use for assembling IR binary
                        test cases, if present an IR corpus will be generated in
                        '<output_dir/ir'
  --ir_fuzzer IR_FUZZER
                        Instance of tint_ir_fuzzer to use for minimization, if
                        this and --ir_as are present a minimized corpus will be
                        generated in '<output_dir>/ir_min'.
                        (This can take over an hour to run.)
  -v, --verbose         Enables verbose logging
  --debug               Enables developer debug logging
"""

import argparse
import logging
import os
import pathlib
import shutil
import subprocess
import sys
from typing import TypedDict


class Options(TypedDict):
    """Container of all the control options for the script"""
    wgsl_fuzzer_bin: str | None
    ir_as_bin: str | None
    ir_fuzzer_bin: str | None
    input_dir: str
    output_dir: str
    wgsl_corpus_dir: str
    wgsl_min_corpus_dir: str
    ir_corpus_dir: str
    ir_min_corpus_dir: str


logger = logging.getLogger(__name__)


def parse_args():
    """Parse command line arguments and produce control options structure.
    Returns:
        A populated Options structure.
        """
    parser = argparse.ArgumentParser(
        prog='generate_tint_corpus.py',
        description=
        'Generates Tint fuzzer corpora from provided test files, using the provided tools.'
    )
    parser.add_argument(
        'input_dir',
        help=
        "Directory containing source WGSL (.wgsl) files to be turned into corpora, non-WGSL and '*.expected.wgsl` files will be ignored",
        type=str)
    parser.add_argument(
        'output_dir',
        help=
        "Output directory that the results should be placed in. The base WGSL fuzzer corpus is always created, in '<output_dir>/wgsl'. Other corpora locations will be specified in their tool specific flags. If a directory already exists, it will be overwritten.",
        type=str)
    parser.add_argument(
        '--wgsl_fuzzer',
        help=
        "Instance of tint_wgsl_fuzzer to use for minimization, if present a minimized corpus will be generated in '<output_dir>/wgsl_min'. (This can take over an hour to run.)",
        type=str)
    parser.add_argument(
        '--ir_as',
        help=
        "Instance of ir_fuzz_as to use for assembling IR binary test cases, if present an IR corpus will be generated in '<output_dir/ir'",
        type=str)
    parser.add_argument(
        '--ir_fuzzer',
        help=
        "Instance of tint_ir_fuzzer to use for minimization, if this and --ir_as are present a minimized corpus will be generated in '<output_dir>/ir_min'. (This can take over an hour to run.)",
        type=str)
    parser.add_argument(
        '--wgsl_merge_control_file',
        help=
        'File passed through to libfuzzer, that allows for restarting WGSL minimization part way through, see https://llvm.org/docs/LibFuzzer.html#id29 for details',
        type=str)
    parser.add_argument(
        '--ir_merge_control_file',
        help=
        'File passed through to libfuzzer, that allows for restarting IR minimization part way through, see https://llvm.org/docs/LibFuzzer.html#id29 for details',
        type=str)
    parser.add_argument('-v',
                        '--verbose',
                        help="Enables verbose logging",
                        action="store_const",
                        dest="loglevel",
                        const=logging.INFO)
    parser.add_argument('--debug',
                        help="Enables developer debug logging",
                        action="store_const",
                        dest="loglevel",
                        const=logging.DEBUG)
    args = parser.parse_args()
    logging.basicConfig(level=args.loglevel)
    logger.debug(vars(args))

    output_dir: str = os.path.abspath(args.output_dir)

    options: Options = {
        "wgsl_fuzzer_bin":
        check_binary_accessible(args.wgsl_fuzzer,
                                "--wgsl_fuzzer=<tint_wgsl_fuzzer>"),
        "ir_as_bin":
        check_binary_accessible(args.ir_as, "--ir_as=<ir_fuzz_as>"),
        "ir_fuzzer_bin":
        check_binary_accessible(args.ir_fuzzer,
                                "--ir_fuzzer=<tint_ir_fuzzer>"),
        "input_dir":
        os.path.abspath(args.input_dir.rstrip(os.sep)),
        "output_dir":
        output_dir,
        "wgsl_corpus_dir":
        os.path.join(output_dir, "wgsl"),
        "wgsl_min_corpus_dir":
        os.path.join(output_dir, "wgsl_min"),
        "ir_corpus_dir":
        os.path.join(output_dir, "ir"),
        "ir_min_corpus_dir":
        os.path.join(output_dir, "ir_min"),
    }

    if options["ir_fuzzer_bin"] and not options["ir_as_bin"]:
        logger.critical(
            "Cannot specify '--ir_fuzzer' without specifying '--ir_as'")
        sys.exit(1)

    return options


def list_wgsl_files(root_search_dir):
    """Lists all the interesting WGSL shader files beneath a root directory.

    Args:
        root_search_dir (str): The directory to search for WGSL files in.

    Returns:
        Yields path to any file ending with .wgsl, but not .expected.wgsl
    """
    for root, folders, files in os.walk(root_search_dir):
        for filename in folders + files:
            if pathlib.Path(filename).suffix == '.wgsl':
                if filename.endswith(".expected.wgsl"):
                    logger.debug(f"Skipping {filename}")
                    continue
                yield os.path.join(root, filename)


def check_binary_accessible(bin_filename, log_text):
    """Check if a binary file exists and accessible.

    Args:
        bin_filename (str): The filename of the binary file to check.
        log_text (str): String describing the related flag for error messages.

    Returns:
        bin_filename if it is executable & accessible, or None if it is None. Causes a fatal error if bin_filename is not None, but not executable & accessible.
    """
    if not bin_filename:
        return None

    which_bin = shutil.which(bin_filename, mode=os.F_OK | os.X_OK, path='.')
    if not which_bin:
        logger.critical(f"Unable to run {log_text} cmd: {bin_filename}")
        sys.exit(1)
    return which_bin


def create_clean_dir(dirname):
    """Makes sure there is an empty directory at location.

    Will remove the directory if it already exists and recreate it.

    Args:
        dirname (str): The directory to create.
        """
    if os.path.exists(dirname):
        shutil.rmtree(dirname)
    os.makedirs(dirname)


def touch_stamp_file(output_dir, task_name):
    """Touches a stamp file to record when a task completed.

    Args:
        output_dir (str): The directory to touch the stamp file in.
        task_name (str): The name of the task being stamped for.
        """
    stamp_file = os.path.join(output_dir, f"{task_name}.stamp")
    logger.debug(f"Touching {task_name}.stamp")
    pathlib.Path(stamp_file).touch(mode=0o644, exist_ok=True)


def main():
    options = parse_args()

    logger.info("Tint fuzzer corpus generation begun")

    logger.info(
        f"Generating WGSL corpus to \'{options['wgsl_corpus_dir']}\' ...")
    create_clean_dir(options["wgsl_corpus_dir"])

    for in_file in list_wgsl_files(options["input_dir"]):
        out_file: str = in_file[len(options["input_dir"]) + 1:].replace(
            os.sep, '_')
        logger.debug("Copying " + in_file + " to " + out_file)
        shutil.copy(in_file, os.path.join(options["wgsl_corpus_dir"],
                                          out_file))

    touch_stamp_file(options["output_dir"], "wgsl")
    logger.info("Finished generating WGSL corpus")

    if options["wgsl_fuzzer_bin"]:
        logger.info(
            f"Minimizing WGSL corpus to \'{options['wgsl_min_corpus_dir']}\' (this will take a while) ..."
        )
        create_clean_dir(options["wgsl_min_corpus_dir"])

        # libFuzzer uses TO FROM args for merging/minimization
        min_cmd = [
            options["wgsl_fuzzer_bin"], '-merge=1',
            options["wgsl_min_corpus_dir"], options["wgsl_corpus_dir"]
        ]
        logger.info(f"Invoking \'{' '.join(min_cmd)}\'")
        subprocess.run(min_cmd, stderr=subprocess.STDOUT)

        touch_stamp_file(options["output_dir"], "wgsl_min")
        logger.info("Finished minimizing WGSL corpus")

    if options["ir_as_bin"]:
        logger.info(
            f"Generating IR corpus to \'{options['ir_corpus_dir']}\' ...")
        create_clean_dir(options["ir_corpus_dir"])

        gen_cmd = [
            options["ir_as_bin"], options["wgsl_corpus_dir"],
            options["ir_corpus_dir"]
        ]
        logger.info(f"Invoking \'{' '.join(gen_cmd)}\'")
        subprocess.run(gen_cmd, stderr=subprocess.STDOUT)

        touch_stamp_file(options["output_dir"], "ir")
        logger.info("Finished generating IR corpus")

        if options["ir_fuzzer_bin"]:
            logger.info(
                f"Minimizing IR corpus to \'{options['ir_min_corpus_dir']}\' (this will take a while) ..."
            )
            create_clean_dir(options["ir_min_corpus_dir"])

            # libFuzzer uses TO FROM args for merging/minimization
            min_cmd = [
                options["ir_fuzzer_bin"], '-merge=1',
                options["ir_min_corpus_dir"], options["ir_corpus_dir"]
            ]
            logger.info(f"Invoking \'{' '.join(min_cmd)}\'")

            subprocess.run(min_cmd, stderr=subprocess.STDOUT)

            touch_stamp_file(options["output_dir"], "ir_min")
            logger.info("Finished minimizing IR corpus")

    logger.info("Tint fuzzer corpus generation completed")


if __name__ == "__main__":
    sys.exit(main())
