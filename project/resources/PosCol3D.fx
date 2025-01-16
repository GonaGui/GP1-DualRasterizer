// Input/Output Structs

struct VS_INPUT
{
    float3 Position         : POSITION;
    float4 WorldPosition    : WORLD;
    float3 Normal           : NORMAL;
    float3 Tangent          : TANGENT;
    float2 UV               : TEXCOORD;
    float3 Color            : COLOR;
};

struct VS_OUTPUT
{
    float4 Position         : SV_POSITION;
    float4 WorldPosition    : WORLD;
    float3 Normal           : NORMAL;
    float3 Tangent          : TANGENT;
    float2 UV               : TEXCOORD;
    float3 Color            : COLOR;
};

SamplerState samPoint
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Wrap;
    AddressV = Wrap;
};

SamplerState samLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

SamplerState samAnisotropic
{
    Filter = ANISOTROPIC;
    AddressU = Wrap; 
    AddressV = Wrap;
    MaxAnisotropy = 16; 
};


BlendState gBlendState
{
};

DepthStencilState gDepthStencilState
{
};



// Variables

float4x4 gWorldViewProj : WorldViewProjection;
Texture2D gDiffuseMap   : DiffuseMap;
Texture2D gNormalMap    : NormalMap;
Texture2D gSpecularMap  : SpecularMap;
Texture2D gGlossinessMap: GlossinessMap;
float3 gLightDirection  : LightDirection = {0.577f, -0.577f, 0.577f};
float4x4 gWorldMatrix   : WorldMatrix;
float3 gCameraPosition  : CameraPosition;
float gPI               : PI = 3.14159265358979323846f;
float gLightIntensity   : LightIntesity = 7.0f;
float gShininess        : Shininess = 25.0f;


// Vertex Shader

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT) 0;
    output.Position = mul(float4(input.Position, 1.f), gWorldViewProj);
    output.WorldPosition = mul(float4(input.Position, 1.0f), gWorldMatrix);
    output.Normal = mul(normalize(input.Normal), (float3x3) gWorldMatrix);
    output.Tangent = mul(normalize(input.Tangent), (float3x3) gWorldMatrix);
    output.UV = input.UV;
    output.Color = input.Color;
    return output;
}

// Pixel Shader

float4 PS_Point(VS_OUTPUT input) : SV_TARGET
{
    float3 ambient = float3(0.025f, 0.025f, 0.025f);
    float3 viewDirection = normalize(gCameraPosition - input.WorldPosition.xyz);

    // Sample Maps
    float3 diffuseMapColor = gDiffuseMap.Sample(samPoint, input.UV);
    float3 normalMapColor = gNormalMap.Sample(samPoint, input.UV).xyz;
    float3 glossMapColor = gGlossinessMap.Sample(samPoint, input.UV).xyz;
    float3 specularMapColor = gSpecularMap.Sample(samPoint, input.UV).xyz;

    // Calculate Tangent Space
    float3 tangent = normalize(input.Tangent);
    float3 normal = normalize(input.Normal);
    float3 binormal = normalize(cross(normal, tangent));
    float4x4 tangentSpaceAxis = float4x4(input.Tangent.x, input.Tangent.y, input.Tangent.z, 0.0f,
                                          binormal.x, binormal.y, binormal.z, 0.0f,
                                          input.Normal.x, input.Normal.y, input.Normal.z, 0.0f,
                                          0.0f, 0.0f, 0.0f, 1.0f);

    // Adjust Normal Map to Tangent Space
    normalMapColor = 2.0f * normalMapColor - float3(1.0f, 1.0f, 1.0f);
    normalMapColor = normalize(mul(normalMapColor, (float3x3)tangentSpaceAxis));

    // Lighting Calculations
    float observedArea = max(dot(normalMapColor, -gLightDirection), 0.0f);
    float3 lightReflect = reflect(gLightDirection, normalMapColor);
    glossMapColor = gShininess * glossMapColor;
    float cosa = max(dot(lightReflect, viewDirection), 0.0f);
    float3 PhongSpecReflec = float3(1.0f, 1.0f, 1.0f) * specularMapColor.r * pow(cosa, glossMapColor.r);

    // Final Color
    float3 finalColor = ((gLightIntensity * diffuseMapColor) / 3.14159265f)
                        + PhongSpecReflec
                        + ambient;

    //return float4(viewDirection, 1.0f);
    return float4(finalColor * observedArea, 1.0f);
}

float4 PS_Linear(VS_OUTPUT input) : SV_TARGET
{
    float3 ambient = float3(0.03f, 0.03f, 0.03f);
    float3 viewDirection = normalize(gCameraPosition - input.WorldPosition.xyz);

    // Sample Maps
    float3 diffuseMapColor = gDiffuseMap.Sample(samLinear, input.UV);
    float3 normalMapColor = gNormalMap.Sample(samLinear, input.UV).xyz;
    float3 glossMapColor = gGlossinessMap.Sample(samLinear, input.UV).xyz;
    float3 specularMapColor = gSpecularMap.Sample(samLinear, input.UV).xyz;

    // Calculate Tangent Space
    float3 tangent = normalize(input.Tangent);
    float3 normal = normalize(input.Normal);
    float3 binormal = normalize(cross(normal, tangent));
    float4x4 tangentSpaceAxis = float4x4(input.Tangent.x, input.Tangent.y, input.Tangent.z, 0.0f,
                                          binormal.x, binormal.y, binormal.z, 0.0f,
                                          input.Normal.x, input.Normal.y, input.Normal.z, 0.0f,
                                          0.0f, 0.0f, 0.0f, 1.0f);

    // Adjust Normal Map to Tangent Space
    normalMapColor = 2.0f * normalMapColor - float3(1.0f, 1.0f, 1.0f);
    normalMapColor = normalize(mul(normalMapColor, (float3x3) tangentSpaceAxis));

    // Lighting Calculations
    float observedArea = max(dot(normalMapColor, -gLightDirection), 0.0f);
    float3 lightReflect = reflect(gLightDirection, normalMapColor);
    glossMapColor = gShininess * glossMapColor;
    float cosa = max(dot(lightReflect, viewDirection), 0.0f);
    float3 PhongSpecReflec = float3(1.0f, 1.0f, 1.0f) * specularMapColor.r * pow(cosa, glossMapColor.r);

    // Final Color
    float3 finalColor = ((gLightIntensity * diffuseMapColor) / 3.14159265f)
                        + PhongSpecReflec
                        + ambient;

    //return float4(viewDirection, 1.0f);
    return float4(finalColor * observedArea, 1.0f);
}

float4 PS_Anisotropic(VS_OUTPUT input) : SV_TARGET
{
    float3 ambient = float3(0.03f, 0.03f, 0.03f);
    float3 viewDirection = normalize(gCameraPosition - input.WorldPosition.xyz);

    // Sample Maps
    float3 diffuseMapColor = gDiffuseMap.Sample(samAnisotropic, input.UV);
    float3 normalMapColor = gNormalMap.Sample(samAnisotropic, input.UV).xyz;
    float3 glossMapColor = gGlossinessMap.Sample(samAnisotropic, input.UV).xyz;
    float3 specularMapColor = gSpecularMap.Sample(samAnisotropic, input.UV).xyz;

    // Calculate Tangent Space
    float3 tangent = normalize(input.Tangent);
    float3 normal = normalize(input.Normal);
    float3 binormal = normalize(cross(normal, tangent));
    float4x4 tangentSpaceAxis = float4x4(input.Tangent.x, input.Tangent.y, input.Tangent.z, 0.0f,
                                          binormal.x, binormal.y, binormal.z, 0.0f,
                                          input.Normal.x, input.Normal.y, input.Normal.z, 0.0f,
                                          0.0f, 0.0f, 0.0f, 1.0f);

    // Adjust Normal Map to Tangent Space
    normalMapColor = 2.0f * normalMapColor - float3(1.0f, 1.0f, 1.0f);
    normalMapColor = normalize(mul(normalMapColor, (float3x3) tangentSpaceAxis));

    // Lighting Calculations
    float observedArea = max(dot(normalMapColor, -gLightDirection), 0.0f);
    float3 lightReflect = reflect(gLightDirection, normalMapColor);
    glossMapColor = gShininess * glossMapColor;
    float cosa = max(dot(lightReflect, viewDirection), 0.0f);
    float3 PhongSpecReflec = float3(1.0f, 1.0f, 1.0f) * specularMapColor.r * pow(cosa, glossMapColor.r);

    // Final Color
    float3 finalColor = ((gLightIntensity * diffuseMapColor) / 3.14159265f)
                        + PhongSpecReflec
                        + ambient;

    //return float4(viewDirection, 1.0f);
    return float4(finalColor * observedArea, 1.0f);
}

// Technique

technique11 LinearTechnique
{
    pass P0
    {
       
        SetDepthStencilState(gDepthStencilState, 0);
        SetBlendState(gBlendState, float4(0.0f, 0.0f, 0.0f, 0.f), 0xFFFFFFFF);
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader(CompileShader(ps_5_0, PS_Linear()));
    }
}

technique11 PointTechnique
{
    pass P0
    {
       
        SetDepthStencilState(gDepthStencilState, 0);
        SetBlendState(gBlendState, float4(0.0f, 0.0f, 0.0f, 0.f), 0xFFFFFFFF);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS_Point()));
    }
}

technique11 AnisotropicTechnique
{
    pass P0
    {
       
        SetDepthStencilState(gDepthStencilState, 0);
        SetBlendState(gBlendState, float4(0.0f, 0.0f, 0.0f, 0.f), 0xFFFFFFFF);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS_Anisotropic()));
    }
}