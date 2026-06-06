# Running clang-tidy

* Add `"checkout_clang_tidy": True` to `.gclient` file in the `custom_vars`
  (or use `standalone-maximal.gclient` which includes this).

  ```py
  "custom_vars": {
    "checkout_clang_tidy": True,
  }
  ```
* `gclient sync`
* Set up a build directory for the configuration you want to run. It doesn't
  need to be built yet. This can be a native build or a cross-compiled build.
* Run `tools/run-tricium-clang-tidy.py --help` for instructions.
* This will output a json file with the full findings, and a txt file with the
  output of the script (which is somewhat readable - search for `warning:`).

  It's recommended to [set up clangd](./development-tips.md) and open a file
  in your IDE to see the errors in context (and to get an autofix button).

For additional info on running clang-tidy locally, see
[these instructions](https://chromium.googlesource.com/chromium/src/+/main/docs/clang_tidy.md).
