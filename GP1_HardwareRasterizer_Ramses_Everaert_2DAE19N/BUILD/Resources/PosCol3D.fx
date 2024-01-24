// --------------------------------------------------------
// Global Variables
// --------------------------------------------------------

// world matrices
float4x4 gWorldViewProj : WorldViewProjection;
float4x4 gWorldMatrix : WorldMatrix;

// textures
Texture2D gDiffuseMap : DiffuseMap;
Texture2D gNormalMap : NormalMap;
Texture2D gSpecularMap : SpecularMap;
Texture2D gGlossMap : GlossMap;

// lights
static const float3 gLightDirection = float3(0.577f, -0.577f, 0.577f);
static const float3 gAmbientColor = float3(0.03f, 0.03f, 0.03f);

// camera
float3 gCameraPosition : CameraPosition;

// light attributes
static const float gLightIntensity = 7.0f;
static const float gShininess = 25.0f;

// extra variables
static const float PI = 3.141592f;

// Sampler states
SamplerState gSamPoint
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Wrap;
    AddressV = Wrap;
};

SamplerState gSamLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

SamplerState gSamAnisotropic
{
    Filter = ANISOTROPIC;
    AddressU = Wrap;
    AddressV = Wrap;
};

// --------------------------------------------------------
// Input/Output Structs
// --------------------------------------------------------

struct VS_INPUT
{
    float3 Position    : POSITION;
    float2 TexCoord    : TEXCOORD;
    float3 Normal      : NORMAL;
    float3 Tangent     : TANGENT;
};

struct VS_OUTPUT
{
    float4 Position         : SV_POSITION;
    float3 WorldPosition    : TEXCOORD0;
    float2 TexCoord         : TEXCOORD1;
    float3 Normal           : NORMAL;
    float3 Tangent          : TANGENT;
};

// --------------------------------------------------------
// Vertex Shader
// --------------------------------------------------------

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    output.Position = mul(float4(input.Position, 1.0f), gWorldViewProj);
	output.WorldPosition = mul(float4(input.Position, 1.0f), gWorldMatrix).xyz;
    output.TexCoord = input.TexCoord;
    output.Normal = mul(float4(input.Normal, 0.0f), gWorldMatrix).xyz;
    output.Tangent = mul(float4(input.Tangent, 0.0f), gWorldMatrix).xyz;
    return output;
}


// --------------------------------------------------------
// Helper Functions
// --------------------------------------------------------

float4 SampleDiffuseMap(SamplerState state, float2 position)
{
    return gDiffuseMap.Sample(state, position);
}

float3 Lambert(float kd, float3 cd)
{
    return (cd * kd) / PI;
}

float3 Phong(float ks, float exp, float3 l, float3 v, float3 n)
{
    float3 reflection = reflect(-l, n);
    float cosAlpha = dot(reflection, v);
    if (cosAlpha < 0) return float3(0, 0, 0);
	
	float specularComponent = ks * pow(cosAlpha, exp);
	return float3(specularComponent, specularComponent, specularComponent);
}

float4 Shade(SamplerState state, VS_OUTPUT input)
{
	// Texture sampling
	float3 normalMap = gNormalMap.Sample(state, input.TexCoord).rgb;
	float3 diffuseColor = gDiffuseMap.Sample(state, input.TexCoord).rgb;
	float specularIntensity = gSpecularMap.Sample(state, input.TexCoord).r; // just need first value
	float gloss = gGlossMap.Sample(state, input.TexCoord).r; // just need first value
	
	// Normal Mapping
    normalMap = 2.0f * normalMap - 1.0f; // 
    float3 binormal = cross(input.Normal, input.Tangent);
    float3x3 tangentSpaceMatrix = float3x3(input.Tangent, binormal, input.Normal);
    float3 normal = mul(normalMap, tangentSpaceMatrix);
	
	// Light Calculations
	float3 nLightDirection = normalize(gLightDirection);
	float cosLaw = max(dot(-nLightDirection, normal), 0.0f);
	float3 viewDirection = normalize(gCameraPosition - input.WorldPosition.xyz);
	
	// lamberBRDF
	float3 lambertDiffuseReflection = Lambert(gLightIntensity, diffuseColor);
	
	// Phong
	float3 phong = Phong(specularIntensity, gloss * gShininess, gLightDirection, viewDirection, normal);
	
	
	// Colors
	float3 observedAreaColor = float3(cosLaw,cosLaw,cosLaw);
	float3 lambertDiffuseReflectionColor = lambertDiffuseReflection;
	float3 specularColor = phong;
	float3 CombinedColor = ((lambertDiffuseReflectionColor + phong) * observedAreaColor) + gAmbientColor;
	CombinedColor = saturate(CombinedColor);
	return float4(CombinedColor,1.0f); 
	
}



// --------------------------------------------------------
// Pixel Shader
// --------------------------------------------------------

// Point texturing
float4 PS_TexturePoint(VS_OUTPUT input) : SV_TARGET
{
   return Shade(gSamPoint, input);
}

// Linear Texturing
float4 PS_TextureLinear(VS_OUTPUT input) : SV_TARGET
{
    return Shade(gSamLinear, input);
}

// Anisotropic Texturing
float4 PS_TextureAnisotropic(VS_OUTPUT input) : SV_TARGET
{
    return Shade(gSamAnisotropic, input);
}

// --------------------------------------------------------
// Technique
// --------------------------------------------------------

technique11 PointTechnique
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS_TexturePoint()));
    }
}

technique11 LinearTechnique
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS_TextureLinear()));
    }
}

technique11 AnisotropicTechnique
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS_TextureAnisotropic()));
    }
}

