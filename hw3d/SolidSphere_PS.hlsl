cbuffer CBuf : register(b0)
{
    float4x4 world;
    float4x4 view;
    float4x4 proj;
    float3 ambientLight;
    float pad1;
    float3 lightDir;
    float pad2;
    float3 lightColor;
    float pad3;
    float3 cameraPos;
    float pad4;
    float3 materialAmbient;
    float pad5;
    float3 materialDiffuse;
    float pad6;
    float3 materialSpecular;
    float pad7;
    float materialShininess;
    float pad8[3];
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
};

float4 main(PSInput input) : SV_TARGET
{
    float3 normal = normalize(input.normal);
    float3 lightDirNorm = normalize(-lightDir);
    
    // 漫反射
    float diff = max(dot(normal, lightDirNorm), 0.0f);
    float3 diffuse = lightColor * materialDiffuse * diff;
    
    // 环境光
    float3 ambient = ambientLight * materialAmbient;
    
    // 高光 (Blinn-Phong)
    float3 viewDir = normalize(cameraPos - input.worldPos);
    float3 halfVec = normalize(lightDirNorm + viewDir);
    float spec = pow(max(dot(normal, halfVec), 0.0f), materialShininess);
    float3 specular = lightColor * materialSpecular * spec;
    
    float3 finalColor = ambient + diffuse + specular;
    return float4(finalColor, 1.0f);
}