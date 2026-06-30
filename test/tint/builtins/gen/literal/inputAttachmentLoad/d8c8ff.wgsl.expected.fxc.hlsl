SKIP: FAILED

invalid value 'hlsl-fxc' for option '--format'
Must be one of: none, wgsl, ir, spirv, spvasm, msl
Usage: tint [options] <input-file>

Options:
--allow-non-uniform-derivatives <value>
                              Allow non-uniform derivatives by inserting a module-scope directive to suppress any uniformity
                              violations that may be produced
                              default: false
--argument-buffer <value>     Mapping for an argument buffer, format is GROUP=ARGUMENT_BUFFER_ID, comma separated
--color <off|dark|light>      Use colored output
                              default: off
 -col                         short name for --color
--diagnostics-format <plain|json>
                              The format to print diagnostics in
                              default: plain
--disable-demote-to-helper <value>
                              Disable demote to helper for discard
                              default: false
--disable-ir-validation-asserts <value>
                              Disable IR validation assertions at each stage of the compilation flow
                              default: false
--disable-robustness <value>  Disable the robustness transform
--disable-unreachable-code-warning <value>
                              Disable the warning for unreachable code when converting to WGSL
                              default: false
--disable-workgroup-init <value>
                              Disable workgroup memory zero initialization
                              default: false
--dump-inspector-bindings <value>
                              Dump reflection data about bindings to stdout
                              default: false
--emit-inspector-bindings     alias for --dump-inspector-bindings
--dump-ir <value>             Dump IR at each stage of the compilation flow
                              default: false
--emit-ir                     alias for --dump-ir
--dynamic-offset <value>      Mapping for dynamic buffers to be attached to the entry point, format is GROUP.BINDING=OFFSET, comma separated. BINDING is the BindingIndex, not @binding BindingNumber)
--dynamic-offset-buffer <value>
                              Mapping for a dynamic offset buffer, format is GROUP=DYNAMIC_BUFFER_ID, comma separated)
--entry-point <name>          Output single entry point
 -ep                          short name for --entry-point
--format <none|wgsl|ir|spirv|spvasm|msl>
                              Output format.
                              If not provided, will be inferred from output filename extension:
                                .spvasm -> spvasm
                                .spv    -> spirv
                                .wgsl   -> wgsl
                                .metal  -> msl
                                .hlsl   -> hlsl
 -f                           short name for --format
--help <value>                Show usage
 -h                           short name for --help
--input-format <wgsl>         Input format.
                              If not provided, will be inferred from input filename extension:
                              .spvasm -> spvasm
                              .spv    -> spirv
                              .wgsl   -> wgsl
 -if                          short name for --input-format
--ir-roundtrip <value>        Converts the Program to IR and then back to a Program
                              default: false
--minify <value>              Minify the output WGSL
                              default: false
--msl-version <2.3|2.4|3.2>   Specify the MSL version.
                              Valid values are 2.3, 2.4, and 3.2
                              default: 2.3
--output-name <name>          Output file name
 -o                           short name for --output-name
--overrides <value>           Override values as IDENTIFIER=VALUE, comma-separated
--parse-only <value>          Stop after parsing the input
                              default: false
--pixel-local-attachments <value>
                              Pixel local storage attachment bindings, comma-separated
                              Each binding is of the form MEMBER_INDEX=ATTACHMENT_INDEX,
                              where MEMBER_INDEX is the pixel-local structure member
                              index and ATTACHMENT_INDEX is the index of the emitted
                              attachment.

--print-hash <value>          Emit the hash of the output program
                              default: false
--rename-all <value>          Renames all symbols
                              default: false
--skip-hash <value>           Skips validation if the hash of the output is equal to any
                              of the hash codes in the comma separated list of hashes
--spirv-version <1.3|1.4>     Specify the SPIR-V binary version.
                              Valid values are 1.3 and 1.4
                              default: 1.3
--treat-samplers-as-filtering <value>
                              Set the ResourceType for any bindful samplers as filtering
                              default: false
--use-argument-buffers <value>
                              Use the Argument Buffers in MSL
                              default: false
--use-storage-input-output-16 <value>
                              Use the StorageInputOutput16 SPIR-V capability
                              default: true
--validate <value>            Validates the generated shader with all available validators
                              default: false
--verbose <value>             Verbose output
                              default: false
 -v                           short name for --verbose
--xcrun <path>                Path to xcrun executable, used to validate MSL output.
                              When specified, automatically enables MSL validation
--ycbcr-bindings <value>      Allows setting an external texture as YCBCR. This allows specifying that the external texture at the given binding point should be considered a YCBCR texture. Entries are provided as binding point pairs (group, binding) (e.g. 1,2). Multiple entries should be separated with a space.

tint executable returned error: exit status 1
