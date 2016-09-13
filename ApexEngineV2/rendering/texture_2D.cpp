#include "texture_2D.h"
#include "../core_engine.h"

namespace apex {
Texture2D::Texture2D()
    : Texture()
{
    is_uploaded = false;
    is_created = false;
}

Texture2D::Texture2D(int width, int height, unsigned char *bytes)
    : Texture(width, height, bytes)
{
    is_uploaded = false;
    is_created = false;
}

Texture2D::~Texture2D()
{
    if (is_created) {
        CoreEngine::GetInstance()->DeleteTextures(1, &id);
    }
    is_uploaded = false;
    is_created = false;
}

void Texture2D::Use()
{
    if (!is_created) {
        CoreEngine::GetInstance()->GenTextures(1, &id);
        is_created = true;
    }

    CoreEngine::GetInstance()->BindTexture(CoreEngine::TEXTURE_2D, id);

    if (!is_uploaded) {
        CoreEngine::GetInstance()->TexParameteri(CoreEngine::TEXTURE_2D,
            CoreEngine::TEXTURE_MAG_FILTER, mag_filter);
        CoreEngine::GetInstance()->TexParameteri(CoreEngine::TEXTURE_2D, 
            CoreEngine::TEXTURE_MIN_FILTER, min_filter);
        CoreEngine::GetInstance()->TexParameteri(CoreEngine::TEXTURE_2D, 
            CoreEngine::TEXTURE_WRAP_S, wrap_s);
        CoreEngine::GetInstance()->TexParameteri(CoreEngine::TEXTURE_2D, 
            CoreEngine::TEXTURE_WRAP_T, wrap_t);

        CoreEngine::GetInstance()->TexImage2D(CoreEngine::TEXTURE_2D, 0, ifmt,
            width, height, 0, fmt, CoreEngine::UNSIGNED_BYTE, bytes);

        CoreEngine::GetInstance()->GenerateMipmap(CoreEngine::TEXTURE_2D);

        is_uploaded = true;
    }
}

void Texture2D::End()
{
    CoreEngine::GetInstance()->BindTexture(CoreEngine::TEXTURE_2D, 0);
}
}