#version 450
layout (location = 0) in vec3 vPosition;

layout (location = 0) out vec4 vColor;

void main(){
	vec4 vScreenPos = vec4(vPosition, 1.0f);
	//if (gl_VertexIndex == 0){
	//	vScreenPos = vec4(1.0f, -1.0f, 0.0f, 1.0f);
	//}
	//
	//if (gl_VertexIndex == 1){
	//	vScreenPos = vec4(1.0f, 1.0f, 0.0f, 1.0f);
	//} 
	//
	//if (gl_VertexIndex == 2){
	//	vScreenPos = vec4(-1.0f, 1.0f, 0.0f, 1.0f);
	//}

    gl_Position = vScreenPos;
}
