// Shared code of the magic moment character shaders:
// - draws Skylander with "PIP scene" variants of the character material, one game shader per combination of specular and
//   sphere map handling (see the magic_moment_*.hlsl files).
// - mod lights Skylanders from below and in front instead (like the Wii U and Giants versions):
//  - the light direction is built from the view vector, so it does not depend on how the portal camera is turned
//  - diffuse = N.L * strength + ambient + a little fill from above, so the lit side has real contrast
//  - surfaces whose model normal faces away from the light get no diffuse / specular light (see MagicMomentTerminator)
// - world space is Z up (fog of these shaders uses the world z as height)
// - rest of the material (normal map, specular, sphere map, highlight, fog) is the game's own code, marked "original"

#include "common.hlsli"

// Tuning (compile-time constants, the shaders are compiled when the mod is built).
#ifndef SSA_LIGHT_FRONT
#define SSA_LIGHT_FRONT 0.5      // how far the light sits towards the viewer, 0 = straight from below
#endif
#ifndef SSA_LIGHT_BELOW
#define SSA_LIGHT_BELOW 1.0      // how far the light sits below the character
#endif
#ifndef SSA_DIFFUSE_STRENGTH
#define SSA_DIFFUSE_STRENGTH 1.2 // scales N.L (the game's is 1.0)
#endif
#ifndef SSA_AMBIENT
#define SSA_AMBIENT 0.4          // light every pixel gets (the game's is 0.5)
#endif
#ifndef SSA_TOP_FILL
#define SSA_TOP_FILL 0.25        // extra light on surfaces that face up, so the character is not lit like a ghost story
#endif
#ifndef SSA_TERMINATOR_SOFTNESS
#define SSA_TERMINATOR_SOFTNESS 0.25 // width of the shadow edge, as N.L of the model's own normal; 0 = hard edge
#endif

// original: world space normal from the normal map (DXT5nm keeps x in alpha), z rebuilt from x and y
float3 MagicMomentNormal(float4 normalMap, float dxt5nm, float heightScale, float3 normal, float3 tangentY, float3 tangentX)
{
    float3 packedNormal = normalMap.xyw * 2 - 1;
    float2 xy = dxt5nm <= 0 ? packedNormal.xy : packedNormal.zy;
    float z = sqrt(abs(dot(xy, xy) - 1));
    xy *= heightScale;
    return normalize(normalize(normal) * z + normalize(tangentY) * xy.y + normalize(tangentX) * xy.x);
}

// the mod's light (the game's fixed direction would be here): direction to the light, from the unit vector to the camera
float3 MagicMomentLightDirection(float3 toCamera)
{
    return normalize(toCamera * SSA_LIGHT_FRONT + float3(0, 0, -SSA_LIGHT_BELOW));
}

// Self shadow without a depth buffer: the normal map can turn a surface towards the light although the model itself faces away from it
// (bumps, scales, the underside of limbs), which lets light leak onto the side that is really in shadow. 0 where the interpolated model
// normal faces away from the light, 1 where it clearly faces it. Scales the diffuse and specular light, never the ambient.
float MagicMomentTerminator(float3 modelNormal, float3 L)
{
    return smoothstep(0, max(SSA_TERMINATOR_SOFTNESS, 1e-4), dot(normalize(modelNormal), L));
}

// the game's diffuse is saturate(N.L) + 0.5; lit = MagicMomentTerminator
float MagicMomentDiffuse(float3 N, float3 L, float lit)
{
    return saturate(dot(N, L)) * SSA_DIFFUSE_STRENGTH * lit + SSA_AMBIENT + saturate(N.z) * SSA_TOP_FILL;
}

// original: Blinn-Phong, normalised by the power (the highlight keeps its brightness when it gets narrower)
float MagicMomentSpecular(float3 N, float3 toCamera, float3 L, float power)
{
    float3 H = normalize(toCamera + L);
    return pow(saturate(dot(N, H)), power) * (power * 0.0333333351 + 1);
}

// original: distance fog that fades out with the height (world z) above fogRange.z
float3 MagicMomentFog(float3 color, float3 worldPos, float3 camPos, float4 fogColor, float4 fogRange)
{
    float fog = saturate((length(worldPos - camPos) - fogRange.x) * fogRange.y) * fogColor.w;
    float h = (worldPos.z - fogRange.z) / fogRange.w;
    float heightFade = 1 - h * h;
    fog = heightFade >= 0 ? fog * heightFade : 0;
    return lerp(color, fogColor.xyz, fog);
}
