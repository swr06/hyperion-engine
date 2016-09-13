#include "lighting_shader.h"
#include "../environment.h"
#include "../../asset/asset_manager.h"
#include "../../asset/text_loader.h"
#include "../../util/shader_preprocessor.h"

namespace apex {
LightingShader::LightingShader(const ShaderProperties &properties)
    : Shader(properties)
{
    const std::string vs_path("res/shaders/default.vert");
    const std::string fs_path("res/shaders/default.frag");

    AddSubShader(SubShader(CoreEngine::VERTEX_SHADER,
        ShaderPreprocessor::ProcessShader(
            AssetManager::GetInstance()->LoadFromFile<TextLoader::LoadedText>(vs_path)->GetText(),
            properties, vs_path)
        ));

    AddSubShader(SubShader(CoreEngine::FRAGMENT_SHADER,
        ShaderPreprocessor::ProcessShader(
            AssetManager::GetInstance()->LoadFromFile<TextLoader::LoadedText>(fs_path)->GetText(),
            properties, fs_path)
        ));

    for (int i = 0; i < 16; i++) {
        SetUniform("poissonDisk[" + std::to_string(i) + "]", 
            Environment::possion_disk[i]);
    }
}

void LightingShader::ApplyMaterial(const Material &mat)
{
    auto *env = Environment::GetInstance();
    if (env->ShadowsEnabled()) {
        for (int i = 0; i < env->NumCascades(); i++) {
            std::string i_str = std::to_string(i);
            Texture::ActiveTexture(5 + i);
            env->GetShadowMap(i)->Use();
            SetUniform("u_shadowMap[" + i_str + "]", 5 + i);
            SetUniform("u_shadowMatrix[" + i_str + "]", env->GetShadowMatrix(i));
        }
    }

    env->GetSun().Bind(0, this);

    SetUniform("u_diffuseColor", mat.diffuse_color);

    if (mat.diffuse_texture != nullptr) {
        Texture::ActiveTexture(0);
        mat.diffuse_texture->Use();
        SetUniform("u_diffuseTexture", 0);
    }
}

void LightingShader::ApplyTransforms(const Matrix4 &transform, Camera *camera)
{
    Shader::ApplyTransforms(transform, camera);
    SetUniform("u_camerapos", camera->GetTranslation());
}
}