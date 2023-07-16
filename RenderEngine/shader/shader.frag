#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 baseLight;
layout (location = 3) in float lIntensity;
layout (location = 4) in vec3 lightPos;
layout (location = 5) in vec3 fragPos;
layout (location = 6) in vec3 normal;
layout (location = 7) in float specularIntensity;
layout (location = 8) in vec3 viewPos;

layout (location = 0) out vec4 FragColor;
layout (binding = 1) uniform sampler2D texSampler;

//初始化基础灯光
vec3 initBaseLight();
//卡通渲染
void cartoonRender();

void main(){
    //vec3 bLight = initBaseLight();
   
    //cartoonRender();
    //FragColor = texture(texSampler, inTexCoord) * vec4(bLight, 1.0);
    FragColor = texture(texSampler, inTexCoord); 
    }

vec3 initBaseLight(){
    //光照强度
    vec3 ambient = lIntensity * baseLight;
    //漫反射
    vec3 norm;
    if(gl_FrontFacing) norm = normalize(normal);
    else norm = normalize(-normal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * baseLight;
    //高光--镜面
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
    vec3 specular = specularIntensity * spec * baseLight;
    return ambient + diffuse + specular;
}

void cartoonRender(){
    float uMagTol = 0.2f;
    float uQuantize = 18.0f;
    ivec2 ires = textureSize(texSampler, 0);
    float uResS = float(ires.s);
    float uResT = float(ires.t);
    vec3 rgb = texture(texSampler, inTexCoord).rgb;
    vec2 stp0 = vec2(1.0/uResS, 0.0);
    vec2 st0p = vec2(0.0, 1.0/uResT);
    vec2 stpp = vec2(1.0/uResS, 1.0/uResT);
    vec2 stpm = vec2(1.0/uResS, -1.0/uResT);
    const vec3 W = vec3(0.2125, 0.7154, 0.0721);

    float imlml = dot(texture(texSampler, inTexCoord - stpp).rgb, W);
    float iplpl = dot(texture(texSampler, inTexCoord + stpp).rgb, W);
    float imlpl = dot(texture(texSampler, inTexCoord - stpm).rgb, W);
    float iplml = dot(texture(texSampler, inTexCoord + stpm).rgb, W);

    float im10 = dot(texture(texSampler, inTexCoord - stp0).rgb, W);
    float ip10 = dot(texture(texSampler, inTexCoord + stp0).rgb, W);
    float i0ml = dot(texture(texSampler, inTexCoord - st0p).rgb, W);
    float i0pl = dot(texture(texSampler, inTexCoord + st0p).rgb, W);

    float h = -1.0 * imlpl - 2.0 * i0pl - 1.0 * iplpl + 1.0 * imlml + 2.0 * i0ml + 1.0 * iplml;
    float v = -1.0 * imlml - 2.0 * im10 - 1.0 * imlpl + 1.0 * iplml + 2.0 * ip10 + 1.0 * iplpl;

    float mag = length(vec2(h,v));
    if(mag > uMagTol){
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        rgb.rgb *= uQuantize;
        rgb.rgb += vec3(0.5, 0.5, 0.5);
        ivec3 intrgb = ivec3(rgb.rgb);
        rgb.rgb = vec3(intrgb) / uQuantize;
        FragColor = vec4(rgb, 1.0);
    }
}

