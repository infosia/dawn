
In the file @src/dawn/replay/Replay.cpp and @src/dawn/replay/ReplayImpl.cpp,
separate ReplayImplBase into it's own file @src/dawn/replay/ReplayImplBase.h
and @src/dawn/replay/ReplayImplBase.cpp

Put all the abstract visitor support in those files and have Replay.cpp
keep only the implementation of ReplyImpl and the dawn related classes
like DawnResourceVisitor, DawnRootCommandVisitor, etc...

You will need to update @src/dawn/replay/BUILD.gn to and @src/dawn/replay/CMakeLists.txt
to include these new files.

You can compile with `autoninja -C out/Debug dawn_end2end_tests` and you can test
with `./out/Debug/dawn_end2end_tests "--gtest_filter=Capt*"`


