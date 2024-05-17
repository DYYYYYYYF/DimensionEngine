#version 450

layout (location = 0) out vec4 FragColor;

layout (set = 1, binding = 0) uniform LocalUniformObject{
	vec4 diffuse_color;
	float shininess;
}ObjectUbo;

struct DirectionalLight{
	vec3 direction;
	vec4 color;
};

DirectionalLight dir_light = {
	vec3(-0.57735f, -0.57735f, -0.57735f),
	vec4(0.8f, 0.8f, 0.8f, 1.0f)
};

// Samplers��diffuse, specular
const int SAMP_DIFFUSE = 0;
const int SAMP_SPECULAR = 1;
const int SAMP_NORMAL = 2;
layout (set = 1, binding = 1) uniform sampler2D Samplers[3];

layout (location = 1) in struct dto{
	vec2 vTexcoord;
	vec3 vNormal;
	vec4 vAmbientColor;
	vec3 vViewPosition;
	vec3 vFragPosition;
	vec4 vColor;
	vec4 vTangent;
}in_dto;

mat3 TBN;

vec4 CalculateDirectionalLight(DirectionalLight light, vec3 normal, vec3 view_direction);

void main(){
	vec3 Normal = in_dto.vNormal;
	vec3 Tangent = in_dto.vTangent.xyz;
	Tangent = (Tangent - dot(Tangent, Normal) * Normal);
	vec3 Bitangent = cross(in_dto.vNormal, in_dto.vTangent.xyz) * in_dto.vTangent.w;
	TBN = mat3(Tangent, Bitangent, Normal);

	// Update the normal to use a sample from the normal map.
	vec3 LocalNormal = 2.0 * texture(Samplers[SAMP_NORMAL], in_dto.vTexcoord).rgb - 1.0f;
	Normal = normalize(TBN * LocalNormal);

	vec3 vViewDirection = normalize(in_dto.vViewPosition - in_dto.vFragPosition);
   FragColor = CalculateDirectionalLight(dir_light, Normal, vViewDirection);
}



vec4 CalculateDirectionalLight(DirectionalLight light, vec3 normal, vec3 view_direction){
	float fDiffuseFactor = max(dot(normal, -light.direction), 0.0f);

	vec3 HalfDirection = normalize(view_direction - light.direction);
	float SpecularFactor = pow(max(dot(HalfDirection, normal), 0.0f), ObjectUbo.shininess);

	vec4 DiffSamp = texture(Samplers[SAMP_DIFFUSE], in_dto.vTexcoord);
	vec4 Ambient = vec4(vec3(in_dto.vAmbientColor * ObjectUbo.diffuse_color), DiffSamp.a);
	vec4 Diffuse = vec4(vec3(light.color * fDiffuseFactor), DiffSamp.a);
	vec4 Specular = vec4(vec3(light.color * SpecularFactor), DiffSamp.a);

	Ambient *= DiffSamp;
	Diffuse *= DiffSamp;
	Specular *= vec4(texture(Samplers[SAMP_SPECULAR], in_dto.vTexcoord).rgb, DiffSamp.a);

	return (Ambient + Diffuse + Specular);
}