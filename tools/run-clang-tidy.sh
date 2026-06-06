#!/usr/bin/env bash

cd "$(dirname "$0")"/..

[[ $# != 1 ]] && echo "Usage: $0 out/YOUR_BUILD_DIR" && exit 1

third_party/chromium-tools-build/src/recipes/recipe_modules/tricium_clang_tidy/resources/tricium_clang_tidy_script.py \
    --base_path="$PWD" \
    --out_dir="$1" \
    --findings_file=all_findings.json \
    --clang_tidy_binary="$PWD/third_party/llvm-build/Release+Asserts/bin/clang-tidy" \
    --windows \
    --all
