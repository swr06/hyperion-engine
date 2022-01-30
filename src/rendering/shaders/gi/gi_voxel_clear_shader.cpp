#include "gi_voxel_clear_shader.h"
#include "../../gi/gi_manager.h"
#include "../../../asset/asset_manager.h"
#include "../../../asset/text_loader.h"
#include "../../../opengl.h"

namespace hyperion {
GIVoxelClearShader::GIVoxelClearShader(const ShaderProperties &properties)
    : ComputeShader(properties, GIManager::voxel_map_size, GIManager::voxel_map_size, GIManager::voxel_map_size)
{
    const std::string cs_path("res/shaders/gi/clear.comp");

    AddSubShader(
        Shader::SubShaderType::SUBSHADER_COMPUTE,
        AssetManager::GetInstance()->LoadFromFile<TextLoader::LoadedText>(cs_path)->GetText(),
        properties,
        cs_path
    );
}
} // namespace hyperion