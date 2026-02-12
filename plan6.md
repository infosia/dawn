
In the file @src/dawn/replay/Replay.cpp

Using the X-macro DAWN_REPLAY_ROOT_COMMANDS, Use an XMacro to generate
a visitor and a variant to process the root commands in ReplayImpl::Play
Separate the deserialization code from the actual usage so that later we
can replace the visitor with other implementations.

Each RootCommand can be associated by name to the schema type for it's
deserialization. On other words, scheme::RootCommand::WriteBuffer maps to schema::RootCommandWriteBufferCmdData
so the X-Macro generating the visitor can auto generate at least part of the
deserialization code. Some cases might need specialization.

DO NOT MAKE A MACRO PER COMMAND. Macros supporting X-Macros are ok.

You can compile with `autoninja -C out/Debug dawn_end2end_tests` and you can test
with `./out/Debug/dawn_end2end_tests "--gtest_filter=Capt*"`

Another goal here is that there is as little hand written code as possible so adding
a new type of buffer binding layout entry to Schema.h will just get auto added.

