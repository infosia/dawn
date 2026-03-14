I want to create a WebGPU WGSL fuzzer inside of a Dawn end2end test.

An example of an end2end test that runs WGSL is `src/dawn/tests/end2end/PolyfillBuiltinSimpleTests.cpp`.

We want to test these things:

- Any simple intrinsic found in the WGSL spec. This could be `add`, `mul`, `mod`, `clamp`.
- We can start with a few and then later add the more complex ones.
- We want to test types. We can start with `u32` and then move on to `i32` and then the vector types.
- We also need to test simple loops, both constant and those whose range is set by storage buffers (i.e., the backend compiler cannot see the range).
- We also want to test conditionals, i.e., branches or `if` statements.

This is what we want to test for now. We will avoid more complex operations like textures and extensions.

Obviously, in addition to the fuzzing shader, we will also need testing data coming in from storage and resulting out from storage.

How to fuzz the shaders:
We don't want to fuzz the parsing of WGSL. We want to always generate correct WGSL.
So the way we do this is we have our own internal, very simple AST representation.
Then, when generating, we can know what is legitimate to add and what is not.
We can have some basic rules as well, like having the structure be 1 or 2 loops with 1 or 2 conditions (ifs).
The expressions can also be fairly simple: a few chained expressions that maybe feed into a result or into a branch.

Locally on the host (CPU) side, we have to simulate execution. We do this by having, again, a very simple AST interpreter.
This AST interpreter will tell us what the result is at every stage and possibly what we expect in the storage buffer post-execution.

Complexities:
We can start with a workgroup size of 1, but ideally, we would like to have a workgroup size of 256.
Keep in mind that one should not introduce racy writes. All writes should be independently indexed. Again, this is actually simple.

The dispatch size should simply be used to check a huge amount of data for a given testing/fuzz shader.

Keep in mind that failure to compile a legit shader is an error that should be surfaced.
Also, failure to compile the pipeline or errors in execution are also legit errors that we need to surface.
These could be the very bugs we are looking for!

## Implementation Progress (March 2026)

We have successfully implemented the initial version of the WGSL fuzzer in `src/dawn/tests/end2end/WGSLFuzzerTests.cpp`.

### Accomplishments:
1.  **Simple AST Representation**: Created `Node` and `Stmt` base classes with implementations for:
    *   `Literal`: Constants for `u32` and `i32`.
    *   `StorageVar`: Access to input and output storage buffers.
    *   `BinaryExpr`: Arithmetic operations (`+`, `-`, `*`, `/`, `%`).
    *   `ClampExpr`: Support for the `clamp` intrinsic.
    *   `Assignment`: Assigning expression results to output buffers.
    *   `IfStmt`: Conditional branching.
    *   `LoopStmt`: `for` loops with ranges determined by input storage buffers.
2.  **Random Program Generator**: A `Generator` class that recursively builds valid ASTs and emits them as WGSL.
3.  **Host-side Interpreter**: An `Eval` system within the AST nodes that simulates execution on the CPU to generate expected values for validation.
4.  **End-to-End Test Integration**: Added `WGSLFuzzerTests` which runs multiple iterations of randomly generated shaders on the GPU and compares the results against the CPU simulation.
5.  **Robustness Fixes**:
    *   **Clamp Validation**: Wrapped `clamp` parameters in `min`/`max` in the generated WGSL to ensure `low <= high`, preventing shader compilation errors.
    *   **Constant Evaluation**: Improved the interpreter and generator to handle edge cases like division by zero and large literal overflows that could trigger validation errors during pipeline creation.
6.  **Vector Support**: Added support for `vec2`, `vec3`, and `vec4` for both `u32` and `i32` types, including component-wise operations and bitcasts.
7.  **Data Parallelism**: Scaled the test to a workgroup size of 256, using `global_invocation_id` to ensure independent indexing and race-free execution across all threads.
8.  **Readable WGSL Generation**: Refactored the generator to use temporary `let` variables for intermediate expressions and local `var` variables to pre-load storage buffer data, significantly improving shader readability and reducing redundant buffer access.
9.  **Enhanced Vector Construction**: Improved vector literals to support unique values for each component and implemented component-wise bitcasting for more diverse and robust testing.
10. **Test Parameterization**: Parameterized the end-to-end test by the number of iterations (1, 8, 64, 256), allowing for both quick checks and extensive fuzzing runs.

### How to Run:
The test can be run using the following command:
```bash
autoninja -C out/active && ./out/active/dawn_end2end_tests --backend=vulkan --gtest_filter=*WGSLFuzzerTests*
```
