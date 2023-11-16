#version 450

layout (location = 0) in vec3 inFragColor;
layout (location = 1) in vec2 TexCoord;
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

   // Rasterization
   float x = TexCoord.x * 30 - int(TexCoord.x * 30);
   float y = TexCoord.y * 30 - int(TexCoord.y * 30);
   vec2 uv = vec2(x, y);
   uv = abs(uv - 0.5f);

   float minVal = min(uv.x, uv.y);
   float alpha = 1 - AlphaTest(0.01, minVal);

   vec3 color = vec3(1.0f, 1.0f, 1.0f);
   color = color * alpha;

   //

   outFragColor = vec4(color, alpha);
}
