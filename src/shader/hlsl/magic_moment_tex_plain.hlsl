// magic moment character shader: specular colour and power from textures (Specular_Color_Texture, Specular_Power_Texture),
// no sphere map, no object highlight. Game shader 28469059F169A843. Lighting: see magic_moment.hlsli.

#include "magic_moment.hlsli"

// constants and samplers use the game's names and registers: the mod checks them against the game's shader (see shader_patcher.h).
float4x4 mViewToWorld : register(c0);   // row 3 = camera position
float4 FogColor : register(c4);
float4 FogRange : register(c5);
float4 IrisParams : register(c6);
float Diffuse_Texture1_Opacity : register(c7);
float Normal_Map_DXT5_NM : register(c8);  // a bool in the game's source, a float constant in the bytecode
float Normal_Map_HeightScale : register(c9);
float Specular_Power_Boost : register(c10);
float AlphaBoost : register(c11);

sampler2D Diffuse_Texture1_Texture : register(s0);
sampler2D Normal_Map_Texture : register(s1);
sampler2D Specular_Color_Texture : register(s2);
sampler2D Specular_Power_Texture : register(s3);

struct PsIn
{
    float3 normal : TEXCOORD0;
    float3 tangentY : TEXCOORD1;     // scaled by the normal map's y
    float3 tangentX : TEXCOORD2;     // scaled by the normal map's x
    float3 worldPos : TEXCOORD3;
    float2 uvDiffuse : TEXCOORD4;
    float4 uvNormalSpec : TEXCOORD5; // xy = normal map, zw = specular colour texture
    float2 uvPower : TEXCOORD6;      // specular power texture
    float4 color : COLOR0;           // vertex colour
};

float4 main(PsIn i) : COLOR0
{
    float3 camPos = mViewToWorld[3].xyz;
    float3 toCam = camPos - i.worldPos;
    float3 V = toCam * rsqrt(dot(toCam, toCam));
    float3 L = MagicMomentLightDirection(V);
    float3 N = MagicMomentNormal(tex2D(Normal_Map_Texture, i.uvNormalSpec.xy), Normal_Map_DXT5_NM, Normal_Map_HeightScale,
                                 i.normal, i.tangentY, i.tangentX);

    // original: the power is a texture value (0 - 1) times the boost, in the 0 - 2000 range
    float power = clamp(tex2D(Specular_Power_Texture, i.uvPower).x * Specular_Power_Boost * 255, 0, 2000);
    float3 specular = MagicMomentSpecular(N, V, L, power) * tex2D(Specular_Color_Texture, i.uvNormalSpec.zw).rgb;

    float4 albedo = tex2D(Diffuse_Texture1_Texture, i.uvDiffuse);
    albedo.w *= Diffuse_Texture1_Opacity;
    albedo *= i.color;
    float lit = MagicMomentTerminator(i.normal, L);
    float3 color = albedo.rgb * MagicMomentDiffuse(N, L, lit) + specular * lit;

    color = MagicMomentFog(color, i.worldPos, camPos, FogColor, FogRange);
    return float4(color * IrisParams.x, albedo.w * AlphaBoost);
}
