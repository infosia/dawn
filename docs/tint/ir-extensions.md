# IR Extensions

The IR can be extended by the various backends as needed. This can be done through
custom transforms, capabilities, types, intrinsics or through subclasses of other values.

## Transforms
In a few cases we need to make transforms on the IR which are specific to a given backend.
In these cases the transforms live in the `writer/raise` folder for the backend (e.g.
`lang/msl/writer/raise/`). These transforms are then added into the `raise.cc` file to run the
transform when necessary.

Each transform defines its own config, if necessary. This is typically done by defining a `struct`
in the transform `.h` file with a naming matching the transform with a  `Config` suffix. (e.g.
`ModuleConstant` transform has a `ModuleConstantConfig` structure). When the transform is called in
`raise` the config will be created from the generator config.

Transforms have a set of `Capabilities` they support. Often the list of supported capabilities is
added to the `.h` file so they can be shared with a fuzzer for the transform. The set of
capabilities is typically named `k` + transform name + `Capabilities` (e.g. `ModuleConstant` has
`kModuleConstantCapabilities`). The set of capabilities should be kept as minimal as possible for
the transform. The capabilities are used to control what extra features the IR validator will allow.

Transforms all follow a similar pattern of a free function named after the transform and then a
`State` struct with a `Process` method. The free function will run the validator if necessary and
then call the `State::Process` method.

## Intrinsics
Each backend has a `.def` file listing the intrinsics for that backend (e.g. `lang/msl/msl.def`).
These custom intrinsics can be types or instructions. See
[intrinsic_definition_files.md](intrinsic_defintion_files.md)


## Types




## Subclasses

