# ANGLE Force Min Version and No Extensions

Available only on the OpenGL ES backend when running on top of ANGLE.


When enabled, force ANGLE to create context with minimum OpenGLES version and no extension required by compat
mode (GLES 3.1 with no extensions), by setting `EGL_CONTEXT_OPENGL_BACKWARDS_COMPATIBLE_ANGLE` and
`EGL_EXTENSIONS_ENABLED_ANGLE` to `EGL_FALSE`.

See the spec for [EGL_ANGLE_create_context_backwards_compatible](https://chromium.googlesource.com/angle/angle/+/main/extensions/EGL_ANGLE_create_context_backwards_compatible.txt) and [EGL_ANGLE_create_context_extensions_enabled](https://chromium.googlesource.com/angle/angle.git/+/refs/heads/chromium/5996/extensions/EGL_ANGLE_create_context_extensions_enabled.txt)
