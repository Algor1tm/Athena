#include "Color.h"

#include "Athena/Math/Common.h"


namespace Athena
{
	static uint32 RGBAToHEX(uint8 r, uint8 g, uint8 b, uint8 a, bool inverse)
	{
		if (inverse)
		{
			return ((uint32_t(a) & 0xff) << 24) +
				((uint32_t(b) & 0xff) << 16) +
				((uint32_t(g) & 0xff) << 8) +
				((uint32_t(r)) & 0xff);
		}
		else
		{
			return ((uint32_t(r) & 0xff) << 24) +
				((uint32_t(g) & 0xff) << 16) +
				((uint32_t(b) & 0xff) << 8) +
				((uint32_t(a)) & 0xff);
		}
	}

	static IntegerColor HEXToRGBA(HEXColor hex)
	{
		IntegerColor out;

		if (hex.Inverse)
		{
			out.a = ((hex.Value >> 24) & 0xFF);
			out.b = ((hex.Value >> 16) & 0xFF);
			out.g = ((hex.Value >> 8) & 0xFF);
			out.r = ((hex.Value) & 0xFF);
		}
		else
		{
			out.r = ((hex.Value >> 24) & 0xFF);
			out.g = ((hex.Value >> 16) & 0xFF);
			out.b = ((hex.Value >> 8) & 0xFF);
			out.a = ((hex.Value) & 0xFF);
		}

		return out;
	}

	static HSVColor RGBAToHSV(float r, float g, float b)
	{
		HSVColor result;

		r = r;
		g = g;
		b = b;

		float cmax = Math::Max(r, g, b); 
		float cmin = Math::Min(r, g, b);
		float diff = cmax - cmin;


		if (cmax == cmin)
			result.H = 0;

		else if (cmax == r)
			result.H = Math::FMod(60 * ((g - b) / diff) + 360, 360.f);

		else if (cmax == g)
			result.H = Math::FMod(60 * ((b - r) / diff) + 120, 360.f);

		else if (cmax == b)
			result.H = Math::FMod(60 * ((r - g) / diff) + 240, 360.f);

		if (cmax == 0)
			result.S = 0;
		else
			result.S = (diff / cmax) * 100;

		result.V = cmax * 100;

		return result;
	}

	static LinearColor HSVToRGBA(const HSVColor& hsv)
	{
		float s = hsv.S / 100;
		float v = hsv.V / 100;
		float C = s * v;
		float X = C * (1 - Math::Abs(Math::FMod(hsv.H / 60.f, 2.f) - 1));
		float m = v - C;
		float r, g, b;

		if (hsv.H >= 0 && hsv.H < 60) 
			r = C, g = X, b = 0;
		
		else if (hsv.H >= 60 && hsv.H < 120) 
			r = X, g = C, b = 0;
		
		else if (hsv.H >= 120 && hsv.H < 180) 
			r = 0, g = C, b = X;
		
		else if (hsv.H >= 180 && hsv.H < 240) 
			r = 0, g = X, b = C;
		
		else if (hsv.H >= 240 && hsv.H < 300) 
			r = X, g = 0, b = C;
		
		else 
			r = C, g = 0, b = X;
		
		LinearColor out;
		out.r = (r + m);
		out.g = (g + m);
		out.b = (b + m);
		out.a = 1;

		return out;
	}


	String ToString(const HSVColor& color)
	{
		std::stringstream stream;
		stream << "HSVColor(H = " << color.H << ", S = " << color.S << ", V = " << color.V << ")";
		return stream.str();
	}

	String ToString(const HEXColor& color)
	{
		std::stringstream stream;
		stream << "HEXColor: #" << std::hex << color.Value;
		return stream.str();
	}



	LinearColor::LinearColor()
		: LinearColor(1.f, 1.f, 1.f) {}

	LinearColor::LinearColor(const IntegerColor& color)
		: LinearColor(color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f) {}

	LinearColor::LinearColor(float R, float G, float B, float A)
		: r(R), g(G), b(B), a(A) {}

	LinearColor::LinearColor(const Vector4& vec4)
		: LinearColor(vec4.x, vec4.y, vec4.z, vec4.w) {}

	LinearColor::LinearColor(const Vector3& vec3)
		: LinearColor(vec3.x, vec3.y, vec3.z) {}

	LinearColor::LinearColor(const HSVColor& hsv)
	{
		LinearColor tmp = HSVToRGBA(hsv);
		r = tmp.r; g = tmp.g; b = tmp.b; a = tmp.a;
	}

	LinearColor::LinearColor(HEXColor hex)
		: LinearColor(HEXToRGBA(hex)) {}

	LinearColor& LinearColor::operator=(const Vector4& vec4)
	{
		r = vec4.x;
		g = vec4.y;
		b = vec4.z;
		a = vec4.w;
		return *this;
	}

	LinearColor& LinearColor::operator=(const Vector3& vec3)
	{
		r = vec3.x;
		g = vec3.y;
		b = vec3.z;
		a = 1.f;
		return *this;
	}

	LinearColor::operator Vector4() const
	{
		return Vector4(r, g, b, a);
	}

	LinearColor::operator Vector3() const
	{
		return Vector3(r, g, b);
	}

	HSVColor LinearColor::ToHSV() const
	{
		return RGBAToHSV(r, g, b);
	}

	HEXColor LinearColor::ToHEX(bool inverse) const
	{
		HEXColor hex;
		hex.Inverse = inverse;
		hex.Value = RGBAToHEX(static_cast<uint8>(r * 255.f), 
			static_cast<uint8>(g * 255.f), 
			static_cast<uint8>(b * 255.f), 
			static_cast<uint8>(a * 255.f), 
			inverse);

		return hex;
	}

	float* LinearColor::Data()
	{
		return &r;
	}

	const float* LinearColor::Data() const
	{
		return &r;
	}

	LinearColor::iterator LinearColor::begin()
	{
		return iterator(&r, 0);
	}

	LinearColor::iterator LinearColor::end()
	{
		return iterator(&r, 4);
	}

	LinearColor::const_iterator LinearColor::cbegin() const
	{
		return const_iterator(&r, 0);
	}

	LinearColor::const_iterator LinearColor::cend() const
	{
		return const_iterator(&r, 4);
	}

	float LinearColor::operator[](uint32 idx) const
	{
		ATN_CORE_ASSERT(idx < 4, "LinearColor subscript out of range");
		return *(&r + idx);
	}

	float& LinearColor::operator[](uint32 idx)
	{
		ATN_CORE_ASSERT(idx < 4, "LinearColor subscript out of range");
		return *(&r + idx);
	}

	LinearColor& LinearColor::operator+=(const LinearColor& other)
	{
		r += other.r;
		g += other.g;
		b += other.b;
		a += other.a;
		return *this;
	}

	LinearColor& LinearColor::operator-=(const LinearColor& other)
	{
		r -= other.r;
		g -= other.g;
		b -= other.b;
		a -= other.a;
		return *this;
	}

	LinearColor& LinearColor::operator*=(const LinearColor& other)
	{
		r *= other.a;
		g *= other.g;
		b *= other.b;
		a *= other.a;
		return *this;
	}

	LinearColor& LinearColor::operator/=(const LinearColor& other)
	{
		r /= other.a;
		g /= other.g;
		b /= other.b;
		a /= other.a;
		return *this;
	}

	LinearColor LinearColor::operator+(const LinearColor& other) const
	{
		return LinearColor(*this) += other;
	}

	LinearColor LinearColor::operator-(const LinearColor& other) const
	{
		return LinearColor(*this) -= other;
	}
	LinearColor LinearColor::operator*(const LinearColor& other) const
	{
		return LinearColor(*this) *= other;
	}

	LinearColor LinearColor::operator/(const LinearColor& other) const
	{
		return LinearColor(*this) /= other;
	}

	bool LinearColor::operator==(const LinearColor& other) const
	{
		return r == other.r && g == other.g && b == other.b && a == other.a;
	}

	bool LinearColor::operator!=(const LinearColor& other) const
	{
		return !(*this == other);
	}

	const LinearColor LinearColor::White = LinearColor(1, 1, 1);
	const LinearColor LinearColor::Black = LinearColor(0, 0, 0);
	const LinearColor LinearColor::Red = LinearColor(1, 0, 0);
	const LinearColor LinearColor::Green = LinearColor(0, 1, 0);
	const LinearColor LinearColor::Blue = LinearColor(0, 0, 1);
	const LinearColor LinearColor::Yellow = LinearColor(1, 1, 0);
	const LinearColor LinearColor::Cyan = LinearColor(0, 1, 1);
	const LinearColor LinearColor::Magenta = LinearColor(1, 0, 1);
	const LinearColor LinearColor::Gray = LinearColor(0.5f, 0.5f, 0.5f);
	const LinearColor LinearColor::Transparent = LinearColor(1, 1, 1, 0);


	LinearColor Lerp(const LinearColor& a, const LinearColor& b, float t)
	{
		return LinearColor(
			Math::Lerp(a.r, b.r, t), 
			Math::Lerp(a.g, b.g, t), 
			Math::Lerp(a.b, b.b, t), 
			Math::Lerp(a.a, b.a, t));
	}

	LinearColor Clamp(const LinearColor& clr, float min, float max)
	{
		return LinearColor(
			Math::Clamp(clr.r, min, max),
			Math::Clamp(clr.g, min, max),
			Math::Clamp(clr.b, min, max),
			Math::Clamp(clr.a, min, max));
	}

	String ToString(const LinearColor& color)
	{
		std::stringstream stream;
		stream << "LinearColor(" << color.r << ", " << color.g << ", " << color.b << ", " << color.a << ")";
		return stream.str();
	}



	IntegerColor::IntegerColor()
		: IntegerColor(255, 255, 255) {}

	IntegerColor::IntegerColor(const LinearColor& color)
		: IntegerColor(
			static_cast<uint8>(color.r * 255.f),
			static_cast<uint8>(color.g * 255.f),
			static_cast<uint8>(color.b * 255.f),
			static_cast<uint8>(color.a * 255.f)) {}


	IntegerColor::IntegerColor(uint8 R, uint8 G, uint8 B, uint8 A)
		: r(R), g(G), b(B), a(A) {}

	IntegerColor::IntegerColor(const Vector<uint8, 4>& vec4)
		: IntegerColor(vec4.x, vec4.y, vec4.z, vec4.w) {}

	IntegerColor::IntegerColor(const Vector<uint8, 3>& vec3)
		: IntegerColor(vec3.x, vec3.y, vec3.z) {}

	IntegerColor::IntegerColor(const HSVColor& hsv)
		: IntegerColor(HSVToRGBA(hsv)) {}

	IntegerColor::IntegerColor(HEXColor hex)
	{
		IntegerColor tmp = HEXToRGBA(hex);
		r = tmp.r; g = tmp.g; b = tmp.b; a = tmp.a;
	}

	IntegerColor& IntegerColor::operator=(const Vector<uint8, 4>& vec4)
	{
		r = vec4.x;
		g = vec4.y;
		b = vec4.z;
		a = vec4.w;
		return *this;
	}

	IntegerColor& IntegerColor::operator=(const Vector<uint8, 3>& vec3)
	{
		r = vec3.x;
		g = vec3.y;
		b = vec3.z;
		a = 255;
		return *this;
	}

	IntegerColor::operator Vector<uint8, 4>() const
	{
		return Vector<uint8, 4>(r, g, b, a);
	}

	IntegerColor::operator Vector<uint8, 3>() const
	{
		return Vector<uint8, 3>(r, g, b);
	}

	HSVColor IntegerColor::ToHSV() const
	{
		return RGBAToHSV(
			static_cast<float>(r) / 255.f,
			static_cast<float>(g) / 255.f,
			static_cast<float>(b) / 255.f);
	}

	HEXColor IntegerColor::ToHEX(bool inverse) const
	{
		HEXColor hex;
		hex.Inverse = inverse;
		hex.Value = RGBAToHEX(r, g, b, a, inverse);
		return hex;
	}

	uint8* IntegerColor::Data()
	{
		return &r;
	}

	const uint8* IntegerColor::Data() const
	{
		return &r;
	}

	IntegerColor::iterator IntegerColor::begin()
	{
		return iterator(&r, 0);
	}

	IntegerColor::iterator IntegerColor::end()
	{
		return iterator(&r, 4);
	}

	IntegerColor::const_iterator IntegerColor::cbegin() const
	{
		return const_iterator(&r, 0);
	}

	IntegerColor::const_iterator IntegerColor::cend() const
	{
		return const_iterator(&r, 4);
	}

	uint8 IntegerColor::operator[](uint32 idx) const
	{
		ATN_CORE_ASSERT(idx < 4, "IntegerColor subscript out of range");
		return *(&r + idx);
	}

	uint8& IntegerColor::operator[](uint32 idx)
	{
		ATN_CORE_ASSERT(idx < 4, "IntegerColor subscript out of range");
		return *(&r + idx);
	}

	bool IntegerColor::operator==(const IntegerColor& other) const
	{
		return r == other.r && g == other.g && b == other.b && a == other.a;
	}

	bool IntegerColor::operator!=(const IntegerColor& other) const
	{
		return !(*this == other);
	}


	const IntegerColor IntegerColor::White = IntegerColor(255, 255, 255);
	const IntegerColor IntegerColor::Black = IntegerColor(0, 0, 0);
	const IntegerColor IntegerColor::Red = IntegerColor(255, 0, 0);
	const IntegerColor IntegerColor::Green = IntegerColor(0, 255, 0);
	const IntegerColor IntegerColor::Blue = IntegerColor(0, 0, 255);
	const IntegerColor IntegerColor::Yellow = IntegerColor(255, 255, 0);
	const IntegerColor IntegerColor::Cyan = IntegerColor(0, 255, 255);
	const IntegerColor IntegerColor::Magenta = IntegerColor(255, 0, 255);
	const IntegerColor IntegerColor::Gray = IntegerColor(128, 128, 128);
	const IntegerColor IntegerColor::Transparent = IntegerColor(255, 255, 255, 0);


	IntegerColor Lerp(const IntegerColor& a, const IntegerColor& b, float t)
	{
		IntegerColor out;
		out.r = (uint8)Math::Lerp((float)a.r, (float)b.r, t);
		out.g = (uint8)Math::Lerp((float)a.g, (float)b.g, t);
		out.b = (uint8)Math::Lerp((float)a.b, (float)b.b, t);
		out.a = (uint8)Math::Lerp((float)a.a, (float)b.a, t);

		return out;
	}

	IntegerColor Clamp(const IntegerColor& color, uint8 min, uint8 max)
	{
		IntegerColor out;
		out.r = Math::Clamp(color.r, min, max);
		out.g = Math::Clamp(color.g, min, max);
		out.b = Math::Clamp(color.b, min, max);
		out.a = Math::Clamp(color.a, min, max);

		return out;
	}

	String ToString(const IntegerColor& color)
	{
		std::stringstream stream;
		stream << "IntegerColor(" << 
			(uint32)color.r << ", " << 
			(uint32)color.g << ", " << 
			(uint32)color.b << ", " << 
			(uint32)color.a << ")";
		return stream.str();
	}
}
