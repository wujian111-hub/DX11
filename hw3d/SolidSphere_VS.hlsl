cbuffer CBuf : register(b0)
{
    matrix world;
    matrix view;
    matrix proj;
};

struct VSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    
    output.worldPos = mul(float4(input.pos, 1.0f), world).xyz;
    output.pos = mul(float4(output.worldPos, 1.0f), view);
    output.pos = mul(output.pos, proj);
    
    output.normal = mul(input.normal, (float3x3) world);
    output.normal = normalize(output.normal);
    
    return output;
}