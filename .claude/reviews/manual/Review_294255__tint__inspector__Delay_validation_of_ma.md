# [Gerrit] CL/PR 294255: [tint][inspector] Delay validation of max immediate size to pipeline creation time

## Metadata Summary
- **Author**: Shaobo Yan
- **Status**: NEW
- **Total Rounds (Gerrit)**: 2
- **URL**: https://dawn-review.googlesource.com/c/dawn/+/294255

## General PR Comments
**General Comment** - @Shaobo Yan (Round 1) (2026-02-27 02:09:06):
> Hashtags added: inspector, tint

**General Comment** - @Shaobo Yan (Round 2) (2026-02-27 02:12:52):
> Uploaded patch set 2: Commit message was updated.

**General Comment** - @Shaobo Yan (Round 2) (2026-03-10 07:31:34):
> Patch Set 2:
> 
> (1 comment)
> 
> This change is ready for review.

**General Comment** - @Shaobo Yan (Round 2) (2026-03-10 07:31:34):
> <GERRIT_ACCOUNT_1000002>, who was added as reviewer owns the following files:
> * `src/dawn/native/ShaderModule.cpp`
> * `src/dawn/tests/unittests/validation/ImmediateDataTests.cpp`
> * `src/tint/lang/wgsl/inspector/inspector.cc`
> * `src/tint/lang/wgsl/inspector/inspector.h`
> * `src/tint/lang/wgsl/inspector/inspector_test.cc`

**General Comment** - @Corentin Wallez (Round 2) (2026-03-10 09:18:15):
> Patch Set 2:
> 
> (1 comment)


## Inline Review Comments

### File: `/PATCHSET_LEVEL`
**[/PATCHSET_LEVEL:File-level] (Round 2)** - @Shaobo Yan (2026-03-10 07:31:34):
> PTAL, this fix a cts validation test failure. The failure is due to we report exceed maxImmediateSize during shaderModule creation time instead of pipeline creation time.


### File: `src/dawn/native/ShaderModule.cpp`
**[src/dawn/native/ShaderModule.cpp:875] (Round 2)** - @Corentin Wallez (2026-03-10 09:18:15):
> We can just use `DelayedInvalidIf` which is designed to handle pushing errors to pipeline creation time (when we detect them at shader creation time). So we don't need the complexity of an overflow bit etc.
