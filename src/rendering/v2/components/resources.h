#ifndef HYPERION_V2_RESOURCES_H
#define HYPERION_V2_RESOURCES_H

#include "shader.h"
#include "framebuffer.h"
#include "compute.h"
#include "graphics.h"
#include "material.h"
#include "texture.h"
#include "mesh.h"
#include "util.h"

#include <mutex>

namespace hyperion::v2 {

class Engine;

struct Resources {
    using Callbacks = EngineCallbacks;

    RefCounter<Shader,  Callbacks>          shaders;
    ObjectHolder<Texture>                   textures;
    ObjectHolder<Framebuffer>               framebuffers;
    ObjectHolder<RenderPass>                render_passes;
    ObjectHolder<Material>                  materials;
    ObjectHolder<ComputePipeline>           compute_pipelines{.defer_create = true};
    
    RefCounter<Spatial, Callbacks>          spatials;
    RefCounter<Mesh,    Callbacks>          meshes;

    Resources(Engine *);
    Resources(const Resources &other) = delete;
    Resources &operator=(const Resources &other) = delete;
    ~Resources();

    void Create(Engine *engine);
    void Destroy(Engine *engine);

    template <class LambdaFunction>
    void Lock(LambdaFunction lambda)
    {
        const auto thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());

        DebugLog(LogType::Debug, "Locking resource mutex in thread %llu\n", thread_id);
        m_mtx.lock();
        DebugLog(LogType::Debug, "Locked resource mutex in thread %llu\n", thread_id);
        
        lambda(*this);

        DebugLog(LogType::Debug, "Unlocking resource mutex in thread %llu\n", thread_id);
        m_mtx.unlock();
        DebugLog(LogType::Debug, "Unlocked resource mutex in thread %llu\n", thread_id);
    }

private:
    std::mutex m_mtx;
};

} // namespace hyperion::v2

#endif