#version 420

#include "../include/attributes.inc"

#define $SCALE 5.0

uniform mat4 u_modelMatrix;
uniform mat4 u_projMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 WorldToNdcMatrix;

out vec4 v_position;
out vec4 v_normal;
out vec2 v_texcoord0;

#if SKINNING
#include "../include/skinning.inc"
#endif

#if VCT_GEOMETRY_SHADER
out VSOutput
{
	vec4 position;
	vec3 normal;
	vec3 texcoord0;
} vs_out;
#endif

void main() 
{
#if SKINNING
	mat4 skinningMat = createSkinningMatrix();
    v_position = u_modelMatrix * skinningMat * vec4(a_position, 1.0);
    v_normal = transpose(inverse(u_modelMatrix * skinningMat)) * vec4(a_normal, 0.0);
#endif

#if !SKINNING
    v_position = u_modelMatrix * vec4(a_position, 1.0);
    v_normal = transpose(inverse(u_modelMatrix)) * vec4(a_normal, 0.0);
#endif

	v_texcoord0 = a_texcoord0;

#if VCT_GEOMETRY_SHADER
	vs_out.texcoord0 = vec3(a_texcoord0, 0.0);
	vs_out.normal = v_normal.xyz;
	vs_out.position = v_position;
	gl_Position = vs_out.position;
#endif
#if !VCT_GEOMETRY_SHADER
	gl_Position = u_projMatrix*u_viewMatrix*v_position;
#endif
}
