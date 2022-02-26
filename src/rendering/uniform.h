#ifndef UNIFORM_H
#define UNIFORM_H

#include <array>
#include <cstring>

#include "../math/vector2.h"
#include "../math/vector3.h"
#include "../math/vector4.h"
#include "../math/matrix4.h"
#include "texture.h"
#include "../util.h"

namespace hyperion {

class Shader;

class Uniform {
public:
    enum UniformType {
        Uniform_None = -1,
        Uniform_Float,
        Uniform_Int,
        Uniform_Vector2,
        Uniform_Vector3,
        Uniform_Vector4,
        Uniform_Matrix4,
        Uniform_Texture2D,
        Uniform_Texture3D,
        Uniform_TextureCube
    } type;

    std::array<float, 16> data;

    Uniform()
    {
        data = { 0.0f };
        type = Uniform_None;
    }

    Uniform(float value)
    {
        data[0] = value;
        type = Uniform_Float;
    }

    Uniform(int value)
    {
        data[0] = (float)value;
        type = Uniform_Int;
    }

    Uniform(const Vector2 &value)
    {
        data[0] = value.x;
        data[1] = value.y;
        type = Uniform_Vector2;
    }

    Uniform(const Vector3 &value)
    {
        data[0] = value.x;
        data[1] = value.y;
        data[2] = value.z;
        type = Uniform_Vector3;
    }

    Uniform(const Vector4 &value)
    {
        data[0] = value.x;
        data[1] = value.y;
        data[2] = value.z;
        data[3] = value.w;
        type = Uniform_Vector4;
    }

    Uniform(const Matrix4 &value)
    {
        std::memcpy(&data[0], &value.values[0], value.values.size() * sizeof(float));
        type = Uniform_Matrix4;
    }

    Uniform(const Texture *texture)
    {
        ex_assert(texture != nullptr);

        data[0] = texture->GetId();
        // texture->GetTextureType() should start at 0 and map to the correct uniform texture type
        type = UniformType(int(Uniform_Texture2D) + int(texture->GetTextureType()));
    }

    Uniform &operator=(const Uniform &other)
    {
        type = other.type;
        data = other.data;
        return *this;
    }

    void BindUniform(Shader *shader, const char *name, int &texture_index);
};
} // namespace hyperion

#endif