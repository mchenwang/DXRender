struct ModelViewProjection
{
    matrix MVP;
    matrix ModelNegaTrans;
    matrix ModelMatrix;
};
ConstantBuffer<ModelViewProjection> MVPCB : register(b0);

struct PassData
{
    float4 lightPos;
    // float3 lightDir;
    float4 eyePos;
    float3 ambientColor;
    float lightIntensity;
    float3 lightColor;
    float spotPower;
};
ConstantBuffer<PassData> passCB : register(b1);

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 worldPos : POSITION;
    float3 normal : NORMAL;
    float3 textureColor : COLOR;
};

PSInput VSMain(VSInput input)
{
    PSInput o;
    o.position = mul(float4(input.position, 1.f), MVPCB.MVP);
    o.normal = normalize(mul(float4(input.normal, 0.f), MVPCB.ModelNegaTrans));
    o.worldPos = mul(float4(input.position, 1.f), MVPCB.ModelMatrix);
    // o.textureColor = o.worldPos*0.5f+0.5f;
    o.textureColor = float3(0.5f, 0.5f, 0.5f);
    return o;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 color = passCB.ambientColor;
    float3 viewDir = normalize(passCB.eyePos.xyz - input.worldPos.xyz);
    float r = length(passCB.lightPos.xyz - input.worldPos.xyz);
    float3 lightDir = normalize(passCB.lightPos.xyz - input.worldPos.xyz);
    float3 halfVec = normalize(lightDir + viewDir);
    color += input.textureColor * passCB.lightIntensity / (r*r) * max(0.f, dot(input.normal, lightDir));
    color += input.textureColor * passCB.lightIntensity / (r*r) * pow(max(0.f, dot(input.normal, halfVec)), passCB.spotPower);
    // return float4((input.normal*0.5+0.5), 1.f);
    return float4(color, 1.f);
}