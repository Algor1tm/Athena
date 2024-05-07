//////////////////////// Athena Deferred Lighting Shader ////////////////////////

#version 460 core
#pragma stage : vertex

layout(location = 0) in vec2 a_Position;
layout(location = 0) out vec2 v_TexCoords;

void main()
{
    v_TexCoords = vec2( (gl_VertexIndex << 1) & 2, (gl_VertexIndex) & 2 );
    gl_Position = vec4( v_TexCoords * vec2( 2.0, 2.0 ) + vec2( -1.0, -1.0), 0.0, 1.0 );
}

#version 460 core
#pragma stage : fragment

#include "Include/Buffers.glslh"
#include "Include/Common.glslh"
#include "Include/Lighting.glslh"


layout(location = 0) in vec2 v_TexCoords;
layout(location = 0) out vec4 o_Color;

layout(set = 1, binding = 9) uniform sampler2D u_SceneDepth;
layout(set = 1, binding = 10) uniform sampler2D u_SceneAlbedo;
layout(set = 1, binding = 11) uniform sampler2D u_SceneNormalsEmission;
layout(set = 1, binding = 12) uniform sampler2D u_SceneRoughnessMetalness;
layout(set = 1, binding = 13) uniform sampler2D u_SceneAO;


void main()
{
    // Unpack GBuffer
    float depth = texture(u_SceneDepth, v_TexCoords).r;
    if(depth == 0.0)
        discard;

    vec3 worldPos = WorldPositionFromDepth(v_TexCoords, depth, u_Camera.InverseProjection, u_Camera.InverseView);

    vec4 normalEmission = texture(u_SceneNormalsEmission, v_TexCoords);
    vec3 viewSpaceNormal = normalEmission.rgb * 2.0 - 1.0;
    vec3 normal = normalize(vec3(u_Camera.InverseView * vec4(viewSpaceNormal, 0.0)));

    vec3 albedo = texture(u_SceneAlbedo, v_TexCoords).rgb;

    float emissionIntensity = normalEmission.a;
    vec3 emissionColor = albedo.rgb;

    vec2 rm = texture(u_SceneRoughnessMetalness, v_TexCoords).rg;
    float roughness = rm.r;
    float metalness = rm.g;
    
    float ao = texture(u_SceneAO, v_TexCoords).r;
    
    // Compute Light
    vec3 viewDir = normalize(u_Camera.Position - worldPos);

    vec3 directional = GetDirectionalLight(normal, albedo.rgb, roughness, metalness, worldPos, viewDir, vec2(gl_FragCoord));
    vec3 ambient = GetAmbientLight(normal, albedo.rgb, metalness, roughness, viewDir);
    vec3 emission = emissionColor * emissionIntensity;

    vec3 hdrColor = directional + ambient * ao + emission;
    
    // Debug info
    if(bool(u_Renderer.DebugShadowCascades))
    {
        vec3 cascadeDebugColor = GetCascadeDebugColor(worldPos, u_Camera.View);
        hdrColor = mix(hdrColor, cascadeDebugColor, 0.3);
    }
    else if(bool(u_Renderer.DebugLightComplexity))
    {
        TileVisibleLights tileData = GetTileData(vec2(gl_FragCoord));
        if(tileData.LightCount != 0)
        {
            vec3 color = GetLightComplexityDebugColor(tileData.LightCount);
            hdrColor = mix(hdrColor, vec3(color), 0.65);
        }   
    }

    o_Color = vec4(hdrColor, 1.0);
}
