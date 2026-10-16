// magic moment character shader: specular colour and power from textures (Specular_Color_Texture, Specular_Power_Texture),
// sphere map ADDED to the colour. Game shader D42086C024107981. Lighting: see magic_moment.hlsli.

#include "magic_moment.hlsli"

// constants and samplers use the game's names and registers: the mod checks them against the game's shader (see shader_patcher.h).
float4x4 mViewToWorld : register(c0);   // row 3 = camera position
float4 FogColor : register(c4);
float4 FogRange : register(c5);
float4 IrisParams : register(c6);
float4 ObjectHighLight : register(c7);
float Diffuse_Texture1_Opacity : register(c8);
float Normal_Map_DXT5_NM : register(c9);  // a bool in the game's source, a float constant in the bytecode
float Normal_Map_HeightScale : register(c10);
float Specular_Power_Boost : register(c11);
float4 SphereMap_Color : register(c12);
float SphereMap_Intensity : register(c13);
float AlphaBoost : register(c14);

sampler2D Diffuse_Texture1_Texture : register(s0);
sampler2D Normal_Map_Texture : register(s1);
sampler2D Specular_Color_Texture : register(s2);
sampler2D Specular_Power_Texture : register(s3);
sampler2D SphereMap_Texture : register(s4);
sampler2D SphereMapFilter_Texture : register(s5);

struct PsIn
{
    float3 normal : TEXCOORD0;
    float3 tangentY : TEXCOORD1;     // scaled by the normal map's y
    float3 tangentX : TEXCOORD2;     // scaled by the normal map's x
    float3 worldPos : TEXCOORD3;
    float2 uvDiffuse : TEXCOORD4;
    float2 uvFilter : TEXCOORD5;     // sphere map filter
    float4 uvNormalSpec : TEXCOORD6; // xy = normal map, zw = specular colour texture
    float4 uvPowerSphere : TEXCOORD7; // xy = specular power texture, zw = sphere map
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
    float power = clamp(tex2D(Specular_Power_Texture, i.uvPowerSphere.xy).x * Specular_Power_Boost * 255, 0, 2000);
    float3 specular = MagicMomentSpecular(N, V, L, power) * tex2D(Specular_Color_Texture, i.uvNormalSpec.zw).rgb;

    float4 albedo = tex2D(Diffuse_Texture1_Texture, i.uvDiffuse);
    albedo.w *= Diffuse_Texture1_Opacity;
    albedo *= i.color;
    float lit = MagicMomentTerminator(i.normal, L);
    float3 color = albedo.rgb * MagicMomentDiffuse(N, L, lit) + specular * lit;

    // original: sphere map reflection, masked by the filter texture
    float3 sphere = tex2D(SphereMap_Texture, i.uvPowerSphere.zw).rgb * SphereMap_Intensity * SphereMap_Color.rgb;
    color += sphere * tex2D(SphereMapFilter_Texture, i.uvFilter).x;

    color = lerp(color, ObjectHighLight.xyz, ObjectHighLight.w);
    color = MagicMomentFog(color, i.worldPos, camPos, FogColor, FogRange);
    return float4(color * IrisParams.x, albedo.w * AlphaBoost);
}
