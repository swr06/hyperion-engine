#include "deferred.h"
#include "../engine.h"

#include <asset/byte_reader.h>
#include <asset/asset_manager.h>

namespace hyperion::v2 {

DeferredRenderingEffect::DeferredRenderingEffect()
    : PostEffect()
{
    
}

DeferredRenderingEffect::~DeferredRenderingEffect() = default;

void DeferredRenderingEffect::CreateShader(Engine *engine)
{
    m_shader = engine->resources.shaders.Add(std::make_unique<Shader>(
        std::vector<SubShader>{
            SubShader{ShaderModule::Type::VERTEX, {
                FileByteReader(AssetManager::GetInstance()->GetRootDir() + "/vkshaders/deferred_vert.spv").Read(),
                {.name = "deferred vert"}
            }},
            SubShader{ShaderModule::Type::FRAGMENT, {
                FileByteReader(AssetManager::GetInstance()->GetRootDir() + "/vkshaders/deferred_frag.spv").Read(),
                {.name = "deferred frag"}
            }}
        }
    ));

    m_shader->Init(engine);
}

void DeferredRenderingEffect::CreateRenderPass(Engine *engine)
{
    m_render_pass = engine->GetRenderList()[GraphicsPipeline::BUCKET_TRANSLUCENT].render_pass.Acquire();
}

void DeferredRenderingEffect::Create(Engine *engine)
{
    m_framebuffer = engine->GetRenderList()[GraphicsPipeline::BUCKET_TRANSLUCENT].framebuffers[0].Acquire();


    CreatePerFrameData(engine);

    engine->callbacks.Once(EngineCallback::CREATE_GRAPHICS_PIPELINES, [this, engine](...) {
        CreatePipeline(engine);
    });

    engine->callbacks.Once(EngineCallback::DESTROY_GRAPHICS_PIPELINES, [this, engine](...) {
        DestroyPipeline(engine);
    });
}

void DeferredRenderingEffect::Destroy(Engine *engine)
{
    PostEffect::Destroy(engine);
}

void DeferredRenderingEffect::Render(Engine *engine, CommandBuffer *primary, uint32_t frame_index)
{
}

DeferredRenderer::DeferredRenderer() = default;
DeferredRenderer::~DeferredRenderer() = default;

void DeferredRenderer::Create(Engine *engine)
{
    using renderer::ImageSamplerDescriptor;
    
    m_voxel_map = new renderer::StorageImage(
        {64, 64, 64},
        Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_RGBA16F,
        Image::Type::TEXTURE_TYPE_3D,
        nullptr
    );
    
    auto *descriptor_set_globals = engine->GetInstance()->GetDescriptorPool()
        .GetDescriptorSet(DescriptorSet::DESCRIPTOR_SET_INDEX_GLOBAL);

    descriptor_set_globals
        ->AddDescriptor<renderer::ImageStorageDescriptor>(17)
        ->AddSubDescriptor({ .image_view = &voxel_map_view });

    HYPERION_ASSERT_RESULT(m_voxel_map->Create(
        engine->GetInstance()->GetDevice(),
        engine->GetInstance(),
        renderer::GPUMemory::ResourceState::UNORDERED_ACCESS
    ));

    HYPERION_ASSERT_RESULT(voxel_map_view.Create(engine->GetInstance()->GetDevice(), m_voxel_map));

    m_post_processing.Create(engine);

    m_effect.CreateShader(engine);
    m_effect.CreateRenderPass(engine);
    m_effect.Create(engine);

    auto &opaque_fbo = engine->GetRenderList()[GraphicsPipeline::BUCKET_OPAQUE].framebuffers[0];

    /* Add our gbuffer textures */
    auto *descriptor_set_pass = engine->GetInstance()->GetDescriptorPool()
        .GetDescriptorSet(DescriptorSet::DESCRIPTOR_SET_INDEX_GLOBAL);

    /* Albedo texture */
    descriptor_set_pass
        ->AddDescriptor<ImageSamplerDescriptor>(0)
        ->AddSubDescriptor({
            .image_view = opaque_fbo->Get().GetRenderPassAttachmentRefs()[0]->GetImageView(),
            .sampler    = opaque_fbo->Get().GetRenderPassAttachmentRefs()[0]->GetSampler()
        });

    /* Normals texture*/
    descriptor_set_pass
        ->AddDescriptor<ImageSamplerDescriptor>(1)
        ->AddSubDescriptor({
            .image_view = opaque_fbo->Get().GetRenderPassAttachmentRefs()[1]->GetImageView(),
            .sampler    = opaque_fbo->Get().GetRenderPassAttachmentRefs()[1]->GetSampler()
        });

    /* Position texture */
    descriptor_set_pass
        ->AddDescriptor<ImageSamplerDescriptor>(2)
        ->AddSubDescriptor({
            .image_view = opaque_fbo->Get().GetRenderPassAttachmentRefs()[2]->GetImageView(),
            .sampler    = opaque_fbo->Get().GetRenderPassAttachmentRefs()[2]->GetSampler()
        });

    /* Depth texture */
    descriptor_set_pass
        ->AddDescriptor<ImageSamplerDescriptor>(3)
        ->AddSubDescriptor({
            .image_view = opaque_fbo->Get().GetRenderPassAttachmentRefs()[3]->GetImageView(),
            .sampler    = opaque_fbo->Get().GetRenderPassAttachmentRefs()[3]->GetSampler()
        });

    uint32_t binding_index = 4; /* TMP */
    m_effect.CreateDescriptors(engine, binding_index);
}

void DeferredRenderer::Destroy(Engine *engine)
{
    m_post_processing.Destroy(engine);
    m_effect.Destroy(engine);
}

void DeferredRenderer::Render(Engine *engine, CommandBuffer *primary, uint32_t frame_index)
{
    m_effect.Record(engine, frame_index);

    auto &render_list = engine->GetRenderList();
    auto &bucket = render_list.Get(GraphicsPipeline::Bucket::BUCKET_OPAQUE);

    bucket.Begin(engine, primary, 0);
    RenderOpaqueObjects(engine, primary, frame_index);
    bucket.End(engine, primary, 0);

    if (m_voxel_render_counter++ % 10 == 0) {
        engine->RenderShadows(primary, frame_index);

        m_voxel_map->GetGPUImage()->InsertBarrier(
            primary,
            { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
            renderer::GPUMemory::ResourceState::UNORDERED_ACCESS
        );
    }

    
    m_post_processing.Render(engine, primary, frame_index);
    
    m_effect.GetFramebuffer()->BeginCapture(primary);

    /* Render deferred shading onto full screen quad */
    auto *secondary_command_buffer = m_effect.GetFrameData()->At(frame_index).Get<CommandBuffer>();
    HYPERION_ASSERT_RESULT(secondary_command_buffer->SubmitSecondary(primary));

    RenderTranslucentObjects(engine, primary, frame_index);
    
    m_effect.GetFramebuffer()->EndCapture(primary);
}

void DeferredRenderer::RenderOpaqueObjects(Engine *engine, CommandBuffer *primary, uint32_t frame_index)
{
    auto &render_list = engine->GetRenderList();

    for (const auto &pipeline : render_list.Get(GraphicsPipeline::Bucket::BUCKET_SKYBOX).pipelines.objects) {
        pipeline->Render(engine, primary, frame_index);
    }

    for (const auto &pipeline : render_list.Get(GraphicsPipeline::Bucket::BUCKET_OPAQUE).pipelines.objects) {
        pipeline->Render(engine, primary, frame_index);
    }
}

void DeferredRenderer::RenderTranslucentObjects(Engine *engine, CommandBuffer *primary, uint32_t frame_index)
{
    const auto &render_list = engine->GetRenderList();

    for (const auto &pipeline : render_list[GraphicsPipeline::Bucket::BUCKET_TRANSLUCENT].pipelines.objects) {
        pipeline->Render(engine, primary, frame_index);
    }
}

} // namespace hyperion::v2