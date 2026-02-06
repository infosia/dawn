
In the file @src/dawn/replay/Replay.cpp

Using the macro DAWN_REPLAY_BINDING_GROUP_LAYOUT_ENTRY_TYPES_ENUM from
@src/dawn/serialization/Schema.h generate a visitor using std::visit in the file
@src/dawn/replay/Replay.cpp and change the code of CreateBindGroupLayout in the
same file to use this visitor?

You can compile with `autoninja -C out/Debug dawn_end2end_tests` and you can test
with `./out/Debug/dawn_end2end_test "--gtest_filter=Capt*"`

The goal here is to separate deserialization from usage because eventually we will
want to replace the concrete visitor with an abstract visitor so we can do different
things.

Another goal here is that there is as little hand written code as possible so adding
a new type of buffer binding layout entry to Schema.h will just get auto added.

Note that each schema::BindGroupLayoutEntryType is associated with a data type in
the macro DAWN_REPLAY_MAKE_BINDGROUP_LAYOUT_VARIANT. Since they are associate by
name it should be easy to generate a macro based on
DAWN_REPLAY_BINDING_GROUP_LAYOUT_ENTRY_TYPES_ENUM the generates the visitor
and generates the deserialization code.
