#if !VCT_ENABLED
#error "VCT_ENABLED must be set to true to include this header"
#endif

// TODO: make this configurable, more mapped to bounding box
#define $VCT_SCALE 0.1
#define $VCT_SPECULAR_MAX_LEN 1.0
#define $VCT_SPECULAR_CONE_SIZE 0.1
#define $VCT_DIFFUSE_MAX_LEN 0.1
#define $VCT_DIFFUSE_CONE_SIZE 0.5

uniform sampler3D VoxelMap[$VCT_NUM_MAPS];
uniform vec3 VoxelProbePosition;

vec4 voxelFetch(vec3 pos, vec3 dir, float lod)
{
	vec4 sampleX =
		dir.x < 0.0
		? textureLod(VoxelMap[0], pos, lod)
		: textureLod(VoxelMap[1], pos, lod);
	
	vec4 sampleY =
		dir.y < 0.0
		? textureLod(VoxelMap[2], pos, lod)
		: textureLod(VoxelMap[3], pos, lod);
	
	vec4 sampleZ =
		dir.z < 0.0
		? textureLod(VoxelMap[4], pos, lod)
		: textureLod(VoxelMap[5], pos, lod);
	
	vec3 sampleWeights = abs(dir);
	float invSampleMag = 1.0 / (sampleWeights.x + sampleWeights.y + sampleWeights.z + .0001);
	sampleWeights *= invSampleMag;
	
	vec4 filtered = 
		sampleX * sampleWeights.x
		+ sampleY * sampleWeights.y
		+ sampleZ * sampleWeights.z;

	return filtered;
}

vec4 voxelTraceCone(float minVoxelDiameter, vec3 origin, vec3 dir, float coneRatio, float maxDist)
{
	float minVoxelDiameterInv = 1.0/minVoxelDiameter;
	vec3 samplePos = origin;
	vec4 accum = vec4(0.0);
	float minDiameter = minVoxelDiameter;
	float startDist = minDiameter;
	float dist = startDist;
	vec4 fadeCol = vec4(1.0, 1.0, 1.0, 1.0)*0.2;
	while (dist <= maxDist && accum.w < 1.0)
	{
		float sampleDiameter = max(minDiameter, coneRatio * dist);
		float sampleLOD = log2(sampleDiameter * minVoxelDiameterInv);
		vec3 samplePos = origin + dir * dist;
		vec4 sampleValue = voxelFetch(samplePos, -dir, sampleLOD);
		sampleValue = mix(sampleValue,fadeCol, clamp(dist/maxDist, 0.0, 1.0));
		float sampleWeight = (1.0 - accum.w);
		accum += sampleValue * sampleWeight;
		dist += sampleDiameter;
	}
	return accum;
}

vec3 decodeVoxelPosition(vec3 voxel)
{
	float voxelImageSize = float($VCT_MAP_SIZE);
	float halfVoxelImageSize = voxelImageSize * 0.5;

	return (vec3((voxel - VoxelProbePosition) * $VCT_SCALE) + vec3(halfVoxelImageSize)) / vec3(voxelImageSize);
}

vec4 VCTDiffuse(vec3 voxel, vec3 position, vec3 normal, vec3 camera, vec3 tangent, vec3 bitangent)
{
	float voxelImageSize = float($VCT_MAP_SIZE);
	
	vec3 voxelPosition = decodeVoxelPosition(voxel);

	vec4 coneTraceRes = vec4(0.0);

	coneTraceRes += voxelTraceCone(1.0/voxelImageSize, voxelPosition, normalize(normal), $VCT_DIFFUSE_CONE_SIZE, $VCT_DIFFUSE_MAX_LEN);
	coneTraceRes += 0.707 * voxelTraceCone(1.0/voxelImageSize, voxelPosition, normalize(normal+tangent), $VCT_DIFFUSE_CONE_SIZE, $VCT_DIFFUSE_MAX_LEN);
	coneTraceRes += 0.707 * voxelTraceCone(1.0/voxelImageSize, voxelPosition, normalize(normal-tangent), $VCT_DIFFUSE_CONE_SIZE, $VCT_DIFFUSE_MAX_LEN);
	coneTraceRes += 0.707 * voxelTraceCone(1.0/voxelImageSize, voxelPosition, normalize(normal+bitangent), $VCT_DIFFUSE_CONE_SIZE, $VCT_DIFFUSE_MAX_LEN);
	coneTraceRes += 0.707 * voxelTraceCone(1.0/voxelImageSize, voxelPosition, normalize(normal-bitangent), $VCT_DIFFUSE_CONE_SIZE, $VCT_DIFFUSE_MAX_LEN);
	
	return coneTraceRes / 5.0;
}

vec4 VCTSpecular(vec3 voxel, vec3 position, vec3 normal, vec3 camera)
{
	float voxelImageSize = float($VCT_MAP_SIZE);

	vec3 viewVector = normalize(position - camera);
	vec3 reflectionDir = reflect(viewVector, normalize(normal));
	
	vec3 voxelPosition = decodeVoxelPosition(voxel);
	
	return voxelTraceCone(1.0 / voxelImageSize, voxelPosition, normalize(reflectionDir), $VCT_SPECULAR_CONE_SIZE, $VCT_SPECULAR_MAX_LEN);
}