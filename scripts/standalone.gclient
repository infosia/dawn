# Copy this file to <dawn clone dir>/.gclient to bootstrap gclient in a
# standalone checkout of Dawn.
solutions = [
  {
    "name": "dawn",
    "url": "https://dawn.googlesource.com/dawn.git",
    "deps_file": "DEPS",
    "managed": False,
    "custom_deps": {},
    "custom_vars": {
        "download_remoteexec_cfg": True,
    },
  },
]
target_os = ["mac"]
