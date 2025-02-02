#version 430

in vec4 v_position;
in vec2 v_texcoord0;


#include "../include/matrices.inc"
#include "../include/frag_output.inc"

layout(binding = 0, rgba32f) uniform image3D framebufferImage;
uniform vec4 C_albedo;
uniform vec3 CameraPosition;

uniform sampler2D DiffuseMap;
uniform int HasDiffuseMap;

uniform vec3 VoxelProbePosition;

uniform vec3 VoxelSceneScale;

uniform float Emissiveness;


#if VCT_GEOMETRY_SHADER
// output
in GSOutput
{
	vec3 normal;
	vec3 texcoord0;
	vec3 offset;
	vec4 position;
} gs_out;
#endif

vec3 worldToTex(vec3 world) 
{
	vec4 ss = u_projMatrix * vec4(world, 1.0);
	ss /= ss.w;
	ss.z = 0.0;
	
	return ss.xyz;

}

vec3 scaleAndBias(vec3 p) { return 0.5f * p + vec3(0.5f); }

void main(void) 
{
	float voxelImageSize = float($VCT_MAP_SIZE);
	float halfVoxelImageSize = voxelImageSize * 0.5;

	
	vec4 imageColor = C_albedo;

	
	if (HasDiffuseMap == 1) {
#if VCT_GEOMETRY_SHADER
	  imageColor = texture(DiffuseMap, gs_out.texcoord0.xy);
#endif

#if !VCT_GEOMETRY_SHADER
	  imageColor = texture(DiffuseMap, v_texcoord0);
#endif
	}
	
	//imageColor = vec4(1.0, 0.0, 0.0, 1.0);

#if VCT_GEOMETRY_SHADER
	vec4 position = gs_out.position;
#endif
#if !VCT_GEOMETRY_SHADER
	vec4 position = v_position;
#endif

	position /= position.w;

	vec3 test_store_pos = (position.xyz - VoxelProbePosition) * $VCT_SCALE;
	imageStore(framebufferImage, ivec3(test_store_pos.x, test_store_pos.y, test_store_pos.z) + ivec3(halfVoxelImageSize), imageColor);
}
