#version 450
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;
layout (location = 3) in vec2 vTexCoord;

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec3 outPosition;
layout (location = 2) out vec2 outTexCoord;
layout (location = 3) out vec3 outViewPos;

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
    
    float distance = 10000.0f;
    vec4 vScreenPos = vec4(vPosition * distance, 1.0f);
	vec2 vInTexCoord = vTexCoord;
    gl_Position = cameraBuf.proj * cameraBuf.view * PushConstants.model * vScreenPos;

	if (gl_VertexIndex == 0){
		vScreenPos = vec4(1.0f, 0.0f, -1.0f, 1.0f);
		vInTexCoord = vec2(1.0f, 1.0f);
	}
	
	if (gl_VertexIndex == 1){
		vScreenPos = vec4(1.0f, 0.0f, 1.0f, 1.0f);
		vInTexCoord = vec2(1.0f, 0.0f);
	} 
	
	if (gl_VertexIndex == 2){
		vScreenPos = vec4(-1.0f, 0.0f, 1.0f, 1.0f);
		vInTexCoord = vec2(0.0f, 0.0f);
	}
	
	if (gl_VertexIndex == 3){
		vScreenPos = vec4(-1.0f, 0.0f, -1.0f, 1.0f);
		vInTexCoord = vec2(0.0f, 1.0f);
	}

    //vec4 vScreenPos = vec4(vPosition, 1.0f);

	outViewPos = cameraBuf.viewPos;
    outColor = vColor;
    outPosition = gl_Position.xyz;
    outTexCoord = vInTexCoord * distance;
}
