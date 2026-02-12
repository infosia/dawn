
In the file @src/dawn/replay/Replay.cpp

Rename RootCommandVisitor to DawnRootCommandVisitor.
Then make RootCommandVisitor be an abstract visitor that DawnRootCommandVisitor
is based on. Then make ReplayImpl::Play take a reference to a RootCommandVisitor

You will need to rename ReplayImpl to ReplayImplBase and then make
a new ReplayImpl class that a ReplayImplBase and creates a DawnRootCommandVisitor
and passes it into ReplayImplBase::Play. ReplayImpl should not inherit from ReplayImplBase.
They are 2 distinct classes but we need code outside to still compile.

RootCommandVisitor should be used, when appropriate, to return other
abstract visitors. For example, in order to call CreateResource you
should pass in an abstract ResourceVisitor. The DawnRootCommandVisitor
will need to provide a concrete DawnResourceVisitor.
There should be no mention of wgpu::Device nor other wgpu objects
outside of the various Dawn visitors.

Note that ReplayImplBase should NOT have GetObjectByLabel and GetObjectById nor GetLabel.
All of those should exist only on DawnRootCommandVisitor and ReplayImpl should call
its instance of a DawnRootCommandVisitor to provide those functions.

That also means mResource, mResourceToIdMap, mDevice, and mBlitBufferToDepthTexture
move to DawnRootCommandVisitor. They should not be needed on ReplayImpl nor ReplayImplBase

This pattern will need to repeat for other lower level visitors.

The goal is to make it possible to put DawnRootCommandVisitor and all
other Dawn related visitors as external to Replay.cpp and the ReplayImplBase
class. Do not put them in a separate file in yet.

You can compile with `autoninja -C out/Debug dawn_end2end_tests` and you can test
with `./out/Debug/dawn_end2end_tests "--gtest_filter=Capt*"`


