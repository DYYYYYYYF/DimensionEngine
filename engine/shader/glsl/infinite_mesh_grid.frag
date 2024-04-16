#version 450

layout (location = 0) in vec3 inFragColor;
layout (location = 1) in vec3 inPosition;
layout (location = 2) in vec2 TexCoord;
layout (location = 3) in vec3 inViewPos;

layout (set = 1, binding = 0) uniform sampler2D tex1;

layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 1) uniform SceneData{
    vec4 fogColor; // w is for exponent
    vec4 fogDistances; //x for min, y for max, zw unused.
    vec4 ambientColor;
    vec4 sunlightDirection; //w for sun power
    vec4 sunlightColor; 
} sceneData;

float AlphaTest(float a, float b){
    if (a < b){
        return 1.0f;
    }

    return 0.0f;
}

void main(){

   vec2 uv = fract(TexCoord * 0.5f);
   vec2 derivative = fwidth(uv);

   // Rasterization
   uv.x = uv.x - int(uv.x);
   uv.y = uv.y - int(uv.y);

   uv = uv / derivative;   // Anti-aliasing

   float minVal = min(uv.x, uv.y);
   float alpha = 1 - AlphaTest(1.0f, minVal);
   float dis = 1 / (length(fwidth(inViewPos - inPosition) * 80));

   vec3 color = vec3(0.9f, 0.9f, 0.9f);
   color = color * alpha * dis;
   outFragColor = vec4(color, alpha);
   
}
