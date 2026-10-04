// omni light pixel shader
// Vanilla code (marked "original"), plus
//  - screen-space shadows: a ray from the pixel towards the light is marched through the depth buffer
//  - a power curve for the falloff
// ssa_Light (set per draw by the mod): x = falloff power, y = falloff gain, z = shadows on (1) or off (0).

#include "common.hlsli"

// Tuning (compile-time constants, the shader is compiled when the mod is built).
#ifndef SSA_STEPS
#define SSA_STEPS 8             // steps along the way from the pixel to the light
#endif
#ifndef SSA_RAY_END
#define SSA_RAY_END 0.9         // the march stops at this fraction of the way to the light
#endif
#ifndef SSA_NORMAL_OFFSET
#define SSA_NORMAL_OFFSET 0.05  // the ray starts this far (world units) away from the surface, against self shadowing
#endif
#ifndef SSA_DEPTH_BIAS
#define SSA_DEPTH_BIAS 0.03     // a step is blocked if it is at least this far behind the surface the camera sees there ...
#endif
#ifndef SSA_DEPTH_BIAS_REL
#define SSA_DEPTH_BIAS_REL 0.002 // ... plus this fraction of its distance from the camera ...
#endif
#ifndef SSA_THICKNESS
#define SSA_THICKNESS 1.0       // ... and at most this far
#endif
#ifndef SSA_HITS_FOR_FULL
#define SSA_HITS_FOR_FULL 2.0   // this many blocked steps make the shadow as dark as SSA_STRENGTH allows
#endif
#ifndef SSA_STRENGTH
#define SSA_STRENGTH 0.85       // 1 = fully shadowed pixels get no light at all
#endif

sampler2D DepthMap : register(s0);
sampler2D GBuffer : register(s1);

float4x4 mScreenToWorld : register(c0);   // screen position + depth -> world position
float4x4 mViewToWorld : register(c4);     // row 3 = camera position
float4 VposParams : register(c8);         // pixel position -> uv: xy = scale, zw = offset
float4 OmniLightPosition : register(c9);
float4 OmniLightColor : register(c10);
float4 OmniLightParams : register(c11);   // z = 1 / radius^2

float4x4 ssa_mWorldToScreen : register(c100);
float4 ssa_Light : register(c112);

static const float kStep = SSA_RAY_END / SSA_STEPS;

// 1 if the point is hidden behind what the camera sees at its screen position (and not by more than the thickness)
float PointBlocked(float3 worldPos, float3 camPos)
{
    float4 projected = mul(float4(worldPos, 1), ssa_mWorldToScreen);
    float2 ndc = projected.xy / projected.w;
    float2 uv = ndc * float2(0.5, -0.5) + 0.5;

    float depth = tex2Dlod(DepthMap, float4(uv, 0, 0)).x;
    float sceneDist = length(WorldPositionFromDepth(ndc, depth, mScreenToWorld) - camPos);
    float pointDist = length(worldPos - camPos);
    float behind = pointDist - sceneDist;

    // usable: on screen, in front of the camera, not sky (depth 0)
    bool usable = all(uv >= 0) && all(uv <= 1) && projected.w >= 0 && depth > 0.00001;
    bool blocked = behind >= SSA_DEPTH_BIAS + pointDist * SSA_DEPTH_BIAS_REL && behind <= SSA_THICKNESS;
    return (usable && blocked) ? 1.0 : 0.0;
}

// how much of the light reaches the pixel: 1 = not shadowed
float LightShadow(float2 vpos, float3 pixelPos, float3 normal, float3 lightPos, float3 camPos)
{
    // the ray starts at the pixel, pushed away from the surface, and ends before the light itself
    float3 start = pixelPos + normal * SSA_NORMAL_OFFSET;
    float3 toLight = lightPos - start;

    // per-pixel jitter of the steps so they do not form visible bands
    float t = kStep * (InterleavedGradientNoise(vpos) * 0.8 + 0.2);

    float hits = 0;
    [loop]
    for (int i = 0; i < SSA_STEPS; ++i)
    {
        hits += PointBlocked(start + toLight * t, camPos);
        t += kStep;
        if (hits >= SSA_HITS_FOR_FULL) break; // fully shadowed already
    }
    return 1 - saturate(hits / SSA_HITS_FOR_FULL) * SSA_STRENGTH;
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
    float3 camPos = mViewToWorld[3].xyz;

    // original: attenuation, 1 - (distance / radius)^2 up to the radius; the mod shapes it with a power
    float3 toLight = OmniLightPosition.xyz - pos;
    float3 toCam = camPos - pos;
    float dist2 = dot(toLight, toLight);
    float falloff = 1 - saturate(dist2 * OmniLightParams.z);
    float atten = pow(falloff, ssa_Light.x) * ssa_Light.y;

    // original: normal (world space) and specular power from the G-buffer
    float3 N = gbuffer.xyz * 2 - 1;
    float specPower = gbuffer.w * 255 + 1;

    // the mod: screen-space shadow, only for pixels inside the light's radius
    float shadow = 1;
    [branch]
    if (ssa_Light.z > 0 && falloff > 0)
        shadow = LightShadow(vpos, pos, N, OmniLightPosition.xyz, camPos);

    // original: Blinn-Phong. rgb = diffuse (scaled by 0.5, the lighting buffer holds 0..2), a = luminance of the specular
    float3 lightColor = atten * shadow * OmniLightColor.xyz;
    float3 L = toLight * rsqrt(dist2);
    float ndl = saturate(dot(N, L));
    float3 V = toCam * rsqrt(dot(toCam, toCam));
    float3 H = normalize(V + L);
    float spec = pow(saturate(dot(N, H)), specPower) * (specPower * 0.0333333351 + 1);

    PsOut o;
    o.color.rgb = lightColor * ndl * 0.5;
    o.color.a = dot(lightColor * spec, float3(0.3, 0.6, 0.1)) * 0.25;
    o.unused = 0;
    return o;
}
