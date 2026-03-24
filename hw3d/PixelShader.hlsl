cbuffer CBuf : register(b1)
{
    float4 face_color[6];
};

float4 main(uint tid : SV_PrimitiveID) : SV_TARGET
{
    uint faceIndex = tid / 2;
    return face_color[faceIndex];
}