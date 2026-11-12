//////////////////////// Athena Preetham Sky Shader ////////////////////////

// References:
// https://www.shadertoy.com/view/llSSDR


#version 460 core
#pragma stage : compute

#include "Include/Common.glslh"

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(r11f_g11f_b10f, set = 1, binding = 0) uniform writeonly imageCube u_EnvironmentMap;

layout(push_constant) uniform u_Params
{
	float u_Turbidity;
	float u_Azimuth;
	float u_Inclination;
};


float SaturatedDot( in vec3 a, in vec3 b )
{
	return max( dot( a, b ), 0.0 );   
}

vec3 YxyToXYZ( in vec3 Yxy )
{
	float Y = Yxy.r;
	float x = Yxy.g;
	float y = Yxy.b;

	float X = x * ( Y / y );
	float Z = ( 1.0 - x - y ) * ( Y / y );

	return vec3(X,Y,Z);
}

vec3 XYZToRGB( in vec3 XYZ )
{
	// CIE/E
	mat3 M = mat3
	(
		 2.3706743, -0.9000405, -0.4706338,
		-0.5138850,  1.4253036,  0.0885814,
 		 0.0052982, -0.0146949,  1.0093968
	);

	return XYZ * M;
}


vec3 YxyToRGB( in vec3 Yxy )
{
	vec3 XYZ = YxyToXYZ( Yxy );
	vec3 RGB = XYZToRGB( XYZ );
	return RGB;
}

void CalculatePerezDistribution( in float t, out vec3 A, out vec3 B, out vec3 C, out vec3 D, out vec3 E )
{
	A = vec3(  0.1787 * t - 1.4630, -0.0193 * t - 0.2592, -0.0167 * t - 0.2608 );
	B = vec3( -0.3554 * t + 0.4275, -0.0665 * t + 0.0008, -0.0950 * t + 0.0092 );
	C = vec3( -0.0227 * t + 5.3251, -0.0004 * t + 0.2125, -0.0079 * t + 0.2102 );
	D = vec3(  0.1206 * t - 2.5771, -0.0641 * t - 0.8989, -0.0441 * t - 1.6537 );
	E = vec3( -0.0670 * t + 0.3703, -0.0033 * t + 0.0452, -0.0109 * t + 0.0529 );
}

vec3 CalculateZenithLuminanceYxy( in float t, in float thetaS )
{
	float chi  	 	= ( 4.0 / 9.0 - t / 120.0 ) * ( PI - 2.0 * thetaS );
	float Yz   	 	= ( 4.0453 * t - 4.9710 ) * tan( chi ) - 0.2155 * t + 2.4192;

	float theta2 	= thetaS * thetaS;
    float theta3 	= theta2 * thetaS;
    float T 	 	= t;
    float T2 	 	= t * t;

	float xz =
      ( 0.00165 * theta3 - 0.00375 * theta2 + 0.00209 * thetaS + 0.0)     * T2 +
      (-0.02903 * theta3 + 0.06377 * theta2 - 0.03202 * thetaS + 0.00394) * T +
      ( 0.11693 * theta3 - 0.21196 * theta2 + 0.06052 * thetaS + 0.25886);

    float yz =
      ( 0.00275 * theta3 - 0.00610 * theta2 + 0.00317 * thetaS + 0.0)     * T2 +
      (-0.04214 * theta3 + 0.08970 * theta2 - 0.04153 * thetaS + 0.00516) * T +
      ( 0.15346 * theta3 - 0.26756 * theta2 + 0.06670 * thetaS + 0.26688);

	return vec3( Yz, xz, yz );
}

vec3 CalculatePerezLuminanceYxy( in float theta, in float gamma, in vec3 A, in vec3 B, in vec3 C, in vec3 D, in vec3 E )
{
	return ( 1.0 + A * exp( B / cos( theta ) ) ) * ( 1.0 + C * exp( D * gamma ) + E * cos( gamma ) * cos( gamma ) );
}

vec3 CalculateSkyLuminanceRGB( in vec3 s, in vec3 e, in float t )
{
	vec3 A, B, C, D, E;
	CalculatePerezDistribution( t, A, B, C, D, E );

	float thetaS = acos( SaturatedDot( s, vec3(0,1,0) ) );
	float thetaE = acos( SaturatedDot( e, vec3(0,1,0) ) );
	float gammaE = acos( SaturatedDot( s, e )		   );

	vec3 Yz = CalculateZenithLuminanceYxy( t, thetaS );

	vec3 fThetaGamma = CalculatePerezLuminanceYxy( thetaE, gammaE, A, B, C, D, E );
	vec3 fZeroThetaS = CalculatePerezLuminanceYxy( 0.0,    thetaS, A, B, C, D, E );

	vec3 Yp = Yz * ( fThetaGamma / fZeroThetaS );

	return YxyToRGB( Yp );
}

void main()
{
    ivec3 unnormalizedTexCoords = ivec3(gl_GlobalInvocationID.xyz);
	vec3 direction = GetWorldDirectionFromCubeCoords(unnormalizedTexCoords, vec2(gl_NumWorkGroups * gl_WorkGroupSize));

    vec3 sunDir = normalize( vec3( sin(u_Inclination) * cos(u_Azimuth), cos(u_Inclination), sin(u_Inclination) * sin(u_Azimuth) ) );
    vec3 skyLuminance = CalculateSkyLuminanceRGB( sunDir, direction, u_Turbidity );
    
	vec3 color = skyLuminance * 0.05;

	// Apply inverse gamma correction
	color = pow(color, vec3(DISPLAY_GAMMA));

	imageStore(u_EnvironmentMap, unnormalizedTexCoords, vec4( color, 1.0 ));
}
