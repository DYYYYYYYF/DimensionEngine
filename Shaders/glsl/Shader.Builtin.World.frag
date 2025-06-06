#version 450
#pragma clang diagnostic ignored "-Wmissing-prototypes"

layout (location = 0) out vec4 FragColor;

layout (set = 1, binding = 0) uniform LocalUniformObject{
	vec4 diffuse_color;
	float shininess;
	float metallic;
	float roughness;
	float ambient_occlusion;
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
	vec3(-5.5f, 10.0f, -5.5f),
	vec4(0.0f, 1.0f, 0.0f, 1.0f),
	1.0f,	// Constant
	0.35f,	// Linear
	0.44f	// Quadratic
);

PointLight point_light_1 = PointLight(
	vec3(5.5f, 10.0f, -5.5f),
	vec4(1.0f, 0.0f, 0.0f, 1.0f),
	1.0f,	// Constant
	0.35f,	// Linear
	0.44f	// Quadratic
);

// Samplers：diffuse, specular
const int SAMP_DIFFUSE = 0;
const int SAMP_SPECULAR = 1;
const int SAMP_NORMAL = 2;
const int SAMP_ROUGHNESS_METALLIC = 3;
layout (set = 1, binding = 1) uniform sampler2D Samplers[4];

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
vec4 PBR(PointLight light, vec3 norm, vec3 albedo, vec3 camPos, vec3 fragPos, float metallic, float roughness, float ao);

void main(){
	vec3 Normal = in_dto.vNormal;
	vec3 Tangent = in_dto.vTangent.xyz;
	Tangent = (Tangent - dot(Tangent, Normal) * Normal);
	vec3 Bitangent = cross(in_dto.vNormal, in_dto.vTangent.xyz) * in_dto.vTangent.w;
	TBN = mat3(Tangent, Bitangent, Normal);

	// Update the normal to use a sample from the normal map.
	vec3 LocalNormal = 2.0 * texture(Samplers[SAMP_NORMAL], in_dto.vTexcoord).rgb - 1.0f;
	// Normal = normalize(TBN * in_dto.vNormal);

	if (in_mode == 0){
		// Roughness / Metallic / AO
		vec3 RMA = texture(Samplers[SAMP_ROUGHNESS_METALLIC], in_dto.vTexcoord).rgb;
		RMA /= 255;
		FragColor = PBR(point_light_0, Normal, in_dto.vAmbientColor.xyz, in_dto.vViewPosition, in_dto.vFragPosition, RMA.r, RMA.g, RMA.b);
	}
	else if (in_mode == 1){
		FragColor = vec4(Normal.x, Normal.y, Normal.z, 1.0f);
	}
	else if (in_mode == 2){
		vec3 vViewDirection = normalize(in_dto.vViewPosition - in_dto.vFragPosition);
		FragColor = CalculateDirectionalLight(dir_light, Normal, vViewDirection);

		FragColor += CalculatePointLight(point_light_0, Normal, in_dto.vFragPosition, vViewDirection);
		FragColor += CalculatePointLight(point_light_1, Normal, in_dto.vFragPosition, vViewDirection);
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
	vec4 Diffuse = vec4(vec3( ObjectUbo.diffuse_color.xyz * fDiffuseFactor), DiffSamp.a);
	vec4 Specular = vec4(vec3(light.color * SpecularFactor), DiffSamp.a);

	if (in_mode == 1){
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


//////////////////////////////   PBR   ////////////////////////////////////////
// 常量
const float PI = 3.14159265359;
// Fresnel-Schlick 近似
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

// GGX 分布函数
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return nom / denom;
}

// Smith 几何遮蔽函数
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float nom = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec4 PBR(PointLight light, vec3 norm, vec3 albedo, vec3 camPos, vec3 fragPos, float metallic, float roughness, float ao){
	vec4 DiffSamp = texture(Samplers[SAMP_DIFFUSE], in_dto.vTexcoord);
	albedo = vec3(1.0f, 1.0f, 1.0f);

	// 视线方向和法线
    vec3 N = normalize(norm);
    vec3 V = normalize(camPos - fragPos);

	// 环境光的基本反射率
    vec3 F0 = vec3(0.04f); // 非金属表面的 F0 反射率
    F0 = mix(F0, albedo, metallic);

    // 计算光线方向和半程向量
	// -dir_light.direction
	// light.position - fragPos
    vec3 L = normalize(light.position - fragPos);
    vec3 H = normalize(V + L);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness); // 法线分布
    float G = GeometrySmith(N, V, L, roughness);  // 几何遮蔽
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0f), F0); // 菲涅耳

    vec3 nominator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f) + 0.001f;
    vec3 specular = nominator / denominator;

    // 漫反射
    vec3 kS = F;
    vec3 kD = vec3(1.0f) - kS;
    kD *= 1.0f - metallic;

    float NdotL = max(dot(N, L), 0.0f);
    vec3 Lo = (kD * DiffSamp.xyz / PI + specular) * in_dto.vAmbientColor.xyz * NdotL;

    // 环境光（近似处理）
    vec3 ambient = vec3(0.03f) * DiffSamp.xyz * ObjectUbo.ambient_occlusion;
    vec3 color = ambient + Lo;

    // HDR 映射 
	vec3 HDR_Map_Param = vec3(1.0f);		// 减小分母的值使得更多高亮区域不被压缩，从而让图像整体更亮。不过要适度调整，以免高光区域过曝，导致细节丢失。	通常为1.0f。
    color = color / (color + HDR_Map_Param);
	// gamma 校正
	float gamma_correct_param = 2.2f;
    color = pow(color, vec3(1.0f / gamma_correct_param));	// 通过减小 gamma 值，可以让图像显示器渲染出更亮的效果。不过 gamma 校正应保持在 1.8 到 2.2 之间，以便显示器正确显示细节。

    return vec4(color, 1.0f);
}