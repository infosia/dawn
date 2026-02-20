# Copyright 2026 The Dawn & Tint Authors
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Test declarations

Tests define a target to be built and executed on a builder. Tests can
be referenced by a suite or bundle to include the test in the
suite/bundle. Tests also define a bundle containing just the test
itself, so they can be used wherever a bundle is expected.
"""

load("@chromium-luci//targets.star", "targets")

targets.tests.gtest_test(
    name = "dawn_end2end_swangle_tests",
    mixins = [
        "result_adapter_gtest_json",
        "true_noop_merge",
        targets.mixin(
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
    ],
    args = [
        "--backend=opengles",
        "--use-angle=swiftshader",
        "--enable-toggles=gl_force_es_31_and_no_extensions",
    ],
    binary = "dawn_end2end_tests",
)

targets.tests.gtest_test(
    name = "dawn_end2end_sws_tests",
    mixins = [
        "adapter_vendor_id_sws",
        "result_adapter_gtest_json",
        "true_noop_merge",
        targets.mixin(
            swarming = targets.swarming(
                shards = 3,
            ),
        ),
    ],
    binary = "dawn_end2end_tests",
)

targets.tests.isolated_script_test(
    name = "dawn_node_sws_cts",
    mixins = [
        "result_adapter_single",
        "true_noop_merge",
    ],
    args = [
        "-backend",
        "vulkan",
        "-adapter",
        "SwiftShader",
        "webgpu:api,operation,adapter,requestDevice:default:*",
    ],
    binary = "dawn_node_cts",
)

targets.tests.gtest_test(
    name = "dawn_unittests",
    mixins = [
        "result_adapter_gtest_json",
        "true_noop_merge",
    ],
    binary = "dawn_unittests",
)

targets.tests.gtest_test(
    name = "dawn_wire_unittests",
    mixins = [
        "result_adapter_gtest_json",
        "true_noop_merge",
        "use_wire",
    ],
    binary = "dawn_unittests",
)

targets.tests.isolated_script_test(
    name = "tint_benchmark",
    mixins = [
        "result_adapter_single",
        "true_noop_merge",
    ],
    binary = "benchmarks",
)

targets.tests.gtest_test(
    name = "tint_unittests",
    mixins = [
        "result_adapter_gtest_json",
        "true_noop_merge",
    ],
    binary = "tint_unittests",
)
