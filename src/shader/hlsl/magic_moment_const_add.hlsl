// magic moment character shader: specular from constants (Specular_Color, Specular_Power) and one specular map,
// sphere map ADDED to the colour. Game shader FAF245AA70A45977. Lighting: see magic_moment.hlsli.

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
float3 Specular_Color : register(c11);
float Specular_Power : register(c12);
float4 SphereMap_Color : register(c13);
float SphereMap_Intensity : register(c14);
float AlphaBoost : register(c15);

sampler2D Diffuse_Texture1_Texture : register(s0);
sampler2D Normal_Map_Texture : register(s1);
sampler2D Specular_Map_Texture : register(s2);
sampler2D SphereMap_Texture : register(s3);
sampler2D SphereMapFilter_Texture : register(s4);

struct PsIn
{
    float3 normal : TEXCOORD0;
    float3 tangentY : TEXCOORD1;     // scaled by the normal map's y
    float3 tangentX : TEXCOORD2;     // scaled by the normal map's x
    float3 worldPos : TEXCOORD3;
    float2 uvDiffuse : TEXCOORD4;
    float2 uvFilter : TEXCOORD5;     // sphere map filter
    float4 uvNormalSpec : TEXCOORD6; // xy = normal map, zw = specular map
    float4 uvSphere : TEXCOORD7;     // zw = sphere map
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

    float3 specular = MagicMomentSpecular(N, V, L, Specular_Power) * tex2D(Specular_Map_Texture, i.uvNormalSpec.zw).rgb * Specular_Color;

    float4 albedo = tex2D(Diffuse_Texture1_Texture, i.uvDiffuse);
    albedo.w *= Diffuse_Texture1_Opacity;
    albedo *= i.color;
    float lit = MagicMomentTerminator(i.normal, L);
    float3 color = albedo.rgb * MagicMomentDiffuse(N, L, lit) + specular * lit;

    // original: sphere map reflection, masked by the filter texture
    float3 sphere = tex2D(SphereMap_Texture, i.uvSphere.zw).rgb * SphereMap_Intensity * SphereMap_Color.rgb;
    color += sphere * tex2D(SphereMapFilter_Texture, i.uvFilter).x;

    color = lerp(color, ObjectHighLight.xyz, ObjectHighLight.w);
    color = MagicMomentFog(color, i.worldPos, camPos, FogColor, FogRange);
    return float4(color * IrisParams.x, albedo.w * AlphaBoost);
}
