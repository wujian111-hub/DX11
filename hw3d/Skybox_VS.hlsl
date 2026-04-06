cbuffer CBuf : register(b0)
{
    matrix view;
    matrix proj;
};

struct VSInput
{
    float4 pos : POSITION;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 texCoord : TEXCOORD0;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    float4 viewPos = mul(input.pos, view);
    output.pos = mul(viewPos, proj);
    output.texCoord = float3(input.pos.x, -input.pos.y, input.pos.z);
    return output;
}