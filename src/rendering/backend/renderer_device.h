//
// Created by emd22 on 2022-02-20.
//

#ifndef HYPERION_RENDERER_DEVICE_H
#define HYPERION_RENDERER_DEVICE_H

#include <vector>
#include <set>
#include <unordered_map>
#include <string>

#include "renderer_result.h"
#include "renderer_structs.h"

#include "../../system/vma/vma_usage.h"

namespace hyperion {
namespace renderer {

class Features;

class Instance;

using ExtensionMap = std::unordered_map<std::string, bool>;

class Device {
    static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

public:
    Device(VkPhysicalDevice physical, VkSurfaceKHR surface);
    ~Device();

    void Destroy();
    
    void SetPhysicalDevice(VkPhysicalDevice);
    void SetRenderSurface(const VkSurfaceKHR &surface);
    void SetRequiredExtensions(const ExtensionMap &extensions);

    VkDevice         GetDevice();
    VkSurfaceKHR     GetRenderSurface();
    VkPhysicalDevice GetPhysicalDevice();

    Result SetupAllocator(Instance *instance);
    Result DestroyAllocator();
    VmaAllocator GetAllocator() const { return allocator; }

    inline const QueueFamilyIndices &GetQueueFamilyIndices() const { return queue_family_indices; }
    inline const Features &GetFeatures() const { return *features; }

    VkQueue GetQueue(QueueFamilyIndices::Index queue_family_index, uint32_t queue_index = 0);

    Result CreateLogicalDevice(const std::set<uint32_t> &required_queue_families);
    Result CheckDeviceSuitable();

    /*  Wait for the device to be idle */
    Result Wait() const;


    /* \brief Check if the set required extensions extensions are supported. Any unsupported extensions are returned. */
    ExtensionMap CheckExtensionSupport();

    std::vector<VkExtensionProperties> GetSupportedExtensions();

private:
    VkDevice                   device;
    VkPhysicalDevice           physical;
    VkSurfaceKHR               surface;
    VmaAllocator               allocator;

    Features           *features;
    QueueFamilyIndices queue_family_indices;

    ExtensionMap required_extensions;
};

} // namespace renderer
} // namespace hyperion

#endif //HYPERION_RENDERER_DEVICE_H
