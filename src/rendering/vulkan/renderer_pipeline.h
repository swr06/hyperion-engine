//
// Created by emd22 on 2022-02-20.
//

#ifndef HYPERION_RENDERER_PIPELINE_H
#define HYPERION_RENDERER_PIPELINE_H

#include <vulkan/vulkan.h>

#include "renderer_device.h"
#include "renderer_swapchain.h"
#include "renderer_buffer.h"
#include "renderer_shader.h"
#include "renderer_render_pass.h"
#include "renderer_descriptor_pool.h"
#include "renderer_descriptor_set.h"
#include "renderer_descriptor.h"
#include "renderer_helpers.h"

#include "../../hash_code.h"

namespace hyperion {
class RendererFramebufferObject;
class RendererPipeline {
public:
    struct ConstructionInfo {
        RendererMeshInputAttributeSet vertex_attributes;
        RendererShader *shader;
        std::unique_ptr<RendererRenderPass> render_pass;
        std::vector<std::unique_ptr<RendererFramebufferObject>> fbos;

        enum class CullMode : int {
            NONE,
            BACK,
            FRONT
        } cull_mode;

        bool depth_test,
            depth_write;

        ConstructionInfo() : shader(nullptr) {}
        ConstructionInfo(ConstructionInfo &&other)
            : vertex_attributes(std::move(other.vertex_attributes)),
              shader(other.shader),
              render_pass(std::move(other.render_pass)),
              fbos(std::move(other.fbos)),
              cull_mode(other.cull_mode),
              depth_test(other.depth_test),
              depth_write(other.depth_write)
        {
        }

        ConstructionInfo &operator=(ConstructionInfo &&other)
        {
            vertex_attributes = std::move(other.vertex_attributes);
            shader = other.shader;
            render_pass = std::move(other.render_pass);
            fbos = std::move(other.fbos);
            cull_mode = other.cull_mode;
            depth_test = other.depth_test;
            depth_write = other.depth_write;
        }

        ConstructionInfo(const ConstructionInfo &other) = delete;
        ConstructionInfo &operator=(const ConstructionInfo &other) = delete;
        ~ConstructionInfo() = default;

        inline HashCode GetHashCode() const
        {
            HashCode hc;

            hc.Add((shader != nullptr) ? shader->GetHashCode().Value() : 0);
            hc.Add(intptr_t(render_pass.get())); // TODO
            for (auto &fbo : fbos) {
                hc.Add(intptr_t(fbo.get()));
            }
            hc.Add(vertex_attributes.GetHashCode());
            hc.Add(int(cull_mode));
            hc.Add(depth_test);
            hc.Add(depth_write);

            return hc;
        }
    };

    class Builder {
    public:
        Builder()
        {
            CullMode(RendererPipeline::ConstructionInfo::CullMode::BACK);
            DepthTest(true);
            DepthWrite(true);
        }

        Builder(Builder &&other) = default;
        Builder(const Builder &other) = delete;
        Builder &operator=(const Builder &other) = delete;
        ~Builder() = default;

        Builder &VertexAttributes(const RendererMeshInputAttributeSet &vertex_attributes)
        {
            m_construction_info.vertex_attributes = vertex_attributes;

            return *this;
        }

        Builder &CullMode(RendererPipeline::ConstructionInfo::CullMode cull_mode)
        {
            m_construction_info.cull_mode = cull_mode;

            return *this;
        }

        Builder &DepthWrite(bool depth_write)
        {
            m_construction_info.depth_write = depth_write;

            return *this;
        }

        Builder &DepthTest(bool depth_test)
        {
            m_construction_info.depth_test = depth_test;

            return *this;
        }

        Builder &Shader(RendererShader *shader)
        {
            m_construction_info.shader = shader;

            return *this;
        }

        Builder &RenderPass(std::function<void(RendererRenderPass &)> fn)
        {
            m_construction_info.render_pass = std::make_unique<RendererRenderPass>();

            fn(*m_construction_info.render_pass);

            return *this;
        }

        Builder &Framebuffer(size_t width, size_t height, std::function<void(RendererFramebufferObject &)> fn)
        {
            auto fbo = std::make_unique<RendererFramebufferObject>(width, height);

            fn(*fbo);

            m_construction_info.fbos.push_back(std::move(fbo));

            return *this;
        }

        std::unique_ptr<RendererPipeline> Build(RendererDevice *device)
        {
            AssertThrow(m_construction_info.render_pass != nullptr);
            AssertThrow(m_construction_info.fbos.size() != 0);

            auto render_pass_result = m_construction_info.render_pass->Create(device);
            AssertThrowMsg(render_pass_result, "%s", render_pass_result.message);

            for (auto &fbo : m_construction_info.fbos) {
                auto fbo_result = fbo->Create(device, m_construction_info.render_pass.get());
                AssertThrowMsg(fbo_result, "%s", fbo_result.message);
            }

            return std::make_unique<RendererPipeline>(device, std::move(m_construction_info));
        }

        inline HashCode GetHashCode() const
        {
            return m_construction_info.GetHashCode();
        }

    private:
        ConstructionInfo m_construction_info;
    };

    RendererPipeline(RendererDevice *_device, ConstructionInfo &&construction_info);
    void Destroy();

    void SetPrimitive(VkPrimitiveTopology _primitive);
    void SetDynamicStates(const std::vector<VkDynamicState> &_states);

    RendererResult CreateCommandPool();
    RendererResult CreateCommandBuffers(uint16_t count);

    void UpdateDynamicStates(VkCommandBuffer cmd);
    void SetViewport(float x, float y, float width, float height, float min_depth = 0.0f, float max_depth = 1.0f);
    void SetScissor(int x, int y, uint32_t width, uint32_t height);
    void SetVertexInputMode(std::vector<VkVertexInputBindingDescription> &binding_descs, std::vector<VkVertexInputAttributeDescription> &vertex_attribs);

    inline void Build(RendererDescriptorPool *descriptor_pool)
        { Rebuild(descriptor_pool); }

    inline void Build(ConstructionInfo &&construction_info, RendererDescriptorPool *descriptor_pool)
    {
        m_construction_info = std::move(construction_info);
        
        Build(descriptor_pool);
    }

    void StartRenderPass(VkCommandBuffer cmd, size_t index);
    void EndRenderPass(VkCommandBuffer cmd, size_t index);

    VkPrimitiveTopology GetPrimitive();
    std::vector<VkDynamicState> GetDynamicStates();

    inline const ConstructionInfo &GetConstructionInfo() const { return m_construction_info; }

    helpers::SingleTimeCommands GetSingleTimeCommands();

    /* Per frame data */
    VkCommandPool command_pool;
    std::vector<VkCommandBuffer> command_buffers;

    VkPipeline pipeline;
    VkPipelineLayout layout;

    struct PushConstants {
        unsigned char data[128];
    } push_constants;

private:
    void Rebuild(RendererDescriptorPool *descriptor_pool);

    VkBuffer *intern_vertex_buffers = nullptr;
    uint64_t  intern_vertex_buffers_size = 0;

    std::vector<VkDynamicState> dynamic_states;

    VkViewport viewport;
    VkRect2D scissor;
    VkPrimitiveTopology primitive;

    std::vector<VkVertexInputBindingDescription>   vertex_binding_descriptions = { };
    std::vector<VkVertexInputAttributeDescription> vertex_attributes = { };

    RendererDevice *device;

    ConstructionInfo m_construction_info;

    std::vector<VkVertexInputAttributeDescription> BuildVertexAttributes(const RendererMeshInputAttributeSet &attribute_set);
    void SetVertexBuffers(std::vector<RendererVertexBuffer> &vertex_buffers);
};

}; // namespace hyperion

#endif //HYPERION_RENDERER_PIPELINE_H
