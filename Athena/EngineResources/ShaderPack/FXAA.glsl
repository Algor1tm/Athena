//////////////////////// Athena FXAA Shader ////////////////////////

// References:
//  https://github.com/bartwronski/CSharpRenderer/blob/master/shaders/FXAA.hlsl#L338
//  https://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf


#version 460 core
#pragma stage : compute

#include "Include/Buffers.glslh"

#define GROUP_SIZE         8
#define GROUP_THREAD_COUNT (GROUP_SIZE * GROUP_SIZE)
#define FILTER_SIZE        3
#define FILTER_RADIUS      (FILTER_SIZE / 2)
#define TILE_SIZE          (GROUP_SIZE + 2 * FILTER_RADIUS)
#define TILE_PIXEL_COUNT   (TILE_SIZE * TILE_SIZE)

layout(local_size_x = GROUP_SIZE, local_size_y = GROUP_SIZE) in;

layout(set = 1, binding = 4) uniform sampler2D u_SceneColor;
layout(rgba8, set = 1, binding = 5) uniform image2D u_PostProcessTex;


shared vec3 s_Pixels[TILE_PIXEL_COUNT];

void StorePixel(int idx, vec3 color)
{
    s_Pixels[idx] = color;
}

vec3 LoadPixel(int x, int y)
{
    uint center = (gl_LocalInvocationID.x + FILTER_RADIUS) + (gl_LocalInvocationID.y + FILTER_RADIUS) * TILE_SIZE;
    uint idx = center + x + y * TILE_SIZE;

    return s_Pixels[idx];
}

void LoadSharedData()
{
    ivec2 baseIndex = ivec2(gl_WorkGroupID) * GROUP_SIZE - FILTER_RADIUS;

    for (int i = int(gl_LocalInvocationIndex); i < TILE_PIXEL_COUNT; i += GROUP_THREAD_COUNT)
    {
        vec2 uv = (vec2(baseIndex) + 0.5) * u_Renderer.InverseViewportSize;
        vec2 uvOffset = vec2(i % TILE_SIZE, i / TILE_SIZE) * u_Renderer.InverseViewportSize;
        
        vec3 color = texture(u_SceneColor, uv + uvOffset).rgb;
        StorePixel(i, color);
    }

    memoryBarrierShared();
    barrier();
}

/*
=========================================================================================================================
++++++++++++++++++++++++++++++++++++++++++++++  MAIN FXAA ALGORITHM   ++++++++++++++++++++++++++++++++++++++++++++++++++++
=========================================================================================================================
*/

#define FXAA_PRESET 6

/*--------------------------------------------------------------------------*/
#if (FXAA_PRESET == 0)
    #define FXAA_EDGE_THRESHOLD      (1.0/4.0)
    #define FXAA_EDGE_THRESHOLD_MIN  (1.0/12.0)
    #define FXAA_SEARCH_STEPS        2
    #define FXAA_SEARCH_ACCELERATION 4
    #define FXAA_SEARCH_THRESHOLD    (1.0/4.0)
    #define FXAA_SUBPIX              1
    #define FXAA_SUBPIX_FASTER       1
    #define FXAA_SUBPIX_CAP          (2.0/3.0)
    #define FXAA_SUBPIX_TRIM         (1.0/4.0)
#endif
/*--------------------------------------------------------------------------*/
#if (FXAA_PRESET == 1)
    #define FXAA_EDGE_THRESHOLD      (1.0/8.0)
    #define FXAA_EDGE_THRESHOLD_MIN  (1.0/16.0)
    #define FXAA_SEARCH_STEPS        4
    #define FXAA_SEARCH_ACCELERATION 3
    #define FXAA_SEARCH_THRESHOLD    (1.0/4.0)
    #define FXAA_SUBPIX              1
    #define FXAA_SUBPIX_FASTER       0
    #define FXAA_SUBPIX_CAP          (3.0/4.0)
    #define FXAA_SUBPIX_TRIM         (1.0/4.0)
#endif
/*--------------------------------------------------------------------------*/
#if (FXAA_PRESET == 2)
    #define FXAA_EDGE_THRESHOLD      (1.0/8.0)
    #define FXAA_EDGE_THRESHOLD_MIN  (1.0/24.0)
    #define FXAA_SEARCH_STEPS        8
    #define FXAA_SEARCH_ACCELERATION 2
    #define FXAA_SEARCH_THRESHOLD    (1.0/4.0)
    #define FXAA_SUBPIX              1
    #define FXAA_SUBPIX_FASTER       0
    #define FXAA_SUBPIX_CAP          (3.0/4.0)
    #define FXAA_SUBPIX_TRIM         (1.0/4.0)
#endif
/*--------------------------------------------------------------------------*/
#if (FXAA_PRESET == 3)
    #define FXAA_EDGE_THRESHOLD      (1.0/8.0)
    #define FXAA_EDGE_THRESHOLD_MIN  (1.0/24.0)
    #define FXAA_SEARCH_STEPS        16
    #define FXAA_SEARCH_ACCELERATION 1
    #define FXAA_SEARCH_THRESHOLD    (1.0/4.0)
    #define FXAA_SUBPIX              1
    #define FXAA_SUBPIX_FASTER       0
    #define FXAA_SUBPIX_CAP          (3.0/4.0)
    #define FXAA_SUBPIX_TRIM         (1.0/4.0)
#endif
/*--------------------------------------------------------------------------*/
#if (FXAA_PRESET == 4)
    #define FXAA_EDGE_THRESHOLD      (1.0/8.0)
    #define FXAA_EDGE_THRESHOLD_MIN  (1.0/24.0)
    #define FXAA_SEARCH_STEPS        24
    #define FXAA_SEARCH_ACCELERATION 1
    #define FXAA_SEARCH_THRESHOLD    (1.0/4.0)
    #define FXAA_SUBPIX              1
    #define FXAA_SUBPIX_FASTER       0
    #define FXAA_SUBPIX_CAP          (3.0/4.0)
    #define FXAA_SUBPIX_TRIM         (1.0/4.0)
#endif
/*--------------------------------------------------------------------------*/
#if (FXAA_PRESET == 5)
    #define FXAA_EDGE_THRESHOLD      (1.0/8.0)
    #define FXAA_EDGE_THRESHOLD_MIN  (1.0/24.0)
    #define FXAA_SEARCH_STEPS        32
    #define FXAA_SEARCH_ACCELERATION 1
    #define FXAA_SEARCH_THRESHOLD    (1.0/4.0)
    #define FXAA_SUBPIX              1
    #define FXAA_SUBPIX_FASTER       0
    #define FXAA_SUBPIX_CAP          (7.0/8.0)
    #define FXAA_SUBPIX_TRIM         (1.0/8.0)
#endif

// CUSTOM
/*--------------------------------------------------------------------------*/
#if (FXAA_PRESET == 6)
    #define FXAA_EDGE_THRESHOLD      (1.0/16.0)
    #define FXAA_EDGE_THRESHOLD_MIN  (1.0/32.0)
    #define FXAA_SEARCH_STEPS        32
    #define FXAA_SEARCH_ACCELERATION 1
    #define FXAA_SEARCH_THRESHOLD    (1.0/4.0)
    #define FXAA_SUBPIX              1
    #define FXAA_SUBPIX_FASTER       0
    #define FXAA_SUBPIX_CAP          1
    #define FXAA_SUBPIX_TRIM         0
#endif

#define FXAA_SUBPIX_TRIM_SCALE (1.0/(1.0 - FXAA_SUBPIX_TRIM))


float Luma(vec3 color)
{
    return color.g * (0.587 / 0.299) + color.r;
    //return dot(color, vec3(0.299, 0.587, 0.114));
}

void main()
{
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    vec2 uv = (pixelCoords + 0.5) * u_Renderer.InverseViewportSize;
    LoadSharedData();

/*----------------------------------------------------------------------------
            EARLY EXIT IF LOCAL CONTRAST BELOW EDGE DETECT LIMIT
------------------------------------------------------------------------------*/
    vec2 pos = uv;
    vec2 rcpFrame = u_Renderer.InverseViewportSize;

    vec3 rgbN = LoadPixel(0, -1);
    vec3 rgbW = LoadPixel(-1, 0);
    vec3 rgbM = LoadPixel(0, 0);
    vec3 rgbE = LoadPixel(1, 0);
    vec3 rgbS = LoadPixel(0, 1);

    float lumaN = Luma(rgbN);
    float lumaW = Luma(rgbW);
    float lumaM = Luma(rgbM);
    float lumaE = Luma(rgbE);
    float lumaS = Luma(rgbS);

    float rangeMax = max(max(max(max(lumaN, lumaE), lumaS), lumaW), lumaM);
    float rangeMin = min(min(min(min(lumaN, lumaE), lumaS), lumaW), lumaM);
    float range = rangeMax - rangeMin;

    if(range < max(FXAA_EDGE_THRESHOLD_MIN, rangeMax * FXAA_EDGE_THRESHOLD))
    {
        imageStore(u_PostProcessTex, pixelCoords, vec4(rgbM, 1));
        return;
    }

/*----------------------------------------------------------------------------
                               COMPUTE LOWPASS
------------------------------------------------------------------------------*/

#if FXAA_SUBPIX > 0
    #if FXAA_SUBPIX_FASTER
        vec3 rgbL = (rgbN + rgbW + rgbE + rgbS + rgbM) * vec3(1.0 / 5.0);
    #else
        vec3 rgbL = rgbN + rgbW + rgbM + rgbE + rgbS;
    #endif
#endif  

#if FXAA_SUBPIX != 0
    float lumaL = (lumaN + lumaW + lumaE + lumaS) * 0.25;
    float rangeL = abs(lumaL - lumaM);
#endif        
#if FXAA_SUBPIX == 1
    float blendL = max(0.0, (rangeL / range) - FXAA_SUBPIX_TRIM) * FXAA_SUBPIX_TRIM_SCALE; 
    blendL = min(FXAA_SUBPIX_CAP, blendL);
#endif
#if FXAA_SUBPIX == 2
    float blendL = rangeL / range; 
#endif
    
/*----------------------------------------------------------------------------
                    CHOOSE VERTICAL OR HORIZONTAL SEARCH
------------------------------------------------------------------------------ */

    vec3 rgbNW = LoadPixel(-1, -1);
    vec3 rgbNE = LoadPixel(1, -1);
    vec3 rgbSW = LoadPixel(-1, 1);
    vec3 rgbSE = LoadPixel(1, 1);

#if (FXAA_SUBPIX_FASTER == 0) && (FXAA_SUBPIX > 0)
    rgbL += (rgbNW + rgbNE + rgbSW + rgbSE);
    rgbL *= vec3(1.0 / 9.0);
#endif

    float lumaNW = Luma(rgbNW);
    float lumaNE = Luma(rgbNE);
    float lumaSW = Luma(rgbSW);
    float lumaSE = Luma(rgbSE);

    float edgeVert = 
        abs((0.25 * lumaNW) + (-0.5 * lumaN) + (0.25 * lumaNE)) +
        abs((0.50 * lumaW ) + (-1.0 * lumaM) + (0.50 * lumaE )) +
        abs((0.25 * lumaSW) + (-0.5 * lumaS) + (0.25 * lumaSE));

    float edgeHorz = 
        abs((0.25 * lumaNW) + (-0.5 * lumaW) + (0.25 * lumaSW)) +
        abs((0.50 * lumaN ) + (-1.0 * lumaM) + (0.50 * lumaS )) +
        abs((0.25 * lumaNE) + (-0.5 * lumaE) + (0.25 * lumaSE));

    bool horzSpan = edgeHorz >= edgeVert;
    float lengthSign = horzSpan ? -rcpFrame.y : -rcpFrame.x;
    if(!horzSpan) lumaN = lumaW;
    if(!horzSpan) lumaS = lumaE;
    float gradientN = abs(lumaN - lumaM);
    float gradientS = abs(lumaS - lumaM);
    lumaN = (lumaN + lumaM) * 0.5;
    lumaS = (lumaS + lumaM) * 0.5;

/*----------------------------------------------------------------------------
                CHOOSE SIDE OF PIXEL WHERE GRADIENT IS HIGHEST
------------------------------------------------------------------------------*/
    
    bool pairN = gradientN >= gradientS;

    if(!pairN) lumaN = lumaS;
    if(!pairN) gradientN = gradientS;
    if(!pairN) lengthSign *= -1.0;

    vec2 posN;
    posN.x = pos.x + (horzSpan ? 0.0 : lengthSign * 0.5);
    posN.y = pos.y + (horzSpan ? lengthSign * 0.5 : 0.0);


/*----------------------------------------------------------------------------
                         CHOOSE SEARCH LIMITING VALUES
------------------------------------------------------------------------------*/

    gradientN *= FXAA_SEARCH_THRESHOLD;

/*----------------------------------------------------------------------------
    SEARCH IN BOTH DIRECTIONS UNTIL FIND LUMA PAIR AVERAGE IS OUT OF RANGE
------------------------------------------------------------------------------*/
    
    vec2 posP = posN;
    vec2 offNP = horzSpan ? vec2(rcpFrame.x, 0.0) : vec2(0.0f, rcpFrame.y); 
    float lumaEndN = lumaN;
    float lumaEndP = lumaN;
    bool doneN = false;
    bool doneP = false;
#if FXAA_SEARCH_ACCELERATION == 1
    posN += offNP * vec2(-1.0, -1.0);
    posP += offNP * vec2( 1.0,  1.0);
#endif
#if FXAA_SEARCH_ACCELERATION == 2
    posN += offNP * vec2(-1.5, -1.5);
    posP += offNP * vec2( 1.5,  1.5);
    offNP *= vec2(2.0, 2.0);
#endif
#if FXAA_SEARCH_ACCELERATION == 3
    posN += offNP * vec2(-2.0, -2.0);
    posP += offNP * vec2( 2.0,  2.0);
    offNP *= vec2(3.0, 3.0);
#endif
#if FXAA_SEARCH_ACCELERATION == 4
    posN += offNP * vec2(-2.5, -2.5);
    posP += offNP * vec2( 2.5,  2.5);
    offNP *= vec2(4.0, 4.0);
#endif

    for(int i = 0; i < FXAA_SEARCH_STEPS; i++) 
    {
    #if FXAA_SEARCH_ACCELERATION == 1
        if(!doneN) lumaEndN = Luma(texture(u_SceneColor, posN.xy).xyz);
        if(!doneP) lumaEndP = Luma(texture(u_SceneColor, posP.xy).xyz);
    #else
        if(!doneN) lumaEndN = Luma(textureGrad(u_SceneColor, posN.xy, offNP, offNP).xyz);
        if(!doneP) lumaEndP = Luma(textureGrad(u_SceneColor, posP.xy, offNP, offNP).xyz);
    #endif

        doneN = doneN || (abs(lumaEndN - lumaN) >= gradientN);
        doneP = doneP || (abs(lumaEndP - lumaN) >= gradientN);
        if(doneN && doneP) break;
        if(!doneN) posN -= offNP;
        if(!doneP) posP += offNP; 
    }

/*----------------------------------------------------------------------------
               HANDLE IF CENTER IS ON POSITIVE OR NEGATIVE SIDE 
------------------------------------------------------------------------------*/

    float dstN = horzSpan ? pos.x - posN.x : pos.y - posN.y;
    float dstP = horzSpan ? posP.x - pos.x : posP.y - pos.y;
    bool directionN = dstN < dstP;
    lumaEndN = directionN ? lumaEndN : lumaEndP;

/*----------------------------------------------------------------------------
         CHECK IF PIXEL IS IN SECTION OF SPAN WHICH GETS NO FILTERING
------------------------------------------------------------------------------*/

    if(((lumaM - lumaN) < 0.0) == ((lumaEndN - lumaN) < 0.0)) 
        lengthSign = 0.0;

/*----------------------------------------------------------------------------
                COMPUTE SUB-PIXEL OFFSET AND FILTER SPAN
------------------------------------------------------------------------------*/
    
    float spanLength = (dstP + dstN);
    dstN = directionN ? dstN : dstP;
    float subPixelOffset = (0.5 + (dstN * (-1.0 / spanLength))) * lengthSign;

    vec3 rgbF = texture(u_SceneColor, vec2(
    pos.x + (horzSpan ? 0.0 : subPixelOffset),
    pos.y + (horzSpan ? subPixelOffset : 0.0))).xyz;

    vec3 result;
#if FXAA_SUBPIX == 0
    result = rgbF; 
#else        
    result = mix(rgbL, rgbF, blendL); 
#endif

    imageStore(u_PostProcessTex, pixelCoords, vec4(result, 1));
}
