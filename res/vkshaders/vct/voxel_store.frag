#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location=0) in vec3 v_position;
layout(location=1) in vec3 v_normal;
layout(location=2) in vec2 v_texcoord0;
layout(location=6) in vec3 v_light_direction;
layout(location=7) in vec3 v_camera_position;

layout(set = 1, binding = 17, rgba16f) uniform writeonly image3D voxel_map;

void main()
{
	float voxel_image_size = float(imageSize(voxel_map).x);
	float half_voxel_image_size = voxel_image_size * 0.5;
    float voxel_map_scale = 1.0 / voxel_image_size;
    
    vec3 pos = v_position * voxel_map_scale + vec3(half_voxel_image_size);
    
    imageStore(voxel_map, ivec3(0), vec4(1.0, 0.0, 0.0, 1.0));
}