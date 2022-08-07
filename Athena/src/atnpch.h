#pragma once

#include <memory>
#include <algorithm>
#include <functional>
#include <utility>
#include <chrono>
#include <iomanip>
#include <thread>

#include <limits>
#include <random>
#include <numeric>
#include <math.h>
#include <xmmintrin.h>

#include <iostream>
#include <sstream>
#include <fstream>

#include <array>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "Athena/Core/Core.h"
#include "Athena/Core/Log.h"
#include "Athena/Debug/Instrumentor.h"

#ifdef ATN_PLATFORM_WINDOWS
	#include <Windows.h>
#endif
