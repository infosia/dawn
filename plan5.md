
In the file @src/dawn/replay/Replay.cpp

Using the macro DAWN_REPLAY_BINDING_GROUP_LAYOUT_ENTRY_TYPES_ENUM from
@src/dawn/serialization/Schema.h you already generated a visitor using std::visit in the file
@src/dawn/replay/Replay.cpp and you changed the code of CreateBindGroup in the
same file to use this visitor?

Do the same for for ReplayImpl::CreateResource. Use an XMacro to generate
a visitor on a variant to create the resources from the macro DAWN_REPLAY_OBJECT_TYPES.
Separate the deserialization code from the actual creation so that later we
can replace the visitor with other implementations.

I believe, each ObjectType can be associated by name to the schema type for it's
deserialization. On other words, scheme::ObjectType::Buffer maps to schema::Buffer
so the X-Macro generating the visitor can auto generate at least part of the
deserialization code. Some cases might need specialization.

You will have to refactor all of the CreateXX functions so they take a const
reference to their corresponding deserialized type. For example. CreateBuffer
will need to take a const scheme::Buffer& as a parameter.

You could rename all of the CreateXXX functions to just CreateResource overloads
by type. Or, you could have the X-Macro generate names Create ## Type. All of the
functions will need to take the same other parameters like label, device, etc...

Some of the functions are doing extra deserialization. That seems like it arguably needs to move
up to the code calling the visitor to separate the deserialization from the visitor.

DO NOT REFACTOR the commands! Only refactor the code for creating a resource related
to ReplayImpl::CreateResource.

You can compile with `autoninja -C out/Debug dawn_end2end_tests` and you can test
with `./out/Debug/dawn_end2end_tests "--gtest_filter=Capt*"`

Another goal here is that there is as little hand written code as possible so adding
a new type of buffer binding layout entry to Schema.h will just get auto added.

