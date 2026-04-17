# [Gerrit] CL/PR 7639034: Reland "Remove D3D KeyedMutex Path in Media Capture patch"

## Metadata Summary
- **Author**: Shaobo Yan
- **Status**: NEW
- **Total Rounds (Gerrit)**: 2
- **URL**: https://chromium-review.googlesource.com/c/chromium/src/+/7639034

## General PR Comments
**General Comment** - @Shaobo Yan (Round 1) (2026-03-09 01:48:18):
> Patch Set 1: Cherry Picked from branch main.

**General Comment** - @Shaobo Yan (Round 1) (2026-03-09 01:48:18):
> <GERRIT_ACCOUNT_1160859>, who was added as reviewer owns the following files:
> * `gpu/ipc/common/dxgi_helpers.cc`

**General Comment** - @Shaobo Yan (Round 1) (2026-03-09 01:48:18):
> <GERRIT_ACCOUNT_1179950>, who was added as reviewer owns the following files:
> * `media/capture/video/win/d3d_capture_test_utils.cc`
> * `media/capture/video/win/d3d_capture_test_utils.h`
> * `media/capture/video/win/gpu_memory_buffer_tracker_win.cc`
> * `media/capture/video/win/video_capture_device_mf_win.cc`

**General Comment** - @Shaobo Yan (Round 2) (2026-03-09 02:59:43):
> Uploaded patch set 2.
> 
> Reland "Remove D3D KeyedMutex Path in Media Capture patch"

**General Comment** - @Shaobo Yan (Round 2) (2026-03-09 03:00:40):
> Patch Set 2: Commit-Queue+1

**General Comment** - @Chromium LUCI CQ (Round 2) (2026-03-09 03:00:46):
> Patch Set 2:
> 
> Dry run: CV is trying the patch.
> 
> Follow status at: https://luci-change-verifier.appspot.com/ui/run/chromium/8849945556854-1-df6f9d11b0ae9d32
> 
> Bot data: {"action":"start","triggered_at":"2026-03-09T03:00:40Z","revision":"1a20c88760786b6e301b89ec06ffcb57449c785e"}

**General Comment** - @Shaobo Yan (Round 2) (2026-03-09 03:59:18):
> Patch Set 2: -Commit-Queue
> 
> (Performed by <GERRIT_ACCOUNT_1347153> on behalf of <GERRIT_ACCOUNT_3753027>)

**General Comment** - @Chromium LUCI CQ (Round 2) (2026-03-09 03:59:21):
> Patch Set 2:
> 
> This CL has passed the run

**General Comment** - @Shaobo Yan (Round 2) (2026-03-09 04:55:35):
> Patch Set 2: Commit-Queue+1
> 
> (1 comment)

**General Comment** - @Chromium LUCI CQ (Round 2) (2026-03-09 04:55:42):
> Patch Set 2:
> 
> Dry run: CV is trying the patch.
> 
> Follow status at: https://luci-change-verifier.appspot.com/ui/run/chromium/8849938661854-1-e231292c70e95893
> 
> Bot data: {"action":"start","triggered_at":"2026-03-09T04:55:35Z","revision":"1a20c88760786b6e301b89ec06ffcb57449c785e"}

**General Comment** - @Shaobo Yan (Round 2) (2026-03-09 04:57:00):
> <GERRIT_ACCOUNT_1163549>, who was added as reviewer owns the following files:
> * `content/test/gpu/gpu_tests/trace_integration_test.py`

**General Comment** - @Shaobo Yan (Round 2) (2026-03-09 04:59:35):
> Patch Set 2: -Commit-Queue
> 
> (Performed by <GERRIT_ACCOUNT_1347153> on behalf of <GERRIT_ACCOUNT_3753027>)

**General Comment** - @Chromium LUCI CQ (Round 2) (2026-03-09 04:59:36):
> Patch Set 2:
> 
> This CL has passed the run

**General Comment** - @Will Harris (Round 2) (2026-03-09 05:17:28):
> Patch Set 2: Code-Review+1
> 
> (1 comment)

**General Comment** - @Ilya Nikolaevskiy (Round 2) (2026-03-09 10:26:55):
> Patch Set 2:
> 
> (1 comment)

**General Comment** - @Ilya Nikolaevskiy (Round 2) (2026-03-09 10:28:37):
> Patch Set 2:
> 
> (1 comment)

**General Comment** - @Ilya Nikolaevskiy (Round 2) (2026-03-09 10:30:02):
> Patch Set 2:
> 
> (1 comment)

**General Comment** - @Shaobo Yan (Round 2) (2026-03-10 01:34:00):
> Patch Set 2:
> 
> (1 comment)

**General Comment** - @Ilya Nikolaevskiy (Round 2) (2026-03-10 09:30:30):
> Patch Set 2:
> 
> (1 comment)


## Inline Review Comments

### File: `/PATCHSET_LEVEL`
**[/PATCHSET_LEVEL:File-level] (Round 2)** - @Will Harris (2026-03-09 05:17:28):
> No change to gpu/ipc/common/dxgi_helpers.cc so lgtm but I don't really understand the difference between ps1 and ps2 and it seems not to have code coverage anyway.. ?

**[/PATCHSET_LEVEL:File-level] (Round 2)** - @Shaobo Yan (2026-03-09 04:55:35):
> Relanding by fix the trace test. PTAL, thanks!

**[/PATCHSET_LEVEL:File-level] (Round 2)** - @Ilya Nikolaevskiy (2026-03-09 10:28:37):
> The issue was the hardcoded test, expecting specific behavior from the GPU process as an evidence that hardware accelerated camera capture works end-to-end. 
> That behavior was removed, but the test wasn't updated.


### File: `content/test/gpu/gpu_tests/trace_integration_test.py`
**[content/test/gpu/gpu_tests/trace_integration_test.py:131] (Round 2)** - @Ilya Nikolaevskiy (2026-03-09 10:26:55):
> This happens when the texture frame is being converted/mapped for software encoding. I'm not sure if this is the correct one. We want something to show evidence that the GPU process will try to render the texture frame without falling back to something different.
> 
> I think something from the D3D_image_backing is more appropriate here, but I'm not sure.

**[content/test/gpu/gpu_tests/trace_integration_test.py:131] (Round 2)** - @Ilya Nikolaevskiy (2026-03-09 10:30:02):
> Sorry, didn't notice that you've added this event yourself. But you've added it in the capture, and that's already covered in _MFD3D11VC_CAPTURE_EVENT_NAME. This PRESENT_EVENT_NAME is supposed to cover something in the GPU process while rendering the texture.

**[content/test/gpu/gpu_tests/trace_integration_test.py:131] (Round 2)** - @Shaobo Yan (2026-03-10 01:34:00):
> Ah, I didn't noticed that CAPTURE_EVENT_NAME. Thanks for pointing it out. So do you think we could remove the `_MFD3D11VC_PRESENT_EVENT_NAME ` ? Because the logic is that we try to handle sync in capture process and then pass the frame to GPU process. So it shouldn't have any events.

**[content/test/gpu/gpu_tests/trace_integration_test.py:131] (Round 2)** - @Ilya Nikolaevskiy (2026-03-10 09:30:30):
> No, I don't think we should remove it. We just have to use something other than KeyedMutex events. It's not checking that the image is synchronized. It tests that image is being presented, i.e. rendered. I suggest adding some event in d3d_image_backing, but I don't know what there is called when the image is rendered.
