// Shared code of the mod's replacement shaders
// Only holds functions and tables; game's constants and samplers are declared by each shader itself, with the game's names
// and registers (the mod checks them against the game's constant table when it loads the compiled shader, see shader_patcher.h).

#pragma pack_matrix(row_major)

// interleaved gradient noise in [0, 1) from the pixel position (Jimenez 2014): a cheap per-pixel dither that is smooth in screen
// space, so sample patterns look like fine noise instead of bands.
float InterleavedGradientNoise(float2 vpos)
{
    return frac(52.9829189 * frac(dot(vpos, float2(0.06711056, 0.00583715))));
}

// world position of a pixel from its screen position (ndc, -1 - 1) and the value in the depth buffer. screenToWorld is the game's
// mScreenToWorld (constants c0 - c3 of the deferred lighting shaders).
float3 WorldPositionFromDepth(float2 ndc, float depth, float4x4 screenToWorld)
{
    float4 h = mul(float4(ndc, depth, 1), screenToWorld);
    return h.xyz / h.w;
}

// 8 points of a Vogel disc of radius 1 (a golden angle spiral): an even spread for a small number of samples.
static const float2 kVogelDisc8[8] = {
    float2(0.2500, 0.0000), float2(-0.3193, 0.2925), float2(0.0487, -0.5569), float2(0.4027, 0.5248),
    float2(-0.7386, -0.1302), float2(0.6994, -0.4455), float2(-0.2333, 0.8707), float2(-0.4470, -0.8588)
};
