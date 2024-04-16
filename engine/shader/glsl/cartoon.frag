#version 450

layout (location = 0) in vec3 inFragColor;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 position;
layout (location = 3) in vec2 texCoord;
layout (location = 4) in vec3 viewPos;

layout (set = 1, binding = 0) uniform sampler2D tex1;
layout (location = 0) out vec4 outFragColor;

void main(){
    float uMagTol = 0.1f;
    float uQuantize = 8.0f;

    ivec2 ires = textureSize(tex1, 0);
    float uResS = float(ires.x);
    float uResT = float(ires.y);

    vec3 rgb = texture(tex1, texCoord).rgb;

    vec2 pLeftRight = vec2(1.0 / uResS, 0.0);
    vec2 pUpDown = vec2(0.0, 1.0 / uResS);
    vec2 pRightUpLeftDown = vec2(1.0 / uResS, 1.0 / uResT);
    vec2 pLeftUpRightDown = vec2(1.0 / uResS, -1.0 / uResT);

    const vec3 W = vec3(0.2125, 0.7154, 0.0721);
    float grayLeft = dot( texture(tex1, texCoord - pLeftRight).rgb, W);
    float grayRight = dot( texture(tex1, texCoord + pLeftRight).rgb, W);
    float grayUp = dot( texture(tex1, texCoord + pUpDown).rgb, W);
    float grayDown = dot( texture(tex1, texCoord - pUpDown).rgb, W);

    float grayLeftUp = dot( texture(tex1, texCoord - pLeftUpRightDown).rgb, W);
    float grayRightUp = dot( texture(tex1, texCoord + pRightUpLeftDown).rgb, W);
    float grayLeftDown = dot( texture(tex1, texCoord - pRightUpLeftDown).rgb, W);
    float grayRightDown = dot( texture(tex1, texCoord + pLeftUpRightDown).rgb, W);

    mat3 Sobel = mat3(
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
