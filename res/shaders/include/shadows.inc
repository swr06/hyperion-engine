#define $SHADOW_BIAS 0.0005
#define $SHADOW_BIAS_SMOOTHING 0
#define $SHADOW_VARIANCE_LOD 5

#if PSSM_ENABLED
uniform sampler2D u_shadowMap[$NUM_SPLITS];
uniform mat4 u_shadowMatrix[$NUM_SPLITS];
uniform float u_shadowSplit[$NUM_SPLITS];
#endif

#if !PSSM_ENABLED
uniform sampler2D u_shadowMap[1];
uniform mat4 u_shadowMatrix[1];
#endif

uniform vec2 poissonDisk[16];

#if PSSM_ENABLED
int getShadowMapSplit(float viewSpaceDepth) {
  // have to do this...
  
  if (viewSpaceDepth < u_shadowSplit[0]) {
    return 0;
  }
  
  if (viewSpaceDepth < u_shadowSplit[1]) {
    return 1;
  }
  
  if (viewSpaceDepth < u_shadowSplit[2]) {
    return 2;
  }
  
  if (viewSpaceDepth < u_shadowSplit[3]) {
    return 3;
  }

  return $NUM_SPLITS - 1;
}

vec3 shadowDebugColor(int shadowSplit) {
  vec3 shadowSplitColors[] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    vec3(0.0, 0.5, 0.5)
  );

  return shadowSplitColors[shadowSplit];
}

#endif

vec3 getShadowCoord(int index, vec3 pos) {
  vec4 shadowPos = u_shadowMatrix[index] * vec4(pos, 1.0);
  shadowPos *= vec4(0.5);
  shadowPos += vec4(0.5);
  shadowPos.xyz /= shadowPos.w;
  return shadowPos.xyz;
}

float getShadow(int index, vec3 coord, float NdotL) {
  vec4 textureSample = vec4(0.0);

#if PSSM_ENABLED
#if SHADOWS_VARIANCE
    if (index == 0) {
        textureSample = texture(u_shadowMap[0], coord.xy, $SHADOW_VARIANCE_LOD);
	} else if (index == 1) {
		textureSample = texture(u_shadowMap[1], coord.xy, $SHADOW_VARIANCE_LOD);
	} else if (index == 2) {
		textureSample = texture(u_shadowMap[2], coord.xy, $SHADOW_VARIANCE_LOD);
	} else {
		textureSample = texture(u_shadowMap[3], coord.xy, $SHADOW_VARIANCE_LOD);
	}
#endif

#if !SHADOWS_VARIANCE
    if (index == 0) {
        textureSample = texture(u_shadowMap[0], coord.xy);
    } else if (index == 1) {
        textureSample = texture(u_shadowMap[1], coord.xy);
    } else if (index == 2) {
        textureSample = texture(u_shadowMap[2], coord.xy);
    } else {
        textureSample = texture(u_shadowMap[3], coord.xy);
    }
#endif // SHADOWS_VARIANCE
#endif // PSSM_ENABLED

#if !PSSM_ENABLED
#if SHADOWS_VARIANCE
    textureSample = texture(u_shadowMap[0], coord.xy, $SHADOW_VARIANCE_LOD);
#endif

#if !SHADOWS_VARIANCE
    textureSample = texture(u_shadowMap[0], coord.xy);
#endif // SHADOWS_VARIANCE
#endif // !PSSM_ENABLED

#if SHADOWS_PACK_DEPTH
  float depth = unpackDepth(textureSample);
#endif
#if !SHADOWS_PACK_DEPTH
  float depth = textureSample.r;
#endif

#if SHADOWS_VARIANCE
  float moment2 = textureSample.g;

  vec2 moments = vec2(depth, moment2);

  float distance = coord.z;

  if (distance <= depth) {
    return 1.0;
  }

  float variance = moments.y - (moments.x * moments.x);
  variance = max(variance, 0.0000005);
  float d = coord.z - moments.x;
  float shadowPCT = variance / (variance + d*d);
  return shadowPCT;
#endif

  float bias = $SHADOW_BIAS;

#if SHADOW_BIAS_SMOOTHING
  bias *= tan(acos(NdotL));
  bias = clamp(bias, 0.0, 0.01);
#endif

#if !SHADOWS_VARIANCE
  return max(step(coord.z-bias, depth), 0.0);
#endif
}
