
In the file @src/dawn/replay/Replay.cpp

Refactor the ProcessEncoderCommands, ProcessRenderPassCommands,
ProcessComputePassCommands, ProcessRenderBundleCommands, ProcessRenderCommand,
ProcessDebugCommands, ProcessSharedCommands using the X-Macro preceding
each function.

Make it so the deserialization is separate from usage using variants and visitors.
The deserialization could should not directly reference a device. The device
should only be used by the visitor.

Each Command can be associated by name to the schema type for its
deserialization. On other words, scheme::CommandBufferCommand::Dispatch maps to schema::CommandBufferCommandDispatchCmdData so the X-Macro generating the visitor
and the deserialization code can auto generate at least part of the
deserialization code. Some cases might need specialization.

Making one abstract visitor for all command is ok but you will need concrete visitors
for different situations as the visitor will need different data. For example
the render pass commands need access to a wgpu::RenderPassEncoder, the compute pass
commands need access to a wgpu::ComputePassEncoder, the encoder commands need
access to a wgpu::CommandEncoder, etc...

DO NOT MAKE CODE MACROS! A Macro for X-Macro expansion is ok. A macro per command
is NOT.

You can compile with `autoninja -C out/Debug dawn_end2end_tests` and you can test
with `./out/Debug/dawn_end2end_tests "--gtest_filter=Capt*"`

Another goal here is that there is as little hand written code as possible so adding
a new command entry just get auto added.

