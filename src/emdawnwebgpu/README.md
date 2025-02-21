# "emdawnwebgpu" (Dawn's fork of Emscripten's WebGPU bindings)

Dawn temporarily maintains a fork of the Emscripten WebGPU bindings
(`library_webgpu.js` and friends). The forked files live in
[`//third_party/emdawnwebgpu`](../third_party/emdawnwebgpu/)
and the build targets in this directory produce the other files needed to build
an Emscripten-based project using these bindings.

This allows the the webgpu.h interface to be kept roughly in sync\* between the
two implementations in a single place (the Dawn repository) instead of two,
while also avoiding constantly breaking the version of webgpu.h that is
currently in Emscripten. (\* Note we don't guarantee it will always be in sync,
though - we don't have any automated testing for this, so we'll periodically fix
it up as needed for import into other projects that use these bindings.)

There are two possible futures for this fork (see https://crbug.com/371024051):

- We'll un-fork back into Emscripten, replacing the outdated bindings there,
  in one big breaking change.
- We'll set up automation to publish it in a downloadable package form, that
  can be linked into Emscripten as external bindings, and eventually delete
  the old version of the bindings in Emscripten.

Until then, projects should use this fork (by compiling Dawn as instructed
below) if they want the latest version, which mostly compatible with the same
version of Dawn Native.

## Setting up Emscripten

- Get an emsdk toolchain (at least Emscripten 4.0.3):
  [instructions](https://emscripten.org/docs/getting_started/downloads.html#installation-instructions-using-the-emsdk-recommended).
  (Note getting a separate copy of the Emscripten source is no longer necessary!)

## Building the bindings

First, get the Dawn code and its dependencies.
See [building.md](../../docs/building.md).

You can either build the bindings "standalone" and link them manually,
or if your project uses CMake you can link to it as a CMake subproject.

### Using Dawn as a CMake subproject (bindings only)

TODO(crbug.com/371024051): Write a sample!

### Using pre-built emdawnwebgpu bindings via CMake (bindings only)

TODO(crbug.com/371024051): Make pre-built bindings and set up this sample!

### Standalone with CMake (bindings and samples)

Set up the build directory using emcmake:

```
mkdir out/cmake-wasm
cd out/cmake-wasm

# Make sure the path is to the source checkout of Emscripten, not emsdk's release.
emcmake cmake ../..

make -j8
```

(To use Ninja instead of Make, for better parallelism, add `-GNinja` to the
`cmake` invocation, and build using `ninja`.)

The resulting html files can then be served and viewed in a compatible Browser.

### Standalone with GN (bindings only)

- Set up a Dawn GN build, with `dawn_emscripten_dir` in the GN args set to point to
  your Emscripten source checkout.

- Build the `emdawnwebgpu` GN build target.

- Configure the Emscripten build with all of the linker flags listed in `emdawnwebgpu_config`
  (and without `-sUSE_WEBGPU`, because we don't want the built-in bindings).
