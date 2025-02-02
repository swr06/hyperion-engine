#include "./cubemap.h"
#include "../gl_util.h"
#include "../util.h"
#include "../core_engine.h"
#include "../math/math_util.h"

#include <iostream>

namespace hyperion {

Cubemap::Cubemap(const std::array<std::shared_ptr<Texture2D>, 6> &textures)
    : Texture(TextureType::TEXTURE_TYPE_CUBEMAP),
      m_textures(textures)
{
    for (auto &texture : m_textures) {
        if (texture == nullptr) {
            continue;
        }

        width = texture->GetWidth();
        height = texture->GetHeight();
        fmt = texture->GetFormat();
        ifmt = texture->GetInternalFormat();
        mag_filter = texture->GetMagFilter();
        min_filter = texture->GetMinFilter();
        wrap_s = texture->GetWrapS();
        wrap_t = texture->GetWrapT();

        break;
    }
}

Cubemap::~Cubemap()
{
}

void Cubemap::Initialize()
{
    CoreEngine::GetInstance()->Enable(CoreEngine::GLEnums::TEXTURE_CUBE_MAP);

    CatchGLErrors("Failed to enable GL_TEXTURE_CUBE_MAP", false);

    Texture::Initialize();
}

void Cubemap::CopyData(Texture * const other)
{
    not_implemented;
}

void Cubemap::UploadGpuData(bool should_upload_data)
{
    if (should_upload_data) {
        for (size_t i = 0; i < m_textures.size(); i++) {
            const auto &tex = m_textures[i];

            if (tex == nullptr) {
                throw std::runtime_error("Could not upload cubemap because texture #" + std::to_string(i + 1) + " was nullptr.");
            } else if (tex->GetBytes() == nullptr) {
                throw std::runtime_error("Could not upload cubemap because texture #" + std::to_string(i + 1) + " had no bytes set.");
            }

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, ToOpenGLInternalFormat(tex->GetInternalFormat()),
                tex->GetWidth(), tex->GetHeight(), 0, ToOpenGLBaseFormat(tex->GetFormat()), ToOpenGLDatumType(tex->GetDatumType()), tex->GetBytes());
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, ToOpenGLFilterMode(mag_filter));
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, ToOpenGLFilterMode(min_filter));
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 8);

    if (should_upload_data) {

        if (min_filter == Texture::TextureFilterMode::TEXTURE_FILTER_LINEAR_MIPMAP) {
            glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
            CatchGLErrors("Failed to generate cubemap mipmaps.", false);
        }
    }
    CatchGLErrors("Failed to upload cubemap");
}

void Cubemap::Use()
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);
}

void Cubemap::End()
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

} // namespace hyperion
