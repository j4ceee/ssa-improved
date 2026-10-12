// water pixel shader
// Vanilla code (marked "original"), plus
//  - screen-space reflections: the reflection ray is marched through the depth buffer and blended over the static cube map
//  - a fix for the refraction leaking above the water (a scene point above the water must not use the distorted lookup)
//  - the Fresnel term from the wave normal instead of the flat mesh normal
//  - caustics on the floor (animated Voronoi) and foam along the shoreline
//
// game's shader works in a z-up world, except the wave normal and the cube map lookup, which are y-up (swizzle .xzy converts).

#include "common.hlsli"

// ---------------------------------------------------------------------------------------------------------------------
// tuning
// ---------------------------------------------------------------------------------------------------------------------

// screen-space reflections
#define SSA_FIRST_STEP 0.02      // first ray step, as a fraction of the distance from the camera to the water pixel
#define SSA_STEP_GROWTH 1.25     // every step is this much longer than the one before (SSA_MARCH_STEPS steps in total)
#define SSA_HIT_THICKNESS 0.5    // how far behind the surface the refined ray point may be, as a fraction of the distance the ray
                                 // travelled (lower values reject more rays that only passed behind a nearer object's edge, but
                                 // also leave gaps). Relative to the travelled distance and not to the march steps: those scale
                                 // with the camera distance and made the reflection edges slide when the camera moved
#define SSA_EDGE_FADE 30.0       // reflections fade out towards the screen edge (higher = narrower fade)
#define SSA_MARCH_STEPS 16       // coarse steps along the ray
#define SSA_REFINE_STEPS 8       // bisection steps between the last step in front of the surface and the hit
#define SSA_HIT_SOFTNESS 0.6     // a hit is fully trusted up to (1 - this) * SSA_HIT_THICKNESS behind the surface and fades out to
                                 // nothing at SSA_HIT_THICKNESS (0 = hard cut)
#define SSA_RAY_FADE_START 0.6   // hits fade out over the end of the ray, starting at this fraction of its length (< 1)
#define SSA_START_JITTER 1.0     // per-pixel jitter of the step lattice, as a fraction of one step-growth period (0 = off)
#define SSA_FIX_REFRACTION_LEAK 1
#define SSA_FRESNEL_FROM_WAVE_NORMAL 1

// caustics
#define SSA_CAUSTICS 1
#define SSA_CAUSTIC_SCALE 0.6        // cell frequency (cell size = 1 / this, in world units; smaller = bigger cells)
#define SSA_CAUSTIC_STRENGTH 1.8     // light added to the floor, relative to the floor's own colour
#define SSA_CAUSTIC_SHARP 2.0        // sharpness of the bright lines (higher = thinner and dimmer)
#define SSA_CAUSTIC_EDGE 2.2         // line width: lines are the band where F2 - F1 < 1 / this (higher = thinner)
#define SSA_CAUSTIC_DEPTH 20.0       // caustics have faded out completely this far below the surface
#define SSA_CAUSTIC_MIN_SUN_Z 0.25   // lowest sun height used to project the pattern to the surface (low sun = long shifts)
#define SSA_CAUSTIC_SPEED 1.5        // how fast the cell points orbit (rad/s of the slower of their two axes)
#define SSA_CAUSTIC_WARP 0.45        // how strongly the coordinates are warped, in cells (0 = plain Voronoi)
#define SSA_CAUSTIC_WARP_SPEED 0.8   // how fast the warp evolves (rad/s)
#define SSA_CAUSTIC_CELL_MIN 0.3     // brightness of the dimmest cell border relative to the brightest (1 = all equal)
#define SSA_CAUSTIC_REGION_MIN 0.25  // brightness in the dimmest regions of the floor relative to the brightest (1 = all equal)

// shoreline foam
#define SSA_FOAM 1
#define SSA_FOAM_WIDTH 0.3     // foam only within this height of the water line, above or below (world units)
#define SSA_FOAM_REACH 1.5     // ... and only if the surface is this close to the water pixel in 3D
#define SSA_FOAM_SCALE 0.7     // frequency of the noise that breaks the foam up (smaller = bigger blobs)
#define SSA_FOAM_BREAKUP 0.6   // how strongly the noise breaks the foam up (0 = solid band)
#define SSA_FOAM_SHARP 6.0     // hardness of the foam edges
#define SSA_FOAM_STRENGTH 0.9  // 1 = pure white foam where it is fully covered

// ---------------------------------------------------------------------------------------------------------------------
// the game's samplers and constants: the names and registers of the game's shader (they are checked after compiling)
// ---------------------------------------------------------------------------------------------------------------------

sampler2D DepthMap : register(s0);
sampler2D Diffuse_Texture1_Texture : register(s1);
sampler2D Diffuse_Texture2_Texture : register(s2);
sampler2D ShadowMap0 : register(s3);
sampler2D ShadowMap1 : register(s4);
sampler2D Normal_Map_Texture : register(s5);
sampler2D Normal_MapB_Texture : register(s6);
sampler2D Normal_MapC_Texture : register(s7);
samplerCUBE ReflectionCubeMap : register(s8);
sampler2D RefractionMap : register(s9);   // the scene colour behind the water
sampler2D simNormalSampler : register(s10);

float4x4 mScreenToWorld : register(c0);   // screen position + depth -> world position
float4x4 mViewToWorld : register(c4);     // row 3 = camera position
float4x4 mShadowMap0 : register(c8);      // world -> shadow map 0 (uv, depth); orthographic, no divide
float4x4 mShadowMap1 : register(c12);
float4 OmniLightPositions[2] : register(c16);
float4 OmniLightColors[2] : register(c18);
float4 SpotLightPositions[2] : register(c20);
float4 SpotLightDirections[2] : register(c22);
float4 SpotLightParameters[2] : register(c24);
float4 SpotLightColors[2] : register(c26);
float3 LightDir : register(c28);          // towards the sun
float3 LightColor : register(c29);
float4 FogColor : register(c30);
float4 FogRange : register(c31);
float4 VposParams : register(c32);        // pixel position -> uv: xy = scale, zw = offset
float4 OmniLightParameters : register(c33);
float4 ShadowBlendFilterParams : register(c34); // x = cascade 0 blend scale, y = its filter radius (uv), z / w = cascade 1
float4 Diffuse_Color : register(c35);
float Diffuse_Texture1_Opacity : register(c36);
float Diffuse_Texture2_Opacity : register(c37);
float Normal_Map_DXT5_NM : register(c38);       // the game declares these as bool, but keeps them in float registers
float Normal_Map_HeightScale : register(c39);
float Normal_MapB_DXT5_NM : register(c40);
float Normal_MapB_HeightScale : register(c41);
float Normal_MapC_DXT5_NM : register(c42);
float Normal_MapC_HeightScale : register(c43);
float3 Specular_Color : register(c44);
float Specular_Power : register(c45);
float Distortion_Scale : register(c46);
float DistortionB_Scale : register(c47);
float DistortionC_Scale : register(c48);
float Ocean_Depth : register(c49);
float3 Ocean_Color : register(c50);
float Ocean_Depth_Density : register(c51);
float Ocean_Depth1 : register(c52);
float Ocean_Depth_Density1 : register(c53);
float Fresnel_Min : register(c54);
float Fresnel_Max : register(c55);

// the mod's own, set per draw by water_patch.h
float4x4 ssa_mWorldToScreen : register(c100);   // the inverse of mScreenToWorld
float4 ssa_Time : register(c113);               // x = seconds, for the caustics and foam. Their animations all use whole multiples
                                                // of it, so it can wrap after 1000 full turns of the fastest one without a visible jump

struct PsIn
{
    float3 normal : TEXCOORD0;
    float3 axisY : TEXCOORD1;       // tangent space axis the normal maps' y goes along
    float3 axisX : TEXCOORD2;       // ... and x
    float3 worldPos : TEXCOORD3;
    float4 uv12 : TEXCOORD4;        // xy = first diffuse texture, zw = second
    float4 simFoam : TEXCOORD5;     // x = weight of the second diffuse texture, zw = uv of the simulation normal
    float2 uvNormal : TEXCOORD6;
    float4 uvNormalBC : TEXCOORD7;  // xy = second normal map, zw = third
    float4 color : COLOR0;          // rgb = vertex tint, a = fade at the water's edge
    float3 light : COLOR1;          // vertex lighting
};

// ---------------------------------------------------------------------------------------------------------------------
// original: normal maps, lights
// ---------------------------------------------------------------------------------------------------------------------

// A normal map texel as a tangent space normal: x and y scaled by the height scale, z from the unscaled x and y. DXT5nm maps
// store x in alpha.
float3 UnpackNormal(float4 texel, float dxt5nm, float heightScale)
{
    float3 t = texel.xyw * 2 - 1;
    float2 xy = dxt5nm > 0 ? t.zy : t.xy;
    return float3(xy * heightScale, sqrt(abs(dot(xy, xy) - 1)));
}

float SmoothUnit(float t)
{
    return t * t * (3 - 2 * t);
}

// the game's range falloff for omni and spot lights: 0 at `outer`, 1 at `inner` (squared distances compared)
float RangeFalloff(float distSq, float inner, float outer)
{
    return SmoothUnit(saturate((distSq - outer * outer) / (inner * inner - outer * outer)));
}

// original: Blinn-Phong with a normalised specular lobe; L is the direction the light is taken to come from
void AddLight(float3 N, float3 V, float3 L, float atten, float3 color, float specNorm, inout float3 diffuse, inout float3 specular)
{
    diffuse += atten * saturate(dot(N, L)) * color;
    specular += atten * specNorm * pow(saturate(dot(N, normalize(V + L))), Specular_Power) * color;
}

// original: the sun's shadow, two cascades blended by the distance from their centres (4 taps on cascade 0, 1 on cascade 1)
static const float2 kShadowTaps[4] = {float2(-0.5, 0.1), float2(-0.1, -0.5), float2(0.1, 0.5), float2(0.5, -0.9)};

float SunShadow(float3 pos)
{
    float3 shadowPos0 = mul(float4(pos, 1), mShadowMap0).xyz;
    float3 shadowPos1 = mul(float4(pos, 1), mShadowMap1).xyz;

    float w0 = saturate(ShadowBlendFilterParams.x * (1 - length(shadowPos0.xy * 2 - 1)));
    float w1 = saturate(ShadowBlendFilterParams.z * (1 - length(shadowPos1.xy * 2 - 1)));
    float3 weights = float3(w0, min(1 - w0, w1), max(1 - w0 - w1, 0));

    float lit0 = 0;
    [unroll]
    for (int i = 0; i < 4; ++i)
        lit0 += (tex2Dlod(ShadowMap0, float4(shadowPos0.xy + kShadowTaps[i] * ShadowBlendFilterParams.y, 0, 0)).x >= shadowPos0.z) ? 1 : 0;
    lit0 *= 0.25;
    float lit1 = (tex2Dlod(ShadowMap1, float4(shadowPos1.xy, 0, 0)).x >= shadowPos1.z) ? 1 : 0;

    return dot(float3(lit0, lit1, 1), weights);
}

// original: how much a light with the wave normal shifts the refraction lookup (the game's "ripple" distortion)
float2 Distort(float3 V, float2 n)
{
    return float2(dot(V.xy, n), dot(float2(V.y, -V.x), n));
}

// ---------------------------------------------------------------------------------------------------------------------
// the mod: screen-space reflection
// ---------------------------------------------------------------------------------------------------------------------

struct RaySample
{
    float2 uv;      // screen position of the ray point
    float behind;   // how far the ray point is behind the surface the camera sees there (distance from the camera, < 0 = in front)
    float usable;   // 1 if that sample can be used: on screen, in front of the camera, not sky (depth 0)
};

RaySample EvalRay(float3 origin, float3 dir, float rayDistance, float3 camPos)
{
    float3 p = origin + dir * rayDistance;
    float4 projected = mul(float4(p, 1), ssa_mWorldToScreen);
    float2 ndc = projected.xy / projected.w;

    RaySample s;
    s.uv = ndc * float2(0.5, -0.5) + 0.5;
    float depth = tex2Dlod(DepthMap, float4(s.uv, 0, 0)).x;
    s.behind = length(p - camPos) - length(WorldPositionFromDepth(ndc, depth, mScreenToWorld) - camPos);
    s.usable = (all(s.uv >= 0) && all(s.uv <= 1) && projected.w >= 0 && depth > 0.00001) ? 1 : 0;
    return s;
}

// total length of the ray as a multiple of the distance from the camera to the water pixel
static const float kMaxRayLength = SSA_FIRST_STEP * SSA_STEP_GROWTH * (pow(SSA_STEP_GROWTH, SSA_MARCH_STEPS) - 1) / (SSA_STEP_GROWTH - 1);

// Marches the reflection ray through the depth buffer. Returns the screen uv of the hit (xy) and how far to trust it (z, 0 = no hit).
float3 TraceReflection(float2 vpos, float3 origin, float3 dir, float3 camPos)
{
    float camDist = length(origin - camPos);

    // per-pixel jitter of the whole step lattice (interleaved gradient noise). Without it every screen row shares one lattice that
    // scales with the camera distance, so its artefacts slide over the world when the camera moves.
    float stepLength = camDist * SSA_FIRST_STEP * (1 + InterleavedGradientNoise(vpos) * (SSA_STEP_GROWTH - 1) * SSA_START_JITTER);

    // 1. coarse march. A hit is where the ray goes from in front of a surface to behind it (no hit if it was behind it before).
    // How close it really is to the surface is checked after the refinement.
    float travelled = 0;
    float inFront = 1;      // the previous sample was in front of the surface
    float hit = 0;
    float2 hitUv = 0;
    float hitTravelled = 0, hitStep = 0, hitBehind = 0;
    [loop]
    for (int i = 0; i < SSA_MARCH_STEPS; ++i)
    {
        stepLength *= SSA_STEP_GROWTH;
        travelled += stepLength;

        RaySample s = EvalRay(origin, dir, travelled, camPos);
        float newHit = (s.behind >= 0 ? 1 : 0) * s.usable * inFront * (1 - hit);
        hit += newHit;
        hitUv += newHit * (s.uv - hitUv);
        hitTravelled += newHit * (travelled - hitTravelled);
        hitStep += newHit * (stepLength - hitStep);
        hitBehind += newHit * (s.behind - hitBehind);

        // a sample that cannot be used (sky, off screen, behind the camera) must not count as "behind the surface": that would block
        // every later hit on this ray, and where that happens depends on the step lattice
        inFront = max(s.behind >= 0 ? 0 : 1, 1 - s.usable);
        if (hit > 0) break; // the first hit is final
    }

    // 2. bisection between the last step in front of the surface and the hit. The hit uv and the distance behind the surface
    // follow the far end.
    float nearEnd = hitTravelled - hitStep;
    [loop]
    for (int j = 0; j < SSA_REFINE_STEPS; ++j)
    {
        if (hit <= 0) break;

        float mid = (hitTravelled + nearEnd) * 0.5;
        RaySample s = EvalRay(origin, dir, mid, camPos);
        float isBehind = (s.behind >= 0 ? 1 : 0) * s.usable;
        hitTravelled += isBehind * (mid - hitTravelled);
        hitBehind += isBehind * (s.behind - hitBehind);
        nearEnd += (1 - isBehind) * (mid - nearEnd);
        hitUv += isBehind * (s.uv - hitUv);
    }

    // 3. confidence of the hit: 1 if the refined point lies on the surface, falling to 0 at SSA_HIT_THICKNESS * the distance travelled
    // behind it (see SSA_HIT_SOFTNESS), times a fade over the last part of the ray
    float thickness = hitTravelled * SSA_HIT_THICKNESS;
    hit *= saturate((thickness - hitBehind) / max(thickness * SSA_HIT_SOFTNESS, 0.00001));

    float rayFadeSpan = kMaxRayLength * (1 - SSA_RAY_FADE_START);
    float rayLength = hitTravelled / max(camDist, 0.00001);
    hit *= saturate((kMaxRayLength - rayLength) / rayFadeSpan);

    return float3(hitUv, hit);
}

// ---------------------------------------------------------------------------------------------------------------------
// the mod: caustics and foam
// ---------------------------------------------------------------------------------------------------------------------

// How much light the caustics add to the floor, relative to the floor's own colour. toFloor = floor position - water pixel position,
// height = height of the floor over the water plane (<= 0: underwater).
float CausticLight(float3 worldPos, float3 toFloor, float height, float time)
{
    // where the sun ray that ends on the floor crossed the water surface: floor xy + sun.xy * depth / sun.z
    float2 p = worldPos.xy + toFloor.xy;
    p += LightDir.xy * (-height / max(LightDir.z, SSA_CAUSTIC_MIN_SUN_Z));
    p *= SSA_CAUSTIC_SCALE;

    // Warp the coordinates twice (the second time finer) so the cells flow and stretch instead of just moving in place.
    float2 warpPhase = time * float2(SSA_CAUSTIC_WARP_SPEED, 1.5 * SSA_CAUSTIC_WARP_SPEED);
    p += float2(sin(p.y * 1.7 + warpPhase.x), sin(p.x * 1.3 - warpPhase.y)) * SSA_CAUSTIC_WARP;
    p += float2(sin(p.y * 3.1 - warpPhase.y), sin(p.x * 2.7 + warpPhase.x)) * (0.5 * SSA_CAUSTIC_WARP);

    // Animated Voronoi over the 4 nearest cells: the pixel's own cell plus its neighbours on the side of the cell the pixel is closer
    // to (one random point per cell, orbiting its cell on a small Lissajous path). F2 - F1 (distance to the second nearest point minus
    // the nearest) is ~0 along the cell borders, which is where real caustics are bright.
    float2 f = frac(p);
    float2 cell = p - f;
    float2 side = float2(f.x >= 0.5 ? 1 : -1, f.y >= 0.5 ? 1 : -1);
    float2 orbitPhase = time * float2(SSA_CAUSTIC_SPEED, 2 * SSA_CAUSTIC_SPEED);
    float d1 = 4, d2 = 4;       // squared distances to the nearest and the second nearest point
    float b1 = 0.5, b2 = 0.5;   // brightness of their cells
    [unroll]
    for (int y = 0; y < 2; ++y)
    {
        [unroll]
        for (int x = 0; x < 2; ++x)
        {
            float2 offset = float2(x, y) * side;
            float2 id = cell + offset;

            // hash of the cell (no sine, Hoskins' hash22) -> random point h in 0..1, and a second random number: the brightness of
            // its borders
            float3 p3 = frac(id.xyx * float3(0.1031, 0.1030, 0.0973));
            p3 += dot(p3, p3.yzx + 33.33);
            float2 h = frac((p3.xx + p3.yz) * p3.zy);
            float brightness = frac(p3.z);

            // orbit: the phase comes from the hash, the two axes run at different speeds. The point is the cell centre (0.3..0.7) plus
            // 0.2 * orbit, so it stays in 0.1..0.9 of its cell (the 4 cell search relies on that).
            float2 phase = h * 6.2831853 + orbitPhase;
            float2 cellPoint = (h * 0.4 + 0.3) + float2(sin(phase.x), sin(phase.y)) * 0.2;
            float2 delta = cellPoint + offset - f;
            float distSq = dot(delta, delta);

            // remember the brightness of the two nearest cells
            float second = (d2 - distSq >= 0) ? brightness : b2;
            b2 = (d1 - distSq >= 0) ? b1 : second;
            b1 = (d1 - distSq >= 0) ? brightness : b1;

            // keep the two smallest distances
            d2 = min(d2, max(d1, distSq));
            d1 = min(d1, distSq);
        }
    }

    // bright where F2 - F1 is small
    float f1 = sqrt(max(d1, 0.00001));
    float f2 = sqrt(max(d2, 0.00001));
    float lines = pow(saturate(1 - SSA_CAUSTIC_EDGE * (f2 - f1)), SSA_CAUSTIC_SHARP);

    // every border gets its own brightness: the mean of the two cells it separates (symmetric, so it does not jump across the line)
    lines *= (b1 + b2) * (0.5 * (1 - SSA_CAUSTIC_CELL_MIN)) + SSA_CAUSTIC_CELL_MIN;

    // ... and a very slow, big pattern makes some regions of the floor bright and some dim
    float2 regionPhase = time * float2(0.15, 0.2);
    float region = sin(dot(p, float2(0.45, 0.2)) + regionPhase.x) + sin(dot(p, float2(-0.3, 0.5)) - regionPhase.y);
    lines *= saturate(region * 0.6 + 0.55) * (1 - SSA_CAUSTIC_REGION_MIN) + SSA_CAUSTIC_REGION_MIN;

    // only under water, fading with depth and with a low sun
    float underwater = (-height >= 0) ? 1 : 0;
    float depthFade = saturate(height / SSA_CAUSTIC_DEPTH + 1);
    float sunFade = saturate(LightDir.z * 4);
    return lines * underwater * depthFade * sunFade * SSA_CAUSTIC_STRENGTH;
}

// How much of the shore foam covers the pixel (0..1). Same meaning of toFloor and height as for the caustics.
float FoamCoverage(float3 worldPos, float3 toFloor, float height, float3 camPos, float time)
{
    // Only where the refracted lookup hit a real surface: where nothing is behind the water (open ocean, sky) the depth is 0, which
    // puts the "floor" at the camera. So the floor must be at least half as far from the camera as the water pixel.
    float3 floorFromCam = worldPos + toFloor - camPos;
    float3 waterFromCam = worldPos - camPos;
    float farEnough = (dot(floorFromCam, floorFromCam) - 0.25 * dot(waterFromCam, waterFromCam) >= 0) ? 1 : 0;

    // ... and close to the water pixel in 3D (a bank or rock far behind it must not put foam on open water)
    float closeBy = saturate(1 - dot(toFloor, toFloor) / (SSA_FOAM_REACH * SSA_FOAM_REACH));
    float mask = farEnough * closeBy;

    // organic noise: the coordinates are warped by two sines, then three waves in different directions are summed
    float2 q = worldPos.xy * SSA_FOAM_SCALE;
    q += float2(sin(q.y * 1.7 + time), sin(q.x * 1.3 - 2 * time)) * 0.6;
    float waves = sin(dot(q, float2(3.1, 1.7)) + 2 * time)
                + sin(dot(q, float2(-1.3, 2.9)) - time)
                + sin(dot(q, float2(-3.0, 0.4)) + 3 * time);
    float breakup = waves * (1.0 / 6.0) + 0.5;

    // coverage: 1 at the water line, 0 at SSA_FOAM_WIDTH above or below it (a surface above the water that does not touch it gets
    // none), squared so the foam hugs the line
    float shoreline = saturate(1 - abs(height) / SSA_FOAM_WIDTH);
    shoreline *= shoreline;
    return saturate((shoreline - breakup * SSA_FOAM_BREAKUP) * SSA_FOAM_SHARP) * SSA_FOAM_STRENGTH * mask;
}

// ---------------------------------------------------------------------------------------------------------------------
// the shader
// ---------------------------------------------------------------------------------------------------------------------

float4 main(PsIn i, float2 vpos : VPOS) : COLOR0
{
    float3 camPos = mViewToWorld[3].xyz;
    float2 screenUv = vpos * VposParams.xy + VposParams.zw;

    // original: the two diffuse textures
    float4 tex1 = tex2D(Diffuse_Texture1_Texture, i.uv12.xy);
    float tex1Alpha = tex1.a * Diffuse_Texture1_Opacity;
    float4 tex2 = tex2D(Diffuse_Texture2_Texture, i.uv12.zw);
    float tex2Alpha = tex2.a * Diffuse_Texture2_Opacity;

    // original: the wave normal. Three tangent space normal maps are summed and brought into world space, then added to the
    // simulation normal (y-up: the simulation texture stores x, height, z).
    float3 meshNormal = normalize(i.normal);
    float3 nA = UnpackNormal(tex2D(Normal_Map_Texture, i.uvNormal), Normal_Map_DXT5_NM, Normal_Map_HeightScale);
    float3 nB = UnpackNormal(tex2D(Normal_MapB_Texture, i.uvNormalBC.xy), Normal_MapB_DXT5_NM, Normal_MapB_HeightScale);
    float3 nC = UnpackNormal(tex2D(Normal_MapC_Texture, i.uvNormalBC.zw), Normal_MapC_DXT5_NM, Normal_MapC_HeightScale);
    float3 sum = nA + nB + nC;
    float3 perturbed = normalize(normalize(i.axisX) * sum.x + normalize(i.axisY) * sum.y + meshNormal * sum.z);
    float3 sim = tex2D(simNormalSampler, i.simFoam.zw).xzy * 2 - 1;
    float3 N = normalize(sim + perturbed.xzy).xzy;   // z-up

    // original: lighting. The sun with its shadow, then up to two omni and two spot lights.
    float3 V = normalize(camPos - i.worldPos);
    float specNorm = Specular_Power * 0.0333333351 + 1;
    float shadow = SunShadow(i.worldPos);
    float3 diffuse = saturate(dot(N, LightDir)) * LightColor * shadow;
    float3 specular = shadow * specNorm * pow(saturate(dot(N, normalize(V + LightDir))), Specular_Power) * LightColor;

    [branch]
    if (OmniLightColors[0].w > 0)
    {
        [unroll]
        for (int l = 0; l < 2; ++l)
        {
            float2 range = (l == 0) ? OmniLightParameters.xy : OmniLightParameters.zw;
            float3 toLight = OmniLightPositions[l].xyz - i.worldPos;
            float distSq = dot(toLight, toLight);
            AddLight(N, V, toLight * rsqrt(distSq), RangeFalloff(distSq, range.x, range.y), OmniLightColors[l].xyz, specNorm, diffuse, specular);
        }
    }

    [branch]
    if (SpotLightColors[0].w > 0)
    {
        [unroll]
        for (int s = 0; s < 2; ++s)
        {
            float4 params = SpotLightParameters[s];
            float3 toLight = SpotLightPositions[s].xyz - i.worldPos;
            float distSq = dot(toLight, toLight);
            float3 spotDir = SpotLightDirections[s].xyz;
            float cone = SmoothUnit(saturate((dot(toLight * rsqrt(distSq), spotDir) - params.y) / (params.x - params.y)));
            // the game takes the spot direction as the light direction for the shading
            AddLight(N, V, spotDir, RangeFalloff(distSq, params.z, params.w) * cone, SpotLightColors[s].xyz, specNorm, diffuse, specular);
        }
    }

    // original: reflection, the static cube map (y-up)...
    float3 R = reflect(-V, N);
    float3 reflection = texCUBE(ReflectionCubeMap, R.xzy).rgb;

    // ... the mod: blended with a screen-space hit where the ray finds one
    float3 trace = TraceReflection(vpos, i.worldPos, R, camPos);
    float3 hitColor = tex2Dlod(RefractionMap, float4(trace.xy, 0, 0)).rgb;
    float edgeFade = saturate(min(min(trace.x, trace.y), min(1 - trace.x, 1 - trace.y)) * SSA_EDGE_FADE);
    reflection = lerp(reflection, hitColor, edgeFade * trace.z);

    // original: refraction. The scene behind the water is looked up with the screen position shifted by the ripples of all the normals.
    float3 refractionPlain = tex2D(RefractionMap, screenUv).rgb;
    float2 refractionUv = screenUv
        + Distort(V, nA.xy) * Distortion_Scale
        + Distort(V, nB.xy) * DistortionB_Scale
        + Distort(V, nC.xy) * DistortionC_Scale
        + Distort(V, sim.xz) * 0.01;
    float3 refraction = tex2D(RefractionMap, refractionUv).rgb;

    // original: where that point is, relative to the water plane
    float depth = tex2D(DepthMap, refractionUv).x;
    float3 floorPos = WorldPositionFromDepth(refractionUv * float2(2, -2) + float2(-1, 1), depth, mScreenToWorld);
    float3 toFloor = floorPos - i.worldPos;
    float height = dot(toFloor, meshNormal);

#if SSA_FIX_REFRACTION_LEAK
    // the mod: a scene point above the water must not use the distorted lookup, it would show things that are not behind the water
    refraction = height <= 0 ? refraction : refractionPlain;
#endif
#if SSA_CAUSTICS
    // caustics are zero above the water line and below SSA_CAUSTIC_DEPTH -> skip whole pattern
    [branch]
    if (height <= 0 && height > -SSA_CAUSTIC_DEPTH)
        refraction += refraction * CausticLight(i.worldPos, toFloor, height, ssa_Time.x);
#endif
#if SSA_FOAM
    // foam is zero when the surface is SSA_FOAM_WIDTH or more away from water line, above or below
    float foam = 0;
    [branch]
    if (abs(height) < SSA_FOAM_WIDTH)
        foam = FoamCoverage(i.worldPos, toFloor, height, camPos, ssa_Time.x);
#endif

    // original: the deeper the water, the more the refraction turns into the ocean colour
    float shallow = saturate(-height / Ocean_Depth);
    float deep = saturate((-height - Ocean_Depth) / (Ocean_Depth1 - Ocean_Depth));
    float oceanAmount = shallow * Ocean_Depth_Density + deep * (Ocean_Depth_Density1 - Ocean_Depth_Density);
    refraction = lerp(refraction, i.color.rgb * Ocean_Color, oceanAmount);

    // original: the water's own colour (diffuse texture, lit) plus the specular
    float3 lighting = diffuse + i.light;
    float3 base = tex1.rgb * i.color.rgb * tex1Alpha * Diffuse_Color.rgb;
    float3 litColor = lighting * base + specular * Specular_Color;

    // original: Fresnel blends refraction and reflection. The mod takes the wave normal for it, the game the flat mesh normal.
#if SSA_FRESNEL_FROM_WAVE_NORMAL
    float NdotV = dot(N, V);
#else
    float NdotV = dot(meshNormal, V);
#endif
    float fresnel = max(Fresnel_Min, min(1 - abs(NdotV), Fresnel_Max));
    float3 color = litColor * 0.5 + lerp(refraction, reflection, fresnel);

    // original: the second diffuse texture, added with its own weight
    color += tex2.rgb * tex2Alpha * i.simFoam.x;

#if SSA_FOAM
    // the mod: shore foam, white where lit and grey in the shade
    color = lerp(color, saturate(lighting * 0.6 + 0.3), foam);
#endif

    // original: fade into the undistorted scene at the water's edge, then fog (distance and height)
    color = lerp(refractionPlain, color, i.color.a);

    float dist = length(i.worldPos - camPos);
    float heightScaled = (i.worldPos.z - FogRange.z) / FogRange.w;
    float heightFalloff = 1 - heightScaled * heightScaled;
    float fog = saturate((dist - FogRange.x) * FogRange.y) * FogColor.w * heightFalloff * i.color.a;
    fog = heightFalloff >= 0 ? fog : 0;
    return float4(lerp(color, FogColor.xyz, fog), 1);
}
