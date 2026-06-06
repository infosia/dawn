# Running clang-tidy

* Add `"checkout_clang_tidy": True` to `.gclient` file in the `custom_vars`
  (or use `standalone-maximal.gclient` which includes this).

  ```py
  {
    "custom_vars": {
      "checkout_clang_tidy": True,
    }
  }
  ```
* `gclient sync`
* Set up a build directory for the configuration you want to run. It doesn't
  need to be built yet. This can be a native build or a cross-compiled build.
* Run `tools/run-clang-tidy-on.py --help` for instructions.

For additional info on running clang-tidy locally, see
[these instructions](https://chromium.googlesource.com/chromium/src/+/main/docs/clang_tidy.md).
