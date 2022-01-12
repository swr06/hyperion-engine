vec4 packDepth(const in float depth)
{
	vec4 bit_shift = vec4(256.0*256.0*256.0, 256.0*256.0, 256.0, 1.0);
  vec4 bit_mask = vec4(0.0, 1.0/256.0, 1.0/256.0, 1.0/256.0);
  vec4 res = fract(depth * bit_shift);
  res -= res.xxyz * bit_mask;
  return res;
}

float unpackDepth(const in vec4 rgba_depth) {
    const vec4 bit_shift =
    vec4(1.0/(256.0*256.0*256.0)
    , 1.0/(256.0*256.0)
    , 1.0/256.0
    , 1.0);
    float depth = dot(rgba_depth, bit_shift);
    return rgba_depth.r;
    return depth;
}