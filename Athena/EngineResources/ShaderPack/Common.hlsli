//////////////////////// PBR Functions ////////////////////////

float DistributionGGX(float3 normal, float3 halfWay, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float normalDotHalfWay = max(dot(normal, halfWay), 0.0);
	
    float numerator = a2;
    float denominator = (normalDotHalfWay * normalDotHalfWay * (a2 - 1.0) + 1.0);
    denominator = PI * denominator * denominator;
	
    return numerator / denominator;
}

float GeometrySchlickGGX(float normalDotView, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float numerator = normalDotView;
    float denominator = normalDotView * (1.0 - k) + k;
	
    return numerator / denominator;
}

float GeometrySmith(float3 normal, float3 view, float3 lightDir, float roughness)
{
    float normalDotView = max(dot(normal, view), 0.0);
    float normalDotLightDir = max(dot(normal, lightDir), 0.0);
    float ggx2 = GeometrySchlickGGX(normalDotView, roughness);
    float ggx1 = GeometrySchlickGGX(normalDotLightDir, roughness);
	
    return ggx1 * ggx2;
}

float3 FresnelShlick(float cosHalfWayAndView, float3 reflectivityAtZeroIncidence)
{
    return reflectivityAtZeroIncidence + (1.0 - reflectivityAtZeroIncidence) * pow(clamp(1.0 - cosHalfWayAndView, 0.0, 1.0), 5.0);
}
