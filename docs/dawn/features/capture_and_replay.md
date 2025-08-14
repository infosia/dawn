# Capture and Replay

We would like to add a capture and replay feature to dawn.
The API would be something like `device.StartCapture`, `device.EndCapture`.

During capture we want to serialize enough information to later, in a separate program,
deserialize and replay the captured data.

We have two design ideas though are open to more. Note: Errors in API usage
are NOT captured in either design, at least as we are doing it now. In both
Designs, if an API is used incorrectly, an error is generated and serialization
is skipped.

## Design 1: "Wrap the API"

In this design, all WebGPU calls into Dawn are "wrapped" such that they go
though a wrapper function. These wrapper functions must record things like
bind group layout descriptions, pipeline layout descriptions, pipeline descriptions,
etc...

When capture is started, then, for each command executed while capture is on,
that command and its parameters must be serialized. If the command references
any resources those resources must also be serialized.

A famous capture and replay tool, RenderDoc, uses this design.

## Design 2: "Serialize the Queue"

This design only serializes the calls into `Queue` which are effectively
`Submit`, `WriteBuffer` and `WriteData` and maybe `Buffer::Unmap`.

This design takes advantage of the fact that the class `dawn::native::CommandBuffer`
already captures all of the commands and the list of resources to run that command
buffer. So, when capture is on, those command buffer commands are serialized
and any resources they need, if not already serialized.

Apple's Metal Debugger uses this design

