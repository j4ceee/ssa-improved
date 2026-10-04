// sunlight pixel shader
// Vanilla code (marked "original"): the sun's shadow from two cascades of a shadow map (16 filter taps on cascade 0,
// 4 on cascade 1, blended by the distance from the cascade centre) and Blinn-Phong lighting.
// Plus: mod replaces the fixed filter radius of cascade 0 with contact-hardening (PCSS): sharp where a shadow touches
// its caster, softer the further it falls.
// The tuning arrives per draw:
//   ssa_PcssA = (min scale, max scale - min scale, depth difference -> softness, search scale)
//   ssa_PcssB.x = blocker bias (in shadow map depth)

#include "common.hlsli"

sampler2D DepthMap : register(s0);
sampler2D GBuffer : register(s1);
sampler2D ShadowMap0 : register(s2);
sampler2D ShadowMap1 : register(s3);

float4x4 mScreenToWorld : register(c0);   // screen position + depth -> world position
float4x4 mViewToWorld : register(c4);     // row 3 = camera position
float4x4 mShadowMap0 : register(c8);      // world -> shadow map 0 (uv, depth); orthographic, no divide
float4x4 mShadowMap1 : register(c12);
float3 LightDir : register(c16);          // towards the sun
float3 LightColor : register(c17);
float4 VposParams : register(c18);        // pixel position -> uv: xy = scale, zw = offset
float4 ShadowBlendFilterParams : register(c19); // x = cascade 0 blend scale, y = its filter radius (uv), z / w = cascade 1

float4 ssa_PcssA : register(c31);
float4 ssa_PcssB : register(c32);

// original: the 16 filter offsets, in units of the filter radius (cascade 1 uses the first four)
static const float2 kPcfTaps[16] = {
    float2(-1.1, -1.5), float2(-1.9, 1.5), float2(1.5, -1.9), float2(1.9, 1.5),
    float2(-1.5, -0.9), float2(-0.9, 1.5), float2(0.9, -1.5), float2(1.1, 0.5),
    float2(-0.5, 0.1), float2(-0.1, -0.5), float2(0.1, 0.5), float2(0.5, -0.9),
    float2(-0.5, -1.9), float2(0.5, 1.1), float2(1.5, -0.1), float2(-1.5, 0.1)
};

// how much of the sun reaches the pixel: 1 = not shadowed
float SunShadow(float2 vpos, float3 pos)
{
    float3 shadowPos0 = mul(float4(pos, 1), mShadowMap0).xyz;
    float3 shadowPos1 = mul(float4(pos, 1), mShadowMap1).xyz;

    // the mod: per-pixel rotation of the sample patterns (angle in [-pi, pi)). With a fixed pattern a wider filter would show 16
    // separate copies of the shadow edge.
    float angle = (InterleavedGradientNoise(vpos) - 0.5) * 6.28318531;
    float sinA, cosA;
    sincos(angle, sinA, cosA);
    float2 axisX = float2(cosA, sinA);
    float2 axisY = float2(-sinA, cosA);

    // the mod: blocker search. How far is the pixel behind the shadow map texels that are nearer to the sun?
    float searchRadius = ShadowBlendFilterParams.y * ssa_PcssA.w;
    float blockers = 0;
    float blockerDepth = 0;
    [unroll]
    for (int i = 0; i < 8; ++i)
    {
        float2 offset = (axisX * kVogelDisc8[i].x + axisY * kVogelDisc8[i].y) * searchRadius;
        float stored = tex2Dlod(ShadowMap0, float4(shadowPos0.xy + offset, 0, 0)).x;
        float isBlocker = (shadowPos0.z - stored - ssa_PcssB.x >= 0) ? 1 : 0;
        blockers += isBlocker;
        blockerDepth += stored * isBlocker;
    }

    // no blockers: the pixel is lit whatever the radius. Otherwise the distance to them picks the filter radius.
    float averageBlocker = blockerDepth / max(blockers, 1);
    float softness = saturate((shadowPos0.z - averageBlocker) * ssa_PcssA.z);
    float radius0 = ShadowBlendFilterParams.y * (softness * ssa_PcssA.y + ssa_PcssA.x);

    // cascade 0: the game's 16 taps (rotated, at the radius above); a tap is lit if the stored depth is not nearer than the pixel
    float lit0 = 0;
    [unroll]
    for (int j = 0; j < 16; ++j)
    {
        float2 offset = (axisX * kPcfTaps[j].x + axisY * kPcfTaps[j].y) * radius0;
        lit0 += (tex2Dlod(ShadowMap0, float4(shadowPos0.xy + offset, 0, 0)).x >= shadowPos0.z) ? 1 : 0;
    }
    lit0 /= 16;

    // original, cascade 1 (far away): 4 taps
    float lit1 = 0;
    [unroll]
    for (int k = 0; k < 4; ++k)
        lit1 += (tex2Dlod(ShadowMap1, float4(shadowPos1.xy + kPcfTaps[k] * ShadowBlendFilterParams.w, 0, 0)).x >= shadowPos1.z) ? 1 : 0;
    lit1 /= 4;

    // original: cascade 0 in its middle, cascade 1 in the ring around it, no shadow beyond
    float w0 = saturate(ShadowBlendFilterParams.x * (1 - length(shadowPos0.xy * 2 - 1)));
    float w1 = saturate(ShadowBlendFilterParams.z * (1 - length(shadowPos1.xy * 2 - 1)));
    return dot(float3(lit0, lit1, 1), float3(w0, min(1 - w0, w1), max(1 - w0 - w1, 0)));
}

struct PsOut
{
    float4 color : COLOR0;
    float4 unused : COLOR1;
};

PsOut main(float2 vpos : VPOS)
{
    // original: world position of the pixel from the depth buffer
    float2 uv = vpos * VposParams.xy + VposParams.zw;
    float2 ndc = uv * float2(2, -2) + float2(-1, 1);
    float depth = tex2D(DepthMap, uv).x;
    float4 gbuffer = tex2D(GBuffer, uv);
    float3 pos = WorldPositionFromDepth(ndc, depth, mScreenToWorld);
    float3 toCam = mViewToWorld[3].xyz - pos;

    float shadow = SunShadow(vpos, pos);

    // original: normal (world space) and specular power from the G-buffer, Blinn-Phong with the shadow applied to both parts.
    // rgb = diffuse (scaled by 0.5, the lighting buffer holds 0..2), a = luminance of the specular
    float3 N = gbuffer.xyz * 2 - 1;
    float specPower = gbuffer.w * 255 + 1;
    float ndl = saturate(dot(N, LightDir));
    float3 V = toCam * rsqrt(dot(toCam, toCam));
    float3 H = normalize(V + LightDir);
    float spec = pow(saturate(dot(N, H)), specPower) * (specPower * 0.0333333351 + 1) * shadow;

    PsOut o;
    o.color.rgb = ndl * LightColor * shadow * 0.5;
    o.color.a = dot(spec * LightColor, float3(0.3, 0.6, 0.1)) * 0.25;
    o.unused = 0;
    return o;
}
