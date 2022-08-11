#pragma once

// Currently does not support ARM NEON
#if defined(ATN_SIMD) && !defined(__ARM_NEON)
	// Visual C++
	#ifdef _MSC_VER
		// SSE
		#if (defined(_M_AMD64) || defined(_M_X64))
			#define ATN_SSE
		#endif

		// AVX
		#if (defined(__AVX__) || defined(__AVX2__))
			#define ATN_AVX
		#endif

	// Other compilers
	#else
		// SSE
		#if (defined(__SSE__) || defined(__SSE2__) || defined(__SSE3__))
			#define ATN_SSE
		#endif

		// AVX
		#if (defined(__AVX__) || defined(__AVX2__))
			#define ATN_AVX
		#endif 
	#endif
	
#ifdef ATN_SSE
	#include <xmmintrin.h>
	#include <immintrin.h>
#endif

#endif
