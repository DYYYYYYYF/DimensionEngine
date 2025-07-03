
struct GSSceneIn
{
    float4 vPosition : SV_POSITION;
    int iMode : INT0;
    float2 vTexcoord : TEXCOORD0;
    float3 vNormal : NORMAL0;
    float4 vAlbientColor : COLOR0;
    float3 vViewPosition : VECTOR0;
    float3 vFragPosition : VECTOR1;
    float3 vVertPosition : VECTOR2;
    float4 vColor : COLOR1;
    float4 vTangent : POSITION0;
	
};

struct GSSceneOut
{
    float4 outPosition : SV_POSITION;
    int outMode : INT0;
    float2 outTexcoord : TEXCOORD0;
    float3 outNormal : NORMAL0;
    float4 outAmbientColor : COLOR0;
    float3 outViewPosition : VECTOR0;
    float3 outFragPosition : VECTOR1;
    float3 outVertPosition : VECTOR2;
    float4 outColor : COLOR1;
    float4 outTangent : POSITION0;
};

[maxvertexcount(3)]
void main(triangle GSSceneIn input[3], inout TriangleStream<GSSceneOut> OutputStream)
{
    GSSceneOut output = (GSSceneOut) 0;
    
    OutputStream.RestartStrip();
    for (uint i = 0; i < 3; i++)
    {
        output.outPosition = input[i].vPosition;
        output.outMode = input[i].iMode;
        output.outTexcoord = input[i].vTexcoord;
        output.outNormal = input[i].vNormal;
        output.outAmbientColor = input[i].vAlbientColor;
        output.outViewPosition = input[i].vViewPosition;
        output.outFragPosition = input[i].vFragPosition;
        output.outVertPosition = input[i].vVertPosition;
        output.outColor = input[i].vColor;
        output.outTangent = input[i].vTangent;
        
        OutputStream.Append(output);
    }
    
}