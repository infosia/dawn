# Dawn's Continuous Testing Infrastructure

Dawn uses Chromium's continuous integration (CI) infrastructure to continually run tests on changes to Dawn and provide a way for developers to run tests against their changes before submitting. CI bots continually build and run tests for every new change, and Try bots build and run developers' pending changes before submission. Dawn uses two different build recipes. There is a Dawn build recipe which checks out Dawn standalone, compiles, and runs the `dawn_unittests`. And, there is the Chromium build recipe which checks out Dawn inside a Chromium checkout. Inside a Chromium checkout, there is more infrastructure available for triggering `dawn_end2end_tests` that run on real GPU hardware, and we are able to run Chromium integration tests as well as tests for WebGPU.

 - Dawn CI Builders: https://ci.chromium.org/p/dawn/g/ci/builders
 - Dawn Try Builders: https://ci.chromium.org/p/dawn/g/try/builders
 - chromium.dawn Waterfall: https://ci.chromium.org/p/chromium/g/chromium.dawn/console

For additional information on GPU testing in Chromium, see: https://chromium.googlesource.com/chromium/src.git/+/main/docs/gpu/gpu_testing_bot_details.md

Additional details on builders, recipes, chromium-based waterfalls and tryjobs are in the original doc.
