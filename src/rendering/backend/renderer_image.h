#ifndef RENDERER_IMAGE_H
#define RENDERER_IMAGE_H

#include "renderer_result.h"
#include "renderer_buffer.h"

#include <math/math_util.h>

namespace hyperion {
namespace renderer {

class Instance;
class Device;

class Image {
public:
    enum Type {
        TEXTURE_TYPE_2D = 0,
        TEXTURE_TYPE_3D = 1,
        TEXTURE_TYPE_CUBEMAP = 2
    };

    enum class BaseFormat {
        TEXTURE_FORMAT_NONE,
        TEXTURE_FORMAT_R,
        TEXTURE_FORMAT_RG,
        TEXTURE_FORMAT_RGB,
        TEXTURE_FORMAT_RGBA,

        TEXTURE_FORMAT_BGRA,

        TEXTURE_FORMAT_DEPTH
    };

    enum class InternalFormat {
        TEXTURE_INTERNAL_FORMAT_NONE,
        TEXTURE_INTERNAL_FORMAT_R8,
        TEXTURE_INTERNAL_FORMAT_RG8,
        TEXTURE_INTERNAL_FORMAT_RGB8,
        TEXTURE_INTERNAL_FORMAT_RGBA8,
        TEXTURE_INTERNAL_FORMAT_R16,
        TEXTURE_INTERNAL_FORMAT_RG16,
        TEXTURE_INTERNAL_FORMAT_RGB16,
        TEXTURE_INTERNAL_FORMAT_RGBA16,
        TEXTURE_INTERNAL_FORMAT_R32,
        TEXTURE_INTERNAL_FORMAT_RG32,
        TEXTURE_INTERNAL_FORMAT_RGB32,
        TEXTURE_INTERNAL_FORMAT_RGBA32,
        TEXTURE_INTERNAL_FORMAT_R16F,
        TEXTURE_INTERNAL_FORMAT_RG16F,
        TEXTURE_INTERNAL_FORMAT_RGB16F,
        TEXTURE_INTERNAL_FORMAT_RGBA16F,
        TEXTURE_INTERNAL_FORMAT_R32F,
        TEXTURE_INTERNAL_FORMAT_RG32F,
        TEXTURE_INTERNAL_FORMAT_RGB32F,
        TEXTURE_INTERNAL_FORMAT_RGBA32F,

        TEXTURE_INTERNAL_FORMAT_BGRA8_UNORM,
        TEXTURE_INTERNAL_FORMAT_BGRA8_SRGB,

        TEXTURE_INTERNAL_FORMAT_DEPTH_16,
        TEXTURE_INTERNAL_FORMAT_DEPTH_24,
        TEXTURE_INTERNAL_FORMAT_DEPTH_32,
        TEXTURE_INTERNAL_FORMAT_DEPTH_32F
    };

    enum class FilterMode {
        TEXTURE_FILTER_NEAREST,
        TEXTURE_FILTER_LINEAR,
        TEXTURE_FILTER_LINEAR_MIPMAP
    };

    enum class WrapMode {
        TEXTURE_WRAP_CLAMP_TO_EDGE,
        TEXTURE_WRAP_CLAMP_TO_BORDER,
        TEXTURE_WRAP_REPEAT
    };
    
    static BaseFormat GetBaseFormat(InternalFormat);
    /* returns a texture format that has a shifted bytes-per-pixel count
     * e.g calling with RGB16 and num components = 4 --> RGBA16 */
    static InternalFormat FormatChangeNumComponents(InternalFormat, uint8_t new_num_components);

    /* Get number of components (bytes-per-pixel) of a texture format */
    static size_t NumComponents(BaseFormat format);

    static bool IsDepthTexture(InternalFormat fmt);
    static bool IsDepthTexture(BaseFormat fmt);

    Result BlitImage(Instance *renderer, Vector4 dst_rect, Image *src_image, Vector4 src_rect);

    static VkFormat ToVkFormat(InternalFormat fmt);
    static VkImageType ToVkType(Type type);
    static VkFilter ToVkFilter(FilterMode);
    static VkSamplerAddressMode ToVkSamplerAddressMode(WrapMode);

    struct InternalInfo {
        VkImageTiling tiling;
        VkImageUsageFlags usage_flags;
    };

    Image(Extent3D extent,
        InternalFormat format,
        Type type,
        FilterMode filter_mode,
        const InternalInfo &internal_info,
        const unsigned char *bytes);

    Image(const Image &other) = delete;
    Image &operator=(const Image &other) = delete;
    ~Image();

    /*
     * Create the image. No texture data will be copied.
     */
    Result Create(Device *device);

    /* Create the image and transfer the provided texture data into it if given.
     * The image is transitioned into the given state.
     */
    Result Create(Device *device, Instance *renderer, GPUMemory::ResourceState state);

    Result Destroy(Device *device);

    bool IsDepthStencilImage() const;

    inline bool HasMipmaps() const
        { return m_filter_mode == FilterMode::TEXTURE_FILTER_LINEAR_MIPMAP; }

    inline uint32_t NumMipmaps() const
    {
        return HasMipmaps()
            ? static_cast<uint32_t>(MathUtil::FastLog2(MathUtil::Max(m_extent.width, m_extent.height, m_extent.depth))) + 1
            : 1;
    }

    inline bool IsCubemap() const
        { return m_type == TEXTURE_TYPE_CUBEMAP; }

    inline uint32_t NumFaces() const
        { return IsCubemap() ? 6 : 1; }

    inline const Extent3D &GetExtent() const { return m_extent; }

    inline GPUImageMemory *GetGPUImage() { return m_image; }
    inline const GPUImageMemory *GetGPUImage() const { return m_image; }

    inline InternalFormat GetTextureFormat() const { return m_format; }
    inline Type GetType() const { return m_type; }

    VkFormat GetImageFormat() const;
    VkImageType GetImageType() const;
    inline VkImageUsageFlags GetImageUsageFlags() const { return m_internal_info.usage_flags; }
    
private:
    Result CreateImage(Device *device,
        VkImageLayout initial_layout,
        VkImageCreateInfo *out_image_info);

    Result GenerateMipmaps(Device *device,
        CommandBuffer *command_buffer);

    Result ConvertTo32Bpp(Device *device,
        VkImageType image_type,
        VkImageCreateFlags image_create_flags,
        VkImageFormatProperties *out_image_format_properties,
        VkFormat *out_format);

    Extent3D m_extent;
    InternalFormat m_format;
    Type m_type;
    FilterMode m_filter_mode;
    unsigned char *m_bytes;
    bool m_assigned_image_data;

    InternalInfo m_internal_info;

    size_t m_size;
    size_t m_bpp; // bytes per pixel
    GPUImageMemory *m_image;
};

class StorageImage : public Image {
public:
    StorageImage(
        Extent3D extent,
        InternalFormat format,
        Type type,
        const unsigned char *bytes
    ) : Image(
        extent,
        format,
        type,
        FilterMode::TEXTURE_FILTER_NEAREST,
        Image::InternalInfo{
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage_flags = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        },
        bytes
    ) {}
};

class TextureImage : public Image {
public:
    TextureImage(
        Extent3D extent,
        InternalFormat format,
        Type type,
        FilterMode filter_mode,
        const unsigned char *bytes
    ) : Image(
        extent,
        format,
        type,
        filter_mode,
        Image::InternalInfo{
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage_flags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        },
        bytes
    ) {}
};

class TextureImage2D : public TextureImage {
public:
    TextureImage2D(
        Extent2D extent,
        InternalFormat format,
        FilterMode filter_mode,
        const unsigned char *bytes
    ) : TextureImage(
        Extent3D(extent),
        format,
        Type::TEXTURE_TYPE_2D,
        filter_mode,
        bytes
    ) {}
};

class TextureImage3D : public TextureImage {
public:
    TextureImage3D(
        Extent3D extent,
        InternalFormat format,
        FilterMode filter_mode,
        const unsigned char *bytes
    ) : TextureImage(
        extent,
        format,
        Type::TEXTURE_TYPE_3D,
        filter_mode,
        bytes
    ) {}
};

class TextureImageCubemap : public TextureImage {
public:
    TextureImageCubemap(
        Extent2D extent,
        InternalFormat format,
        FilterMode filter_mode,
        const unsigned char *bytes
    ) : TextureImage(
        Extent3D(extent),
        format,
        Type::TEXTURE_TYPE_CUBEMAP,
        filter_mode,
        bytes
    ) {}
};

class FramebufferImage : public Image {
public:
    FramebufferImage(
        Extent3D extent,
        InternalFormat format,
        Type type,
        const unsigned char *bytes
    ) : Image(
        extent,
        format,
        type,
        FilterMode::TEXTURE_FILTER_NEAREST,
        Image::InternalInfo{
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage_flags = VkImageUsageFlags(((GetBaseFormat(format) == BaseFormat::TEXTURE_FORMAT_DEPTH)
                ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
                | VK_IMAGE_USAGE_SAMPLED_BIT)
        },
        bytes
    ) {}
};

class FramebufferImage2D : public FramebufferImage {
public:
    FramebufferImage2D(
        Extent2D extent,
        InternalFormat format,
        const unsigned char *bytes
    ) : FramebufferImage(
        Extent3D(extent),
        format,
        Type::TEXTURE_TYPE_2D,
        bytes
    ) {}
};


} // namespace renderer
} // namespace hyperion

#endif