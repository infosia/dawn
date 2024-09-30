// Copyright 2024 The Dawn & Tint Authors
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

#include "dawn/native/gles2/CommandBufferES2.h"

#include "dawn/common/MatchVariant.h"
#include "dawn/native/BindGroupTracker.h"
#include "dawn/native/Commands.h"
#include "dawn/native/gl/OpenGLFunctions.h"
#include "dawn/native/gl/opengl_platform.h"
#include "dawn/native/gles2/BufferES2.h"
#include "dawn/native/gles2/DeviceES2.h"
#include "dawn/native/gles2/Forward.h"
#include "dawn/native/gles2/GLSLData.h"
#include "dawn/native/gles2/RenderPipelineES2.h"
#include "dawn/native/gles2/TextureES2.h"

namespace dawn::native::gles2 {

namespace {

class VertexBufferTracker {
  public:
    void Add(const gl::OpenGLFunctions& gl, GLuint program, int slot, GLSLType type, const std::string& name) {
        if (slot >= mBindings.size()) {
            mBindings.resize(slot + 1);
        }

        GLenum glType;
        GLint elementSize;
        switch (type) {
            case GLSLType::kFloat:
                glType = GL_FLOAT;
                elementSize = 1;
                break;
            case GLSLType::kFloat2:
                glType = GL_FLOAT;
                elementSize = 2;
                break;
            case GLSLType::kFloat4:
                glType = GL_FLOAT;
                elementSize = 4;
                break;
            case GLSLType::kInt:
                glType = GL_INT;
                elementSize = 1;
                break;
            case GLSLType::kMatrix4:
                glType = GL_FLOAT;
                elementSize = 16;
                break;
            default:
                DAWN_UNREACHABLE();
        }

        GLint location = gl.GetAttribLocation(program, name.c_str());
        gl.CheckErrors("GetAttribLocation");
        mBindings[slot] = { glType, elementSize, location };
    }

    void OnSetVertexBuffer(const gl::OpenGLFunctions& gl, VertexBufferSlot slot, BufferBase* buffer, uint64_t offset) {
        if (uint8_t(slot) >= mBindings.size()) {
            printf("!! vertex buffer slot outside range of locations gathered from shader\n");
            return;
        }

        gl.BindBuffer(GL_ARRAY_BUFFER, ToBackend(buffer)->GetHandle());
        gl.CheckErrors("BindBuffer");

        const Binding& binding = mBindings[uint8_t(slot)];
        gl.VertexAttribPointer(binding.mLocation, binding.mElementSize, binding.mType, GL_FALSE, 0, reinterpret_cast<const void*>(offset));
        gl.CheckErrors("VertexAttribPointer");
        gl.EnableVertexAttribArray(binding.mLocation);
        gl.CheckErrors("EnableVertexAttribArray");
    }

  private:
    struct Binding {
        GLenum mType;
        GLint mElementSize;
        GLint mLocation;
    };

    // TODO use PerVertexBuffer?
    std::vector<Binding> mBindings;
};

class BindGroupTracker : public BindGroupTrackerBase<false, uint64_t> {
  public:
    void Add(const gl::OpenGLFunctions& gl, GLuint program, int binding, GLSLType type, const std::string& name) {
        if (binding >= mBindings.size()) {
            mBindings.resize(binding + 1);
        }
        GLint location = gl.GetUniformLocation(program, name.c_str());
        gl.CheckErrors("GetUniformLocation");
        mBindings[binding].push_back({ type, location });
    }

    void Apply(const gl::OpenGLFunctions& gl) {
        BeforeApply();
        for (BindGroupIndex groupIndex : IterateBitSet(mDirtyBindGroupsObjectChangedOrIsDynamic)) {
            ApplyBindGroup(gl, mBindGroups[groupIndex], mDynamicOffsets[groupIndex]);
        }
        AfterApply();
    }

  private:
    struct Binding {
        GLSLType mType;
        GLint mLocation;
    };

    void ApplyBindGroup(const gl::OpenGLFunctions& gl, BindGroupBase* group, const ityp::vector<BindingIndex, uint64_t>& dynamicOffsets) {
        for (BindingIndex bindingIndex{0}; bindingIndex < group->GetLayout()->GetBindingCount(); ++bindingIndex) {
            const BindingInfo& bindingInfo = group->GetLayout()->GetBindingInfo(bindingIndex);
            MatchVariant(
                bindingInfo.bindingLayout,
                [this, &gl, group, &dynamicOffsets, bindingIndex](const BufferBindingInfo& layout) {
                    BufferBinding binding = group->GetBindingAsBufferBinding(bindingIndex);
                    Buffer* buffer = ToBackend(binding.buffer);

                    if (uint32_t(bindingIndex) >= mBindings.size()) {
                        printf("!! binding index outside range of indices gathered from shader: %u\n", uint32_t(bindingIndex));
                        return;
                    }

                    GLuint offset = binding.offset;
                    if (layout.hasDynamicOffset) {
                        offset += dynamicOffsets[bindingIndex];
                    }

                    const void* data = static_cast<const uint8_t*>(buffer->GetOffsetPointer()) + offset;

                    for (const Binding& glBinding : mBindings[uint32_t(bindingIndex)]) {
                        const GLint loc = glBinding.mLocation;
                        switch (glBinding.mType) {
                            case GLSLType::kFloat:
                                gl.Uniform1fv(loc, 1, static_cast<const float*>(data));
                                gl.CheckErrors("Uniform1fv");
                                data = static_cast<const float*>(data) + 1;
                                break;
                            case GLSLType::kFloat2:
                                gl.Uniform2fv(loc, 1, static_cast<const float*>(data));
                                gl.CheckErrors("Uniform2fv");
                                data = static_cast<const float*>(data) + 2;
                                break;
                            case GLSLType::kFloat4:
                                gl.Uniform4fv(loc, 1, static_cast<const float*>(data));
                                gl.CheckErrors("Uniform4fv");
                                data = static_cast<const float*>(data) + 4;
                                break;
                            case GLSLType::kInt:
                                gl.Uniform1iv(loc, 1, static_cast<const int32_t*>(data));
                                gl.CheckErrors("Uniform1iv");
                                data = static_cast<const int32_t*>(data) + 1;
                                break;
                            case GLSLType::kMatrix4:
                                gl.UniformMatrix4fv(loc, 1, GL_FALSE, static_cast<const float*>(data));
                                gl.CheckErrors("UniformMatrix4fv");
                                data = static_cast<const float*>(data) + 16;
                                break;
                            default:
                                DAWN_UNREACHABLE();
                        }
                    }
                },

                [](const StaticSamplerBindingInfo&) {
                    printf("!! unexpected StaticSamplerBindingInfo\n");
                },
                [](const SamplerBindingInfo&) {
                    printf("!! unexpected SamplerBindingInfo\n");
                },
                [](const TextureBindingInfo&) {
                    printf("!! unexpected TextureBindingInfo\n");
                },
                [](const StorageTextureBindingInfo&) {
                    printf("!! unexpected StorageTextureBindingInfo\n");
                },
                [](const InputAttachmentBindingInfo&) { DAWN_UNREACHABLE(); });
        }
    }

    std::vector<std::vector<Binding>> mBindings;
};

ResultOrError<GLuint> CreateShader(const gl::OpenGLFunctions& gl, GLenum type, const char* src) {
    GLuint shader = gl.CreateShader(type);
    gl.CheckErrors("CreateShader");
    if (!shader) {
        return DAWN_FORMAT_INTERNAL_ERROR("Error creating shader of type %x\n", type);
    }

    gl.ShaderSource(shader, 1, &src, nullptr);
    gl.CheckErrors("ShaderSource");
    gl.CompileShader(shader);
    gl.CheckErrors("CompileShader");

    GLint compileStatus = GL_FALSE;
    gl.GetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
    gl.CheckErrors("GetShaderiv");

    if (compileStatus == GL_FALSE) {
        GLint infoLogLength = 0;
        gl.GetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
        gl.CheckErrors("GetShaderiv");

        char buffer[infoLogLength];
        if (infoLogLength > 1) {
            gl.GetShaderInfoLog(shader, infoLogLength, nullptr, buffer);
            gl.CheckErrors("GetShaderInfoLog");
        }

        gl.DeleteShader(shader);
        gl.CheckErrors("DeleteShader");

        if (infoLogLength > 1) {
            return DAWN_FORMAT_INTERNAL_ERROR("Error compiling shader:\n%s", std::string(buffer));
        } else {
            return DAWN_INTERNAL_ERROR("Error compiling shader\n");
        }
    }

    return shader;
}

ResultOrError<GLuint> CreateProgram(const gl::OpenGLFunctions& gl, GLuint vertexShader, GLuint fragmentShader) {
    GLuint program = gl.CreateProgram();
    gl.CheckErrors("CreateProgram");
    if (!program) {
        return DAWN_INTERNAL_ERROR("Error creating program");
    }

    gl.AttachShader(program, vertexShader);
    gl.CheckErrors("AttachShader");
    gl.AttachShader(program, fragmentShader);
    gl.CheckErrors("AttachShader");
    gl.LinkProgram(program);
    gl.CheckErrors("LinkProgram");

    GLint linkStatus = GL_FALSE;
    gl.GetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    gl.CheckErrors("GetProgramiv");

    if (linkStatus == GL_FALSE) {
        GLint infoLogLength = 0;
        gl.GetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
        gl.CheckErrors("GetProgramiv");

        char buffer[infoLogLength];
        if (infoLogLength > 1) {
            gl.GetProgramInfoLog(program, infoLogLength, nullptr, buffer);
            gl.CheckErrors("GetProgramInfoLog");
        }

        gl.DeleteProgram(program);
        gl.CheckErrors("DeleteProgram");

        if (infoLogLength > 1) {
            return DAWN_FORMAT_INTERNAL_ERROR("Error linking program:\n%s", std::string(buffer));
        } else {
            return DAWN_INTERNAL_ERROR("Error linking program\n");
        }
    }

    return program;
}

MaybeError ExecuteRenderPass(const gl::OpenGLFunctions& gl, BeginRenderPassCmd* renderPass, CommandIterator& commands) {
    if (renderPass->colorAttachments.empty()) {
        return DAWN_INTERNAL_ERROR("No color attachments provided in BeginRenderPassCmd");
    }

    constexpr ColorAttachmentIndex ca0(uint8_t(0));
    TextureView* textureView = ToBackend(renderPass->colorAttachments[ca0].view.Get());
    if (!textureView) {
        return DAWN_INTERNAL_ERROR("Null color attachment provided in BeginRenderPassCmd");
    }
    if (textureView->IsError()) {
        return DAWN_INTERNAL_ERROR("Color attachments provided in BeginRenderPassCmd is an error");
    }

    GLuint fbo = 0;
    gl.GenFramebuffers(1, &fbo);
    gl.CheckErrors("GenFramebuffers");
    gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl.CheckErrors("BindFramebuffer");
    gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureView->GetHandle(), 0);
    gl.CheckErrors("FramebufferTexture2D");

    GLenum framebufferStatus = gl.CheckFramebufferStatus(GL_FRAMEBUFFER);
    gl.CheckErrors("CheckFramebufferStatus");
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
        return DAWN_FORMAT_INTERNAL_ERROR("Framebuffer status: %x", framebufferStatus);
    }

    gl.Viewport(0, 0, renderPass->width, renderPass->height);
    gl.CheckErrors("Viewport");
    gl.Scissor(0, 0, renderPass->width, renderPass->height);
    gl.CheckErrors("Scissor");

    GLuint vertexShader = 0;
    GLuint fragmentShader = 0;
    GLuint program = 0;

    VertexBufferTracker vertexBufferTracker;
    BindGroupTracker bindGroupTracker;

    Command type;
    while (commands.NextCommandId(&type)) {
        switch (type) {
            case Command::Draw: {
                DrawCmd* cmd = commands.NextCommand<DrawCmd>();
                if (cmd->instanceCount > 1) {
                    return DAWN_UNIMPLEMENTED_ERROR("Instanced rendering not supported");
                }

                bindGroupTracker.Apply(gl);

                gl.DrawArrays(GL_TRIANGLES, cmd->firstVertex, cmd->vertexCount);
                gl.CheckErrors("DrawArrays");
                break;
            }

            case Command::SetRenderPipeline: {
                SetRenderPipelineCmd* cmd = commands.NextCommand<SetRenderPipelineCmd>();
                RenderPipeline* pipeline = ToBackend(cmd->pipeline).Get();

                bindGroupTracker.OnSetPipeline(pipeline);

                DAWN_TRY_ASSIGN(vertexShader, CreateShader(gl, GL_VERTEX_SHADER, pipeline->VertexSource()));
                DAWN_TRY_ASSIGN(fragmentShader, CreateShader(gl, GL_FRAGMENT_SHADER, pipeline->FragmentSource()));
                DAWN_TRY_ASSIGN(program, CreateProgram(gl, vertexShader, fragmentShader));

                gl.UseProgram(program);
                gl.CheckErrors("UseProgram");

                for (const InputDeclaration& decl : pipeline->VertexInputs()) {
                    vertexBufferTracker.Add(gl, program, decl.mLocation, decl.mType, decl.mName);
                }
                for (const UniformDeclaration& decl : pipeline->Uniforms()) {
                    bindGroupTracker.Add(gl, program, decl.mBinding, decl.mType, decl.mName);
                }

                break;
            }

            case Command::SetBindGroup: {
                SetBindGroupCmd* cmd = commands.NextCommand<SetBindGroupCmd>();
                uint32_t* dynamicOffsets = nullptr;
                if (cmd->dynamicOffsetCount > 0) {
                    dynamicOffsets = commands.NextData<uint32_t>(cmd->dynamicOffsetCount);
                }
                bindGroupTracker.OnSetBindGroup(cmd->index, cmd->group.Get(), cmd->dynamicOffsetCount, dynamicOffsets);
                break;
            }

            case Command::SetVertexBuffer: {
                SetVertexBufferCmd* cmd = commands.NextCommand<SetVertexBufferCmd>();
                vertexBufferTracker.OnSetVertexBuffer(gl, cmd->slot, cmd->buffer.Get(), cmd->offset);
                break;
            }

            case Command::EndRenderPass: {
                commands.NextCommand<EndRenderPassCmd>();
                gl.DeleteShader(vertexShader);
                gl.CheckErrors("DeleteShader");
                gl.DeleteShader(fragmentShader);
                gl.CheckErrors("DeleteShader");
                gl.DeleteProgram(program);
                gl.CheckErrors("DeleteProgram");
                gl.DeleteFramebuffers(1, &fbo);
                gl.CheckErrors("DeleteFramebuffers");
                return {};
            }

            default:
                SkipCommand(&commands, type);
                break;
        }
    }

    // EndRenderPass should have been called.
    DAWN_UNREACHABLE();
}

MaybeError CopyTextureToBuffer(const gl::OpenGLFunctions& gl, CopyTextureToBufferCmd* copy) {
    const TextureCopy& src = copy->source;
    const BufferCopy& dst = copy->destination;
    const Extent3D& copySize = copy->copySize;

    if (src.mipLevel > 0) {
        return DAWN_VALIDATION_ERROR("No copying from non-base mip level in GLES2");
    }

    GLuint fbo = 0;
    gl.GenFramebuffers(1, &fbo);
    gl.CheckErrors("GenFramebuffers");
    gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl.CheckErrors("BindFramebuffer");

    Texture* texture = ToBackend(src.texture.Get());
    gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->GetHandle(), 0);
    gl.CheckErrors("FramebufferTexture2D");

    GLenum framebufferStatus = gl.CheckFramebufferStatus(GL_FRAMEBUFFER);
    gl.CheckErrors("CheckFramebufferStatus");
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
        return DAWN_FORMAT_INTERNAL_ERROR("Framebuffer status: %x", framebufferStatus);
    }

    // TODO use texture's format and type to determine data size
    const size_t dataSize = copySize.width * copySize.height * 4;
    Buffer* buffer = ToBackend(dst.buffer.Get());
    DAWN_TRY(buffer->MapSync(dst.offset, dataSize));
    void* data = static_cast<uint8_t*>(buffer->GetMappedPointer()) + dst.offset;

    // TODO use texture's format and type
    gl.ReadPixels(src.origin.x, src.origin.y, copySize.width, copySize.height, GL_RGBA, GL_UNSIGNED_BYTE, data);
    gl.CheckErrors("ReadPixels");

    buffer->UnmapSync();

    gl.DeleteFramebuffers(1, &fbo);
    gl.CheckErrors("DeleteFramebuffers");
    return {};
}

}  // anonymous namespace

CommandBuffer::CommandBuffer(CommandEncoder* encoder, const CommandBufferDescriptor* descriptor)
    : CommandBufferBase(encoder, descriptor) {}

MaybeError CommandBuffer::Execute() {
    const gl::OpenGLFunctions& gl = ToBackend(GetDevice())->GetGL();

    Command type;
    while (mCommands.NextCommandId(&type)) {
        switch (type) {
            case Command::BeginRenderPass: {
                BeginRenderPassCmd* renderPass = mCommands.NextCommand<BeginRenderPassCmd>();
                DAWN_TRY(ExecuteRenderPass(gl, renderPass, mCommands));
                break;
            }

            case Command::CopyTextureToBuffer: {
                CopyTextureToBufferCmd* copy = mCommands.NextCommand<CopyTextureToBufferCmd>();
                DAWN_TRY(CopyTextureToBuffer(gl, copy));
                break;
            }

            case Command::WriteBuffer: {
                WriteBufferCmd* write = mCommands.NextCommand<WriteBufferCmd>();
                uint64_t offset = write->offset;
                uint64_t size = write->size;
                if (size == 0) {
                    continue;
                }

                Buffer* buffer = ToBackend(write->buffer.Get());
                // TODO don't really need to update cpu data? es3 backend just calls BufferSubData directly. any concern about cpu/gpu data mismatch?
                DAWN_TRY(buffer->MapSync(offset, size));

                uint8_t* srcData = mCommands.NextData<uint8_t>(size);
                uint8_t* dstData = static_cast<uint8_t*>(buffer->GetOffsetPointer());

                memcpy(dstData, srcData, size);
                buffer->UnmapSync();
                break;
            }

            default:
                // TODO
                SkipCommand(&mCommands, type);
                break;
        }
    }
    return {};
}

}  // namespace dawn::native::gles2
