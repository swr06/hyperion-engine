//
// Created by emd22 on 2022-02-20.
//

#ifndef HYPERION_RENDERER_SHADER_H
#define HYPERION_RENDERER_SHADER_H

#include "renderer_device.h"

#include <asset/byte_reader.h>

#include <hash_code.h>
#include <types.h>

namespace hyperion {
namespace renderer {
struct ShaderObject {
    std::vector<ubyte> bytes;

    struct Metadata {
        std::string name;
        std::string path;
    } metadata;

    inline HashCode GetHashCode() const
    {
        HashCode hc;

        for (size_t i = 0; i < bytes.size(); i++) {
            hc.Add(bytes[i]);
        }

        hc.Add(metadata.name);
        hc.Add(metadata.path);

        return hc;
    }
};

struct ShaderModule {
    enum class Type : int {
        UNSET = 0,
        VERTEX,
        FRAGMENT,
        GEOMETRY,
        COMPUTE,
        /* Mesh shaders */
        TASK,
        MESH,
        /* Tesselation */
        TESS_CONTROL,
        TESS_EVAL,
        /* Raytracing */
        RAY_GEN,
        RAY_INTERSECT,
        RAY_ANY_HIT,
        RAY_CLOSEST_HIT,
        RAY_MISS,
    } type;

    ShaderObject spirv;
    VkShaderModule shader_module;

    ShaderModule(Type type)
        : type(type),
          spirv{},
          shader_module{}
    {}

    ShaderModule(Type type, const ShaderObject &spirv, VkShaderModule shader_module = nullptr)
        : type(type),
          spirv(spirv),
          shader_module(shader_module)
    {}

    ShaderModule(const ShaderModule &other) = default;
    ~ShaderModule() = default;

    inline bool operator<(const ShaderModule &other) const
        { return type < other.type; }

    inline bool IsRaytracing() const
    {
        return type == Type::RAY_GEN
            || type == Type::RAY_INTERSECT
            || type == Type::RAY_ANY_HIT
            || type == Type::RAY_CLOSEST_HIT
            || type == Type::RAY_MISS;
    }
};

class ShaderProgram {
public:
    static constexpr const char *entry_point = "main";

    ShaderProgram();
    ShaderProgram(const ShaderProgram &other) = delete;
    ShaderProgram &operator=(const ShaderProgram &other) = delete;
    ~ShaderProgram();

    inline const std::vector<ShaderModule> &GetShaderModules() const
        { return m_shader_modules; }

    inline const std::vector<VkPipelineShaderStageCreateInfo> &GetShaderStages() const
        { return m_shader_stages; }

    /* For raytracing only */
    inline const std::vector<VkRayTracingShaderGroupCreateInfoKHR> &GetShaderGroups() const
        { return m_shader_groups; }

    inline bool IsRaytracing() const
    {
        return std::any_of(
            m_shader_modules.begin(),
            m_shader_modules.end(),
            [](const ShaderModule &it) {
                return it.IsRaytracing();
            }
        );
    }

    Result AttachShader(Device *device, ShaderModule::Type type, const ShaderObject &spirv);

    Result Create(Device *device);
    Result Destroy(Device *device);

    inline HashCode GetHashCode() const
    {
        HashCode hc;

        for (const auto &shader_module : m_shader_modules) {
            hc.Add(int(shader_module.type));
            hc.Add(shader_module.spirv.GetHashCode());
        }

        return hc;
    }

private:
    VkPipelineShaderStageCreateInfo CreateShaderStage(const ShaderModule &);
    Result CreateShaderGroups();

    std::vector<ShaderModule> m_shader_modules;

    std::vector<VkPipelineShaderStageCreateInfo>      m_shader_stages;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_shader_groups;
};

} // namespace renderer
} // namespace hyperion

#endif //HYPERION_RENDERER_SHADER_H
