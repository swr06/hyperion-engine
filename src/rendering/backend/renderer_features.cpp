#include "renderer_features.h"

namespace hyperion {
namespace renderer {
Features::Features()
    : m_physical_device(nullptr),
      m_properties({}),
      m_features({})
{
}

Features::Features(VkPhysicalDevice physical_device)
    : Features()
{
    SetPhysicalDevice(physical_device);
}

void Features::SetPhysicalDevice(VkPhysicalDevice physical_device)
{
    if ((m_physical_device = physical_device)) {
        vkGetPhysicalDeviceProperties(physical_device, &m_properties);
        vkGetPhysicalDeviceFeatures(physical_device, &m_features);
        vkGetPhysicalDeviceMemoryProperties(physical_device, &m_memory_properties);

        m_buffer_device_address_features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
            .pNext = VK_NULL_HANDLE
        };
        
        m_raytracing_pipeline_features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
            .pNext = &m_buffer_device_address_features
        };
        
        m_acceleration_structure_features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
            .pNext = &m_raytracing_pipeline_features
        };
        
        m_indexing_features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
            .pNext = &m_acceleration_structure_features
        };

        m_features2 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &m_indexing_features
        };

        vkGetPhysicalDeviceFeatures2(m_physical_device, &m_features2);

        m_raytracing_pipeline_properties = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
            .pNext = VK_NULL_HANDLE
        };

        m_properties2 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            .pNext = &m_raytracing_pipeline_properties
        };

        vkGetPhysicalDeviceProperties2(m_physical_device, &m_properties2);
    }
}

void Features::LoadDynamicFunctions(Device *device)
{
#define HYP_LOAD_FN(name) do { \
        auto proc_addr = vkGetDeviceProcAddr(device->GetDevice(), #name); \
        AssertThrowMsg(proc_addr != nullptr, "Failed to load dynamic function " #name "\n"); \
        dyn_functions.##name = reinterpret_cast<PFN_##name>(proc_addr); \
    } while (0)

    HYP_LOAD_FN(vkGetBufferDeviceAddressKHR);
    HYP_LOAD_FN(vkCmdBuildAccelerationStructuresKHR);
    HYP_LOAD_FN(vkBuildAccelerationStructuresKHR);
    HYP_LOAD_FN(vkCreateAccelerationStructureKHR);
    HYP_LOAD_FN(vkDestroyAccelerationStructureKHR);
    HYP_LOAD_FN(vkGetAccelerationStructureBuildSizesKHR);
    HYP_LOAD_FN(vkGetAccelerationStructureDeviceAddressKHR);
    HYP_LOAD_FN(vkCmdTraceRaysKHR);
    HYP_LOAD_FN(vkGetRayTracingShaderGroupHandlesKHR);
    HYP_LOAD_FN(vkCreateRayTracingPipelinesKHR);

#undef HYP_LOAD_FN
}

} // namespace renderer
} // namespace hyperion