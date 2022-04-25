#ifndef HYPERION_RENDERER_FEATURES_H
#define HYPERION_RENDERER_FEATURES_H

#include "renderer_result.h"
#include "renderer_image.h"
#include "renderer_structs.h"

#include <vulkan/vulkan.h>

#include <array>

namespace hyperion {
namespace renderer {

class Features {
public:
    struct {
#define HYP_DECL_FN(name) PFN_##name name = nullptr

        HYP_DECL_FN(vkGetBufferDeviceAddressKHR);
        HYP_DECL_FN(vkCmdBuildAccelerationStructuresKHR);
        HYP_DECL_FN(vkBuildAccelerationStructuresKHR);
        HYP_DECL_FN(vkCreateAccelerationStructureKHR);
        HYP_DECL_FN(vkDestroyAccelerationStructureKHR);
        HYP_DECL_FN(vkGetAccelerationStructureBuildSizesKHR);
        HYP_DECL_FN(vkGetAccelerationStructureDeviceAddressKHR);
        HYP_DECL_FN(vkCmdTraceRaysKHR);
        HYP_DECL_FN(vkGetRayTracingShaderGroupHandlesKHR);
        HYP_DECL_FN(vkCreateRayTracingPipelinesKHR);

#undef HYP_DECL_FN
    } dyn_functions;

    Features();
    Features(VkPhysicalDevice);

    Features(const Features &other) = delete;
    Features &operator=(const Features &other) = delete;
    ~Features() = default;

    void SetPhysicalDevice(VkPhysicalDevice);
    inline VkPhysicalDevice GetPhysicalDevice() const { return m_physical_device; }

    inline bool IsDiscreteGpu() const { return m_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU; }

    inline const char *GetDeviceName() const { return m_properties.deviceName; }

    inline const VkPhysicalDeviceProperties &GetPhysicalDeviceProperties() const
        { return m_properties; }

    inline const VkPhysicalDeviceFeatures &GetPhysicalDeviceFeatures() const
        { return m_features; }

    inline const VkPhysicalDeviceFeatures2 &GetPhysicalDeviceFeatures2() const
        { return m_features2; }

    inline const VkPhysicalDeviceMemoryProperties &GetPhysicalDeviceMemoryProperties() const
        { return m_memory_properties; }

    inline const VkPhysicalDeviceRayTracingPipelineFeaturesKHR &GetRaytracingPipelineFeatures() const
        { return m_raytracing_pipeline_features; }

    inline const VkPhysicalDeviceRayTracingPipelinePropertiesKHR &GetRaytracingPipelineProperties() const
        { return m_raytracing_pipeline_properties; }

    inline const VkPhysicalDeviceBufferDeviceAddressFeatures &GetBufferDeviceAddressFeatures() const
        { return m_buffer_device_address_features; }

    struct DeviceRequirementsResult {
        enum {
            DEVICE_REQUIREMENTS_OK = 0,
            DEVICE_REQUIREMENTS_ERR = 1
        } result;

        const char *message;

        DeviceRequirementsResult(decltype(result) result, const char *message = "")
            : result(result), message(message) {}
        DeviceRequirementsResult(const DeviceRequirementsResult &other)
            : result(other.result), message(other.message) {}
        inline operator bool() const { return result == DEVICE_REQUIREMENTS_OK; }
    };

#define REQUIRES_VK_FEATURE_MSG(cond, feature) \
    do { \
        if (!(cond)) { \
            return DeviceRequirementsResult(DeviceRequirementsResult::DEVICE_REQUIREMENTS_ERR, "Feature constraint '" #feature "' not satisfied."); \
        } \
    } while (0)

#define REQUIRES_VK_FEATURE(cond) \
    do { \
        if (!(cond)) { \
            return DeviceRequirementsResult(DeviceRequirementsResult::DEVICE_REQUIREMENTS_ERR, "Feature constraint '" #cond "' not satisfied."); \
        } \
    } while (0)

    DeviceRequirementsResult SatisfiesMinimumRequirements()
    {
        REQUIRES_VK_FEATURE_MSG(m_features.geometryShader, "Geometry shaders");
        REQUIRES_VK_FEATURE_MSG(m_features.fragmentStoresAndAtomics, "Image stores and atomics in fragment shaders"); /* for imageStore() in fragment shaders */
        REQUIRES_VK_FEATURE_MSG(m_features.shaderSampledImageArrayDynamicIndexing, "Dynamic sampler / image array indexing"); /* for accessing textures based on dynamic index (push constant) */
        REQUIRES_VK_FEATURE_MSG(m_indexing_features.descriptorBindingPartiallyBound && m_indexing_features.runtimeDescriptorArray, "Bindless descriptors"); /* bindless support */
        REQUIRES_VK_FEATURE(m_properties.limits.maxDescriptorSetSamplers >= 16);
        REQUIRES_VK_FEATURE(m_properties.limits.maxDescriptorSetUniformBuffers >= 16);

        return DeviceRequirementsResult(DeviceRequirementsResult::DEVICE_REQUIREMENTS_OK);
    }

#undef REQUIRES_VK_FEATURE

    void LoadDynamicFunctions(Device *device);

    SwapchainSupportDetails QuerySwapchainSupport(VkSurfaceKHR _surface) const
    {
        SwapchainSupportDetails details{};

        if (m_physical_device == nullptr) {
            DebugLog(LogType::Debug, "No physical device set -- cannot query swapchain support!\n");
            return details;
        }

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, _surface, &details.capabilities);

        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_family_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_family_count, queue_family_properties.data());

        uint32_t count = 0;
        /* Get device surface formats */
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, _surface, &count, nullptr);
        std::vector<VkSurfaceFormatKHR> surface_formats(count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, _surface, &count, surface_formats.data());
        if (count == 0)
            DebugLog(LogType::Warn, "No surface formats available!\n");

        vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, _surface, &count, nullptr);
        std::vector<VkPresentModeKHR> present_modes(count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, _surface, &count, present_modes.data());
        if (count == 0)
            DebugLog(LogType::Warn, "No present modes available!\n");

        details.queue_family_properties = queue_family_properties;
        details.formats = surface_formats;
        details.present_modes = present_modes;

        return details;
    }

    inline bool IsSupportedFormat(VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags features) const
    {
        if (m_physical_device == nullptr) {
            return false;
        }

        DebugLog(
            LogType::Debug,
            "Checking support for Vulkan format %d with tiling mode %d and feature flags %d.\n",
            format,
            tiling,
            features
        );

        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_physical_device, format, &props);

        const VkFormatFeatureFlags flags = (tiling == VK_IMAGE_TILING_LINEAR)
            ? props.linearTilingFeatures
            : (tiling == VK_IMAGE_TILING_OPTIMAL) ? props.optimalTilingFeatures : 0;

        if ((flags & features) == features) {
            DebugLog(
                LogType::Debug,
                "Vulkan format %d with tiling mode %d and feature flags %d support found!\n",
                format,
                tiling,
                features
            );

            return true;
        }

        DebugLog(
            LogType::Debug,
            "Vulkan format %d with tiling mode %d and feature flags %d not supported.\n",
            format,
            tiling,
            features
        );

        return false;
    }

    /* get the first supported format out of the provided list of format choices. */
    template <size_t Size>
    inline Image::InternalFormat FindSupportedFormat(const std::array<Image::InternalFormat, Size> &possible_formats, VkImageTiling tiling, VkFormatFeatureFlags features) const
    {
        static_assert(Size > 0, "Size must be greater than zero!");

        DebugLog(
            LogType::Debug,
            "Looking for format to use with tiling option %d and format features %d. First choice: %d\n",
            tiling,
            features,
            Image::ToVkFormat(possible_formats[0])
        );

        if (m_physical_device == nullptr) {
            DebugLog(LogType::Debug, "No physical device set -- cannot find supported format!\n");
            return Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_NONE;
        }

        for (size_t i = 0; i < Size; i++) {
            if (IsSupportedFormat(Image::ToVkFormat(possible_formats[i]), tiling, features) != VK_FORMAT_UNDEFINED) {
                return possible_formats[i];
            }
        }

        return Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_NONE;
    }

    /* get the first supported format out of the provided list of format choices. */
    template <size_t Size>
    inline VkFormat FindSupportedFormat(const std::array<VkFormat, Size> &possible_formats, VkImageTiling tiling, VkFormatFeatureFlags features) const
    {
        static_assert(Size > 0, "Size must be greater than zero!");

        DebugLog(
            LogType::Debug,
            "Looking for format to use with tiling option %d and format features %d. First choice: %d\n",
            tiling,
            features,
            possible_formats[0]
        );

        if (m_physical_device == nullptr) {
            DebugLog(LogType::Debug, "No physical device set -- cannot find supported format!\n");
            return VK_FORMAT_UNDEFINED;
        }

        for (size_t i = 0; i < Size; i++) {
            VkFormat format = possible_formats[i];

            if (IsSupportedFormat(format, tiling, features)) {
                return format;
            }
        }

        return VK_FORMAT_UNDEFINED;
    }

    /* get the first supported format out of the provided list of format choices. */
    template <size_t Size, class LambdaFunction>
    inline Image::InternalFormat FindSupportedSurfaceFormat(const SwapchainSupportDetails &details, const std::array<Image::InternalFormat, Size> &possible_formats, LambdaFunction predicate) const
    {
        static_assert(Size > 0, "Size must be greater than zero!");

        DebugLog(
            LogType::Debug,
            "Looking for format to use for surface. First choice: %d\n",
            possible_formats[0]
        );

        DebugLog(LogType::Debug, "Available options:\n");

        for (const VkSurfaceFormatKHR &surface_format : details.formats) {
            DebugLog(
                LogType::Debug,
                "\tFormat: %d\tColor space: %d\n",
                surface_format.format,
                surface_format.colorSpace
            );
        }

        for (auto wanted_format : possible_formats) {
            DebugLog(
                LogType::Debug,
                "Try format: %d\n",
                wanted_format
            );

            const VkFormat wanted_vk_format = Image::ToVkFormat(wanted_format);

            if (std::any_of(
                details.formats.begin(),
                details.formats.end(),
                [wanted_vk_format, &predicate](const VkSurfaceFormatKHR &surface_format) {
                    return surface_format.format == wanted_vk_format && predicate(surface_format);
                }
            )) {
                DebugLog(
                    LogType::Debug,
                    "Found surface format: %d\n",
                    wanted_format
                );

                return wanted_format;
            }
        }

        DebugLog(
            LogType::Debug,
            "No surface format found out of the selected options!\n"
        );

        return Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_NONE;
    }

    inline Result GetImageFormatProperties(VkFormat format,
        VkImageType             type,
        VkImageTiling           tiling,
        VkImageUsageFlags       usage,
        VkImageCreateFlags      flags,
        VkImageFormatProperties *out_properties) const
    {
        if (m_physical_device == nullptr) {
            return Result(Result::RENDERER_ERR, "Cannot find supported format; physical device is not initialized.");
        }

        if (vkGetPhysicalDeviceImageFormatProperties(m_physical_device, format, type, tiling, usage, flags, out_properties) != VK_SUCCESS) {
            return Result(Result::RENDERER_ERR, "Failed to get image format properties");
        }

        HYPERION_RETURN_OK;
    }

    inline bool IsSupportedImageFormat(VkFormat format,
        VkImageType             type,
        VkImageTiling           tiling,
        VkImageUsageFlags       usage,
        VkImageCreateFlags      flags) const
    {
        VkImageFormatProperties tmp;

        return GetImageFormatProperties(format, type, tiling, usage, flags, &tmp).result == Result::RENDERER_OK;
    }

    template <class StructType>
    constexpr size_t PaddedSize() const
    {
        return PaddedSize<StructType>(m_properties.limits.minUniformBufferOffsetAlignment);
    }

    template <class StructType>
    constexpr size_t PaddedSize(size_t alignment) const
    {
        return PaddedSize(sizeof(StructType), alignment);
    }
    
    constexpr size_t PaddedSize(size_t size, size_t alignment) const
    {
        return alignment
            ? (size + alignment - 1) & ~(alignment - 1)
            : size;
    }

    inline bool SupportsRaytracing() const
    {
        return m_raytracing_pipeline_features.rayTracingPipeline
            && m_acceleration_structure_features.accelerationStructure
            && m_buffer_device_address_features.bufferDeviceAddress;
    }

private:
    VkPhysicalDevice m_physical_device;
    VkPhysicalDeviceProperties m_properties;
    VkPhysicalDeviceFeatures m_features;

    VkPhysicalDeviceBufferDeviceAddressFeatures      m_buffer_device_address_features;
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR    m_raytracing_pipeline_features;
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR  m_raytracing_pipeline_properties;
    VkPhysicalDeviceAccelerationStructureFeaturesKHR m_acceleration_structure_features;
    VkPhysicalDeviceDescriptorIndexingFeatures       m_indexing_features;
    VkPhysicalDeviceFeatures2                        m_features2;
    VkPhysicalDeviceProperties2                      m_properties2;

    VkPhysicalDeviceMemoryProperties m_memory_properties;
};

} // namespace renderer
} // namespace hyperion

#endif