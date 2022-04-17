#include "renderer_image.h"
#include "renderer_graphics_pipeline.h"
#include "renderer_instance.h"
#include "renderer_helpers.h"
#include "renderer_device.h"
#include "renderer_features.h"

#include <util/img/image_util.h>
#include <system/debug.h>

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <cstring>

namespace hyperion {
namespace renderer {

Image::BaseFormat Image::GetBaseFormat(InternalFormat fmt)
{
    switch (fmt) {
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_R8:
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_R16:
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_R32:
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_R16F:
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_R32F:
        return BaseFormat::TEXTURE_FORMAT_R;
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_RG8:
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_RG16:
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_RG32:
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_RG16F:
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_RG32F:
        return BaseFormat::TEXTURE_FORMAT_RG;
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_RGB8:
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_RGB16:
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_RGB32:
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_RGB16F:
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_RGB32F:
        return BaseFormat::TEXTURE_FORMAT_RGB;
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_RGBA8:
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_RGBA16:
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_RGBA32:
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_RGBA16F:
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_RGBA32F:
        return BaseFormat::TEXTURE_FORMAT_RGBA;
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_BGRA8_UNORM:
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_BGRA8_SRGB:
        return BaseFormat::TEXTURE_FORMAT_BGRA;
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_DEPTH_16:
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_DEPTH_24:
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_DEPTH_32:
    case InternalFormat::TEXTURE_INTERNAL_FORMAT_DEPTH_32F:
        return BaseFormat::TEXTURE_FORMAT_DEPTH;
    }

    unexpected_value_msg(format, "Unhandled image format case");
}

Image::InternalFormat Image::FormatChangeNumComponents(InternalFormat fmt, uint8_t new_num_components)
{
    if (new_num_components == 0) {
        return InternalFormat::TEXTURE_INTERNAL_FORMAT_NONE;
    }

    new_num_components = MathUtil::Clamp(new_num_components, uint8_t(1), uint8_t(4));

    int current_num_components = int(NumComponents(GetBaseFormat(fmt)));

    return InternalFormat(int(fmt) + int(new_num_components) - current_num_components);
}

size_t Image::NumComponents(BaseFormat format)
{
    switch (format) {
    case BaseFormat::TEXTURE_FORMAT_NONE:  return 0;
    case BaseFormat::TEXTURE_FORMAT_R:     return 1;
    case BaseFormat::TEXTURE_FORMAT_RG:    return 2;
    case BaseFormat::TEXTURE_FORMAT_RGB:   return 3;
    case BaseFormat::TEXTURE_FORMAT_RGBA:  return 4;
    case BaseFormat::TEXTURE_FORMAT_BGRA:  return 4;
    case BaseFormat::TEXTURE_FORMAT_DEPTH: return 1;
    }

    unexpected_value_msg(format, "Unknown number of components for format");
}

bool Image::IsDepthTexture(InternalFormat fmt)
{
    return IsDepthTexture(GetBaseFormat(fmt));
}

bool Image::IsDepthTexture(BaseFormat fmt)
{
    return fmt == BaseFormat::TEXTURE_FORMAT_DEPTH;
}

VkFormat Image::ToVkFormat(InternalFormat fmt)
{
    switch (fmt) {
    case Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_R8:          return VK_FORMAT_R8_UNORM;
    case Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_RG8:         return VK_FORMAT_R8G8_UNORM;
    case Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_RGB8:        return VK_FORMAT_R8G8B8_UNORM;
    case Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_RGBA8:       return VK_FORMAT_R8G8B8A8_UNORM;
    case Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_R16:         return VK_FORMAT_R16_UNORM;
    case Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_RG16:        return VK_FORMAT_R16G16_UNORM;
    case Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_RGB16:       return VK_FORMAT_R16G16B16_UNORM;
    case Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_RGBA16:      return VK_FORMAT_R16G16B16A16_UNORM;
    case Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_R16F:        return VK_FORMAT_R16_SFLOAT;
    case Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_RG16F:       return VK_FORMAT_R16G16_SFLOAT;
    case Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_RGB16F:      return VK_FORMAT_R16G16B16_SFLOAT;
    case Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_RGBA16F:     return VK_FORMAT_R16G16B16A16_SFLOAT;
    case Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_R32F:        return VK_FORMAT_R32_SFLOAT;
    case Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_RG32F:       return VK_FORMAT_R32G32_SFLOAT;
    case Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_RGB32F:      return VK_FORMAT_R32G32B32_SFLOAT;
    case Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_RGBA32F:     return VK_FORMAT_R32G32B32A32_SFLOAT;
    case Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_DEPTH_16:    return VK_FORMAT_D16_UNORM;
    case Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_DEPTH_24:    return VK_FORMAT_D24_UNORM_S8_UINT;
    case Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_DEPTH_32F:   return VK_FORMAT_D32_SFLOAT;
    case Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_BGRA8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
    case Image::InternalFormat::TEXTURE_INTERNAL_FORMAT_BGRA8_SRGB:  return VK_FORMAT_B8G8R8A8_SRGB;
    }

    unexpected_value_msg(format, "Unhandled texture format case");
}

VkImageType Image::ToVkType(Type type)
{
    switch (type) {
    case Image::Type::TEXTURE_TYPE_2D: return VK_IMAGE_TYPE_2D;
    case Image::Type::TEXTURE_TYPE_3D: return VK_IMAGE_TYPE_3D;
    case Image::Type::TEXTURE_TYPE_CUBEMAP: return VK_IMAGE_TYPE_2D;
    }

    unexpected_value_msg(format, "Unhandled texture type case");
}

VkFilter Image::ToVkFilter(FilterMode filter_mode)
{
    switch (filter_mode) {
    case FilterMode::TEXTURE_FILTER_NEAREST: return VK_FILTER_NEAREST;
    case FilterMode::TEXTURE_FILTER_LINEAR_MIPMAP: // fallthrough
    case FilterMode::TEXTURE_FILTER_LINEAR: return VK_FILTER_LINEAR;
    }

    unexpected_value_msg(format, "Unhandled texture filter mode case");
}

VkSamplerAddressMode Image::ToVkSamplerAddressMode(WrapMode texture_wrap_mode)
{
    switch (texture_wrap_mode) {
    case WrapMode::TEXTURE_WRAP_CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case WrapMode::TEXTURE_WRAP_CLAMP_TO_BORDER: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    case WrapMode::TEXTURE_WRAP_REPEAT: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }

    unexpected_value_msg(format, "Unhandled texture wrap mode case");
}

Image::Image(Extent3D extent,
    Image::InternalFormat format,
    Image::Type type,
    Image::FilterMode filter_mode,
    const InternalInfo &internal_info,
    const unsigned char *bytes)
    : m_extent(extent),
      m_format(format),
      m_type(type),
      m_filter_mode(filter_mode),
      m_internal_info(internal_info),
      m_image(nullptr),
      m_bpp(NumComponents(GetBaseFormat(format))),
      m_assigned_image_data(bytes != nullptr)
{
    m_size = m_extent.width * m_extent.height * m_extent.depth * m_bpp * NumFaces();

    m_bytes = new unsigned char[m_size];

    if (bytes != nullptr) {
        std::memcpy(m_bytes, bytes, m_size);
    }
}

Image::~Image()
{
    AssertExit(m_image == nullptr);

    delete[] m_bytes;
}

bool Image::IsDepthStencilImage() const { return IsDepthTexture(m_format); }
VkFormat Image::GetImageFormat() const { return ToVkFormat(m_format); }
VkImageType Image::GetImageType() const { return ToVkType(m_type); }

Result Image::CreateImage(Device *device,
    VkImageLayout initial_layout,
    VkImageCreateInfo *out_image_info)
{
    VkFormat format = GetImageFormat();
    VkImageType image_type = GetImageType();
    VkImageCreateFlags image_create_flags = 0;
    VkFormatFeatureFlags format_features = 0;
    VkImageFormatProperties image_format_properties{};

    if (HasMipmaps()) {
        /* Mipmapped image needs linear blitting. */
        DebugLog(LogType::Debug, "Mipmapped image needs linear blitting support. Checking...\n");

        format_features |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

        m_internal_info.usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    if (IsCubemap()) {
        DebugLog(LogType::Debug, "Creating cubemap , enabling VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT flag.\n");

        image_create_flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    auto format_support_result = device->GetFeatures().GetImageFormatProperties(
        format,
        image_type,
        m_internal_info.tiling,
        m_internal_info.usage_flags,
        image_create_flags,
        &image_format_properties
    );

    if (!format_support_result) {
        // try a series of fixes to get the image in a valid state.

        std::vector<std::pair<const char *, std::function<Result()>>> potential_fixes;

        // convert to 32bpp image
        if (m_bpp != 4) {
            potential_fixes.emplace_back(std::make_pair(
                "Convert to 32-bpp image",
                [&]() -> Result {
                    return ConvertTo32Bpp(
                        device,
                        image_type,
                        image_create_flags,
                        &image_format_properties,
                        &format
                    );
                }
            ));
        }

        for (auto &fix : potential_fixes) {
            DebugLog(LogType::Debug, "Attempting fix: '%s' ...\n", fix.first);

            auto fix_result = fix.second();

            AssertContinueMsg(fix_result, "Image fix function returned an invalid result: %s\n", fix_result.message);

            // try checking format support result again
            if ((format_support_result = device->GetFeatures().GetImageFormatProperties(
                format, image_type, m_internal_info.tiling, m_internal_info.usage_flags, image_create_flags, &image_format_properties))) {
                DebugLog(LogType::Debug, "Fix '%s' successful!\n", fix.first);
                break;
            }

            DebugLog(LogType::Warn, "Fix '%s' did not change image state to valid.\n", fix.first);
        }

        HYPERION_BUBBLE_ERRORS(format_support_result);
    }

    if (format_features != 0) {
        if (!device->GetFeatures().IsSupportedFormat(format, m_internal_info.tiling, format_features)) {
            return Result(Result::RENDERER_ERR, "Format does not support requested features.");
        }
    }

    const QueueFamilyIndices &qf_indices = device->GetQueueFamilyIndices();
    const uint32_t image_family_indices[] = { qf_indices.graphics_family.value(), qf_indices.compute_family.value() };

    VkImageCreateInfo image_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    image_info.imageType             = image_type;
    image_info.extent.width          = m_extent.width;
    image_info.extent.height         = m_extent.height;
    image_info.extent.depth          = m_extent.depth;
    image_info.mipLevels             = uint32_t(NumMipmaps());
    image_info.arrayLayers           = uint32_t(NumFaces());
    image_info.format                = format;
    image_info.tiling                = m_internal_info.tiling;
    image_info.initialLayout         = initial_layout;
    image_info.usage                 = m_internal_info.usage_flags;
    image_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples               = VK_SAMPLE_COUNT_1_BIT;
    image_info.flags                 = image_create_flags; // TODO: look into flags for sparse s for VCT
    image_info.pQueueFamilyIndices   = image_family_indices;
    image_info.queueFamilyIndexCount = uint32_t(std::size(image_family_indices));

    *out_image_info = image_info;

    m_image = new GPUImageMemory(m_internal_info.usage_flags);

    HYPERION_BUBBLE_ERRORS(m_image->Create(device, m_size, &image_info));

    HYPERION_RETURN_OK;
}

Result Image::Create(Device *device)
{
    VkImageCreateInfo image_info;

    return CreateImage(device, VK_IMAGE_LAYOUT_UNDEFINED, &image_info);
}

Result Image::Create(Device *device, Instance *renderer, GPUMemory::ResourceState state)
{
    auto result = Result::OK;

    VkImageCreateInfo image_info;

    HYPERION_BUBBLE_ERRORS(CreateImage(device, VK_IMAGE_LAYOUT_UNDEFINED, &image_info));

    VkImageSubresourceRange range{};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = NumMipmaps();
    range.baseArrayLayer = 0;
    range.layerCount = NumFaces();

    auto commands = renderer->GetSingleTimeCommands();
    StagingBuffer staging_buffer;

    if (m_assigned_image_data)  {
        /* transition from 'undefined' layout state into one optimal for transfer */
        HYPERION_BUBBLE_ERRORS(staging_buffer.Create(device, m_size), result);

        staging_buffer.Copy(device, m_size, m_bytes);
        // safe to delete m_bytes here?

        commands.Push([&](CommandBuffer *command_buffer) {
            m_image->InsertBarrier(command_buffer, range, GPUMemory::ResourceState::COPY_DST);

            HYPERION_RETURN_OK;
        });

        // copy from staging to image
        const auto num_faces = NumFaces();
        const auto num_mipmaps = NumMipmaps();

        size_t buffer_offset_step = m_size / num_faces;

        for (uint32_t i = 0; i < num_faces; i++) {
            commands.Push([this, &staging_buffer, &image_info, i, buffer_offset_step](CommandBuffer *command_buffer) {
                VkBufferImageCopy region{};
                region.bufferOffset = i * buffer_offset_step;
                region.bufferRowLength = 0;
                region.bufferImageHeight = 0;
                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region.imageSubresource.mipLevel = 0;
                region.imageSubresource.baseArrayLayer = i;
                region.imageSubresource.layerCount = 1;
                region.imageOffset = { 0, 0, 0 };
                region.imageExtent = image_info.extent;

                vkCmdCopyBufferToImage(
                    command_buffer->GetCommandBuffer(),
                    staging_buffer.buffer,
                    m_image->image,
                    GPUMemory::GetImageLayout(m_image->GetResourceState()),
                    1,
                    &region
                );

                HYPERION_RETURN_OK;
            });
        }

        /* Generate mipmaps if it applies */
        if (HasMipmaps()) {
            /* Assuming device supports this format with linear blitting -- check is done in CreateImage() */

            /* Generate our mipmaps
            */
            commands.Push([this, device](CommandBuffer *command_buffer) {
                return GenerateMipmaps(device, command_buffer);
            });
        }
    }

    /* Transition from whatever the previous layout state was to our destination state */
    commands.Push([&](CommandBuffer *command_buffer) {
        m_image->InsertBarrier(
            command_buffer,
            range,
            state
        );

        HYPERION_RETURN_OK;
    });

    // execute command stack
    HYPERION_PASS_ERRORS(commands.Execute(device), result);

    if (m_assigned_image_data) {
        // destroy staging buffer
        HYPERION_PASS_ERRORS(staging_buffer.Destroy(device), result);

        m_assigned_image_data = false;
    }

    return result;
}

Result Image::BlitImage(Instance *renderer, Vector4 dst_rect, Image *src_image, Vector4 src_rect)
{
    auto commands = renderer->GetSingleTimeCommands();

    VkImageSubresourceLayers input_layer{};
    input_layer.aspectMask = (VK_IMAGE_ASPECT_COLOR_BIT);
    input_layer.layerCount = 1;

    VkImageBlit region{};
    region.srcSubresource = input_layer;
    region.dstSubresource = input_layer;

    DebugLog(LogType::Info, "Blitting texture (%f, %f) :: (%f, %f)\n", src_rect.x, src_rect.y, src_rect.z, src_rect.w);

    region.srcOffsets[0] = { (int32_t)src_rect.x, (int32_t)src_rect.y, 0 };
    region.srcOffsets[1] = { (int32_t)src_rect.z, (int32_t)src_rect.w, 1 };

    region.dstOffsets[0] = { (int32_t)dst_rect.x, (int32_t)dst_rect.y, 0 };
    region.dstOffsets[1] = { (int32_t)dst_rect.z, (int32_t)dst_rect.w, 1 };

    commands.Push([&](CommandBuffer *cmd) {
        vkCmdBlitImage(
            cmd->GetCommandBuffer(),
            src_image->GetGPUImage()->image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            this->GetGPUImage()->image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region,
            VK_FILTER_NEAREST
        );

        HYPERION_RETURN_OK;
    });

    HYPERION_BUBBLE_ERRORS(commands.Execute(renderer->GetDevice()));

    HYPERION_RETURN_OK;
}

Result Image::Destroy(Device *device)
{
    auto result = Result::OK;

    HYPERION_PASS_ERRORS(m_image->Destroy(device), result);

    delete m_image;
    m_image = nullptr;

    return result;
}

Result Image::GenerateMipmaps(Device *device,
    CommandBuffer *command_buffer)
{
    const auto num_faces = NumFaces();
    const auto num_mipmaps = NumMipmaps();

    for (uint32_t face = 0; face < num_faces; face++) {
        for (int32_t i = 1; i < static_cast<int32_t>(num_mipmaps + 1); i++) {
            const auto mip_width  = static_cast<int32_t>(helpers::MipmapSize(m_extent.width, i)),
                       mip_height = static_cast<int32_t>(helpers::MipmapSize(m_extent.height, i)),
                       mip_depth  = static_cast<int32_t>(helpers::MipmapSize(m_extent.depth, i));

            /* Memory barrier for transfer - note that after generating the mipmaps,
                we'll still need to transfer into a layout primed for reading from shaders. */
            
            m_image->InsertSubResourceBarrier(
                command_buffer,
                ImageSubResource{
                    .aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .base_array_layer = face,
                    .base_mip_level = static_cast<uint32_t>(i - 1)
                },
                GPUMemory::ResourceState::COPY_SRC
            );

            if (i == static_cast<int32_t>(num_mipmaps)) {
                if (face == num_faces - 1) {
                    m_image->SetResourceState(GPUMemory::ResourceState::COPY_SRC);
                }

                break;
            }

            /* Blit the image into the subresource */
            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {
                static_cast<int32_t>(helpers::MipmapSize(m_extent.width, i - 1)),
                static_cast<int32_t>(helpers::MipmapSize(m_extent.height, i - 1)),
                static_cast<int32_t>(helpers::MipmapSize(m_extent.depth, i - 1))
            };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1; /* Read from previous mip level */
            blit.srcSubresource.baseArrayLayer = face;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {
                mip_width,
                mip_height,
                mip_depth
            };

            blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel       = i;
            blit.dstSubresource.baseArrayLayer = face;
            blit.dstSubresource.layerCount     = 1;

            vkCmdBlitImage(command_buffer->GetCommandBuffer(),
                m_image->image,
                GPUMemory::GetImageLayout(GPUMemory::ResourceState::COPY_SRC),
                m_image->image,
                GPUMemory::GetImageLayout(GPUMemory::ResourceState::COPY_DST),
                1, &blit,
                VK_FILTER_LINEAR
            );
        }
    }

    HYPERION_RETURN_OK;
}

Result Image::ConvertTo32Bpp(
    Device *device,
    VkImageType image_type,
    VkImageCreateFlags image_create_flags,
    VkImageFormatProperties *out_image_format_properties,
    VkFormat *out_format)
{
    constexpr size_t new_bpp = 4;

    const size_t num_faces = NumFaces();
    const size_t face_offset_step = m_size / num_faces;

    const size_t new_size = num_faces * new_bpp * m_extent.width * m_extent.height * m_extent.depth;
    const size_t new_face_offset_step = new_size / num_faces;

    auto *new_bytes = new unsigned char[new_size];

    for (int i = 0; i < num_faces; i++) {
        ImageUtil::ConvertBpp(
            m_extent.width, m_extent.height, m_extent.depth,
            m_bpp, new_bpp,
            &m_bytes[i * face_offset_step],
            &new_bytes[i * new_face_offset_step]
        );
    }

    delete[] m_bytes;
    m_bytes = new_bytes;

    m_format = FormatChangeNumComponents(m_format, new_bpp);
    m_bpp = new_bpp;
    m_size = new_size;

    *out_format = GetImageFormat();

    HYPERION_RETURN_OK;
}

} // namespace renderer
} // namespace hyperion