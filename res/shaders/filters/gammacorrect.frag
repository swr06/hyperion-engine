#version 330 core

#include "../include/frag_output.inc"
#include "../include/depth.inc"

in vec2 v_texcoord0;
uniform sampler2D ColorMap;
uniform sampler2D DepthMap;
uniform sampler2D PositionMap;
uniform sampler2D NormalMap;

#define $GAMMA 2.1

void main()
{
  vec4 tex = texture(ColorMap, v_texcoord0);
  output0 = vec4(vec3(pow(tex.rgb, vec3(1.0 / $GAMMA))), 1.0);
}
