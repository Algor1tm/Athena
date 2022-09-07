#pragma once


#include "TypesImpl/Quaternion.h"

#include "QuaternionCommon.h"
#include "TypeCasts.h"


namespace Athena
{
	template <typename T>
	using Quaternion = Athena::Math::Quaternion<T>;

	using Quat = Quaternion<float>;
	using FQuat = Quaternion<float>;
	using DQuat = Quaternion<double>;
}
