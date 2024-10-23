#version 450
#pragma clang diagnostic ignored "-Wmissing-prototypes"

layout (location = 0) out vec4 FragColor;

layout (set = 1, binding = 0) uniform LocalUniformObject{
	vec4 diffuse_color;
	float shininess;
}ObjectUbo;

 struct DirectionalLight{
 	vec3 direction;
 	vec4 color;
 };

 struct PointLight{
 	vec3 position;
 	vec4 color;
 	float fconstant;
 	float linear;
 	float quadratic;
 };

DirectionalLight dir_light = DirectionalLight(
	vec3(-0.57735f, -0.57735f, -0.57735f),
	vec4(0.8f, 0.8f, 0.8f, 1.0f)
);

PointLight point_light_0 = PointLight(
	vec3(-5.5f, 0.0f, -5.5f),
	vec4(0.0f, 1.0f, 0.0f, 1.0f),
	1.0f,	// Constant
	0.35f,	// Linear
	0.44f	// Quadratic
);

PointLight point_light_1 = PointLight(
	vec3(5.5f, 0.0f, -5.5f),
	vec4(1.0f, 0.0f, 0.0f, 1.0f),
	1.0f,	// Constant
	0.35f,	// Linear
	0.44f	// Quadratic
);

// Samplers£ºdiffuse, specular
const int SAMP_DIFFUSE = 0;
const int SAMP_SPECULAR = 1;
const int SAMP_NORMAL = 2;
layout (set = 1, binding = 1) uniform sampler2D Samplers[3];

layout (location = 0) flat in int in_mode;

layout (location = 1) in struct dto{
	vec2 vTexcoord;
	vec3 vNormal;
	vec4 vAmbientColor;
	vec3 vViewPosition;
	vec3 vFragPosition;
	vec3 vVertPosition;
	vec4 vColor;
	vec4 vTangent;
}in_dto;

mat3 TBN;

vec4 CalculateDirectionalLight(DirectionalLight light, vec3 normal, vec3 view_direction);
vec4 CalculatePointLight(PointLight light, vec3 normal, vec3 frag_position, vec3 view_direction);

void main(){
	vec3 Normal = in_dto.vNormal;
	vec3 Tangent = in_dto.vTangent.xyz;
	Tangent = (Tangent - dot(Tangent, Normal) * Normal);
	vec3 Bitangent = cross(in_dto.vNormal, in_dto.vTangent.xyz) * in_dto.vTangent.w;
	TBN = mat3(Tangent, Bitangent, Normal);

	// Update the normal to use a sample from the normal map.
	vec3 LocalNormal = 2.0 * texture(Samplers[SAMP_NORMAL], in_dto.vTexcoord).rgb - 1.0f;
	// Normal = normalize(TBN * in_dto.vNormal);

	if (in_mode == 0 || in_mode == 1){
		vec3 vViewDirection = normalize(in_dto.vViewPosition - in_dto.vFragPosition);
		FragColor = CalculateDirectionalLight(dir_light, Normal, vViewDirection);

		FragColor += CalculatePointLight(point_light_0, Normal, in_dto.vFragPosition, vViewDirection);
		FragColor += CalculatePointLight(point_light_1, Normal, in_dto.vFragPosition, vViewDirection);
	}
	else if (in_mode == 2){
		FragColor = vec4(abs(Normal), 1.0f);
	} 
	else if (in_mode == 3){
		FragColor = vec4(in_dto.vVertPosition.z, in_dto.vVertPosition.z, in_dto.vVertPosition.z, 1.0f);
	}
	else {
		FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
}

vec4 CalculateDirectionalLight(DirectionalLight light, vec3 normal, vec3 view_direction){
	float fDiffuseFactor = max(dot(normal, -light.direction), 0.0f);

	vec3 HalfDirection = normalize(-view_direction - light.direction);
	float SpecularFactor = pow(max(dot(HalfDirection, normal), 0.0f), ObjectUbo.shininess);

	vec4 DiffSamp = texture(Samplers[SAMP_DIFFUSE], in_dto.vTexcoord);
	vec4 Ambient = vec4(vec3(in_dto.vAmbientColor * ObjectUbo.diffuse_color), DiffSamp.a);
	vec4 Diffuse = vec4(vec3(light.color * fDiffuseFactor), DiffSamp.a);
	vec4 Specular = vec4(vec3(light.color * SpecularFactor), DiffSamp.a);

	if (in_mode == 0){
		Ambient *= DiffSamp;
		Diffuse *= DiffSamp;
		Specular *= vec4(texture(Samplers[SAMP_SPECULAR], in_dto.vTexcoord).rgb, Diffuse.a);
	}

	return (Ambient + Diffuse + Specular);
}

vec4 CalculatePointLight(PointLight light, vec3 normal, vec3 frag_position, vec3 view_direction){
	vec3 LightDirection = normalize(light.position - frag_position);
	float Diff = max(dot(normal, LightDirection), 0.0f);

	vec3 ReflectDirection = reflect(-LightDirection, normal);
	float Spec = pow(max(dot(view_direction, ReflectDirection), 0.0f), ObjectUbo.shininess);

	// Calculate attenuation, or light falloff over distance.
	float Distance = length(light.position - frag_position);
	float Attenuation = 1.0f / (light.fconstant + light.linear * Distance + light.quadratic * (Distance * Distance));

	vec4 Ambient = in_dto.vAmbientColor;
	vec4 Diffuse = light.color * Diff;
	vec4 Specular = light.color * Spec;
	
	vec4 DiffSamp = texture(Samplers[SAMP_DIFFUSE], in_dto.vTexcoord);
	Diffuse *= DiffSamp;
	Ambient *= DiffSamp;
	Specular *= vec4(texture(Samplers[SAMP_SPECULAR], in_dto.vTexcoord).rgb, Diffuse.a);

	// fall off
	Diffuse *= Attenuation;
	Ambient *= Attenuation;
	Specular *= Attenuation;

	return (Ambient + Diffuse + Specular);
}
