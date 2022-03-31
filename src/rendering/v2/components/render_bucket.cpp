#include "render_bucket.h"
#include "../engine.h"

namespace hyperion::v2 {

RenderBucketContainer::RenderBucketContainer()
{
    for (auto &bucket : m_buckets) {
        bucket = Bucket{
            .render_pass = {},
            .pipelines = {.defer_create = true}
        };
    }
}

void RenderBucketContainer::Bucket::CreateRenderPass(Engine *engine)
{
    auto render_pass = std::make_unique<RenderPass>(renderer::RenderPass::RENDER_PASS_STAGE_SHADER, renderer::RenderPass::RENDER_PASS_INLINE);

    /* For our color attachment */
    render_pass->Get().AddAttachment({
        .format = engine->GetDefaultFormat(Engine::TEXTURE_FORMAT_DEFAULT_COLOR)
    });

    /* For our normals attachment */
    render_pass->Get().AddAttachment({
        .format = engine->GetDefaultFormat(Engine::TEXTURE_FORMAT_DEFAULT_GBUFFER)
    });

    /* For our positions attachment */
    render_pass->Get().AddAttachment({
        .format = engine->GetDefaultFormat(Engine::TEXTURE_FORMAT_DEFAULT_GBUFFER)
    });

    render_pass->Get().AddAttachment({
        .format = engine->GetDefaultFormat(Engine::TEXTURE_FORMAT_DEFAULT_DEPTH)
    });

    this->render_pass = engine->AddRenderPass(std::move(render_pass));
}

void RenderBucketContainer::Bucket::CreateFramebuffer(Engine *engine)
{
    this->framebuffer = engine->AddFramebuffer(
        engine->GetInstance()->swapchain->extent.width,
        engine->GetInstance()->swapchain->extent.height,
        render_pass
    );
}

void RenderBucketContainer::CreatePipelines(Engine *engine)
{
    for (auto &bucket : m_buckets) {
        RenderPass *render_pass = engine->GetRenderPass(bucket.render_pass);
        AssertThrow(render_pass != nullptr);

        bucket.pipelines.CreateAll(engine, render_pass);
    }
}

void RenderBucketContainer::Create(Engine *engine)
{
    for (auto &bucket : m_buckets) {
        bucket.CreateRenderPass(engine);
        bucket.CreateFramebuffer(engine);
    }
}

void RenderBucketContainer::Destroy(Engine *engine)
{
    for (auto &bucket : m_buckets) {
        engine->RemoveFramebuffer(bucket.framebuffer);
        engine->RemoveRenderPass(bucket.render_pass);

        bucket.framebuffer = {};
        bucket.render_pass = {};

        bucket.pipelines.RemoveAll(engine);
    }
}

GraphicsPipeline::ID RenderBucketContainer::Bucket::AddGraphicsPipeline(Engine *engine, std::unique_ptr<GraphicsPipeline> &&pipeline)
{
    auto *render_pass = engine->GetRenderPass(this->render_pass);

    AssertThrow(render_pass != nullptr);

    return pipelines.Add(engine, std::move(pipeline), render_pass);
}

void RenderBucketContainer::Bucket::RemoveGraphicsPipeline(Engine *engine, GraphicsPipeline::ID id)
{
    return pipelines.Remove(engine, id);
}

void RenderBucketContainer::Bucket::Render(Engine *engine, CommandBuffer *primary, uint32_t frame_index) const
{
    auto *render_pass = engine->GetRenderPass(this->render_pass);
    auto *framebuffer = engine->GetFramebuffer(this->framebuffer);

    render_pass->Get().Begin(
        primary,
        framebuffer->Get().GetFramebuffer(),
        VkExtent2D{ uint32_t(framebuffer->Get().GetWidth()), uint32_t(framebuffer->Get().GetHeight()) },
        VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
    );

    for (const auto &pipeline : pipelines.objects) {
        pipeline->Render(engine, primary, render_pass, frame_index);
    }

    render_pass->Get().End(primary);
}

} // namespace hyperion::v2