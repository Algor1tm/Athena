#include "Random.h"


namespace Athena::Math
{
	std::mt19937_64 Random::s_RandomEngine;
	std::uniform_int_distribution<Random::internal_type> Random::s_Distribution;
}
