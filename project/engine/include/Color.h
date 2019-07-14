#pragma once

struct Color
{
	union
	{
		uint value;
		struct
		{
			byte b;
			byte g;
			byte r;
			byte a;
		};
	};

	Color() {}
	Color(uint value) : value(value) {}
	Color(byte r, byte g, byte b, byte a = 255) : r(r), g(g), b(b), a(a) {}
	Color(const Color& c) : value(c.value) {}

	bool operator == (const Color& c) const { return value == c.value; }
	bool operator != (const Color& c) const { return value != c.value; }

	operator uint () const { return value; }
	operator Vec4 () const { return Vec4(float(r) / 255.f, float(g) / 255.f, float(b) / 255.f, float(a) / 255.f); }
	operator Vec3 () const { return Vec3(float(r) / 255.f, float(g) / 255.f, float(b) / 255.f); }

	static Color Lerp(Color c1, Color c2, float t)
	{
		return Color(::Lerp(c1.r, c2.r, t), ::Lerp(c1.g, c2.g, t), ::Lerp(c1.b, c2.b, t), ::Lerp(c1.a, c2.a, t));
	}
	static Color Alpha(byte a)
	{
		return Color(255, 255, 255, a);
	}
	static constexpr Vec4 Hex(uint h)
	{
		return Vec4(1.f / 256 * (((h) & 0xFF0000) >> 16), 1.f / 256 * (((h) & 0xFF00) >> 8), 1.f / 256 * ((h) & 0xFF), 1.f);
	}

	static Color None;
	static Color Black;
	static Color White;
	static Color Red;
	static Color Green;
	static Color Blue;
	static Color Yellow;
};
