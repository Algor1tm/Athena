#pragma once

#include "Athena/Math/Vector.h"
#include "Athena/Math/Utils.h"


namespace Athena
{
	class ATHENA_API Color
	{
	public:
		using iterator = VectorIterator<float, 4>;
		using const_iterator = VectorConstIterator<float, 4>;

	public:
		Color();
		Color(float r, float g, float b, float a = 1);
		Color(const Vector4& vec4);
		Color(const Vector3& vec3);

		Color& operator=(const Vector4& vec4);
		Color& operator=(const Vector3& vec3);

		operator Vector4() const;
		operator Vector3() const;

		float operator[](uint32_t idx) const;
		float& operator[](uint32_t idx);

		iterator begin();
		iterator end();
		const_iterator cbegin() const;
		const_iterator cend() const;

		bool operator==(const Color& other) const;
		bool operator!=(const Color& other) const;

	public:
		static const Color White;
		static const Color Black;
		static const Color Red;
		static const Color Green;
		static const Color Blue;
		static const Color Yellow;
		static const Color Cyan;
		static const Color Magenta;
		static const Color Gray;
		static const Color Transparent;
	
	public:
		float r, g, b, a;
	};


	Color Lerp(const Color& min, const Color& max, float mid);
	std::string ToString(const Color& color);
}
