#include "shader_code.h"

namespace hyperion {

const char *ShaderCode::aabb_debug_vs =
    "#version 330 core\n"
    "layout(location = 0) in vec3 a_position;"
    "out vec4 v_position;"
    "uniform mat4 u_modelMatrix;"
    "uniform mat4 u_viewMatrix;"
    "uniform mat4 u_projMatrix;"
    "void main() {"
    "    v_position = vec4(a_position, 1.0);"
    "    gl_Position = u_projMatrix * u_viewMatrix * v_position;"
    "}";

const char *ShaderCode::aabb_debug_fs =
    "#version 330 core\n"
    "layout(location = 0) out vec4 output0;"
    "in vec4 v_position;"
    "void main() {"
    "    output0 = vec4(0.0, 1.0, 0.0, 1.0);"
    "}";

} // namespace hyperion
