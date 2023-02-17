#include "Random.h"


namespace Athena::Math
{
	std::random_device Random::s_RandomDevice;
	std::mt19937_64 Random::s_RandomEngine(s_RandomDevice());
	std::uniform_int_distribution<Random::internal_type> Random::s_Distribution;
}
