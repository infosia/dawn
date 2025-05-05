#!/bin/bash
set -euo pipefail

# Version format: vYYYYMMDD.HHMMSS or vYYYYMMDD.HHMMSS-FORKOWNER.dawn.BRANCHNAME
DAWN_REVISION=$(git rev-parse HEAD)
VERSION_DATETIME=$(git show -s --date=format:'%Y%m%d.%H%M%S' --format=%cd)
VERSION_SUFFIX=${GITHUB_REPOSITORY/\//.}.${GITHUB_REF_NAME}
if [[ "$VERSION_SUFFIX" != "google.dawn.main" ]] ; then
    PKG_VERSION=v${VERSION_DATETIME}-${VERSION_SUFFIX}
else
    PKG_VERSION=v${VERSION_DATETIME}
fi
PKG_FILE=emdawnwebgpu_pkg-${PKG_VERSION}.zip

# Initialize dependencies.
git submodule update --init --depth=1 third_party/depot_tools
EMSDK_RELEASE=$(third_party/depot_tools/gclient getdep --var=dawn_emscripten_release)
# We could use gclient to install all deps, but we only need these three.
git submodule update --init --depth=1 third_party/abseil-cpp
git submodule update --init --depth=1 third_party/googletest
git submodule update --init --depth=1 third_party/emsdk
python3 tools/activate-emsdk "${EMSDK_RELEASE}"

# Build
mkdir -p out/wasm
third_party/emsdk/upstream/emscripten/emcmake cmake -S=. -B=out/wasm
make -j4 -C out/wasm emdawnwebgpu_pkg

# Create zip
cat << EOF > out/wasm/emdawnwebgpu_pkg/VERSION.txt
Dawn release ${PKG_VERSION} at revision <https://dawn.googlesource.com/dawn/+/${DAWN_REVISION}>.
Built/tested with emsdk release ${EMSDK_RELEASE}.
EOF
(cd out/wasm && zip -9 -r ../../${PKG_FILE} emdawnwebgpu_pkg)

# Save version numbers for later steps
if [[ -n "${GITHUB_OUTPUT:-}" ]] ; then
    echo "PKG_VERSION=$PKG_VERSION" >> $GITHUB_OUTPUT
    echo "PKG_FILE=$PKG_FILE" >> $GITHUB_OUTPUT
fi
