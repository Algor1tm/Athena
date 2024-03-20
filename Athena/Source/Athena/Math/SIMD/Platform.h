#pragma once


// If defined ATN_SIMD Athena will use SIMD instructions for optimizations
// But this will lead to the following problems with these classes:
// Vector<float, 4>,  Matrix<float, 4, 4> 
// 1. Cannot be constexpr.
// 2. Have strict alignment.


// Athena currently does not support ARM NEON
#if defined(__ARM_NEON)
	#if defined(ATN_SIMD)
		#undef ATN_SIMD
	#endif
#endif 


#if defined(ATN_SIMD)
	// Visual C++
	#ifdef _MSC_VER
		#if (defined(_M_AMD64) || defined(_M_X64))
			#define ATN_SSE_2
			#define ATN_SSE_4_1
		#endif

	// Other compilers
	#else
		// SSE2
		#if defined(__SSE2__)
			#define ATN_SSE_2
		#endif
		// SSE4.1
		#if defined(__SSE4_1__ )
			#define ATN_SSE_4_1		
		#endif

	#endif
	
#if defined(ATN_SSE_2) || defined(ATN_SSE_4_1)
	#include <xmmintrin.h>
	#include <immintrin.h>
#endif

#endif
