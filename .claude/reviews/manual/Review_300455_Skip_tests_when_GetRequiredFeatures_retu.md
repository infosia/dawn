# [Gerrit] CL/PR 300455: Skip tests when GetRequiredFeatures returns unsupported features

## Metadata Summary
- **Author**: Shaobo Yan
- **Status**: NEW
- **Total Rounds (Gerrit)**: 1
- **URL**: https://dawn-review.googlesource.com/c/dawn/+/300455

## General PR Comments
**General Comment** - @Shaobo Yan (Round 1) (2026-03-31 09:11:59):
> Uploaded patch set 1.

**General Comment** - @Shaobo Yan (Round 1) (2026-03-31 09:13:57):
> Patch Set 1: Commit-Queue+1

**General Comment** - @Dawn LUCI CQ (Round 1) (2026-03-31 09:14:13):
> Patch Set 1:
> 
> Dry run: CV is trying the patch.
> 
> Follow status at: https://luci-change-verifier.appspot.com/ui/run/dawn/8848022359854-1-09d2301e0ed9cbcf
> 
> Bot data: {"action":"start","triggered_at":"2026-03-31T09:13:57Z","revision":"800cd373a594bd756695152da97f5c1ee2fcff08"}

**General Comment** - @Shaobo Yan (Round 1) (2026-03-31 10:41:42):
> Patch Set 1: -Commit-Queue
> 
> (Performed by <GERRIT_ACCOUNT_1002656> on behalf of <GERRIT_ACCOUNT_3758334>)

**General Comment** - @Dawn LUCI CQ (Round 1) (2026-03-31 10:41:43):
> Patch Set 1:
> 
> This CL has passed the run

**General Comment** - @Shaobo Yan (Round 1) (2026-03-31 14:24:44):
> Patch Set 1:
> 
> (1 comment)
> 
> This change is ready for review.

**General Comment** - @Shaobo Yan (Round 1) (2026-03-31 14:24:44):
> <GERRIT_ACCOUNT_1000002>, who was added as reviewer owns the following files:
> * `src/dawn/tests/BUILD.gn`
> * `src/dawn/tests/CMakeLists.txt`
> * `src/dawn/tests/DawnTest.cpp`
> * `src/dawn/tests/end2end/DawnTestInfraTests.cpp`

**General Comment** - @Corentin Wallez (Round 1) (2026-03-31 14:34:34):
> Patch Set 1: Code-Review+1
> 
> (5 comments)


## Inline Review Comments

### File: `/PATCHSET_LEVEL`
**[/PATCHSET_LEVEL:File-level] (Round 1)** - @Corentin Wallez (2026-03-31 14:34:34):
> LGTM, will you also audit the `GetRequiredFeatures` to remove the checks for `SupportsFeature` that are unnecessary? (some tests really do optionally support features, but most don't)

**[/PATCHSET_LEVEL:File-level] (Round 1)** - @Shaobo Yan (2026-03-31 14:24:44):
> PTAL, thanks!


### File: `src/dawn/tests/DawnTest.cpp`
**[src/dawn/tests/DawnTest.cpp:1408] (Round 1)** - @Corentin Wallez (2026-03-31 14:34:34):
> That won't be necessary, by coincidence I'm making https://dawn-review.googlesource.com/c/dawn/+/300495

**[src/dawn/tests/DawnTest.cpp:1613] (Round 1)** - @Corentin Wallez (2026-03-31 14:34:34):
> nit: Can we do something like this somehow? `"Skipping test because " << theFeature << " is not supported by the adapter"`.


### File: `src/dawn/tests/end2end/DawnTestInfraTests.cpp`
**[src/dawn/tests/end2end/DawnTestInfraTests.cpp:41] (Round 1)** - @Corentin Wallez (2026-03-31 14:34:34):
> nit: not neeeded.

**[src/dawn/tests/end2end/DawnTestInfraTests.cpp:60] (Round 1)** - @Corentin Wallez (2026-03-31 14:34:34):
> nit: `.contains`
