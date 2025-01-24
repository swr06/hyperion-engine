#define $PI 3.141592654

uniform float u_shininess;
uniform float u_roughness;
uniform float RimShading;

uniform int HasDiffuseMap;
uniform vec4 u_diffuseColor;
uniform sampler2D DiffuseMap;

uniform int HasNormalMap;
uniform sampler2D NormalMap;

#if ROUGHNESS_MAPPING
uniform int HasRoughnessMap;
uniform sampler2D RoughnessMap;
#endif

#if METALNESS_MAPPING
uniform int HasMetalnessMap;
uniform sampler2D MetalnessMap;
#endif

uniform int HasAoMap;
uniform sampler2D AoMap;

uniform int HasSphericalHarmonicsMap;
uniform sampler2D SphericalHarmonicsMap;


// cubemap probe
struct Probe {
	vec3 position;
	vec3 min;
	vec3 max;
};

uniform Probe EnvProbe; // TODO: multiple

struct DirectionalLight
{
	vec3 direction;
	vec4 color;
	float intensity;
};

uniform DirectionalLight env_DirectionalLight;

struct PointLight
{
    vec3 position;
    vec4 color;
    float radius;
};

uniform PointLight env_PointLights[$MAX_POINT_LIGHTS];
uniform int env_NumPointLights;

uniform samplerCube env_GlobalCubemap;
uniform samplerCube env_GlobalIrradianceCubemap;

// brdf
uniform sampler2D BrdfMap;
uniform int HasBrdfMap;

#define $SIN_1 0.84147098
#define $COS_1 0.54030231
#define $SIN_2 0.90929743
#define $COS_2 -0.4161468

vec4 Irradiance(vec3 normal) {
	vec4 irradiance = vec4(0.0);  

	vec3 up    = vec3(0.0, 1.0, 0.0);
	vec3 right = normalize(cross(up, normal));
	up         = normalize(cross(normal, right));

	float sampleDelta = 0.15;
	float nrSamples = 0.0; 

	for(float phi = 0.0; phi < 2.0 * $PI; phi += sampleDelta)
	{
			for(float theta = 0.0; theta < 0.5 * $PI; theta += sampleDelta)
			{
					// spherical to cartesian (in tangent space)
					vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
					// tangent space to world
					vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal; 

					irradiance += textureLod(env_GlobalCubemap, sampleVec, 4) * cos(theta) * sin(theta);
					nrSamples++;
			}
	}
	return $PI * irradiance * (1.0 / float(nrSamples));
}

vec3 ReflectionVector(vec3 n, vec3 worldPos, vec3 camPos)
{
	  vec3 v = normalize(camPos - worldPos);

	  return reflect(-v, n);
}

float sqr(float x)
{
  return x * x;
}

const float PI = 3.14159265358979323846;


vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness*roughness;

    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space vector to world-space sample vector
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

float ChiGGXTerm(float v) {
    return v > 0 ? 1 : 0;
}

float ChiGGX(vec3 view, vec3 norm, vec3 halfV, float alpha) {
    float VoH2 = clamp(dot(view,halfV),0.0,1.0);
    float chi = ChiGGXTerm( VoH2 / clamp(dot(view,norm),0.0,1.0));
    VoH2 = VoH2 * VoH2;
    float tan2 = (1-VoH2) / VoH2;
    return (chi*2)/(1+sqrt(1+alpha*alpha*tan2));

}

vec3 FresnelTerm(vec3 specularColor, float vdoth)
{
	vec3 fresnel = specularColor + (vec3(1.0) - specularColor) * pow((1.0 - vdoth), 5.0);
	return fresnel;
}

vec4 EnvRemap(vec4 c)
{
	return pow(2.0 * c, vec4(2.2));
}


float GGX(float alpha, float cosThetaM)
{
  float CosSquared = cosThetaM * cosThetaM;
  float TanSquared = (1.0 - CosSquared) / CosSquared;
  return (1.0 / PI) * sqr(alpha / (CosSquared * (alpha * alpha + TanSquared)));
}


vec3 SchlickFresnel(vec3 f0, vec3 f90, float u)
{
  return f0 + (f90 - f0) * pow(1.0 - u, 5.0);
}

vec3 SchlickFresnelRoughness2(float cosTheta, vec3 F0, float roughness) {
  return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 SchlickFresnelRoughness(vec3 f0, float u)
{
  return f0 + (vec3(1.0) - f0) * pow(1.0 - u, 5.0);
}

float CookTorranceG(float NdotL, float NdotV, float LdotH, float NdotH)
{
  return min(1.0, 2.0 * (NdotH / LdotH) * min(NdotL, NdotV));
}

float NormalizedTrowbridgeReitz(float costhetaH, float w)
{
    float w2 = w*w;
    return w2 / (PI * sqr( costhetaH*costhetaH * (w2 - 1) + 1 ));
}


// Smith's Schlick-GGX with k = (a + 1)2 / 8
float GeometrySchlickGGX(float NdotV, float roughness) {
  float r = roughness + 1.0;
  float k = (r * r) / 8.0;

  float nom = NdotV;
  float denom = NdotV * (1.0 - k) + k;

  return nom / denom;
}

float GeometrySmith(float NdotV, float NdotL, float roughness) {
  float ggx1 = GeometrySchlickGGX(NdotV, roughness);
  float ggx2 = GeometrySchlickGGX(NdotL, roughness);

  return ggx1 * ggx2;
}

float smithG1_GGX(float Ndotv, float alphaG)
{
    return 2/(1 + sqrt(1 + alphaG*alphaG * (1-Ndotv*Ndotv)/(Ndotv*Ndotv)));
}

float smithG_GGX(float NdotL, float NdotV, float alphaG)
{
    return smithG1_GGX(NdotL, alphaG) * smithG1_GGX(NdotV, alphaG);
}

float schlickG1(float v, float m)
{
    float k = sqrt(2*m*m/PI);
    return v > 0 ? v/(v-k*v + k) : 0.0;
}

float schlickG(float NdotL, float NdotV, float m)
{
    return schlickG1(NdotL, m) * schlickG1(NdotV, m);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}


float cookTorranceG(float NdotL, float NdotV, float LdotH, float NdotH)
{
    return min(1, 2 * (NdotH / LdotH) * min(NdotL, NdotV));
}

// Smith GGX corrected Visibility
// NdotL        = the dot product of the normal and direction to the light
// NdotV        = the dot product of the normal and the camera view direction
// roughness    = the roughness of the pixel
float SmithGGXSchlickVisibility(float NdotL, float NdotV, float roughness)
{
    float alphaRoughnessSq = roughness * roughness;

    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}

vec2 BRDFMap(float NdotV, float metallic)
{
		const vec4 c0 = vec4(-1, -0.0275, -0.572, 0.022);
		const vec4 c1 = vec4(1, 0.0425, 1.04, -0.04);
		vec4 r = metallic * c0 + c1;
		float a004 = min(r.x * r.x, exp2(-9.28 * NdotV)) * r.x + r.y;
	
		return vec2(-1.04, 1.04) * a004 + r.zw;
}


vec4 CalculateFogExp(vec4 inColor, vec4 toColor, vec3 worldPos, vec3 camPos, float fogStart, float fogEnd)
{
	float dist = distance(worldPos, camPos);

	float density = 0.003;
	float fogFactor = 1.0 / exp(dist * density);
	vec4 color = mix(inColor, toColor, 1.0-fogFactor);
	color.a = inColor.a;
	return color;
}

vec4 CalculateFogLinear(vec4 inColor, vec4 toColor, vec3 worldPos, vec3 camPos, float fogStart, float fogEnd)
{
	float dist = distance(worldPos, camPos);

	float fogFactor = (fogEnd - dist)/(fogEnd - fogStart);
	fogFactor = clamp(fogFactor, 0.0, 1.0);
	return mix(inColor, toColor, 1.0-fogFactor);
}

vec3 SphericalHarmonics(const vec3 sph[9], const in vec3 normal) {
  float x = normal.x;
  float y = normal.y;
  float z = normal.z;

  vec3 result = (
    sph[0] +

    sph[1] * x +
    sph[2] * y +
    sph[3] * z +

    sph[4] * z * x +
    sph[5] * y * z +
    sph[6] * y * x +
    sph[7] * (3.0 * z * z - 1.0) +
    sph[8] * (x*x - y*y)
  );

  return max(result, vec3(0.0));
}

vec3 SampleSphericalHarmonics(vec3 n)
{
	vec3 sh[9];
    sh[0] = texelFetch(SphericalHarmonicsMap, ivec2(0, 0), 0).rgb;
    sh[1] = texelFetch(SphericalHarmonicsMap, ivec2(1, 0), 0).rgb;
    sh[2] = texelFetch(SphericalHarmonicsMap, ivec2(2, 0), 0).rgb;
    sh[3] = texelFetch(SphericalHarmonicsMap, ivec2(3, 0), 0).rgb;
    sh[4] = texelFetch(SphericalHarmonicsMap, ivec2(4, 0), 0).rgb;
    sh[5] = texelFetch(SphericalHarmonicsMap, ivec2(5, 0), 0).rgb;
    sh[6] = texelFetch(SphericalHarmonicsMap, ivec2(6, 0), 0).rgb;
    sh[7] = texelFetch(SphericalHarmonicsMap, ivec2(7, 0), 0).rgb;
    sh[8] = texelFetch(SphericalHarmonicsMap, ivec2(0, 1), 0).rgb;
	return SphericalHarmonics(sh, n);
}

vec4 SampleEnvProbeRefl(samplerCube tex, vec3 worldPos, vec3 refl, float lod)
{

  vec4 samp;

  vec3 rbmax = (EnvProbe.max - worldPos) / refl;
  vec3 rbmin = (EnvProbe.min - worldPos) / refl;

  vec3 rbminmax = max(rbmax, rbmin);
  

  float correction = min(min(rbminmax.x, rbminmax.y), rbminmax.z);
  vec3 boxIntersection = worldPos + refl * correction;

  vec3 coord = boxIntersection - EnvProbe.position;

  samp = textureLod(tex, coord, lod);

  return samp;

}

vec4 SampleEnvProbe(samplerCube tex, vec3 n, vec3 worldPos, vec3 camPos, float lod)
{
  return SampleEnvProbeRefl(tex, worldPos, ReflectionVector(n, worldPos, camPos), lod);
}

vec3 ComputeDirectionalLight(DirectionalLight dl, vec3 n, vec3 v, vec3 vert, vec3 albedo, float shadowness, float roughness, float metalness)
{
	vec3 lightDir = dl.direction;
	lightDir.y = abs(lightDir.y);
	lightDir = normalize(lightDir);


  float fresnel;
  fresnel = max(1.0 - dot(n, v), 0.0);
  fresnel = pow(fresnel, 2.0);

	vec3 F0 = vec3(0.04);
  F0 = mix(albedo, F0, metalness);

  float NdotL = max(min(dot(n, lightDir), 1.0), 0.0);
	float NdotV = max(min(dot(n, v), 1.0), 0.0);

	vec3 H = normalize(lightDir + v);
	float NdotH = dot(n, H);
	float LdotH = dot(lightDir, H);
  float HdotV = dot(H, v);

  float D = DistributionGGX(n, H, roughness);
	vec3 F = SchlickFresnelRoughness(F0, LdotH);
	float G = smithG_GGX(NdotL, NdotV, roughness);

  vec3 nominator = vec3(D * G * F);
  float denominator = (4 * NdotL * NdotV) + 0.001;
  vec3 brdf = nominator / vec3(denominator);
  vec3 kS = F;
  vec3 kD = vec3(1.0) - kS;
  kD *= vec3(1.0 - metalness);
  kD = max(min(kD, 1.0), 0.0);

	return (kD * albedo / vec3(PI) + brdf) * vec3(NdotL) * dl.color.rgb;
}


// vec3 ComputePointLight(PointLight pl, vec3 n, vec3 v, vec3 vert, vec3 albedo, float shadowness, float roughness, float metalness)
// {
// 	vec3 F0 = vec3(0.04);
// 	F0 = mix(albedo, F0, metalness);

// 	vec3 p_direction = normalize(pl.position - vert);

// 	float dist = distance(pl.position, vert);
// 	float attenuation = clamp(1.0 - (dist * dist)/(pl.radius * pl.radius), 0.0, 1.0);
// 	attenuation *= attenuation;
// 	vec3 radiance = pl.color.rgb * attenuation;

// 	float NdotL = dot(n, p_direction);
// 	float NdotV = dot(n, v);

// 	vec3 H = normalize(p_direction + v);
// 	float NdotH = dot(n, H);
// 	float LdotH = dot(p_direction, H);

// 	float D = DistributionGGX(n, H, roughness);
// 	vec3 F = SchlickFresnelRoughness(F0, LdotH);
// 	float G = smithG_GGX(NdotL, NdotV, roughness);

// 	vec3 nominator = vec3(D * G * F);
// 	float denominator = (4 * NdotL * NdotV) + 0.001;
// 	vec3 brdf = nominator / vec3(denominator);
// 	vec3 kS = F;
// 	vec3 kD = vec3(1.0) - kS;
// 	kD *= vec3(1.0 - metalness);

// 	return (kD * albedo / vec3(PI) + brdf) * radiance * vec3(NdotL);
// }


vec3 ComputePointLight(PointLight pl, vec3 n, vec3 cameraPosition, vec3 vert, vec3 albedo, float shadowness, float roughness, float metallic)
{
	vec3 viewVector = normalize(cameraPosition - vert);
	vec3 lightDir = normalize(pl.position - vert);

	float dist = distance(pl.position, vert);
	float attenuation = clamp(1.0 - (dist * dist)/max(pl.radius * pl.radius, 0.001), 0.0, 1.0);
	attenuation *= attenuation;
	//float attenuation = 20.0 / dot(local_light_pos - v_pos, local_light_pos - v_pos);
	vec3 radiance = pl.color.rgb * attenuation;

	float NdotL = max(0.001, dot(n, lightDir));
    float NdotV = max(0.001, dot(n, viewVector));
    vec3 H = normalize(lightDir + viewVector);
    float NdotH = max(0.001, dot(n, H));
    float LdotH = max(0.001, dot(lightDir, H));
    float VdotH = max(0.001, dot(viewVector, H));
    float HdotV = max(0.001, dot(H, viewVector));

    vec3 diffuseCubemap = texture(env_GlobalIrradianceCubemap, n).rgb;

//#if PROBE_ENABLED
    //vec3 specularCubemap = SampleEnvProbe(env_GlobalCubemap, n, vert, cameraPosition).rgb;
    //vec3 blurredSpecularCubemap = SampleEnvProbe(env_GlobalIrradianceCubemap, n, vert, cameraPosition).rgb;
//#endif

//#if !PROBE_ENABLED
    vec3 reflectionVector = ReflectionVector(n, vert, cameraPosition);
    vec3 specularCubemap = texture(env_GlobalCubemap, reflectionVector).rgb;
    vec3 blurredSpecularCubemap = texture(env_GlobalIrradianceCubemap, reflectionVector).rgb;
//#endif


    float roughnessMix = 1.0 - exp(-(roughness / 1.0 * log(100.0)));
    specularCubemap = mix(specularCubemap, blurredSpecularCubemap, roughnessMix);

    vec3 F0 = vec3(0.04);
    F0 = mix(vec3(1.0), F0, metallic);

    vec2 AB = BRDFMap(NdotV, metallic);

    vec3 metallicSpec = mix(vec3(0.04), albedo.rgb, metallic);
    vec3 metallicDiff = mix(albedo.rgb, vec3(0.0), metallic);

    vec3 F = FresnelTerm(metallicSpec, VdotH) * clamp(NdotL, 0.0, 1.0);
    float D = clamp(DistributionGGX(n, H, roughness), 0.0, 1.0);
    float G = clamp(SmithGGXSchlickVisibility(clamp(NdotL, 0.0, 1.0), clamp(NdotV, 0.0, 1.0), roughness), 0.0, 1.0);
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= vec3(1.0 - metallic);

    vec3 reflectedLight = vec3(0.0, 0.0, 0.0);
    vec3 diffuseLight = vec3(0.0, 0.0, 0.0);

    float rim = mix(1.0 - roughnessMix * 1.0 * 0.9, 1.0, NdotV);
    vec3 specRef = vec3((1.0 / max(rim, 0.0001)) * F * G * D) * NdotL;
    reflectedLight += specRef;

	return specRef * radiance;

    vec3 ibl = min(vec3(0.99), FresnelTerm(metallicSpec, NdotV) * AB.x + AB.y);
	
	

    reflectedLight += ibl * specularCubemap;

    vec3 diffRef = vec3((vec3(1.0) - F) * (1.0 / $PI) * NdotL);
    //diffRef += gi;
    diffuseLight += diffRef;
    diffuseLight += EnvRemap(Irradiance(n)).rgb * (1.0 / $PI);
    diffuseLight *= metallicDiff;

	return (diffuseLight + reflectedLight) * radiance;
}


#if VCT_ENABLED
#include "gi/vct.inc"
#endif