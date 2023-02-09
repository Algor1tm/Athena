#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Math/Vector.h"

#include <string>


namespace Athena
{
	struct HSVColor
	{
		HSVColor() = default;
		HSVColor(float h, float s, float v)
			: H(h), S(s), V(v) {}

		float H = 0, S = 0, V = 100;
	};

	ATHENA_API String ToString(const HSVColor& color);

	struct HEXColor
	{
		HEXColor() = default;
		HEXColor(uint32 value)
			: Value(value) {}

		uint32 Value = 0xFFFFFFFF;
		bool Inverse = false;
	};

	ATHENA_API String ToString(const HEXColor& color);

	class IntegerColor;


	// A linear, 32-bit/component floating point RGBA color
	class ATHENA_API LinearColor
	{
	public:
		using iterator = VectorIterator<float, 4>;
		using const_iterator = VectorConstIterator<float, 4>;

	public:
		LinearColor();
		LinearColor(const IntegerColor& color);
		LinearColor(float r, float g, float b, float a = 1);
		LinearColor(const Vector4& vec4);
		LinearColor(const Vector3& vec3);
		LinearColor(const HSVColor& hsv);
		LinearColor(HEXColor hex);

		LinearColor& operator=(const Vector4& vec4);
		LinearColor& operator=(const Vector3& vec3);

		operator Vector4() const;
		operator Vector3() const;

		HSVColor ToHSV() const;
		HEXColor ToHEX(bool inverse = false) const;

		float* Data();
		const float* Data() const;

		iterator begin();
		iterator end();
		const_iterator cbegin() const;
		const_iterator cend() const;

		float operator[](uint32 idx) const;
		float& operator[](uint32 idx);

		LinearColor& operator+=(const LinearColor& other);
		LinearColor& operator-=(const LinearColor& other);
		LinearColor& operator*=(const LinearColor& other);
		LinearColor& operator/=(const LinearColor& other);

		LinearColor operator+(const LinearColor& other) const;
		LinearColor operator-(const LinearColor& other) const;
		LinearColor operator*(const LinearColor& other) const;
		LinearColor operator/(const LinearColor& other) const;

		bool operator==(const LinearColor& other) const;
		bool operator!=(const LinearColor& other) const;

	public:
		static const LinearColor White;
		static const LinearColor Black;
		static const LinearColor Red;
		static const LinearColor Green;
		static const LinearColor Blue;
		static const LinearColor Yellow;
		static const LinearColor Cyan;
		static const LinearColor Magenta;
		static const LinearColor Gray;
		static const LinearColor Transparent;

	public:
		float r, g, b, a;
	};


	ATHENA_API LinearColor Lerp(const LinearColor& a, const LinearColor& b, float t);
	ATHENA_API LinearColor Clamp(const LinearColor& clr, float min, float max);
	ATHENA_API String ToString(const LinearColor& color);



	// Stores a color with 8 bits(uint8) of precision per channel.
	class ATHENA_API IntegerColor
	{
	public:
		using iterator = VectorIterator<uint8, 4>;
		using const_iterator = VectorConstIterator<uint8, 4>;

	public:
		IntegerColor();
		IntegerColor(const LinearColor& color);
		IntegerColor(uint8 r, uint8 g, uint8 b, uint8 a = 255);
		IntegerColor(const Vector<uint8, 4>& vec4);
		IntegerColor(const Vector<uint8, 3>& vec3);
		IntegerColor(const HSVColor& hsv);
		IntegerColor(HEXColor hex);

		IntegerColor& operator=(const Vector<uint8, 4>& vec4);
		IntegerColor& operator=(const Vector<uint8, 3>& vec3);

		operator Vector<uint8, 4>() const;
		operator Vector<uint8, 3>() const;

		HSVColor ToHSV() const;
		HEXColor ToHEX(bool inverse = false) const;

		uint8* Data();
		const uint8* Data() const;

		iterator begin();
		iterator end();
		const_iterator cbegin() const;
		const_iterator cend() const;

		uint8 operator[](uint32 idx) const;
		uint8& operator[](uint32 idx);

		bool operator==(const IntegerColor& other) const;
		bool operator!=(const IntegerColor& other) const;

	public:
		static const IntegerColor White;
		static const IntegerColor Black;
		static const IntegerColor Red;
		static const IntegerColor Green;
		static const IntegerColor Blue;
		static const IntegerColor Yellow;
		static const IntegerColor Cyan;
		static const IntegerColor Magenta;
		static const IntegerColor Gray;
		static const IntegerColor Transparent;

	public:
		uint8 r, g, b, a;
	};


	ATHENA_API IntegerColor Lerp(const IntegerColor& a, const IntegerColor& b, float t);
	ATHENA_API IntegerColor Clamp(const IntegerColor& clr, uint8 min, uint8 max);
	ATHENA_API String ToString(const IntegerColor& color);
}
