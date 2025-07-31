# Copyright 2023 The Dawn & Tint Authors
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

"""Project declaration for the Dawn project."""

def _milestone_details(*, chromium_project, ref, platforms):
    """Define the details for an active milestone.

    Args:
      * chromium_project - The name of the LUCI project that is configured for the
        milestone.
      * ref - The ref in the Dawn git repository that contains the code for the
        milestone.
      * platforms - A list of platform strings that the milestone is active for.
    """
    return struct(
        chromium_project = chromium_project,
        ref = ref,
        platforms = platforms,
    )

ACTIVE_MILESTONES = {
    m["name"]: _milestone_details(
        chromium_project = m["chromium_project"],
        ref = m["ref"],
        platforms = m["platforms"],
    )
    for m in json.decode(io.read_file("./milestones.json")).values()
}

platform = struct(
    ANDROID = "android",
    LINUX = "linux",
    MAC = "mac",
    WINDOWS = "windows",
)

PLATFORMS = tuple([getattr(platform, a) for a in dir(platform)])

def _platform_settings(
        *,
        description,  # buildifier: disable=unused-variable
        gardener_rotation = None):
    """Declare settings for a platform on the project.

    This provides the project-wide settings for a platform, which should be
    modified based on what channels the milestone is being shipped to for each
    platform.

    Args:
      * description - A string describing why the platform is enabled in the
        project. This is not used by the starlark, but enables providing some
        form of documentaion in the json file.
      * gardener_rotation - The name of a gardener rotation in which to include
        builders that are selected for the corresponding platform.
    """
    return struct(
        gardener_rotation = gardener_rotation,
    )

def _project_settings(
        *,
        project,
        ref,
        is_main,
        platforms = {}):
    """Declare settings for the project.

    This provides the centrail location for what must be modified when setting
    up the project for a new branch or when a branch changes category.

    Args:
        * project - The name of the LUCI project. No logic should depend on this
          value, it should only be used where the name of a project is required.
        * ref - The git ref containing the code for this branch. No logic should
          depend on this value, it should only be used where a ref is required.
        * is_main - Whether or not this is the project for the main ref.
        * platforms - A mapping from a platform ID value to the settings for the
          platform. The valid platform ID values are the members of the platform
          struct. The settings for the platforms are dicts giving arguments for
          the _platform_settings function.

    Returns:
        A struct with attributes set to the input parameters.
    """
    if is_main:
        if platforms:
            fail("main project should not have any platforms set")
    else:
        if not platforms:
            fail("Non-main projects must have at least one platform set")
        invalid_platforms = [p for p in platforms if not p in PLATFORMS]
        if invalid_platforms:
            fail("The following platforms are invalid: {}".format(invalid_platforms))

    platforms = {k: _platform_settings(**v) for k, v in platforms.items()}

    return struct(
        project = project,
        ref = ref,
        is_main = is_main,
        platforms = platforms,
    )

settings = _project_settings(**json.decode(io.read_file("./settings.json")))