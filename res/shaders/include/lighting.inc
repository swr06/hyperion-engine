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

struct DirectionalLight
{
	vec3 direction;
	vec4 color;
};

uniform DirectionalLight env_DirectionalLight;

struct PointLight
{
    vec3 position;
    vec4 color;
    float radius;
};

uniform PointLight env_PointLights[4]; // @TODO: add default shader properties so this can be passed
uniform int env_NumPointLights;

/* cubemapping */
uniform samplerCube env_GlobalCubemap;
uniform samplerCube env_GlobalIrradianceCubemap;

// brdf
uniform sampler2D BrdfMap;
uniform int HasBrdfMap;

#define $SIN_1 0.84147098
#define $COS_1 0.54030231
#define $SIN_2 0.90929743
#define $COS_2 -0.4161468

vec3 Irradiance(vec3 normal) {
	vec3 irradiance = vec3(0.0);  

	vec3 up    = vec3(0.0, 1.0, 0.0);
	vec3 right = normalize(cross(up, normal));
	up         = normalize(cross(normal, right));

	float sampleDelta = 0.8;
	float nrSamples = 0.0; 

	for(float phi = 0.0; phi < 2.0 * $PI; phi += sampleDelta)
	{
			for(float theta = 0.0; theta < 0.5 * $PI; theta += sampleDelta)
			{
					// spherical to cartesian (in tangent space)
					vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
					// tangent space to world
					vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal; 

					irradiance += texture(env_GlobalIrradianceCubemap, sampleVec).rgb * cos(theta) * sin(theta);
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
/* ========== */

float sqr(float x)
{
  return x * x;
}

/* lighting functions */

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
	vec3 fresnel = specularColor + (1. - specularColor) * pow((1. - vdoth), 5.);
	return fresnel;
}

vec3 EnvRemap(vec3 c)
{
	return pow(2.0 * c, vec3(2.2));
}


float GGX(float alpha, float cosThetaM)
{
  float CosSquared = cosThetaM * cosThetaM;
  float TanSquared = (1.0 - CosSquared) / CosSquared;
  return (1.0 / PI) * sqr(alpha / (CosSquared * (alpha * alpha + TanSquared)));
}


float SchlickFresnel(float f0, float f90, float u)
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

		/*float NdotH = max(dot(N, H), 0.0);
    float r1 = 1.0 / ( 4.0 * roughness * roughness * pow(NdotH, 4.0));
    float r2 = (NdotH * NdotH - 1.0) / (roughness * roughness * NdotH * NdotH);
    return r1 * exp(r2);*/
}


float cookTorranceG(float NdotL, float NdotV, float LdotH, float NdotH)
{
    return min(1, 2 * (NdotH / LdotH) * min(NdotL, NdotV));
}


/* ========== */


vec3 BRDF(vec3 n, vec3 v, vec3 lightDir, float roughness, float shininess, float metallic)
{
	float NdotL = dot(n, lightDir);
	float NdotV = dot(n, v);
  if (NdotL < 0 || NdotV < 0) return vec3(0);

	vec3 H = normalize(lightDir + v);
	float NdotH = dot(n, H);
	float LdotH = dot(lightDir, H);


	float D = NormalizedTrowbridgeReitz(NdotH, roughness);

	float F = SchlickFresnel(metallic, 1, LdotH);

	float G = smithG_GGX(NdotL, NdotV, roughness) / (4 * NdotL * NdotV);

	return vec3(shininess * D * F * G);
}

float SpecularDirectional(vec3 n, vec3 v, vec3 lightDir, float roughness)
{


  vec3 vHalfVector = normalize(lightDir + v);

	float NdotH = dot(n, vHalfVector);
	float LdotH = dot(lightDir, vHalfVector);

	float D = GGX(roughness, NdotH);
	float F = SchlickFresnel(0.2, 1.0, LdotH);

	return F * D;
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


vec3 ComputePointLight(PointLight pl, vec3 n, vec3 v, vec3 vert, vec3 albedo, float shadowness, float roughness, float metalness)
{
	vec3 F0 = vec3(0.04);
	F0 = mix(albedo, F0, metalness);

	vec3 p_direction = normalize(pl.position - vert);

	float dist = distance(pl.position, vert);
	float attenuation = clamp(1.0 - (dist * dist)/(pl.radius * pl.radius), 0.0, 1.0);
	attenuation *= attenuation;
	vec3 radiance = pl.color.rgb * attenuation;

	float NdotL = dot(n, p_direction);
	float NdotV = dot(n, v);

	vec3 H = normalize(p_direction + v);
	float NdotH = dot(n, H);
	float LdotH = dot(p_direction, H);

	float D = DistributionGGX(n, H, roughness);
	vec3 F = SchlickFresnelRoughness(F0, LdotH);
	float G = smithG_GGX(NdotL, NdotV, roughness);

	vec3 nominator = vec3(D * G * F);
	float denominator = (4 * NdotL * NdotV) + 0.001;
	vec3 brdf = nominator / vec3(denominator);
	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= vec3(1.0 - metalness);

	return (kD * albedo / vec3(PI) + brdf) * radiance * vec3(NdotL);
}