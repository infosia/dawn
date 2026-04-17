# [Gerrit] CL/PR 293955: vulkan: Fix descriptor set rebinding on push constant range change

## Metadata Summary
- **Author**: Shaobo Yan
- **Status**: NEW
- **Total Rounds (Gerrit)**: 12
- **URL**: https://dawn-review.googlesource.com/c/dawn/+/293955

## General PR Comments
**General Comment** - @Shaobo Yan (Round 1) (2026-02-26 05:14:59):
> Hashtag added: vulkan

**General Comment** - @Shaobo Yan (Round 2) (2026-02-28 07:26:54):
> Uploaded patch set 2.

**General Comment** - @Shaobo Yan (Round 3) (2026-02-28 10:31:00):
> Uploaded patch set 3.

**General Comment** - @Shaobo Yan (Round 3) (2026-02-28 10:34:31):
> Patch Set 3: Commit-Queue+1

**General Comment** - @Dawn LUCI CQ (Round 3) (2026-02-28 10:34:42):
> Patch Set 3:
> 
> Dry run: CV is trying the patch.
> 
> Follow status at: https://luci-change-verifier.appspot.com/ui/run/dawn/8850695925854-1-18e40a6a1231fdf6
> 
> Bot data: {"action":"start","triggered_at":"2026-02-28T10:34:31Z","revision":"bbf7ca790b3e413f0db1ad41c6c976712521ad64"}

**General Comment** - @Shaobo Yan (Round 4) (2026-02-28 10:54:25):
> Uploaded patch set 4.
> 
> Outdated Votes:
> * Commit-Queue+1 (copy condition: "changekind:NO_CHANGE")

**General Comment** - @Shaobo Yan (Round 4) (2026-02-28 10:54:56):
> Patch Set 4: Commit-Queue+1

**General Comment** - @Dawn LUCI CQ (Round 4) (2026-02-28 10:55:02):
> Patch Set 4:
> 
> Dry run: CV is trying the patch.
> 
> Follow status at: https://luci-change-verifier.appspot.com/ui/run/dawn/8850694700854-1-19e1a60dae5f409f
> 
> Bot data: {"action":"start","triggered_at":"2026-02-28T10:54:56Z","revision":"8a6cf325b15a576ee922fc07960f3c5c0c7a8bae"}

**General Comment** - @Shaobo Yan (Round 4) (2026-02-28 12:16:35):
> Patch Set 4: -Commit-Queue
> 
> (Performed by <GERRIT_ACCOUNT_1002656> on behalf of <GERRIT_ACCOUNT_3758334>)

**General Comment** - @Dawn LUCI CQ (Round 4) (2026-02-28 12:16:36):
> Patch Set 4:
> 
> This CL has passed the run

**General Comment** - @Shaobo Yan (Round 5) (2026-03-02 01:56:12):
> Uploaded patch set 5.

**General Comment** - @Shaobo Yan (Round 5) (2026-03-05 01:33:27):
> Patch Set 5:
> 
> (1 comment)
> 
> This change is ready for review.

**General Comment** - @Shaobo Yan (Round 5) (2026-03-05 01:33:35):
> <GERRIT_ACCOUNT_1000002>, who was added as reviewer owns the following files:
> * `src/dawn/native/vulkan/CommandBufferVk.cpp`
> * `src/dawn/tests/end2end/ImmediateDataTests.cpp`

**General Comment** - @Shaobo Yan (Round 5) (2026-03-05 01:33:44):
> Patch Set 5: Commit-Queue+1

**General Comment** - @Dawn LUCI CQ (Round 5) (2026-03-05 01:33:55):
> Patch Set 5:
> 
> Dry run: CV is trying the patch.
> 
> Follow status at: https://luci-change-verifier.appspot.com/ui/run/dawn/8850296372854-1-b2020c82583016b0
> 
> Bot data: {"action":"start","triggered_at":"2026-03-05T01:33:44Z","revision":"5cf09724f569b3cb35135e8d43ea3d6249f198a7"}

**General Comment** - @Shaobo Yan (Round 5) (2026-03-05 02:09:18):
> Patch Set 5: -Commit-Queue
> 
> (Performed by <GERRIT_ACCOUNT_1002656> on behalf of <GERRIT_ACCOUNT_3758334>)

**General Comment** - @Dawn LUCI CQ (Round 5) (2026-03-05 02:09:20):
> Patch Set 5:
> 
> This CL has failed the run. Reason:
> 
> Tryjob [dawn/try/dawn-cq-mac-x64-rel](https://cr-buildbucket.appspot.com/build/8688208666916648321) has failed with summary ([view all results](https://dawn-review.googlesource.com/c/293955?checksPatchset=5&tab=checks)):
> 
> ---
> 1 Test Suite(s) failed.
> 
> **dawn_perf_tests on AMD 0x67ef GPU on Mac Retina** failed with invalid results. Did a shard fail early?

**General Comment** - @Corentin Wallez (Round 5) (2026-03-05 09:49:32):
> Patch Set 5:
> 
> (22 comments)

**General Comment** - @Shaobo Yan (Round 5) (2026-03-06 09:06:31):
> Patch Set 5:
> 
> (1 comment)

**General Comment** - @Shaobo Yan (Round 6) (2026-03-09 03:30:49):
> Uploaded patch set 6.

**General Comment** - @Shaobo Yan (Round 6) (2026-03-09 05:06:09):
> Patch Set 6: Commit-Queue+1

**General Comment** - @Dawn LUCI CQ (Round 6) (2026-03-09 05:06:25):
> Patch Set 6:
> 
> Dry run: CV is trying the patch.
> 
> Follow status at: https://luci-change-verifier.appspot.com/ui/run/dawn/8849938027854-1-23918a070b293811
> 
> Bot data: {"action":"start","triggered_at":"2026-03-09T05:06:09Z","revision":"1985adeb37bf011a71212ca780ee476ac058ce02"}

**General Comment** - @Shaobo Yan (Round 6) (2026-03-09 05:09:33):
> Set Work In Progress

**General Comment** - @Shaobo Yan (Round 6) (2026-03-09 05:10:04):
> Patch Set 6: -Commit-Queue
> 
> (Performed by <GERRIT_ACCOUNT_1002656> on behalf of <GERRIT_ACCOUNT_3758334>)

**General Comment** - @Dawn LUCI CQ (Round 6) (2026-03-09 05:10:04):
> Patch Set 6:
> 
> This CL has failed the run. Reason:
> 
> Tryjob [dawn/try/dawn-cq-mac-arm64-rel](https://cr-buildbucket.appspot.com/build/8687834090546692001) has failed with summary ([view all results](https://dawn-review.googlesource.com/c/293955?checksPatchset=6&tab=checks)):
> 
> ---
> 5 Test Suite(s) failed.
> 
> **dawn_end2end_implicit_sync_tests on (apple:m2) GPU on Mac Retina** failed because of:
> 
> - ImmediateDataWithResourceTableTests.ResourceTableReboundOnDifferentImmediateSize/Metal_Apple_M2
> 
> **dawn_end2end_skip_validation_tests on (apple:m2) GPU on Mac Retina** failed because of:
> 
> - ImmediateDataWithResourceTableTests.ResourceTableReboundOnDifferentImmediateSize/Metal_Apple_M2
> 
> **dawn_end2end_tests on (apple:m2) GPU on Mac Retina** failed because of:
> 
> - ImmediateDataWithResourceTableTests.ResourceTableReboundOnDifferentImmediateSize/Metal_Apple_M2
> 
> **dawn_end2end_validation_layers_tests on (apple:m2) GPU on Mac Retina** failed because of:
> 
> - ImmediateDataWithResourceTableTests.ResourceTableReboundOnDifferentImmediateSize/Metal_Apple_M2
> 
> #### ...1 more test(s)...

**General Comment** - @Shaobo Yan (Round 7) (2026-03-09 05:48:02):
> Uploaded patch set 7.

**General Comment** - @Shaobo Yan (Round 8) (2026-03-09 06:15:59):
> Uploaded patch set 8.

**General Comment** - @Shaobo Yan (Round 8) (2026-03-09 06:16:47):
> Patch Set 8: Commit-Queue+1

**General Comment** - @Dawn LUCI CQ (Round 8) (2026-03-09 06:17:03):
> Patch Set 8:
> 
> Dry run: CV is trying the patch.
> 
> Follow status at: https://luci-change-verifier.appspot.com/ui/run/dawn/8849933789854-1-b06c106ee69c8c1f
> 
> Bot data: {"action":"start","triggered_at":"2026-03-09T06:16:47Z","revision":"fb358ca166bec8c17afe3c5430365a817132902a"}

**General Comment** - @Shaobo Yan (Round 8) (2026-03-09 06:20:26):
> Patch Set 8: -Commit-Queue
> 
> (Performed by <GERRIT_ACCOUNT_1002656> on behalf of <GERRIT_ACCOUNT_3758334>)

**General Comment** - @Dawn LUCI CQ (Round 8) (2026-03-09 06:20:28):
> Patch Set 8:
> 
> This CL has failed the run. Reason:
> 
> Tryjob [dawn/try/dawn-cq-mac-arm64-rel](https://cr-buildbucket.appspot.com/build/8687829645656633745) has failed with summary ([view all results](https://dawn-review.googlesource.com/c/293955?checksPatchset=8&tab=checks)):
> 
> ---
> 5 Test Suite(s) failed.
> 
> **dawn_end2end_implicit_sync_tests on (apple:m2) GPU on Mac Retina** failed because of:
> 
> - ImmediateDataWithResourceTableTests.ResourceTableReboundOnDifferentImmediateSize/Metal_Apple_M2
> 
> **dawn_end2end_skip_validation_tests on (apple:m2) GPU on Mac Retina** failed because of:
> 
> - ImmediateDataWithResourceTableTests.ResourceTableReboundOnDifferentImmediateSize/Metal_Apple_M2
> 
> **dawn_end2end_tests on (apple:m2) GPU on Mac Retina** failed because of:
> 
> - ImmediateDataWithResourceTableTests.ResourceTableReboundOnDifferentImmediateSize/Metal_Apple_M2
> 
> **dawn_end2end_validation_layers_tests on (apple:m2) GPU on Mac Retina** failed because of:
> 
> - ImmediateDataWithResourceTableTests.ResourceTableReboundOnDifferentImmediateSize/Metal_Apple_M2
> 
> #### ...1 more test(s)...

**General Comment** - @Shaobo Yan (Round 9) (2026-03-09 08:32:08):
> Uploaded patch set 9.

**General Comment** - @Shaobo Yan (Round 9) (2026-03-09 08:55:34):
> Patch Set 9:
> 
> (17 comments)
> 
> This change is ready for review.

**General Comment** - @Shaobo Yan (Round 10) (2026-03-09 08:57:58):
> Uploaded patch set 10.

**General Comment** - @Shaobo Yan (Round 11) (2026-03-09 09:08:43):
> Uploaded patch set 11.

**General Comment** - @Corentin Wallez (Round 11) (2026-03-09 09:49:26):
> Patch Set 11: Code-Review+1
> 
> (3 comments)

**General Comment** - @Shaobo Yan (Round 11) (2026-03-09 11:07:44):
> Patch Set 11:
> 
> (3 comments)

**General Comment** - @Shaobo Yan (Round 12) (2026-03-10 07:26:03):
> Uploaded patch set 12.
> 
> Copied Votes:
> * Code-Review+1 by <GERRIT_ACCOUNT_1000002> (copy condition\*: "changekind:NO_CODE_CHANGE OR changekind:TRIVIAL_REBASE OR (**has:unchanged-files** AND **uploaderin:cria/project-dawn-committers**) OR is:MIN")
> 
> \* The label has `labelCopyEnforcement` or `labelCopyRestriction` configured. Only the most relevant condition that determined the outcome is shown.

**General Comment** - @Shaobo Yan (Round 12) (2026-03-10 07:28:54):
> Patch Set 12:
> 
> (3 comments)

**General Comment** - @Corentin Wallez (Round 12) (2026-03-10 09:07:06):
> Patch Set 12: Code-Review+2
> 
> (1 comment)

**General Comment** - @Shaobo Yan (Round 12) (2026-03-11 00:55:58):
> Patch Set 12:
> 
> (1 comment)


## Inline Review Comments

### File: `/PATCHSET_LEVEL`
**[/PATCHSET_LEVEL:File-level] (Round 5)** - @Shaobo Yan (2026-03-05 01:33:27):
> PTAL this fix, thanks!


### File: `src/dawn/native/vulkan/CommandBufferVk.cpp`
**[src/dawn/native/vulkan/CommandBufferVk.cpp:184] (Round 5)** - @Corentin Wallez (2026-03-05 09:49:32):
> This comment should be associated with the dirtying caused by the resource table at [1]

**[src/dawn/native/vulkan/CommandBufferVk.cpp:184] (Round 5)** - @Shaobo Yan (2026-03-09 08:55:34):
> Done

**[src/dawn/native/vulkan/CommandBufferVk.cpp:191] (Round 5)** - @Corentin Wallez (2026-03-05 09:49:32):
> [2]

**[src/dawn/native/vulkan/CommandBufferVk.cpp:194] (Round 5)** - @Corentin Wallez (2026-03-05 09:49:32):
> [1]

**[src/dawn/native/vulkan/CommandBufferVk.cpp:200] (Round 5)** - @Corentin Wallez (2026-03-05 09:49:32):
> Instead of this, let's keep this block in [1] and instead clear the mLastResourceTable at [2].

**[src/dawn/native/vulkan/CommandBufferVk.cpp:200] (Round 5)** - @Shaobo Yan (2026-03-09 08:55:34):
> Done


### File: `src/dawn/tests/end2end/ImmediateDataTests.cpp`
**[src/dawn/tests/end2end/ImmediateDataTests.cpp:726] (Round 5)** - @Corentin Wallez (2026-03-05 09:49:32):
> Why? Can we have a TODO?

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:726] (Round 5)** - @Shaobo Yan (2026-03-09 08:55:34):
> No need. Done.

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:730] (Round 5)** - @Corentin Wallez (2026-03-05 09:49:32):
> Any reason we need two bindgroups in this test?

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:730] (Round 5)** - @Shaobo Yan (2026-03-06 09:06:31):
> To ensure this is "dirty all bindgroups" but not "dirty highest index " bind groups. But it seems not necessary.

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:730] (Round 5)** - @Shaobo Yan (2026-03-09 08:55:34):
> Done

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:738] (Round 5)** - @Corentin Wallez (2026-03-05 09:49:32):
> We could add a push constant size defaulted to 0 to [`utils::MakePipelineLayout`](https://source.chromium.org/chromium/chromium/src/+/main:third_party/dawn/src/dawn/utils/WGPUHelpers.cpp;drc=2546bab52b03777485a99528329c161ad1e751b3;l=236)

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:738] (Round 5)** - @Shaobo Yan (2026-03-09 08:55:34):
> Done

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:757] (Round 5)** - @Corentin Wallez (2026-03-05 09:49:32):
> This VS is the same between both pipelines, we could have one module and just set the entryPoint to something different for the FS of both pipelines.

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:757] (Round 5)** - @Shaobo Yan (2026-03-09 08:55:34):
> Done

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:761] (Round 5)** - @Corentin Wallez (2026-03-05 09:49:32):
> This could directly return the values in one of the bindgroups, which would remove the need for the math to compute the expected values below.

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:761] (Round 5)** - @Shaobo Yan (2026-03-09 08:55:34):
> Done

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:764] (Round 5)** - @Corentin Wallez (2026-03-05 09:49:32):
> ```suggestion
>     auto MakePipeline = [&](wgpu::ShaderModule mod, bool useDepthWrite) {
> ```

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:764] (Round 5)** - @Shaobo Yan (2026-03-09 08:55:34):
> Done

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:790] (Round 5)** - @Corentin Wallez (2026-03-05 09:49:32):
> A size of 1 is sufficient?

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:790] (Round 5)** - @Shaobo Yan (2026-03-10 07:28:54):
> Do an experiment and it seems test could catch the issue https://dawn-review.googlesource.com/c/dawn/+/296155?tab=checks

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:803] (Round 5)** - @Corentin Wallez (2026-03-05 09:49:32):
> ```suggestion
>     // Draw with Pipeline A (red), then switch to Pipeline B without re-setting bind groups.
>     // The Vulkan backend needs to reapply VkDescriptorSets after setting B because the change in push constant size will invalidate the currently bound VkDescriptorSets.
> ```

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:803] (Round 5)** - @Shaobo Yan (2026-03-09 08:55:34):
> Done

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:816] (Round 5)** - @Corentin Wallez (2026-03-05 09:49:32):
> ```suggestion
> // bind groups.

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:816] (Round 5)** - @Shaobo Yan (2026-03-09 08:55:34):
> Done

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:827] (Round 5)** - @Corentin Wallez (2026-03-05 09:49:32):
> ditto `MakePipelineLayout` for both layouts.

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:827] (Round 5)** - @Shaobo Yan (2026-03-09 08:55:34):
> Done

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:862] (Round 5)** - @Corentin Wallez (2026-03-05 09:49:32):
> Why do we need the uniform buffer? A single VkDescriptorSet should be sufficient to test the issue. In shaders we can write immediate values to `output` and that's sufficient to test the potential issue?

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:862] (Round 5)** - @Shaobo Yan (2026-03-09 08:55:34):
> Done

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:886] (Round 5)** - @Corentin Wallez (2026-03-05 09:49:32):
> ```suggestion
> // bind groups. Also tests that defaulted pipeline layouts work correctly.
> ```

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:886] (Round 5)** - @Shaobo Yan (2026-03-09 11:07:44):
> Done

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:926] (Round 5)** - @Corentin Wallez (2026-03-05 09:49:32):
> Defaulted layouts are not compatible between pipelines at all. (I hope we change that one day)

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:926] (Round 5)** - @Shaobo Yan (2026-03-09 11:07:44):
> Done

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:954] (Round 5)** - @Corentin Wallez (2026-03-05 09:49:32):
> Maybe copy the comment at https://source.chromium.org/chromium/chromium/src/+/main:third_party/dawn/src/dawn/tests/end2end/ResourceTableTests.cpp;drc=492a81a423b225a848d5b0aaf8baac729c2c6dba;l=50 ?

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:954] (Round 5)** - @Shaobo Yan (2026-03-09 08:55:34):
> Done

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:963] (Round 5)** - @Corentin Wallez (2026-03-05 09:49:32):
> ```suggestion
>         DawnTest::SetUp();
>         DAWN_SUPPRESS_TEST_IF(IsSwiftshader() && !DAWN_PLATFORM_IS(ARM64));
>     }
> 
>     std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
>         return {wgpu::FeatureName::ChromiumExperimentalSamplingResourceTable};
>     }
> 
> ```

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:963] (Round 5)** - @Shaobo Yan (2026-03-09 11:07:44):
> For example, this one. https://ci.chromium.org/ui/p/dawn/builders/try/dawn-cq-mac-arm64-rel/b8687829645656633745/test-results?q=ExactID%3Aninja%3A%2F%2Fsrc%2Fdawn%2Ftests%3Adawn_end2end_tests%2FImmediateDataWithResourceTableTests.ResourceTableReboundOnDifferentImmediateSize%2FMetal_Apple_M2+VHash%3Ac103dd1a63d99530&clean=

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:963] (Round 5)** - @Shaobo Yan (2026-03-11 00:55:58):
> Sure. Issue filed https://issues.chromium.org/u/1/issues/491515776 (No techdebt in type in my drop box list. Choose internal cleanup and use [TechDebt] in title).

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:963] (Round 5)** - @Corentin Wallez (2026-03-09 09:49:26):
> This is a bit surprising to me, can you link one of the failures?

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:963] (Round 5)** - @Shaobo Yan (2026-03-09 08:55:34):
> This fails the bots. So keep the origin logic.

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:963] (Round 5)** - @Corentin Wallez (2026-03-10 09:07:06):
> Wow I forgot that GetRequiredFeatures didn't cause DawnTest to actually skip if the feature isn't available. That's a big gotcha. Can you file a issue with type techdebt to fix that?

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:996] (Round 11)** - @Corentin Wallez (2026-03-09 09:49:26):
> nit: It also needs to exist in the layout, otherwise as far as the Vulkan backend is concerned, it doesn't exist.

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:996] (Round 11)** - @Shaobo Yan (2026-03-10 07:28:54):
> Done

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:1013] (Round 5)** - @Corentin Wallez (2026-03-05 09:49:32):
> nit: created on pipeline fully then the other?

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:1013] (Round 5)** - @Shaobo Yan (2026-03-09 08:55:34):
> Done

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:1021] (Round 5)** - @Corentin Wallez (2026-03-05 09:49:32):
> We don't even need to put something in the resource table for this test, since all we need is the resource table to be bound correctly in the Vulkan backend.

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:1021] (Round 5)** - @Shaobo Yan (2026-03-09 08:55:34):
> Done

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:1045] (Round 5)** - @Corentin Wallez (2026-03-05 09:49:32):
> And the VVL will complain, which should be sufficient for the test, or did the VVLs not complain? (with `--enable-backend-validation` (`=full`?))?

**[src/dawn/tests/end2end/ImmediateDataTests.cpp:1045] (Round 5)** - @Shaobo Yan (2026-03-09 08:55:34):
> I cannot see the error in my local machine. But the experiment here https://dawn-review.googlesource.com/c/dawn/+/295916 shows validation layer (at least in bots) should catch this. Done.


### File: `src/dawn/utils/WGPUHelpers.cpp`
**[src/dawn/utils/WGPUHelpers.cpp:242] (Round 11)** - @Corentin Wallez (2026-03-09 09:49:26):
> This can be unconditional since the default is 0 in the struct as well.

**[src/dawn/utils/WGPUHelpers.cpp:242] (Round 11)** - @Shaobo Yan (2026-03-10 07:28:54):
> Done
