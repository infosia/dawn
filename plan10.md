
In the file @src/dawn/replay/ReplayImplBase.h and @src/dawn/replay/ReplayImplBase.cpp,
Make the operator() for DeviceData and InvalidData have defaults so that derived classes
don't have to deal with. Also, if possible, refactor so derived classes do not need
an `operator()(const std::monostate&)`. If possible those should all be handled by
ReplayImplBase.

You can compile with `autoninja -C out/Debug dawn_end2end_tests` and you can test
with `./out/Debug/dawn_end2end_tests "--gtest_filter=Capt*"`
