// Copyright 2025 The Dawn & Tint Authors
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <vector>

#include "dawn/common/Constants.h"
#include "dawn/tests/unittests/validation/ValidationTest.h"
#include "dawn/utils/ComboRenderPipelineDescriptor.h"
#include "dawn/utils/ScopedIgnoreValidationErrors.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

class DynamicBindingArrayTests : public ValidationTest {
  protected:
    std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
        return {wgpu::FeatureName::ChromiumExperimentalBindless};
    }

    // Helper similar to utils::MakeBindGroupLayout but that adds a dynamic array.
    wgpu::BindGroupLayout MakeBindGroupLayout(
        wgpu::DynamicBindingKind kind,
        uint32_t dynamicArrayStart = 0,
        std::initializer_list<utils::BindingLayoutEntryInitializationHelper> entriesInitializer =
            {}) {
        std::vector<wgpu::BindGroupLayoutEntry> entries;
        for (const utils::BindingLayoutEntryInitializationHelper& entry : entriesInitializer) {
            entries.push_back(entry);
        }

        wgpu::BindGroupLayoutDynamicBindingArray dynamic;
        dynamic.dynamicArray.kind = kind;
        dynamic.dynamicArray.start = dynamicArrayStart;

        wgpu::BindGroupLayoutDescriptor descriptor;
        descriptor.nextInChain = &dynamic;
        descriptor.entryCount = entries.size();
        descriptor.entries = entries.data();
        return device.CreateBindGroupLayout(&descriptor);
    }

    // Helper similar to utils::MakeBindGroup but that adds a dynamic array.
    wgpu::BindGroup MakeBindGroup(
        const wgpu::BindGroupLayout& layout,
        uint32_t dynamicArraySize,
        std::initializer_list<utils::BindingInitializationHelper> entriesInitializer) {
        std::vector<wgpu::BindGroupEntry> entries;
        for (const utils::BindingInitializationHelper& helper : entriesInitializer) {
            entries.push_back(helper.GetAsBinding());
        }

        wgpu::BindGroupDynamicBindingArray dynamic;
        dynamic.dynamicArraySize = dynamicArraySize;

        wgpu::BindGroupDescriptor descriptor;
        descriptor.nextInChain = &dynamic;
        descriptor.layout = layout;
        descriptor.entryCount = entries.size();
        descriptor.entries = entries.data();

        return device.CreateBindGroup(&descriptor);
    }

    // Helper to make sure that the dynamic array is marked as used. Even if internally Dawn doesn't
    // track this, it makes tests more clearly correct.
    void UseDynamicArrayInSubmit(wgpu::BindGroup dynamicArray) {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.SetBindGroup(0, dynamicArray);
        pass.End();

        wgpu::CommandBuffer commands = encoder.Finish();
        device.GetQueue().Submit(1, &commands);
    }
};

class DynamicBindingArrayTests_FeatureDisabled : public ValidationTest {};

// Test that it is not allowed to call Update, RemoveBinding or InsertBinding when there is no
// dynamic binding array.
TEST_F(DynamicBindingArrayTests, MutatorsWithoutDynamicBindingArray) {
    // TODO(435317394): Implemented bindless in the wire.
    if (UsesWire()) {
        GTEST_SKIP();
    }

    // Test on a valid bindgroup that doesn't have a dynamic array.
    {
        wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(device, {});
        wgpu::BindGroup bg = utils::MakeBindGroup(device, bgl, {});

        wgpu::BindGroupEntry entry{.binding = 0};
        EXPECT_EQ(wgpu::Status::Error, bg.Update(&entry));

        wgpu::BindGroupEntryContents contents = {};
        EXPECT_EQ(wgpu::kInvalidBinding, bg.InsertBinding(&contents));

        EXPECT_EQ(wgpu::Status::Error, bg.RemoveBinding(0));
    }
    // Test on a invalid bindgroup that doesn't have a dynamic array.
    {
        wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Uniform}});
        wgpu::BindGroup bg;
        ASSERT_DEVICE_ERROR(bg = utils::MakeBindGroup(device, bgl, {}));

        wgpu::BindGroupEntry entry{.binding = 0};
        EXPECT_EQ(wgpu::Status::Error, bg.Update(&entry));

        wgpu::BindGroupEntryContents contents = {};
        EXPECT_EQ(wgpu::kInvalidBinding, bg.InsertBinding(&contents));

        EXPECT_EQ(wgpu::Status::Error, bg.RemoveBinding(0));
    }
}

// Test that it is not allowed to call Update, RemoveBinding or InsertBinding after the bind group
// is destroyed.
TEST_F(DynamicBindingArrayTests, MutatorsAfterDestroy) {
    // TODO(435317394): Implemented bindless in the wire.
    if (UsesWire()) {
        GTEST_SKIP();
    }

    // Test on a valid bindgroup that doesn't have a dynamic array.
    {
        wgpu::BindGroupLayout bgl = MakeBindGroupLayout(wgpu::DynamicBindingKind::SampledTexture);
        wgpu::BindGroup bg = MakeBindGroup(bgl, 3, {});
        bg.Destroy();

        wgpu::BindGroupEntry entry{.binding = 0};
        EXPECT_EQ(wgpu::Status::Error, bg.Update(&entry));

        wgpu::BindGroupEntryContents contents = {};
        EXPECT_EQ(wgpu::kInvalidBinding, bg.InsertBinding(&contents));

        EXPECT_EQ(wgpu::Status::Error, bg.RemoveBinding(0));
    }
    // Test on a invalid bindgroup that doesn't have a dynamic array.
    {
        // Buffer that we'll try to put in the dynamic binding array to make it fail its creation.
        wgpu::BufferDescriptor bufDesc{
            .usage = wgpu::BufferUsage::Uniform,
            .size = 4,
        };
        wgpu::Buffer buffer = device.CreateBuffer(&bufDesc);

        wgpu::BindGroupLayout bgl = MakeBindGroupLayout(wgpu::DynamicBindingKind::SampledTexture);
        wgpu::BindGroup bg;
        ASSERT_DEVICE_ERROR(bg = MakeBindGroup(bgl, 3, {{0, buffer}}));
        bg.Destroy();

        wgpu::BindGroupEntry entry{.binding = 0};
        EXPECT_EQ(wgpu::Status::Error, bg.Update(&entry));

        wgpu::BindGroupEntryContents contents = {};
        EXPECT_EQ(wgpu::kInvalidBinding, bg.InsertBinding(&contents));

        EXPECT_EQ(wgpu::Status::Error, bg.RemoveBinding(0));
    }
}

// Test that the binding used for Update/RemoveBinding must be in range of the dynamic binding
// array.
TEST_F(DynamicBindingArrayTests, MutatorsOutOfRangeOfDynamicBindingArray) {
    // TODO(435317394): Implemented bindless in the wire.
    if (UsesWire()) {
        GTEST_SKIP();
    }

    auto Test = [&](wgpu::BindGroup bg, uint32_t start, uint32_t effectiveSize) {
        // Ignore all validation errors for this test as they are tested in other places, and we're
        // checking immediate validation returned as a wgpu::Status and supposed to be the same for
        // valid and invalid objects.
        utils::ScopedIgnoreValidationErrors ignoreErrors(device);

        wgpu::BindGroupEntry entry;

        // Valid case: the first entry can be modified.
        entry.binding = start;
        EXPECT_EQ(wgpu::Status::Success, bg.Update(&entry));
        EXPECT_EQ(wgpu::Status::Success, bg.RemoveBinding(entry.binding));

        // Valid case: the last entry can be modified.
        entry.binding = start + effectiveSize - 1;
        EXPECT_EQ(wgpu::Status::Success, bg.Update(&entry));
        EXPECT_EQ(wgpu::Status::Success, bg.RemoveBinding(entry.binding));

        // Error case: the entry before the first one cannot be modified.
        if (start != 0) {
            entry.binding = start - 1;
            EXPECT_EQ(wgpu::Status::Error, bg.Update(&entry));
            EXPECT_EQ(wgpu::Status::Error, bg.RemoveBinding(entry.binding));
        }

        // Error case: the entry after the last cannot be modified.
        entry.binding = start + effectiveSize;
        EXPECT_EQ(wgpu::Status::Error, bg.Update(&entry));
        EXPECT_EQ(wgpu::Status::Error, bg.RemoveBinding(entry.binding));
    };

    // Test on a valid dynamic binding array starting at 0.
    {
        wgpu::BindGroupLayout bgl =
            MakeBindGroupLayout(wgpu::DynamicBindingKind::SampledTexture, 0);
        wgpu::BindGroup bg = MakeBindGroup(bgl, 42, {});
        Test(bg, 0, 42);
    }

    // Test on a valid dynamic binding array starting not at 0.
    {
        wgpu::BindGroupLayout bgl =
            MakeBindGroupLayout(wgpu::DynamicBindingKind::SampledTexture, 17);
        wgpu::BindGroup bg = MakeBindGroup(bgl, 42, {});
        Test(bg, 17, 42);
    }

    // Test on an invalid dynamic binding array starting at 0.
    {
        // Buffer that we'll try to put in the dynamic binding array to make it fail its creation.
        wgpu::BufferDescriptor bufDesc{
            .usage = wgpu::BufferUsage::Uniform,
            .size = 4,
        };
        wgpu::Buffer buffer = device.CreateBuffer(&bufDesc);

        wgpu::BindGroupLayout bgl =
            MakeBindGroupLayout(wgpu::DynamicBindingKind::SampledTexture, 0);
        wgpu::BindGroup bg;
        ASSERT_DEVICE_ERROR(bg = MakeBindGroup(bgl, 84, {{0, buffer}}));
        Test(bg, 0, 84);
    }

    // Test on a invalid dynamic binding array starting not at 0.
    {
        wgpu::BindGroupLayout bgl = MakeBindGroupLayout(
            wgpu::DynamicBindingKind::SampledTexture, 17,
            {{0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::Uniform}});
        wgpu::BindGroup bg;
        ASSERT_DEVICE_ERROR(bg = MakeBindGroup(bgl, 42, {}));
        Test(bg, 17, 42);
    }

    // Test on a invalid dynamic binding array with a size larger than kMaxDynamicBindingArraySize.
    // The effective size must be clamped to kMaxDynamicBindingArraySize.
    {
        wgpu::BindGroupLayout bgl = MakeBindGroupLayout(
            wgpu::DynamicBindingKind::SampledTexture, 1,
            {{0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::Uniform}});
        wgpu::BindGroup bg;
        ASSERT_DEVICE_ERROR(bg = MakeBindGroup(bgl, kMaxDynamicBindingArraySize * 2, {}));
        Test(bg, 1, kMaxDynamicBindingArraySize);
    }
}

// Test that Update/RemoveBinding return success but generate a validation error when used on an
// invalid bindgroup.
TEST_F(DynamicBindingArrayTests, UpdateRemoveOnInvalidBindgroup) {
    // TODO(435317394): Implemented bindless in the wire.
    if (UsesWire()) {
        GTEST_SKIP();
    }

    wgpu::TextureDescriptor tDesc{
        .usage = wgpu::TextureUsage::TextureBinding,
        .size = {1, 1},
        .format = wgpu::TextureFormat::R32Uint,
    };
    wgpu::Texture tex = device.CreateTexture(&tDesc);
    wgpu::BindGroupEntry entry{.binding = 0, .textureView = tex.CreateView()};

    // Test on a valid bindgroup that has a dynamic array.
    {
        wgpu::BindGroupLayout bgl = MakeBindGroupLayout(wgpu::DynamicBindingKind::SampledTexture);
        wgpu::BindGroup bg = MakeBindGroup(bgl, 3, {});

        EXPECT_EQ(wgpu::Status::Success, bg.Update(&entry));
        EXPECT_EQ(wgpu::Status::Success, bg.RemoveBinding(entry.binding));
    }
    // Test on a invalid bindgroup that has a dynamic array.
    {
        wgpu::BindGroupLayout bgl = MakeBindGroupLayout(wgpu::DynamicBindingKind::SampledTexture);
        wgpu::BindGroup bg;
        ASSERT_DEVICE_ERROR(bg = MakeBindGroup(bgl, 3, {{20, tex.CreateView()}}));

        ASSERT_DEVICE_ERROR(EXPECT_EQ(wgpu::Status::Success, bg.Update(&entry)));
        ASSERT_DEVICE_ERROR(EXPECT_EQ(wgpu::Status::Success, bg.RemoveBinding(entry.binding)));
    }
}

// TODO(435317394): Add tests that the resources passed to Update() and InsertBinding() are
// compatible with the DynamicArrayKind (and generate a validation error otherwise).

// Test that Update() can be called on a dynamic array slot if it has never been used before.
TEST_F(DynamicBindingArrayTests, UpdateBindingWhenNeverUsed) {
    // TODO(435317394): Implemented bindless in the wire.
    if (UsesWire()) {
        GTEST_SKIP();
    }

    wgpu::TextureDescriptor tDesc{
        .usage = wgpu::TextureUsage::TextureBinding,
        .size = {1, 1},
        .format = wgpu::TextureFormat::R32Uint,
    };
    wgpu::Texture tex = device.CreateTexture(&tDesc);

    wgpu::BindGroupLayout bgl = MakeBindGroupLayout(wgpu::DynamicBindingKind::SampledTexture);
    wgpu::BindGroup bg = MakeBindGroup(bgl, 3, {{0, tex.CreateView()}});

    // Trying to update slot 0 that contains a resource is invalid.
    wgpu::BindGroupEntry entry0{.binding = 0, .textureView = tex.CreateView()};
    EXPECT_EQ(wgpu::Status::Error, bg.Update(&entry0));

    // Immediately updating slot 1 which is empty is valid.
    wgpu::BindGroupEntry entry1{.binding = 1, .textureView = tex.CreateView()};
    EXPECT_EQ(wgpu::Status::Success, bg.Update(&entry1));

    // Even after using the dynamic array, updating a previously unused is valid.
    UseDynamicArrayInSubmit(bg);

    wgpu::BindGroupEntry entry2{.binding = 2, .textureView = tex.CreateView()};
    EXPECT_EQ(wgpu::Status::Success, bg.Update(&entry2));
}

// Test that Remove() can be called on a dynamic array slot even when it was never used.
TEST_F(DynamicBindingArrayTests, RemoveBindingWhenNeverUsed) {
    // TODO(435317394): Implemented bindless in the wire.
    if (UsesWire()) {
        GTEST_SKIP();
    }

    wgpu::BindGroupLayout bgl = MakeBindGroupLayout(wgpu::DynamicBindingKind::SampledTexture);
    wgpu::BindGroup bg = MakeBindGroup(bgl, 1, {});
    EXPECT_EQ(wgpu::Status::Success, bg.RemoveBinding(0));
}

// Test that Remove() can be called on a dynamic array slot set at creation.
TEST_F(DynamicBindingArrayTests, RemoveBindingWhenSetAtCreation) {
    // TODO(435317394): Implemented bindless in the wire.
    if (UsesWire()) {
        GTEST_SKIP();
    }

    wgpu::TextureDescriptor tDesc{
        .usage = wgpu::TextureUsage::TextureBinding,
        .size = {1, 1},
        .format = wgpu::TextureFormat::R32Uint,
    };
    wgpu::Texture tex = device.CreateTexture(&tDesc);

    wgpu::BindGroupLayout bgl = MakeBindGroupLayout(wgpu::DynamicBindingKind::SampledTexture);
    wgpu::BindGroup bg = MakeBindGroup(bgl, 1, {{0, tex.CreateView()}});
    EXPECT_EQ(wgpu::Status::Success, bg.RemoveBinding(0));
}

// Check that a dynamic array slot can be updated only after all commands submitted prior to
// RemoveBinding are completed.
TEST_F(DynamicBindingArrayTests, UpdateAfterRemoveRequiresGPUIsFinished) {
    // TODO(435317394): Implemented bindless in the wire.
    if (UsesWire()) {
        GTEST_SKIP();
    }

    wgpu::TextureDescriptor tDesc{
        .usage = wgpu::TextureUsage::TextureBinding,
        .size = {1, 1},
        .format = wgpu::TextureFormat::R32Uint,
    };
    wgpu::Texture tex = device.CreateTexture(&tDesc);
    wgpu::BindGroupEntry entry{.binding = 0, .textureView = tex.CreateView()};

    wgpu::BindGroupLayout bgl = MakeBindGroupLayout(wgpu::DynamicBindingKind::SampledTexture);
    wgpu::BindGroup bg = MakeBindGroup(bgl, 1, {{0, tex.CreateView()}});

    // Removing while the dynamic array is still potentially in used by the GPU is an error. But
    // immediately after we know that the GPU is finished, it is valid.
    bool updateValid = false;
    UseDynamicArrayInSubmit(bg);
    device.GetQueue().OnSubmittedWorkDone(
        wgpu::CallbackMode::AllowSpontaneous,
        [&](wgpu::QueueWorkDoneStatus, wgpu::StringView) { updateValid = true; });
    EXPECT_EQ(wgpu::Status::Success, bg.RemoveBinding(0));

    // The null backend happens to call OnSubmittedWorkDone immediately because commands take 0
    // time. This test is duplicated in the end2end tests where OnSubmittedWorkDone won't fire
    // immediately.
    if (updateValid) {
        EXPECT_EQ(wgpu::Status::Success, bg.Update(&entry));
        updateValid = false;
    } else {
        EXPECT_EQ(wgpu::Status::Error, bg.Update(&entry));
    }

    WaitForAllOperations();

    if (updateValid) {
        EXPECT_EQ(wgpu::Status::Success, bg.Update(&entry));
    } else {
        EXPECT_EQ(wgpu::Status::Error, bg.Update(&entry));
    }
}

// Check that a dynamic array slot can be updated only after all commands submitted prior to
// RemoveBinding are completed.
TEST_F(DynamicBindingArrayTests, UpdateAfterRemoveRequiresGPUIsFinished_ErrorBindGroup) {
    // TODO(435317394): Implemented bindless in the wire.
    if (UsesWire()) {
        GTEST_SKIP();
    }

    wgpu::TextureDescriptor tDesc{
        .usage = wgpu::TextureUsage::TextureBinding,
        .size = {1, 1},
        .format = wgpu::TextureFormat::R32Uint,
    };
    wgpu::Texture tex = device.CreateTexture(&tDesc);
    wgpu::BindGroupEntry entry{.binding = 1, .textureView = tex.CreateView()};

    wgpu::BindGroupLayout bgl =
        MakeBindGroupLayout(wgpu::DynamicBindingKind::SampledTexture, 1,
                            {{0, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Uniform}});
    wgpu::BindGroup bg;
    ASSERT_DEVICE_ERROR(bg = MakeBindGroup(bgl, 1, {{1, tex.CreateView()}}));

    {
        // Ignore all validation errors for this test as they are tested in other places, and we're
        // checking immediate validation returned as a wgpu::Status and supposed to be the same for
        // valid and invalid objects.
        utils::ScopedIgnoreValidationErrors ignoreErrors(device);

        // Removing while the dynamic array is still potentially in used by the GPU is an error. But
        // immediately after we know that the GPU is finished, it is valid.
        bool updateValid = false;
        device.GetQueue().Submit(0, nullptr);
        device.GetQueue().OnSubmittedWorkDone(
            wgpu::CallbackMode::AllowSpontaneous,
            [&](wgpu::QueueWorkDoneStatus, wgpu::StringView) { updateValid = true; });
        EXPECT_EQ(wgpu::Status::Success, bg.RemoveBinding(1));

        // The null backend happens to call OnSubmittedWorkDone immediately because commands take 0
        // time. This test is duplicated in the end2end tests where OnSubmittedWorkDone won't fire
        // immediately.
        if (updateValid) {
            EXPECT_EQ(wgpu::Status::Success, bg.Update(&entry));
            updateValid = false;
        } else {
            EXPECT_EQ(wgpu::Status::Error, bg.Update(&entry));
        }

        WaitForAllOperations();

        if (updateValid) {
            EXPECT_EQ(wgpu::Status::Success, bg.Update(&entry));
        } else {
            EXPECT_EQ(wgpu::Status::Error, bg.Update(&entry));
        }
    }
}

// Check that InsertBinding on a bindgroup without a dynamic array returns an wgpu::kInvalidBinding.
TEST_F(DynamicBindingArrayTests, InsertBindingNoDynamicBindingArray) {
    // TODO(435317394): Implemented bindless in the wire.
    if (UsesWire()) {
        GTEST_SKIP();
    }

    wgpu::TextureDescriptor tDesc{
        .usage = wgpu::TextureUsage::TextureBinding,
        .size = {1, 1},
        .format = wgpu::TextureFormat::R32Uint,
    };
    wgpu::Texture tex = device.CreateTexture(&tDesc);
    wgpu::BindGroupEntryContents contents = {.textureView = tex.CreateView()};

    // Success case: bind group has a dynamic array and is not an error.
    {
        wgpu::BindGroupLayout bgl = MakeBindGroupLayout(wgpu::DynamicBindingKind::SampledTexture);
        wgpu::BindGroup bg = MakeBindGroup(bgl, 1, {});
        EXPECT_EQ(0u, bg.InsertBinding(&contents));
    }

    // Also a success case: bind group has a dynamic array even if it is an error bind group.
    {
        wgpu::BindGroupLayout bgl = MakeBindGroupLayout(wgpu::DynamicBindingKind::SampledTexture);
        wgpu::BindGroup bg;
        ASSERT_DEVICE_ERROR(bg = MakeBindGroup(bgl, 1, {{20, tex.CreateView()}}));
        ASSERT_DEVICE_ERROR(EXPECT_EQ(0u, bg.InsertBinding(&contents)));
    }

    // Error case: bind group does not have a dynamic array, and is not an error bind group.
    {
        wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(device, {});
        wgpu::BindGroup bg = utils::MakeBindGroup(device, bgl, {});
        EXPECT_EQ(wgpu::kInvalidBinding, bg.InsertBinding(&contents));
    }

    // Also a success case: bind group does not have a dynamic array even if it is an error bind
    // group.
    {
        wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::Uniform}});
        wgpu::BindGroup bg;
        ASSERT_DEVICE_ERROR(bg = utils::MakeBindGroup(device, bgl, {}));
        EXPECT_EQ(wgpu::kInvalidBinding, bg.InsertBinding(&contents));
    }
}

// Check that InsertBinding on a destroyed bindgroup returns an wgpu::kInvalidBinding.
TEST_F(DynamicBindingArrayTests, InsertBindingDestroyed) {
    // TODO(435317394): Implemented bindless in the wire.
    if (UsesWire()) {
        GTEST_SKIP();
    }

    wgpu::TextureDescriptor tDesc{
        .usage = wgpu::TextureUsage::TextureBinding,
        .size = {1, 1},
        .format = wgpu::TextureFormat::R32Uint,
    };
    wgpu::Texture tex = device.CreateTexture(&tDesc);
    wgpu::BindGroupEntryContents contents = {.textureView = tex.CreateView()};

    // Success case: bind group has a dynamic array and is not destroyed.
    {
        wgpu::BindGroupLayout bgl = MakeBindGroupLayout(wgpu::DynamicBindingKind::SampledTexture);
        wgpu::BindGroup bg = MakeBindGroup(bgl, 1, {});
        EXPECT_EQ(0u, bg.InsertBinding(&contents));
    }

    // Also a success case: bind group isn't destroyed even if it is an error bind group.
    {
        wgpu::BindGroupLayout bgl = MakeBindGroupLayout(wgpu::DynamicBindingKind::SampledTexture);
        wgpu::BindGroup bg;
        ASSERT_DEVICE_ERROR(bg = MakeBindGroup(bgl, 1, {{20, tex.CreateView()}}));
        ASSERT_DEVICE_ERROR(EXPECT_EQ(0u, bg.InsertBinding(&contents)));
    }

    // Error case: bind group is destroyed.
    {
        wgpu::BindGroupLayout bgl = MakeBindGroupLayout(wgpu::DynamicBindingKind::SampledTexture);
        wgpu::BindGroup bg = MakeBindGroup(bgl, 1, {});
        bg.Destroy();
        EXPECT_EQ(wgpu::kInvalidBinding, bg.InsertBinding(&contents));
    }

    // Also a success case: bind group is destroyed even if it is an error bind group.
    {
        wgpu::BindGroupLayout bgl = MakeBindGroupLayout(wgpu::DynamicBindingKind::SampledTexture);
        wgpu::BindGroup bg;
        ASSERT_DEVICE_ERROR(bg = MakeBindGroup(bgl, 1, {{20, tex.CreateView()}}));
        bg.Destroy();
        EXPECT_EQ(wgpu::kInvalidBinding, bg.InsertBinding(&contents));
    }
}

// Check that InsertBinding works (even if it produces a validation error) on error bindgroups.
TEST_F(DynamicBindingArrayTests, InsertBindingWorksOnErrorBindGroup) {
    // TODO(435317394): Implemented bindless in the wire.
    if (UsesWire()) {
        GTEST_SKIP();
    }

    wgpu::TextureDescriptor tDesc{
        .usage = wgpu::TextureUsage::TextureBinding,
        .size = {1, 1},
        .format = wgpu::TextureFormat::R32Uint,
    };
    wgpu::Texture tex = device.CreateTexture(&tDesc);
    wgpu::BindGroupEntryContents contents = {.textureView = tex.CreateView()};

    wgpu::BindGroupLayout bgl = MakeBindGroupLayout(wgpu::DynamicBindingKind::SampledTexture);
    wgpu::BindGroup bg;
    ASSERT_DEVICE_ERROR(bg = MakeBindGroup(bgl, 1, {{20, tex.CreateView()}}));
    // InsertBinding returns a valid new binding, just that it also produces a validation error.
    ASSERT_DEVICE_ERROR(EXPECT_EQ(0u, bg.InsertBinding(&contents)));
}

// Check that InsertBinding correctly takes into account that the dynamic array may not start at 0.
TEST_F(DynamicBindingArrayTests, InsertBindingDynamicArrayStartNot0) {
    // TODO(435317394): Implemented bindless in the wire.
    if (UsesWire()) {
        GTEST_SKIP();
    }

    wgpu::TextureDescriptor tDesc{
        .usage = wgpu::TextureUsage::TextureBinding,
        .size = {1, 1},
        .format = wgpu::TextureFormat::R32Uint,
    };
    wgpu::Texture tex = device.CreateTexture(&tDesc);
    wgpu::BindGroupEntryContents contents = {.textureView = tex.CreateView()};

    wgpu::BindGroupLayout bgl = MakeBindGroupLayout(wgpu::DynamicBindingKind::SampledTexture, 42);
    wgpu::BindGroup bg = MakeBindGroup(bgl, 1, {});
    EXPECT_EQ(42u, bg.InsertBinding(&contents));
}

// Check that trying to insert bindings fail when no more are available.
TEST_F(DynamicBindingArrayTests, InsertBindingFailWhenNoMoreSpace) {
    // TODO(435317394): Implemented bindless in the wire.
    if (UsesWire()) {
        GTEST_SKIP();
    }

    wgpu::TextureDescriptor tDesc{
        .usage = wgpu::TextureUsage::TextureBinding,
        .size = {1, 1},
        .format = wgpu::TextureFormat::R32Uint,
    };
    wgpu::Texture tex = device.CreateTexture(&tDesc);
    wgpu::BindGroupEntryContents contents = {.textureView = tex.CreateView()};

    for (bool errorBG : {false, true}) {
        wgpu::BindGroupLayout bgl = MakeBindGroupLayout(wgpu::DynamicBindingKind::SampledTexture);
        wgpu::BindGroup bg;
        if (errorBG) {
            ASSERT_DEVICE_ERROR(bg = MakeBindGroup(bgl, 3, {{1000000, tex.CreateView()}}));
        } else {
            bg = MakeBindGroup(bgl, 3, {});
        }

        // Ignore all validation errors caused by using an error bindgroup.
        utils::ScopedIgnoreValidationErrors ignoreErrors(errorBG ? device : nullptr);

        // There is space for only three bindings.
        EXPECT_EQ(0u, bg.InsertBinding(&contents));
        EXPECT_EQ(1u, bg.InsertBinding(&contents));
        EXPECT_EQ(2u, bg.InsertBinding(&contents));
        EXPECT_EQ(wgpu::kInvalidBinding, bg.InsertBinding(&contents));

        // Remove one binding (and wait for it to be recycled), it will be available for
        // InsertBinding after which a new InsertBinding will still run out of space.
        EXPECT_EQ(wgpu::Status::Success, bg.RemoveBinding(1));
        UseDynamicArrayInSubmit(bg);
        WaitForAllOperations();

        EXPECT_EQ(1u, bg.InsertBinding(&contents));
        EXPECT_EQ(wgpu::kInvalidBinding, bg.InsertBinding(&contents));
    }
}

// Check that bindings that are inserted are unavailable for Update() until RemoveBinding.
TEST_F(DynamicBindingArrayTests, InsertBindingPreventsUpdate) {
    // TODO(435317394): Implemented bindless in the wire.
    if (UsesWire()) {
        GTEST_SKIP();
    }

    wgpu::TextureDescriptor tDesc{
        .usage = wgpu::TextureUsage::TextureBinding,
        .size = {1, 1},
        .format = wgpu::TextureFormat::R32Uint,
    };
    wgpu::Texture tex = device.CreateTexture(&tDesc);
    wgpu::BindGroupEntry entry = {.binding = 0, .textureView = tex.CreateView()};
    wgpu::BindGroupEntryContents contents = {.textureView = tex.CreateView()};

    for (bool errorBG : {false, true}) {
        wgpu::BindGroupLayout bgl = MakeBindGroupLayout(wgpu::DynamicBindingKind::SampledTexture);
        wgpu::BindGroup bg;
        if (errorBG) {
            ASSERT_DEVICE_ERROR(bg = MakeBindGroup(bgl, 1, {{1000000, tex.CreateView()}}));
        } else {
            bg = MakeBindGroup(bgl, 1, {});
        }

        // Ignore all validation errors caused by using an error bindgroup.
        utils::ScopedIgnoreValidationErrors ignoreErrors(errorBG ? device : nullptr);

        EXPECT_EQ(0u, bg.InsertBinding(&contents));
        EXPECT_EQ(wgpu::Status::Error, bg.Update(&entry));

        // Remove one binding (and wait for it to be recycled), it will be available for Update.
        EXPECT_EQ(wgpu::Status::Success, bg.RemoveBinding(0));
        UseDynamicArrayInSubmit(bg);
        WaitForAllOperations();

        EXPECT_EQ(wgpu::Status::Success, bg.Update(&entry));
    }
}

// Check that InsertBinding skips over used slots.
TEST_F(DynamicBindingArrayTests, InsertBindingSkipsOverUsedSlots) {
    // TODO(435317394): Implemented bindless in the wire.
    if (UsesWire()) {
        GTEST_SKIP();
    }

    wgpu::TextureDescriptor tDesc{
        .usage = wgpu::TextureUsage::TextureBinding,
        .size = {1, 1},
        .format = wgpu::TextureFormat::R32Uint,
    };
    wgpu::Texture tex = device.CreateTexture(&tDesc);
    wgpu::BindGroupEntry entry1 = {.binding = 1, .textureView = tex.CreateView()};
    wgpu::BindGroupEntry entry3 = {.binding = 3, .textureView = tex.CreateView()};
    wgpu::BindGroupEntryContents contents = {.textureView = tex.CreateView()};

    for (bool errorBG : {false, true}) {
        // Create a bind group with entry 1 used at creation.
        wgpu::BindGroupLayout bgl = MakeBindGroupLayout(wgpu::DynamicBindingKind::SampledTexture);
        wgpu::BindGroup bg;
        if (errorBG) {
            ASSERT_DEVICE_ERROR(
                bg = MakeBindGroup(bgl, 5, {{1, tex.CreateView()}, {1000000, tex.CreateView()}}));
            // TODO(435317394): This shouldn't be needed and instead valid bindings should mark the
            // slot as used on creation?
            ASSERT_DEVICE_ERROR(EXPECT_EQ(wgpu::Status::Success, bg.Update(&entry1)));
        } else {
            bg = MakeBindGroup(bgl, 5, {{1, tex.CreateView()}});
        }

        // Ignore all validation errors caused by using an error bindgroup.
        utils::ScopedIgnoreValidationErrors ignoreErrors(errorBG ? device : nullptr);

        EXPECT_EQ(wgpu::Status::Success, bg.Update(&entry3));

        // InsertBinding skips over entries used at creation of by Update()
        EXPECT_EQ(0u, bg.InsertBinding(&contents));
        EXPECT_EQ(2u, bg.InsertBinding(&contents));
        EXPECT_EQ(4u, bg.InsertBinding(&contents));
    }
}

}  // anonymous namespace
}  // namespace dawn
