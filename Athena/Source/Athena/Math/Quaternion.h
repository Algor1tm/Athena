#pragma once


#include "Types/Quaternion.h"
#include "Types/QuaternionRelational.h"
#include "Utils/QuaternionTransforms.h"
#include "Utils/QuaternionCommon.h"


namespace Athena
{
	template <typename T>
	using Quaternion = Athena::Math::Quaternion<T>;

	using Quat = Quaternion<float>;
	using FQuat = Quaternion<float>;
	using DQuat = Quaternion<double>;
}