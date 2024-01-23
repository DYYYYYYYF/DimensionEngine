#version 450

layout (location = 0) in vec3 inFragColor;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vPosition;
layout (location = 3) in vec2 texCoord;
layout (location = 4) in vec3 viewPos;

layout (set = 1, binding = 0) uniform sampler2D tex1;

layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 1) uniform SceneData{
    vec4 fogColor; // w is for exponent
    vec4 fogDistances; //x for min, y for max, zw unused.
    vec4 ambientColor;
    vec4 ambientDirection; //w for sun power

    vec4 pointLightPos;
    vec4 pointLightCol;
    vec4 lightSpecular;
} sceneData;

void main(){
    float uMagTol = 0.1f;
    float uQuantize = 8.0f;

    ivec2 ires = textureSize(tex1, 0);
    float uResS = float(ires.x);
    float uResT = float(ires.y);

    vec3 rgb = texture(tex1, texCoord).rgb;
    vec2 stp0 = vec2(1.0 / uResS, 0.0);             // Left,Right
    vec2 st0p = vec2(0.0, 1.0 / uResT);             // Up,Down
    vec2 stpp = vec2(1.0 / uResS,  1.0 / uResT);    // Right Up, Left Down
    vec2 stpm = vec2(1.0 / uResS, -1.0 / uResT);    // Left Up, Right Down

    const vec3 W = vec3(0.2125, 0.7154, 0.0721);

    float grayLeft = dot( texture(tex1, texCoord - stp0).rgb, W);
    float grayRight = dot( texture(tex1, texCoord + stp0).rgb, W);
    float grayUp = dot( texture(tex1, texCoord + st0p).rgb, W);
    float grayDown = dot( texture(tex1, texCoord - st0p).rgb, W);

    float grayLeftUp = dot( texture(tex1, texCoord - stpm).rgb, W);
    float grayRightUp = dot( texture(tex1, texCoord + stpp).rgb, W);
    float grayLeftDown = dot( texture(tex1, texCoord - stpp).rgb, W);
    float grayRightDown = dot (texture(tex1, texCoord + stpm).rgb, W);

    mat3 sobel = mat3(
        -1, 0, 1,
        -2, 0, 2,
        -1, 0, 1 
    );

    float h = grayLeftDown + 2 * grayDown + grayRightDown - grayLeftUp - 2 * grayUp - grayRightUp;
    float v = grayRightUp + 2 * grayRight + grayRightDown - grayLeftUp - 2 * grayLeft - grayLeftDown;

    float mag = length(vec2(h, v));
    if (mag > uMagTol){
        outFragColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    } else {
        rgb *= uQuantize;
        ivec3 intrgb = ivec3(rgb);
        rgb = vec3(intrgb) / uQuantize;
        outFragColor = vec4(rgb, 1.0f);
    }
}
