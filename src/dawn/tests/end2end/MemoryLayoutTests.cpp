#include <cmath>
#include <string>
#include <vector>

#include "dawn/tests/DawnTest.h"
#include "dawn/utils/ComboRenderPipelineDescriptor.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

// Data structure to hold test case parameters, similar to the JS example.
struct LayoutCase {
    const char* type;
    const char* decl;
    const char* write_assign;
    size_t offset;
    bool f16 = false;
    bool f32 = false;
};

const std::vector<std::pair<std::string, LayoutCase>> kLayoutCases = {
    {"mat3x3h_align8",
     {"S_mat3x3h_align", "struct S_mat3x3h_align { x : u32, y : mat3x3h, }",
      "out.y[0][0] = f16(in)", 8, true}},
    {"mat3x4h_align8",
     {"S_mat3x4h_align", "struct S_mat3x4h_align { x : u32, y : mat3x4h, }",
      "out.y[0][0] = f16(in)", 8, true}},
    {"mat4x3h_align8",
     {"S_mat4x3h_align", "struct S_mat4x3h_align { x : u32, y : mat4x3h, }",
      "out.y[0][0] = f16(in)", 8, true}},
    {"mat4x4h_align8",
     {"S_mat4x4h_align", "struct S_mat4x4h_align { x : u32, y : mat4x4h, }",
      "out.y[0][0] = f16(in)", 8, true}},

    {"mat3x2f_size",
     {"S_mat3x2f_size", "struct S_mat3x2f_size { x : mat3x2f, y : u32, }", "out.y = in", 24}},
    {"mat4x2f_size",
     {"S_mat4x2f_size", "struct S_mat4x2f_size { x : mat4x2f, y : u32, }", "out.y = in", 32}},
    {"mat2x3f_size",
     {"S_mat2x3f_size", "struct S_mat2x3f_size { x : mat2x3f, y : u32, }", "out.y = in", 32}},
    {"mat3x3f_size",
     {"S_mat3x3f_size", "struct S_mat3x3f_size { x : mat3x3f, y : u32, }", "out.y = in", 48}},
    {"mat4x3f_size",
     {"S_mat4x3f_size", "struct S_mat4x3f_size { x : mat4x3f, y : u32, }", "out.y = in", 64}},
    {"mat2x4f_size",
     {"S_mat2x4f_size", "struct S_mat2x4f_size { x : mat2x4f, y : u32, }", "out.y = in", 32}},
    {"mat3x4f_size",
     {"S_mat3x4f_size", "struct S_mat3x4f_size { x : mat3x4f, y : u32, }", "out.y = in", 48}},
    {"mat4x4f_size",
     {"S_mat4x4f_size", "struct S_mat4x4f_size { x : mat4x4f, y : u32, }", "out.y = in", 64}},

    {"mat3x3h_size",
     {"S_mat3x3h_size", "struct S_mat3x3h_size { x : mat3x3h, y : f16, }", "out.y = f16(in)", 24,
      true}},
    {"mat4x3h_size",
     {"S_mat4x3h_size", "struct S_mat4x3h_size { x : mat4x3h, y : f16, }", "out.y = f16(in)", 32,
      true}},
    {"mat3x4h_size",
     {"S_mat3x4h_size", "struct S_mat3x4h_size { x : mat3x4h, y : f16, }", "out.y = f16(in)", 24,
      true}},
    {"mat4x4h_size",
     {"S_mat4x4h_size", "struct S_mat4x4h_size { x : mat4x4h, y : f16, }", "out.y = f16(in)", 32,
      true}},
};

// Base class for memory layout tests.
class MemoryLayoutTest : public DawnTest {
  protected:
    std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
        std::vector<wgpu::FeatureName> requiredFeatures = {};
        if (SupportsFeatures({wgpu::FeatureName::ShaderF16})) {
            requiredFeatures.push_back(wgpu::FeatureName::ShaderF16);
        }
        return requiredFeatures;
    }

    void RunComputeTest(const LayoutCase& testCase) {
        if (testCase.f16) {
            DAWN_TEST_UNSUPPORTED_IF(!device.HasFeature(wgpu::FeatureName::ShaderF16));
        }
        std::string code = "";
        if (testCase.f16) {
            code += "enable f16;\n";
        }
        if (testCase.decl) {
            code += testCase.decl;
            code += "\n";
        }

        // Define the workgroup and storage buffers
        code += "@group(0) @binding(0) var<storage> in : u32;\n";
        code += "@group(0) @binding(1) var<storage, read_write> post_out : " +
                std::string(testCase.type) + ";\n";
        code += "var<workgroup> out : " + std::string(testCase.type) + ";\n";

        // Main compute function
        code += R"(
@compute @workgroup_size(1,1,1)
fn main() {
)";

        std::string write_line = std::string(testCase.write_assign);
        code += "    " + write_line + ";\n";

        code += R"(
    workgroupBarrier();
    post_out = out;
}
)";

        wgpu::ShaderModule csModule = utils::CreateShaderModule(device, code);

        // Input buffer with value 42
        uint32_t inputValue = 42;
        wgpu::Buffer inBuffer = utils::CreateBufferFromData(device, &inputValue, sizeof(inputValue),
                                                            wgpu::BufferUsage::Storage);

        // Output buffer for verification, large enough to hold the struct.
        // We'll use 1024 bytes as a safe size to avoid memory issues.
        std::vector<uint32_t> initialData(128, 0);
        wgpu::Buffer outBuffer = utils::CreateBufferFromData(
            device, initialData.data(), initialData.size() * sizeof(uint32_t),
            wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc);

        wgpu::ComputePipelineDescriptor pipelineDesc;
        pipelineDesc.compute.module = csModule;
        pipelineDesc.compute.entryPoint = "main";
        wgpu::ComputePipeline pipeline = device.CreateComputePipeline(&pipelineDesc);

        wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0),
                                                         {{0, inBuffer}, {1, outBuffer}});

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.SetPipeline(pipeline);
        pass.SetBindGroup(0, bindGroup);
        pass.DispatchWorkgroups(1, 1, 1);
        pass.End();
        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        // Calculate the expected value
        uint32_t expectedValue = 0;
        uint16_t hf = 0;
        if (testCase.f16) {
            hf = 0x5140;
        } else if (testCase.f32) {
            float f = 42.0f;
            memcpy(&expectedValue, &f, sizeof(float));
        } else {
            expectedValue = 42;
        }

        std::vector<uint32_t> expectedData(initialData.size(), 0);
        size_t byteOffset = testCase.offset;
        size_t uint32Index = byteOffset / sizeof(uint32_t);

        if (testCase.f16) {
            expectedData[uint32Index] = static_cast<uint32_t>(hf);
        } else {
            expectedData[uint32Index] = expectedValue;
        }

        /*  std::cout << "===== expectedData  =====" << std::endl;
          for (size_t j = 0; j < expectedData.size(); ++j) {
              std::cout << "expectedData[" << j << "] = 0x" << std::hex << expectedData[j]
                        << " (10 jinzhi: " << std::dec << expectedData[j] << ")" << std::endl;
          }
          std::cout << "=====================================" << std::endl << std::endl;*/

        for (size_t i = 0; i < initialData.size(); ++i) {
            /*  std::cout << "current i = " << i << std::endl;
              std::cout << "expetced i = " << expectedData[i] << std::endl;*/
            //std::cout << "current i = " << i << std::endl;
            EXPECT_BUFFER_U32_EQ(expectedData[i], outBuffer, i * 4);
            //std::cout << "ok" << std::endl;
        }

        // if (testCase.f16) {
        //     std::vector<uint16_t> expectedData(initialData.size(), 0);
        //     expectedData[byteOffset/2] = hf;
        //     EXPECT_BUFFER_U16_EQ(expectedData[byteOffset /2], outBuffer, byteOffset+3);
        //     /*for (size_t i = 0; i < initialData.size(); ++i) {
        //         EXPECT_BUFFER_U8_EQ(expectedData[i], outBuffer, i);
        //     }*/
        // } else {
        //     expectedData[uint32Index] = expectedValue;
        //     for (size_t i = 0; i < initialData.size(); ++i) {
        //         EXPECT_BUFFER_U32_EQ(expectedData[i], outBuffer, i * 4);
        //     }
        // }
    }
};

// Use a macro to define tests for each case.
#define DAWN_DECLARE_MEMORY_LAYOUT_TEST(caseName)                                   \
    TEST_P(MemoryLayoutTest, write_layout_##caseName) {                             \
        auto it = std::find_if(kLayoutCases.begin(), kLayoutCases.end(),            \
                               [](const auto& p) { return p.first == #caseName; }); \
        ASSERT_TRUE(it != kLayoutCases.end());                                      \
        RunComputeTest(it->second);                                                 \
    }

// Define test instances for each case in the data table

DAWN_DECLARE_MEMORY_LAYOUT_TEST(mat3x3h_align8)
DAWN_DECLARE_MEMORY_LAYOUT_TEST(mat3x4h_align8)
DAWN_DECLARE_MEMORY_LAYOUT_TEST(mat4x3h_align8)
DAWN_DECLARE_MEMORY_LAYOUT_TEST(mat4x4h_align8)

DAWN_DECLARE_MEMORY_LAYOUT_TEST(mat3x2f_size)
DAWN_DECLARE_MEMORY_LAYOUT_TEST(mat4x2f_size)
DAWN_DECLARE_MEMORY_LAYOUT_TEST(mat2x3f_size)
DAWN_DECLARE_MEMORY_LAYOUT_TEST(mat3x3f_size)
DAWN_DECLARE_MEMORY_LAYOUT_TEST(mat4x3f_size)
DAWN_DECLARE_MEMORY_LAYOUT_TEST(mat2x4f_size)
DAWN_DECLARE_MEMORY_LAYOUT_TEST(mat3x4f_size)
DAWN_DECLARE_MEMORY_LAYOUT_TEST(mat4x4f_size)

DAWN_DECLARE_MEMORY_LAYOUT_TEST(mat3x3h_size)
DAWN_DECLARE_MEMORY_LAYOUT_TEST(mat4x3h_size)
DAWN_DECLARE_MEMORY_LAYOUT_TEST(mat3x4h_size)
DAWN_DECLARE_MEMORY_LAYOUT_TEST(mat4x4h_size)

DAWN_INSTANTIATE_TEST(MemoryLayoutTest,
                      D3D11Backend(),
                      D3D12Backend(),
                      MetalBackend(),
                      VulkanBackend(),
                      OpenGLBackend());

}  // anonymous namespace
}  // namespace dawn