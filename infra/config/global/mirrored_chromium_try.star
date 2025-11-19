# Copyright 2025 The Dawn & Tint Authors
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

"""Try Dawn builders that mirror Chromium-defined builders."""

load("//project.star", "ACTIVE_MILESTONES")

################################################################################
# Manual Trybots                                                               #
################################################################################

## Templates

def manual_only_mirror(**kwargs):
    """Creates a manual-only trybot mirror."""
    kwargs.setdefault("cq_group", "Dawn-CQ")
    kwargs.setdefault("includable_only", True)
    luci.cq_tryjob_verifier(**kwargs)

def manual_only_branch_verifier_mirror(*, base_builder_name, platform, min_milestone = None, **kwargs):
    """Creates a manual-only trybot mirror for active milestone branches.

    Args:
        base_builder_name: The name of builder to add without the project/group
            prefix.
        platform: The platform that the builder tests as a string. Used to omit
            builders on branches that do not need to test that platform.
        min_milestone: An optional int representing the first milestone branch
            that the verifier should be added to.
        **kwargs: Other keyword arguments to pass to manual_only_mirror()
    """
    for milestone, details in ACTIVE_MILESTONES.items():
        if platform not in details.platforms:
            continue
        if min_milestone != None and int(milestone[1:]) < min_milestone:
            continue
        manual_only_mirror(
            cq_group = "Dawn-CQ-" + milestone,
            builder = "{}:try/{}".format(details.chromium_project, base_builder_name),
            **kwargs
        )

## Functional testers with branch verifiers.

manual_only_mirror(builder = "chromium:try/dawn-try-mac-arm64-rel")
manual_only_branch_verifier_mirror(base_builder_name = "dawn-try-mac-arm64-deps-rel", min_milestone = 144, platform = "mac")

manual_only_mirror(builder = "chromium:try/dawn-try-win10-x86-rel")
manual_only_branch_verifier_mirror(base_builder_name = "dawn-win10-x86-deps-rel", platform = "win")

## Functional testers without branch verifiers.

manual_only_mirror(builder = "chromium:try/android-dawn-arm64-exp-rel")

# No DEPS equivalent to mirror for branches.
manual_only_mirror(builder = "chromium:try/dawn-try-linux-x64-intel-uhd770-rel")
manual_only_mirror(builder = "chromium:try/linux-dawn-nvidia-1660-exp-rel")

manual_only_mirror(builder = "chromium:try/dawn-try-mac-amd-exp")
manual_only_mirror(builder = "chromium:try/dawn-try-mac-arm64-m2-exp")
manual_only_mirror(builder = "chromium:try/dawn-try-mac-intel-exp")

manual_only_mirror(builder = "chromium:try/dawn-try-win-x64-intel-exp")

# No DEPS equivalent to mirror for branches.
manual_only_mirror(builder = "chromium:try/dawn-try-win-x64-intel-uhd770-rel")
manual_only_mirror(builder = "chromium:try/dawn-try-win-x64-nvidia-exp")
