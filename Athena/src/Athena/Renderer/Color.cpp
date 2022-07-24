#include "atnpch.h"
#include "Color.h"


namespace Athena
{
	Color::Color()
		: Color(1.f, 1.f, 1.f)
	{

	}

	Color::Color(float r, float g, float b, float a)
		: r(r), g(g), b(b), a(a)
	{

	}

	Color::Color(const Vector4& vec4)
		: Color(vec4.x, vec4.y, vec4.z, vec4.w)
	{

	}

	Color::Color(const Vector3& vec3)
		: Color(vec3.x, vec3.y, vec3.z)
	{

	}

	Color& Color::operator=(const Vector4& vec4)
	{
		r = vec4.x;
		g = vec4.y;
		b = vec4.z;
		a = vec4.w;
		return *this;
	}

	Color& Color::operator=(const Vector3& vec3)
	{
		r = vec3.x;
		g = vec3.y;
		b = vec3.z;
		a = 1.f;
		return *this;
	}

	Color::operator Vector4() const
	{
		return Vector4(r, g, b, a);
	}

	Color::operator Vector3() const
	{
		return Vector3(r, g, b);
	}

	float* Color::Data()
	{
		return &r;
	}

	const float* Color::Data() const
	{
		return &r;
	}

	float Color::operator[](uint32_t idx) const
	{
		ATN_CORE_ASSERT(idx < 4, "Color subscript out of range");
		return *(&r + idx);
	}

	float& Color::operator[](uint32_t idx)
	{
		ATN_CORE_ASSERT(idx < 4, "Color subscript out of range");
		return *(&r + idx);
	}

	Color::iterator Color::begin()
	{
		return iterator(&r, 0);
	}

	Color::iterator Color::end()
	{
		return iterator(&r, 4);
	}

	Color::const_iterator Color::cbegin() const
	{
		return const_iterator(&r, 0);
	}

	Color::const_iterator Color::cend() const
	{
		return const_iterator(&r, 4);
	}

	bool Color::operator==(const Color& other) const
	{
		return r == other.r && g == other.g && b == other.b && a == other.a;
	}

	bool Color::operator!=(const Color& other) const
	{
		return !(*this == other);
	}

	const Color Color::White = Color(1, 1, 1);
	const Color Color::Black = Color(0, 0, 0);
	const Color Color::Red = Color(1, 0, 0);
	const Color Color::Green = Color(0, 1, 0);
	const Color Color::Blue = Color(0, 0, 1);
	const Color Color::Yellow = Color(1, 1, 0);
	const Color Color::Cyan = Color(0, 1, 1);
	const Color Color::Magenta = Color(1, 0, 1);
	const Color Color::Gray = Color(0.5f, 0.5f, 0.5f);
	const Color Color::Transparent = Color(0, 0, 0, 0);


	Color Lerp(const Color& a, const Color& b, float t)
	{
		return Color(
			Lerp(a.r, b.r, t), 
			Lerp(a.g, b.g, t), 
			Lerp(a.b, b.b, t), 
			Lerp(a.a, b.a, t));
	}

	Color Clamp(const Color& clr, float min, float max)
	{
		return Color(
			Clamp(clr.r, min, max),
			Clamp(clr.g, min, max),
			Clamp(clr.b, min, max),
			Clamp(clr.a, min, max));
	}

	std::string ToString(const Color& color)
	{
		std::string out = "Color(";
		out += std::to_string(color.r) + ", ";
		out += std::to_string(color.g) + ", ";
		out += std::to_string(color.b) + ", ";
		out += std::to_string(color.a) + ", ";
		out += ")";
		return out;
	}
}
