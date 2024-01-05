#version 450
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;
layout (location = 3) in vec2 vTexCoord;

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec3 outPosition;
layout (location = 3) out vec2 texCoord;
layout (location = 4) out vec3 viewPos;

layout (set = 0, binding = 0) uniform CameraBuffer{
	mat4 view;
	mat4 proj;
	vec3 viewPos;
} cameraBuf;

layout( push_constant ) uniform constants
{
	vec4 data;
	mat4 model;
} PushConstants;

void main(){

	vec3 vPos = vPosition;
	float radius = PushConstants.data.y * 3.1415926 / 180;

	vPos = vPos + normalize(vNormal) * sin(radius);
    gl_Position = cameraBuf.proj * cameraBuf.view * PushConstants.model * vec4(vPos, 1.0f);

    outColor = vColor;
	outNormal = vNormal;
	outPosition = vPosition;
    texCoord = vTexCoord;
	viewPos = cameraBuf.viewPos;
}
