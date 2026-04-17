# Raw Reviewer Comments
Collected from .claude/reviews/model/ on 2026-04-15T07:03:22Z
Filtered to project: dawn

---

## CL 287075: [dawn][tint] Handle trailing padding in immediate data size
URL: https://dawn-review.googlesource.com/c/dawn/+/287075

- [/COMMIT_MSG:10] @Corentin Wallez: Did the group explicitly agree that this is the expected behavior?

- [/COMMIT_MSG:10] @Corentin Wallez: The group agreed that this is the expected behavior.

- [/PATCHSET_LEVEL:File-level] @Corentin Wallez: I didn't look in details at the dawn::native changes as I'm hoping we can rely on the decompose transform instead.

- [/PATCHSET_LEVEL:File-level] @Corentin Wallez: LGTM +R Dan for the Tint part.

- [src/dawn/native/Pipeline.cpp:300] @Corentin Wallez: ```suggestion
        mUserImmediateSlots |= metadata.immediateDataUsedSlots;
```

- [src/dawn/native/Pipeline.cpp:301] @Corentin Wallez: This is only used in Initialize, IMHO it's not worth keeping around. We could loop over stages again in Initialize, or set `mImmediateMask` here.

- [src/dawn/native/Pipeline.h:130] @Corentin Wallez: WDYT of using the tint transform to decompose immediates to U32 in Metal like we do in the GL CL? It would avoid the need to keep two different sizes.

- [src/dawn/native/Pipeline.h:130] @Stephen White: Hi Shaobo! For the immediate size passed to Tint, I used ImmediateConstantMask::count() to count the number of actually-used immediate slots (including internal uses). It's "compressed", skipping any unused slots in the immediate data, matching what Tint will produce in the IR struct. I then round it up by 4 slots to match the widest possible alignment of members (e.g., vec4) in case that, e.g., one stage contains a vec4 and the other does not. Code here: https://source.chromium.org/chromium/chromium/src/+/main:third_party/dawn/src/dawn/native/opengl/ShaderModuleGL.cpp;l=438

Tint's decompose transform then maxes that length with the WGSL-computed struct size to determine the array length. This may produce an array that is larger than strictly required by one stage or the other, but that shouldn't be a problem. Code here: https://source.chromium.org/chromium/chromium/src/+/main:third_party/dawn/src/tint/lang/core/ir/transform/decompose_access.cc;l=183

Validation is left as-is, using the pipeline immediate size as computed by the front-end.

- [src/dawn/native/ShaderModule.h:332] @Corentin Wallez: Maybe rename them to `immediateDataByteSizeNoPadding/WithPadding` to make things clear?

- [src/dawn/tests/end2end/ImmediateDataTests.cpp:591] @Corentin Wallez: ditto small comment

- [src/dawn/tests/unittests/validation/ImmediateDataTests.cpp:596] @Corentin Wallez: Can we have minimal test coverage of the validation not requiring the trailing padding bytes as described in the commit message?

- [src/dawn/tests/unittests/validation/ImmediateDataTests.cpp:599] @Corentin Wallez: Can we have a small comment explaining what this tries to test?

- [src/dawn/tests/unittests/validation/ImmediateDataTests.cpp:614] @Corentin Wallez: Where is the error case?

- [src/tint/lang/wgsl/inspector/entry_point.h:172] @Corentin Wallez: ditto rename

- [src/tint/lang/wgsl/inspector/inspector.cc:83] @Corentin Wallez: Could we have test coverage in inspector_test.cc?

- [src/tint/lang/wgsl/inspector/inspector.cc:84] @Corentin Wallez: I think we might be reconsidering that and adding array support in https://github.com/gpuweb/gpuweb/issues/5555 so we might as well add it here already.

- [src/tint/lang/wgsl/inspector/inspector.h:223] @Corentin Wallez: Pairs with the same type twice are super confusing, consider making a simple struct for the return value.

---

## CL 293955: vulkan: Fix descriptor set rebinding on push constant range change
URL: https://dawn-review.googlesource.com/c/dawn/+/293955

- [src/dawn/native/vulkan/CommandBufferVk.cpp:184] @Corentin Wallez: This comment should be associated with the dirtying caused by the resource table at [1]

- [src/dawn/native/vulkan/CommandBufferVk.cpp:191] @Corentin Wallez: [2]

- [src/dawn/native/vulkan/CommandBufferVk.cpp:194] @Corentin Wallez: [1]

- [src/dawn/native/vulkan/CommandBufferVk.cpp:200] @Corentin Wallez: Instead of this, let's keep this block in [1] and instead clear the mLastResourceTable at [2].

- [src/dawn/tests/end2end/ImmediateDataTests.cpp:726] @Corentin Wallez: Why? Can we have a TODO?

- [src/dawn/tests/end2end/ImmediateDataTests.cpp:730] @Corentin Wallez: Any reason we need two bindgroups in this test?

- [src/dawn/tests/end2end/ImmediateDataTests.cpp:738] @Corentin Wallez: We could add a push constant size defaulted to 0 to [`utils::MakePipelineLayout`](https://source.chromium.org/chromium/chromium/src/+/main:third_party/dawn/src/dawn/utils/WGPUHelpers.cpp;drc=2546bab52b03777485a99528329c161ad1e751b3;l=236)

- [src/dawn/tests/end2end/ImmediateDataTests.cpp:757] @Corentin Wallez: This VS is the same between both pipelines, we could have one module and just set the entryPoint to something different for the FS of both pipelines.

- [src/dawn/tests/end2end/ImmediateDataTests.cpp:761] @Corentin Wallez: This could directly return the values in one of the bindgroups, which would remove the need for the math to compute the expected values below.

- [src/dawn/tests/end2end/ImmediateDataTests.cpp:764] @Corentin Wallez: ```suggestion
    auto MakePipeline = [&](wgpu::ShaderModule mod, bool useDepthWrite) {
```

- [src/dawn/tests/end2end/ImmediateDataTests.cpp:790] @Corentin Wallez: A size of 1 is sufficient?

- [src/dawn/tests/end2end/ImmediateDataTests.cpp:803] @Corentin Wallez: ```suggestion
    // Draw with Pipeline A (red), then switch to Pipeline B without re-setting bind groups.
    // The Vulkan backend needs to reapply VkDescriptorSets after setting B because the change in push constant size will invalidate the currently bound VkDescriptorSets.
```

- [src/dawn/tests/end2end/ImmediateDataTests.cpp:816] @Corentin Wallez: ```suggestion
// bind groups.

- [src/dawn/tests/end2end/ImmediateDataTests.cpp:827] @Corentin Wallez: ditto `MakePipelineLayout` for both layouts.

- [src/dawn/tests/end2end/ImmediateDataTests.cpp:862] @Corentin Wallez: Why do we need the uniform buffer? A single VkDescriptorSet should be sufficient to test the issue. In shaders we can write immediate values to `output` and that's sufficient to test the potential issue?

- [src/dawn/tests/end2end/ImmediateDataTests.cpp:886] @Corentin Wallez: ```suggestion
// bind groups. Also tests that defaulted pipeline layouts work correctly.
```

- [src/dawn/tests/end2end/ImmediateDataTests.cpp:926] @Corentin Wallez: Defaulted layouts are not compatible between pipelines at all. (I hope we change that one day)

- [src/dawn/tests/end2end/ImmediateDataTests.cpp:954] @Corentin Wallez: Maybe copy the comment at https://source.chromium.org/chromium/chromium/src/+/main:third_party/dawn/src/dawn/tests/end2end/ResourceTableTests.cpp;drc=492a81a423b225a848d5b0aaf8baac729c2c6dba;l=50 ?

- [src/dawn/tests/end2end/ImmediateDataTests.cpp:963] @Corentin Wallez: ```suggestion
        DawnTest::SetUp();
        DAWN_SUPPRESS_TEST_IF(IsSwiftshader() && !DAWN_PLATFORM_IS(ARM64));
    }

    std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
        return {wgpu::FeatureName::ChromiumExperimentalSamplingResourceTable};
    }

```

- [src/dawn/tests/end2end/ImmediateDataTests.cpp:963] @Corentin Wallez: This is a bit surprising to me, can you link one of the failures?

- [src/dawn/tests/end2end/ImmediateDataTests.cpp:963] @Corentin Wallez: Wow I forgot that GetRequiredFeatures didn't cause DawnTest to actually skip if the feature isn't available. That's a big gotcha. Can you file a issue with type techdebt to fix that?

- [src/dawn/tests/end2end/ImmediateDataTests.cpp:996] @Corentin Wallez: nit: It also needs to exist in the layout, otherwise as far as the Vulkan backend is concerned, it doesn't exist.

- [src/dawn/tests/end2end/ImmediateDataTests.cpp:1013] @Corentin Wallez: nit: created on pipeline fully then the other?

- [src/dawn/tests/end2end/ImmediateDataTests.cpp:1021] @Corentin Wallez: We don't even need to put something in the resource table for this test, since all we need is the resource table to be bound correctly in the Vulkan backend.

- [src/dawn/tests/end2end/ImmediateDataTests.cpp:1045] @Corentin Wallez: And the VVL will complain, which should be sufficient for the test, or did the VVLs not complain? (with `--enable-backend-validation` (`=full`?))?

- [src/dawn/utils/WGPUHelpers.cpp:242] @Corentin Wallez: This can be unconditional since the default is 0 in the struct as well.

---

## CL 294255: [tint][inspector] Delay validation of max immediate size to pipeline creation time
URL: https://dawn-review.googlesource.com/c/dawn/+/294255

- [src/dawn/native/ShaderModule.cpp:875] @Corentin Wallez: We can just use `DelayedInvalidIf` which is designed to handle pushing errors to pipeline creation time (when we detect them at shader creation time). So we don't need the complexity of an overflow bit etc.

---

## CL 300455: Skip tests when GetRequiredFeatures returns unsupported features
URL: https://dawn-review.googlesource.com/c/dawn/+/300455

- [/PATCHSET_LEVEL:File-level] @Corentin Wallez: LGTM, will you also audit the `GetRequiredFeatures` to remove the checks for `SupportsFeature` that are unnecessary? (some tests really do optionally support features, but most don't)

- [src/dawn/tests/DawnTest.cpp:1408] @Corentin Wallez: That won't be necessary, by coincidence I'm making https://dawn-review.googlesource.com/c/dawn/+/300495

- [src/dawn/tests/DawnTest.cpp:1613] @Corentin Wallez: nit: Can we do something like this somehow? `"Skipping test because " << theFeature << " is not supported by the adapter"`.

- [src/dawn/tests/end2end/DawnTestInfraTests.cpp:41] @Corentin Wallez: nit: not neeeded.

- [src/dawn/tests/end2end/DawnTestInfraTests.cpp:60] @Corentin Wallez: nit: `.contains`

---
