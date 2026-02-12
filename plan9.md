
In the file @src/dawn/replay/ReplayImplBase.h and @src/dawn/replay/ReplayImplBase.cpp,
remove `#include dawn/webgpu_cpp.h`. It should not be needed. Only ReplayImpl should
need that. Similarly, the Resource variant. It should not be in ReplayImplBase
as how a Resource is represented should be outside the scope of ReplayImplBase.

You can compile with `autoninja -C out/Debug dawn_end2end_tests` and you can test
with `./out/Debug/dawn_end2end_tests "--gtest_filter=Capt*"`


