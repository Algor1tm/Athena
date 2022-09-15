#pragma once


#ifdef ATN_ASSERTS
	// Alteratively we could use the same "default" message for both "WITH_MSG" and "NO_MSG" and
	// provide support for custom formatting by concatenating the formatting string instead of having the format inside the default message
	#define ATN_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { ATN##type##ERROR(msg, __VA_ARGS__); ATN_DEBUGBREAK(); } }
	#define ATN_INTERNAL_ASSERT_WITH_MSG(type, check, ...) ATN_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
	#define ATN_INTERNAL_ASSERT_NO_MSG(type, check) ATN_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", ATN_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)
		
	#define ATN_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
	#define ATN_INTERNAL_ASSERT_GET_MACRO(...) ATN_EXPAND_MACRO( ATN_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, ATN_INTERNAL_ASSERT_WITH_MSG, ATN_INTERNAL_ASSERT_NO_MSG) )
		
	// CurreATNly accepts at least the condition and one additional parameter (the message) being optional
	#define ATN_ASSERT(...) ATN_EXPAND_MACRO( ATN_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
	#define ATN_CORE_ASSERT(...) ATN_EXPAND_MACRO( ATN_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )
#else
	#define ATN_ASSERT(x, ...) 
	#define ATN_CORE_ASSERT(x, ...)
#endif
