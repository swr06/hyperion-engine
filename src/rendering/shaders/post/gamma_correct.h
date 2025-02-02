#ifndef GAMMA_CORRECT_H
#define GAMMA_CORRECT_H

#include "../post_shader.h"

namespace hyperion {

class GammaCorrectShader : public PostShader {
public:
    GammaCorrectShader(const ShaderProperties &properties);
    virtual ~GammaCorrectShader() = default;

    virtual void ApplyTransforms(const Transform &transform, Camera *camera);
};

} // namespace hyperion

#endif
