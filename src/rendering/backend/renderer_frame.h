#ifndef HYPERION_RENDERER_FRAME_H
#define HYPERION_RENDERER_FRAME_H

#include "renderer_result.h"
#include "renderer_device.h"
#include "renderer_command_buffer.h"
#include "renderer_fence.h"
#include "renderer_semaphore.h"
#include "renderer_queue.h"

#include <util/non_owning_ptr.h>

#include <vulkan/vulkan.h>

namespace hyperion {
namespace renderer {

class Swapchain;

using ::hyperion::non_owning_ptr;

class Frame {
    Result CreateSyncObjects();
    Result DestroySyncObjects();
public:
    Frame();
    Frame(const Frame &other) = delete;
    Frame &operator=(const Frame &other) = delete;
    ~Frame();

    Result Create(Device *device, const non_owning_ptr<CommandBuffer> &cmd);
    Result Destroy(Device *device);

    inline CommandBuffer *GetCommandBuffer() { return command_buffer.get(); }
    inline const CommandBuffer *GetCommandBuffer() const { return command_buffer.get(); }

    inline SemaphoreChain &GetPresentSemaphores() { return present_semaphores; }
    inline const SemaphoreChain &GetPresentSemaphores() const { return present_semaphores; }

    /* Start recording into the command buffer */
    Result BeginCapture(Device *device);
    /* Stop recording into the command buffer */
    Result EndCapture(Device *device);
    /* Submit command buffer to the given queue */
    Result Submit(Queue *queue);
    
    non_owning_ptr<CommandBuffer> command_buffer;
    std::unique_ptr<Fence>        fc_queue_submit;
    SemaphoreChain present_semaphores;
};

} // namespace renderer
} // namespace hyperion

#endif