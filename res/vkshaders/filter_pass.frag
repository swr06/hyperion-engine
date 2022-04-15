#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec3 v_position;
layout(location=1) in vec3 v_normal;
layout(location=2) in vec2 v_texcoord0;

// TODO: read in from prev pass.

layout(location=0) out vec4 color_output;

layout(set = 1, binding = 0) uniform sampler2D tex;

layout(std140, set = 2, binding = 0, row_major) uniform SceneDataBlock {
    mat4 view;
    mat4 projection;
    vec4 camera_position;
    vec4 light_direction;
} scene;


void main()
{
    color_output = vec4(texture(tex, vec2(v_texcoord0.x, 1.0 - v_texcoord0.y)).rgb, 1.0);
}