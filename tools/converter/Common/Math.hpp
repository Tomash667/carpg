/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za¿ó³æ gêœl¹ jaŸñ
 * Math - Modu³ matematyczny
 * Dokumentacja: Patrz plik doc/Math.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#pragma once

#include <windows.h> // bo bez tego w d3d9types.h s¹ b³êdy bo nie zna DWORD itp.
#include <d3d9types.h>
#include <d3dx9math.h>

namespace common
{

// Szablon bazowy - nie dzia³a, specjalizacje bêd¹ dzia³aæ
template <typename DestT, typename SrcT>
DestT & math_cast(const SrcT &x)
{
	assert(0 && "Unsupported type in math_cast.");
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// PUNKT 2D Z LICZB CA£KOWITYCH

/*
Kompatybilny ze struktur¹ POINT z Win32API.
Nazywa siê tak dziwnie, bo POINT to w Windows API marko i robi problem z nazw¹.
Konwersja do i ze stringa: "x,y", np. "10,1".
*/

struct POINT_
{
	union
	{
		struct
		{
			int x, y;
		};
		int v[2];
	};

	static const POINT_ ZERO;

	POINT_() { }
	POINT_(int x, int y) : x(x), y(y) { }

	bool operator == (const POINT_ &v) const { return x == v.x && y == v.y; }
	bool operator != (const POINT_ &v) const { return x != v.x || y != v.y; }

	POINT_ operator - () const { return POINT_(-x, -y); }
	POINT_ operator + (const POINT_ &v) const { return POINT_(x+v.x, y+v.y); }
	POINT_ operator - (const POINT_ &v) const { return POINT_(x-v.x, y-v.y); }
	POINT_ & operator += (const POINT_ &v) { x += v.x; y += v.y; return *this; }
	POINT_ & operator -= (const POINT_ &v) { x -= v.x; y -= v.y; return *this; }
	POINT_ & operator *= (int v) { x *= v; y *= v; return *this; }
	POINT_ & operator /= (int v) { x /= v; y /= v; return *this; }

	int & operator [] (uint4 Index) { return v[Index]; }
	const int & operator [] (uint4 Index) const { return v[Index]; }

	int * GetArray() { return &x; }
	const int * GetArray() const { return &x; }
};

inline POINT_ operator * (const POINT_ pt, int v) { return POINT_(pt.x*v, pt.y*v); }
inline POINT_ operator / (const POINT_ pt, int v) { return POINT_(pt.x/v, pt.y/v); }
inline POINT_ operator * (int v, const POINT_ pt) { return POINT_(pt.x*v, pt.y*v); }
inline POINT_ operator / (int v, const POINT_ pt) { return POINT_(pt.x/v, pt.y/v); }

// Konwersja do i z POINT z Win32API
template <> inline ::POINT & math_cast<::POINT, POINT_>(const POINT_ &x) { return (::POINT&)(x); }
template <> inline POINT_ & math_cast<POINT_, ::POINT>(const ::POINT &x) { return (POINT_&)(x); }

inline void swap(POINT_ &v1, POINT_ &v2)
{
	std::swap(v1.x, v2.x);
	std::swap(v1.y, v2.y);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// WEKTOR 2D, 3D i 4D Z FLOATÓW

/* Kompatybilny ze struktur¹ odpowiednio D3DXVECTOR2, D3DXVECTOR3 i D3DXVECTOR4
z D3DX. Konwersja do i ze stringa: "x,y,z", np. "10,-1,2.5".
*/

struct VEC2
{
	union
	{
		struct
		{
			float x, y;
		};
		float v[2];
	};

	static const VEC2 ZERO;
	static const VEC2 ONE;
	static const VEC2 POSITIVE_X;
	static const VEC2 POSITIVE_Y;
	static const VEC2 NEGATIVE_X;
	static const VEC2 NEGATIVE_Y;

	VEC2() { }
	VEC2(float x, float y) : x(x), y(y) { }
	VEC2(float *Array) : x(Array[0]), y(Array[1]) { }

	bool operator == (const VEC2 &v) const { return x == v.x && y == v.y; }
	bool operator != (const VEC2 &v) const { return x != v.x || y != v.y; }

	VEC2 operator - () const { return VEC2(-x, -y); }
	VEC2 operator + (const VEC2 &v) const { return VEC2(x+v.x, y+v.y); }
	VEC2 operator - (const VEC2 &v) const { return VEC2(x-v.x, y-v.y); }
	VEC2 & operator += (const VEC2 &v) { x += v.x; y += v.y; return *this; }
	VEC2 & operator -= (const VEC2 &v) { x -= v.x; y -= v.y; return *this; }
	VEC2 & operator *= (float v) { x *= v; y *= v; return *this; }
	VEC2 & operator /= (float v) { x /= v; y /= v; return *this; }

	float & operator [] (uint4 Index) { return v[Index]; }
	const float & operator [] (uint4 Index) const { return v[Index]; }

	float * GetArray() { return &x; }
	const float * GetArray() const { return &x; }
};

inline VEC2 operator * (const VEC2 pt, float v) { return VEC2(pt.x*v, pt.y*v); }
inline VEC2 operator / (const VEC2 pt, float v) { return VEC2(pt.x/v, pt.y/v); }
inline VEC2 operator * (float v, const VEC2 pt) { return VEC2(pt.x*v, pt.y*v); }
inline VEC2 operator / (float v, const VEC2 pt) { return VEC2(pt.x/v, pt.y/v); }

template <> inline ::D3DXVECTOR2 & math_cast<::D3DXVECTOR2, VEC2>(const VEC2 &x) { return (::D3DXVECTOR2&)(x); }
template <> inline VEC2 & math_cast<VEC2, ::D3DXVECTOR2>(const ::D3DXVECTOR2 &x) { return (VEC2&)(x); }

inline void swap(VEC2 &v1, VEC2 &v2)
{
	std::swap(v1.x, v2.x);
	std::swap(v1.y, v2.y);
}

struct VEC3
{
	union
	{
		struct
		{
			float x, y, z;
		};
		float v[3];
	};

	static const VEC3 ZERO;
	static const VEC3 ONE;
	static const VEC3 POSITIVE_X;
	static const VEC3 POSITIVE_Y;
	static const VEC3 POSITIVE_Z;
	static const VEC3 NEGATIVE_X;
	static const VEC3 NEGATIVE_Y;
	static const VEC3 NEGATIVE_Z;

	VEC3() { }
	VEC3(float x, float y, float z) : x(x), y(y), z(z) { }
	VEC3(float *Array) : x(Array[0]), y(Array[1]), z(Array[2]) { }

	bool operator == (const VEC3 &v) const { return x == v.x && y == v.y && z == v.z; }
	bool operator != (const VEC3 &v) const { return x != v.x || y != v.y || z != v.z; }

	VEC3 operator - () const { return VEC3(-x, -y, -z); }
	VEC3 operator + (const VEC3 &v) const { return VEC3(x+v.x, y+v.y, z+v.z); }
	VEC3 operator - (const VEC3 &v) const { return VEC3(x-v.x, y-v.y, z-v.z); }
	VEC3 & operator += (const VEC3 &v) { x += v.x; y += v.y; z += v.z; return *this; }
	VEC3 & operator -= (const VEC3 &v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
	VEC3 & operator *= (float v) { x *= v; y *= v; z *= v; return *this; }
	VEC3 & operator /= (float v) { x /= v; y /= v; z /= v; return *this; }

	float & operator [] (uint4 Index) { return v[Index]; }
	const float & operator [] (uint4 Index) const { return v[Index]; }

	float * GetArray() { return &x; }
	const float * GetArray() const { return &x; }

	bool operator<( const VEC3& v ) const
	{
		if (x == v.x)
		{
			if (y == v.y)
			{
				if (z == v.z)
				{
					return false;
				}
				else
				{
					return z < v.z;
				}
			}
			else
			{
				return y < v.y;
			}
		}
		else
		{
			return x < v.x;
		}
	}
};

inline VEC3 operator * (const VEC3 pt, float v) { return VEC3(pt.x*v, pt.y*v, pt.z*v); }
inline VEC3 operator / (const VEC3 pt, float v) { return VEC3(pt.x/v, pt.y/v, pt.z/v); }
inline VEC3 operator * (float v, const VEC3 pt) { return VEC3(pt.x*v, pt.y*v, pt.z*v); }
inline VEC3 operator / (float v, const VEC3 pt) { return VEC3(pt.x/v, pt.y/v, pt.z/v); }

template <> inline ::D3DXVECTOR3 & math_cast<::D3DXVECTOR3, VEC3>(const VEC3 &x) { return (::D3DXVECTOR3&)(x); }
template <> inline VEC3 & math_cast<VEC3, ::D3DXVECTOR3>(const ::D3DXVECTOR3 &x) { return (VEC3&)(x); }

inline void swap(VEC3 &v1, VEC3 &v2)
{
	std::swap(v1.x, v2.x);
	std::swap(v1.y, v2.y);
	std::swap(v1.z, v2.z);
}

struct VEC4
{
	union
	{
		struct
		{
			float x, y, z, w;
		};
		float v[4];
	};

	static const VEC4 ZERO;
	static const VEC4 ONE;
	static const VEC4 POSITIVE_X;
	static const VEC4 POSITIVE_Y;
	static const VEC4 POSITIVE_Z;
	static const VEC4 POSITIVE_W;
	static const VEC4 NEGATIVE_X;
	static const VEC4 NEGATIVE_Y;
	static const VEC4 NEGATIVE_Z;
	static const VEC4 NEGATIVE_W;

	VEC4() { }
	VEC4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) { }
	VEC4(float *Array) : x(Array[0]), y(Array[1]), z(Array[2]), w(Array[3]) { }

	bool operator == (const VEC4 &v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
	bool operator != (const VEC4 &v) const { return x != v.x || y != v.y || z != v.z || w != v.w; }

	VEC4 operator - () const { return VEC4(-x, -y, -z, -w); }
	VEC4 operator + (const VEC4 &v) const { return VEC4(x+v.x, y+v.y, z+v.z, w+v.w); }
	VEC4 operator - (const VEC4 &v) const { return VEC4(x-v.x, y-v.y, z-v.z, w-v.w); }
	VEC4 & operator += (const VEC4 &v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
	VEC4 & operator -= (const VEC4 &v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
	VEC4 & operator *= (float v) { x *= v; y *= v; z *= v; w *= v; return *this; }
	VEC4 & operator /= (float v) { x /= v; y /= v; z /= v; w /= v; return *this; }

	float & operator [] (uint4 Index) { return v[Index]; }
	const float & operator [] (uint4 Index) const { return v[Index]; }

	float * GetArray() { return &x; }
	const float * GetArray() const { return &x; }
};

inline VEC4 operator * (const VEC4 pt, float v) { return VEC4(pt.x*v, pt.y*v, pt.z*v, pt.w*v); }
inline VEC4 operator / (const VEC4 pt, float v) { return VEC4(pt.x/v, pt.y/v, pt.z/v, pt.w/v); }
inline VEC4 operator * (float v, const VEC4 pt) { return VEC4(pt.x*v, pt.y*v, pt.z*v, pt.w*v); }
inline VEC4 operator / (float v, const VEC4 pt) { return VEC4(pt.x/v, pt.y/v, pt.z/v, pt.w/v); }

template <> inline ::D3DXVECTOR4 & math_cast<::D3DXVECTOR4, VEC4>(const VEC4 &x) { return (::D3DXVECTOR4&)(x); }
template <> inline VEC4 & math_cast<VEC4, ::D3DXVECTOR4>(const ::D3DXVECTOR4 &x) { return (VEC4&)(x); }

inline void swap(VEC4 &v1, VEC4 &v2)
{
	std::swap(v1.x, v2.x);
	std::swap(v1.y, v2.y);
	std::swap(v1.z, v2.z);
	std::swap(v1.w, v2.w);
}

// Zwraca kwadrat d³ugoœci wektora
inline float LengthSq(const VEC2 &v)
{
	return v.x*v.x + v.y*v.y;
}
inline float LengthSq(const VEC3 &v)
{
	return v.x*v.x + v.y*v.y + v.z*v.z;
}
inline float LengthSq(const VEC4 &v)
{
	return v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w;
}

// Zwraca d³ugoœæ wektora
inline float Length(const VEC2 &v)
{
	return sqrtf(LengthSq(v));
}
inline float Length(const VEC3 &v)
{
	return sqrtf(LengthSq(v));
}
inline float Length(const VEC4 &v)
{
	return sqrtf(LengthSq(v));
}

// Zwraca wektor z³o¿ony z mniejszych sk³adowych podanych wektorów
inline void Min(VEC2 *Out, const VEC2 &v1, const VEC2 &v2)
{
	Out->x = std::min(v1.x, v2.x);
	Out->y = std::min(v1.y, v2.y);
}
inline void Min(VEC3 *Out, const VEC3 &v1, const VEC3 &v2)
{
	Out->x = std::min(v1.x, v2.x);
	Out->y = std::min(v1.y, v2.y);
	Out->z = std::min(v1.z, v2.z);
}
inline void Min(VEC4 *Out, const VEC4 &v1, const VEC4 &v2)
{
	Out->x = std::min(v1.x, v2.x);
	Out->y = std::min(v1.y, v2.y);
	Out->z = std::min(v1.z, v2.z);
	Out->w = std::min(v1.w, v2.w);
}

// Zwraca wektor z³o¿ony z wiêkszych sk³adowych podanych wektorów
inline void Max(VEC2 *Out, const VEC2 &v1, const VEC2 &v2)
{
	Out->x = std::max(v1.x, v2.x);
	Out->y = std::max(v1.y, v2.y);
}
inline void Max(VEC3 *Out, const VEC3 &v1, const VEC3 &v2)
{
	Out->x = std::max(v1.x, v2.x);
	Out->y = std::max(v1.y, v2.y);
	Out->z = std::max(v1.z, v2.z);
}
inline void Max(VEC4 *Out, const VEC4 &v1, const VEC4 &v2)
{
	Out->x = std::max(v1.x, v2.x);
	Out->y = std::max(v1.y, v2.y);
	Out->z = std::max(v1.z, v2.z);
	Out->w = std::max(v1.w, v2.w);
}

// Operacje na poszczególnych sk³adowych wektorów

inline void Mul(VEC2 *Out, const VEC2 &v, float f)
{
	Out->x = v.x * f;
	Out->y = v.y * f;
}
inline void Mul(VEC3 *Out, const VEC3 &v, float f)
{
	Out->x = v.x * f;
	Out->y = v.y * f;
	Out->z = v.z * f;
}
inline void Mul(VEC4 *Out, const VEC4 &v, float f)
{
	Out->x = v.x * f;
	Out->y = v.y * f;
	Out->z = v.z * f;
	Out->w = v.w * f;
}

inline void Add(VEC2 *Out, const VEC2 &v1, const VEC2 &v2)
{
	Out->x = v1.x + v2.x;
	Out->y = v1.y + v2.y;
}
inline void Add(VEC3 *Out, const VEC3 &v1, const VEC3 &v2)
{
	Out->x = v1.x + v2.x;
	Out->y = v1.y + v2.y;
	Out->z = v1.z + v2.z;
}
inline void Add(VEC4 *Out, const VEC4 &v1, const VEC4 &v2)
{
	Out->x = v1.x + v2.x;
	Out->y = v1.y + v2.y;
	Out->z = v1.z + v2.z;
	Out->w = v1.w + v2.w;
}

inline void Sub(VEC2 *Out, const VEC2 &v1, const VEC2 &v2)
{
	Out->x = v1.x - v2.x;
	Out->y = v1.y - v2.y;
}
inline void Sub(VEC3 *Out, const VEC3 &v1, const VEC3 &v2)
{
	Out->x = v1.x - v2.x;
	Out->y = v1.y - v2.y;
	Out->z = v1.z - v2.z;
}
inline void Sub(VEC4 *Out, const VEC4 &v1, const VEC4 &v2)
{
	Out->x = v1.x - v2.x;
	Out->y = v1.y - v2.y;
	Out->z = v1.z - v2.z;
	Out->w = v1.w - v2.w;
}

inline void Mul(VEC2 *Out, const VEC2 &v1, const VEC2 &v2)
{
	Out->x = v1.x * v2.x;
	Out->y = v1.y * v2.y;
}
inline void Mul(VEC3 *Out, const VEC3 &v1, const VEC3 &v2)
{
	Out->x = v1.x * v2.x;
	Out->y = v1.y * v2.y;
	Out->z = v1.z * v2.z;
}
inline void Mul(VEC4 *Out, const VEC4 &v1, const VEC4 &v2)
{
	Out->x = v1.x * v2.x;
	Out->y = v1.y * v2.y;
	Out->z = v1.z * v2.z;
	Out->w = v1.w * v2.w;
}

inline void Div(VEC2 *Out, const VEC2 &v1, const VEC2 &v2)
{
	Out->x = v1.x / v2.x;
	Out->y = v1.y / v2.y;
}
inline void Div(VEC3 *Out, const VEC3 &v1, const VEC3 &v2)
{
	Out->x = v1.x / v2.x;
	Out->y = v1.y / v2.y;
	Out->z = v1.z / v2.z;
}
inline void Div(VEC4 *Out, const VEC4 &v1, const VEC4 &v2)
{
	Out->x = v1.x / v2.x;
	Out->y = v1.y / v2.y;
	Out->z = v1.z / v2.z;
	Out->w = v1.w / v2.w;
}

// Iloczyn skalarny wektorów
inline float Dot(const VEC2 &v1, const VEC2 &v2)
{
	return v1.x*v2.x + v1.y*v2.y;
}
inline float Dot(const VEC3 &v1, const VEC3 &v2)
{
	return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
}
inline float Dot(const VEC4 &v1, const VEC4 &v2)
{
	return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z + v1.w*v2.w;
}

// Odwraca znak sk³adowych wektora
inline void Minus(VEC2 *Out, VEC2 &v) { Out->x = -v.x; Out->y = -v.y; }
inline void Minus(VEC3 *Out, VEC3 &v) { Out->x = -v.x; Out->y = -v.y; Out->z = -v.z; }
inline void Minus(VEC4 *Out, VEC4 &v) { Out->x = -v.x; Out->y = -v.y; Out->z = -v.z; Out->w = -v.w; }
inline void Minus(VEC2 *InOut) { InOut->x = -InOut->x; InOut->y = -InOut->y; }
inline void Minus(VEC3 *InOut) { InOut->x = -InOut->x; InOut->y = -InOut->y; InOut->z = -InOut->z; }
inline void Minus(VEC4 *InOut) { InOut->x = -InOut->x; InOut->y = -InOut->y; InOut->z = -InOut->z; InOut->w = -InOut->w; }

// Iloczyn wektorowy dwóch wektorów daj¹cy liczbê mówi¹c¹ czy drugi wskazuje
// na lewo, czy na prawo wzglêdem pierwszego
// W uk³adzie gdzie X jest w prawo a Y do góry, > 0 znaczy na lewo
// W uk³adzie gdzie X jest w prawo a Y w dó³, > 0 znaczy na prawo
inline float Cross(const VEC2 &v1, const VEC2 &v2)
{
	return v1.x*v2.y - v1.y*v2.x;
}

// Iloczyn wektorowy dwóch wektorów 3D
inline void Cross(VEC3 *Out, const VEC3 &v1, const VEC3 &v2)
{
	Out->x = v1.y * v2.z - v1.z * v2.y;
	Out->y = v1.z * v2.x - v1.x * v2.z;
	Out->z = v1.x * v2.y - v1.y * v2.x;
}

// Kwadrat odleg³oœci miêdzy dwoma punktami
inline float DistanceSq(const VEC2 &p1, const VEC2 &p2)
{
	return LengthSq(p2-p1);
}
inline float DistanceSq(const VEC3 &p1, const VEC3 &p2)
{
	return LengthSq(p2-p1);
}
inline float DistanceSq(const VEC4 &p1, const VEC4 &p2)
{
	return LengthSq(p2-p1);
}

// Odleg³oœæ miêdzy dwoma punktami
inline float Distance(const VEC2 &p1, const VEC2 &p2)
{
	return Length(p2-p1);
}
inline float Distance(const VEC3 &p1, const VEC3 &p2)
{
	return Length(p2-p1);
}
inline float Distance2D(const VEC3& p1, const VEC3& p2)
{
	return Distance(VEC2(p1.x,p1.z), VEC2(p2.x,p2.z));
}
inline float Distance(const VEC4 &p1, const VEC4 &p2)
{
	return Length(p2-p1);
}

// Liniowa interpolacja wektorów
inline void Lerp(VEC2 *Out, const VEC2 &v1, const VEC2 &v2, float t)
{
	Out->x = v1.x + t*(v2.x-v1.x);
	Out->y = v1.y + t*(v2.y-v1.y);
}
inline void Lerp(VEC3 *Out, const VEC3 &v1, const VEC3 &v2, float t)
{
	Out->x = v1.x + t*(v2.x-v1.x);
	Out->y = v1.y + t*(v2.y-v1.y);
	Out->z = v1.z + t*(v2.z-v1.z);
}
inline void Lerp(VEC4 *Out, const VEC4 &v1, const VEC4 &v2, float t)
{
	Out->x = v1.x + t*(v2.x-v1.x);
	Out->y = v1.y + t*(v2.y-v1.y);
	Out->z = v1.z + t*(v2.z-v1.z);
	Out->w = v1.w + t*(v2.w-v1.w);
}

// Normalizuje wektor, czyli zmienia mu d³ugoœæ na 1
inline void Normalize(VEC2 *Out, const VEC2 &v)
{
	float l = 1.0f / Length(v);
	Out->x = v.x * l;
	Out->y = v.y * l;
}
inline void Normalize(VEC3 *Out, const VEC3 &v)
{
	float l = 1.0f / Length(v);
	Out->x = v.x * l;
	Out->y = v.y * l;
	Out->z = v.z * l;
}
inline void Normalize(VEC4 *Out, const VEC4 &v)
{
	float l = 1.0f / Length(v);
	Out->x = v.x * l;
	Out->y = v.y * l;
	Out->z = v.z * l;
	Out->w = v.w * l;
}

// Normalizuje wektor w miejscu
inline void Normalize(VEC2 *v)
{
	float rcl = 1.0f / Length(*v);
	v->x *= rcl;
	v->y *= rcl;
}
inline void Normalize(VEC3 *v)
{
	float rcl = 1.0f / Length(*v);
	v->x *= rcl;
	v->y *= rcl;
	v->z *= rcl;
}
inline void Normalize(VEC4 *v)
{
	float rcl = 1.0f / Length(*v);
	v->x *= rcl;
	v->y *= rcl;
	v->z *= rcl;
	v->w *= rcl;
}

// Konwersja z wektora 3D na 4D dopisuj¹ca W=1
inline void Vec3ToVec4(VEC4 *Out, const VEC3 &v)
{
	Out->x = v.x;
	Out->y = v.y;
	Out->z = v.z;
	Out->w = 1.0f;
}

// Konwersja z wektora 4D na 3D ze zignorowaniem sk³adowej W
inline void Vec4ToVec3_Ignore(VEC3 *Out, const VEC4 &v)
{
	Out->x = v.x;
	Out->y = v.y;
	Out->z = v.z;
}
// Konwersja z wektora 4D na 3D ze podzieleniem sk³adowych przez W
inline void Vec4ToVec3_Div(VEC3 *Out, const VEC4 &v)
{
	float rcw = 1.0f / v.w;
	Out->x = v.x * rcw;
	Out->y = v.y * rcw;
	Out->z = v.z * rcw;
}

// Zwraca punkt w po³owie drogi miêdzy podanymi punktami
inline void MidPoint(VEC2 *Out, const VEC2 &p1, const VEC2 &p2)
{
	Out->x = (p1.x+p2.x) * 0.5f;
	Out->y = (p1.y+p2.y) * 0.5f;
}
inline void MidPoint(VEC3 *Out, const VEC3 &p1, const VEC3 &p2)
{
	Out->x = (p1.x+p2.x) * 0.5f;
	Out->y = (p1.y+p2.y) * 0.5f;
	Out->z = (p1.z+p2.z) * 0.5f;
}
inline void MidPoint(VEC4 *Out, const VEC4 &p1, const VEC4 &p2)
{
	Out->x = (p1.x+p2.x) * 0.5f;
	Out->y = (p1.y+p2.y) * 0.5f;
	Out->z = (p1.z+p2.z) * 0.5f;
	Out->w = (p1.w+p2.w) * 0.5f;
}

// Zwraca true, jeœli podane wektory/punkty s¹ sobie mniej wiêcej równe
inline bool VecEqual(const VEC2 &v1, const VEC2 &v2) {
	return float_equal(v1.x, v2.x) && float_equal(v1.y, v2.y);
}
inline bool VecEqual(const VEC3 &v1, const VEC3 &v2) {
	return float_equal(v1.x, v2.x) && float_equal(v1.y, v2.y) && float_equal(v1.z, v2.z);
}
inline bool VecEqual(const VEC4 &v1, const VEC4 &v2) {
	return float_equal(v1.x, v2.x) && float_equal(v1.y, v2.y) && float_equal(v1.z, v2.z) && float_equal(v1.w, v2.w);
}

// Zwraca true, jeœli wszystkie sk³adowe podanego wektora s¹ odpowiednio
// mniejsze / wiêksze / mniejsze lub równe / wiêksze lub równe od drugiego

inline bool Less(const VEC2 &v1, const VEC2 &v2)
{
	return v1.x < v2.x && v1.y < v2.y;
}
inline bool Less(const VEC3 &v1, const VEC3 &v2)
{
	return v1.x < v2.x && v1.y < v2.y && v1.z < v2.z;
}
inline bool Less(const VEC4 &v1, const VEC4 &v2)
{
	return v1.x < v2.x && v1.y < v2.y && v1.z < v2.z && v1.w < v2.w;
}

inline bool LessOrEqual(const VEC2 &v1, const VEC2 &v2)
{
	return v1.x <= v2.x && v1.y <= v2.y;
}
inline bool LessOrEqual(const VEC3 &v1, const VEC3 &v2)
{
	return v1.x <= v2.x && v1.y <= v2.y && v1.z <= v2.z;
}
inline bool LessOrEqual(const VEC4 &v1, const VEC4 &v2)
{
	return v1.x <= v2.x && v1.y <= v2.y && v1.z <= v2.z && v1.w <= v2.w;
}

inline bool Greater(const VEC2 &v1, const VEC2 &v2)
{
	return v1.x > v2.x && v1.y > v2.y;
}
inline bool Greater(const VEC3 &v1, const VEC3 &v2)
{
	return v1.x > v2.x && v1.y > v2.y && v1.z > v2.z;
}
inline bool Greater(const VEC4 &v1, const VEC4 &v2)
{
	return v1.x > v2.x && v1.y > v2.y && v1.z > v2.z && v1.w > v2.w;
}

inline bool GreaterOrEqual(const VEC2 &v1, const VEC2 &v2)
{
	return v1.x >= v2.x && v1.y >= v2.y;
}
inline bool GreaterOrEqual(const VEC3 &v1, const VEC3 &v2)
{
	return v1.x >= v2.x && v1.y >= v2.y && v1.z >= v2.z;
}
inline bool GreaterOrEqual(const VEC4 &v1, const VEC4 &v2)
{
	return v1.x >= v2.x && v1.y >= v2.y && v1.z >= v2.z && v1.w >= v2.w;
}

inline bool around(const VEC2 &v1, const VEC2 &v2, float Epsilon) {
	return around(v1.x, v2.x, Epsilon) && around(v1.y, v2.y, Epsilon);
}
inline bool around(const VEC3 &v1, const VEC3 &v2, float Epsilon) {
	return around(v1.x, v2.x, Epsilon) && around(v1.y, v2.y, Epsilon) && around(v1.z, v2.z, Epsilon);
}
inline bool around(const VEC4 &v1, const VEC4 &v2, float Epsilon) {
	return around(v1.x, v2.x, Epsilon) && around(v1.y, v2.y, Epsilon) && around(v1.z, v2.z, Epsilon) && around(v1.w, v2.w, Epsilon);
}

// Zwraca wektor prostopad³y do podanego wektora 2D
// W uk³adzie w którym X jest w prawo a Y do góry wektor le¿y na lewo od danego.
// W uk³adzie w którym X jest w prawo a Y w dó³, le¿y na prawo.
inline void Perpedicular(VEC2 *Out, const VEC2 &v)
{
	Out->x = -v.y;
	Out->y = v.x;
}

// Oblicza wektor kierunku odbicia od p³aszczyzny o podanej normalnej
// v wskazuje na kierunek *do* tej p³aszczyzny.
// PlaneNormal musi byæ znormalizowany.
inline void Reflect(VEC2 *Out, const VEC2 &v, const VEC2 &PlaneNormal)
{
	// Wzór: *Out = v - ( 2.0f * Dot(v, PlaneNormal) * PlaneNormal );
	float f = 2.0f * Dot(v, PlaneNormal);
	Out->x = v.x - f * PlaneNormal.x;
	Out->y = v.y - f * PlaneNormal.y;
}
inline void Reflect(VEC3 *Out, const VEC3 &v, const VEC3 &PlaneNormal)
{
	// Wzór: *Out = v - ( 2.0f * Dot(v, PlaneNormal) * PlaneNormal );
	float f = 2.0f * Dot(v, PlaneNormal);
	Out->x = v.x - f * PlaneNormal.x;
	Out->y = v.y - f * PlaneNormal.y;
	Out->z = v.z - f * PlaneNormal.z;
}

// Rzutowanie wektora V na wektor N
// Wektor wyjœciowy Out bêdzie równoleg³y do N.
// Jeœli chcemy roz³o¿yæ wektor V na wektor prostopad³y i równoleg³y do N,
// to wektorem równoleg³ym jest Out, a prostopad³y obliczamy jako V - Out.
inline void Project(VEC2 *Out, const VEC2 &V, const VEC2 &N)
{
	// Oryginalnie:
	// *Out = N * ( Dot(V, N) / LengthSq(N) );
	// Wersja szybsza:
	float T = (V.x*N.x + V.y*N.y) / (N.x*N.x + N.y*N.y);
	Out->x = N.x * T;
	Out->y = N.y * T;
}
inline void Project(VEC3 *Out, const VEC3 &V, const VEC3 &N)
{
	float T = (V.x*N.x + V.y*N.y + V.z*N.z) / (N.x*N.x + N.y*N.y + N.z*N.z);
	Out->x = N.x * T;
	Out->y = N.y * T;
	Out->z = N.z * T;
}
// Wersja kiedy wektor N jest ju¿ na pewno znormalizowany.
inline void Project_N(VEC2 *Out, const VEC2 &V, const VEC2 &N)
{
	// Oryginalnie:
	// *Out = N * Dot(V, N);
	// Wersja szybsza:
	float T = V.x*N.x + V.y*N.y;
	Out->x = N.x * T;
	Out->y = N.y * T;
}
inline void Project_N(VEC3 *Out, const VEC3 &V, const VEC3 &N)
{
	float T = V.x*N.x + V.y*N.y + V.z*N.z;
	Out->x = N.x * T;
	Out->y = N.y * T;
	Out->z = N.z * T;
}

// Ortogonalizacja trzech wektorów
// - Po tym przekszta³ceniu bêd¹ do siebie prostopad³e.
// - Wektory wejœciowe nie musz¹ byæ znormalizowane.
// - Wektory wyjœciowe niekoniecznie bêd¹ znormalizowane (nawet jeœli wejœciowe by³y).
// > Metoda szybka
void Orthogonalize_Fast(VEC3 *OutR1, VEC3 *OutR2, VEC3 *OutR3, const VEC3 &R1, const VEC3 &R2, const VEC3 &R3);
// > Metoda dok³adna
void Orthogonalize(VEC3 *OutR1, VEC3 *OutR2, VEC3 *OutR3, const VEC3 &R1, const VEC3 &R2, const VEC3 &R3);

// Wyznacza dwa dowolne wektory prostopad³e do podanego
// - v nie musi byæ znormalizowany.
// - Zwrócone wektory nie s¹ znormalizowane.
void PerpedicularVectors(VEC3 *Out1, VEC3 *Out2, const VEC3 &v);


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// TRÓJK¥T

// Oblicza signed pole powierzchni trójk¹ta
// Wychodzi dodanie, kiedy wierzcho³ki s¹ w kierunku wskazówek zegara.
float TriangleArea(const VEC2 &p1, const VEC2 &p2, const VEC2 &p3);
float TriangleArea(const VEC3 &p1, const VEC3 &p2, const VEC3 &p3);

// Znajduje obwód trójk¹ta
float TrianglePerimeter(const VEC2 &p1, const VEC2 &p2, const VEC2 &p3);
float TrianglePerimeter(const VEC3 &p1, const VEC3 &p2, const VEC3 &p3);

// Oblicza punkt na powierzhchni trójk¹ta we wsp. barycentrycznych
// f to waga punktu p2, g to waga punktu p3.
// Wzór: *Out = p1 + f*(p2-p1) + g*(p3-p1)
// Inny wzór: *Out = p1*(1-f-g) + p2*f + p3*g
inline void Barycentric(VEC2 *Out, const VEC2 &p1, const VEC2 &p2, const VEC2 &p3, float f, float g)
{
	float e = 1.0f-f-g;
	Out->x = e*p1.x + f*p2.x + g*p3.x;
	Out->y = e*p1.y + f*p2.y + g*p3.y;
}
inline void Barycentric(VEC3 *Out, const VEC3 &p1, const VEC3 &p2, const VEC3 &p3, float f, float g)
{
	// Stara wersja, na podstawie DX SDK, du¿o wolniejsza:
	//VEC3 Diff2 = p2-p1;
	//VEC3 Diff3 = p3-p1;
	//Out->x = p1.x + f*Diff2.x + g*Diff3.x;
	//Out->y = p1.y + f*Diff2.y + g*Diff3.y;
	//Out->z = p1.z + f*Diff2.z + g*Diff3.z;

	float e = 1.0f-f-g;
	Out->x = e*p1.x + f*p2.x + g*p3.x;
	Out->y = e*p1.y + f*p2.y + g*p3.y;
	Out->z = e*p1.z + f*p2.z + g*p3.z;
}
inline void Barycentric(VEC4 *Out, const VEC4 &p1, const VEC4 &p2, const VEC4 &p3, float f, float g)
{
	float e = 1.0f-f-g;
	Out->x = e*p1.x + f*p2.x + g*p3.x;
	Out->y = e*p1.y + f*p2.y + g*p3.y;
	Out->z = e*p1.z + f*p2.z + g*p3.z;
	Out->w = e*p1.w + f*p2.w + g*p3.w;
}

// Oblicz wspó³rzêdne barycentryczne podanego punktu p w ramach trójk¹ta
// p1-p2-p3. Punkt musi na pewno le¿eæ na p³aszczyŸnie tego trójk¹ta.
// Normal mo¿na podaæ jeœli mamy ju¿ obliczony jako Cross(v[1]-v[0], v[2]-v[1]) (nie musi byæ znormalizowany) - ale nie trzeba.
void CalculateBarycentric(const VEC3 &p, const VEC3 &p1, const VEC3 &p2, const VEC3 &p3, const VEC3 *Normal, float *e, float *f, float *g);

// Znajduje okr¹g wpisany w trójk¹t
void TriangleInscribedCircle(VEC2 *OutCenter, float *OutRadius, const VEC2 &v1, const VEC2 &v2, const VEC2 &v3);
void TriangleInscribedCircle(VEC3 *OutCenter, float *OutRadius, const VEC3 &v1, const VEC3 &v2, const VEC3 &v3);
// Znajduje okr¹g opisany na trójk¹cie
void TriangleCircumscribedCircle(VEC2 *OutCenter, float *OutRadius, const VEC2 &v1, const VEC2 &v2, const VEC2 &v3);
void TriangleCircumscribedCircle(VEC3 *OutCenter, float *OutRadius, const VEC3 &v1, const VEC3 &v2, const VEC3 &v3);


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// PROMIEÑ

// Zwraca punkt na promieniu
inline void PointOnRay(VEC2 *Out, const VEC2 &Origin, const VEC2 &Direction, float t)
{
	Out->x = Origin.x + t * Direction.x;
	Out->y = Origin.y + t * Direction.y;
}
inline void PointOnRay(VEC3 *Out, const VEC3 &Origin, const VEC3 &Direction, float t)
{
	Out->x = Origin.x + t * Direction.x;
	Out->y = Origin.y + t * Direction.y;
	Out->z = Origin.z + t * Direction.z;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// PROSTOK¥T

/*
RECTI jest kompatybilny ze struktur¹ RECT z Win32API.
Konwersja do i ze stringa: "left,top,right,bottom", np. "1,2,3,4".
Niezale¿nie od uk³adu wspó³rzêdnych, prawid³owy prostok¹t jest wtedy, kiedy top
jest mniejsze co do wartoœci od bottom, a left od right.
*/

struct RECTI
{
	union
	{
		struct
		{
			int left;
			int top;
			int right;
			int bottom;
		};
		int v[4];
	};

	static const RECTI ZERO;

	RECTI() { }
	RECTI(int left, int top, int right, int bottom) : left(left), top(top), right(right), bottom(bottom) { }

	bool operator == (const RECTI &r) const { return left == r.left && top == r.top && right == r.right && bottom == r.bottom; }
	bool operator != (const RECTI &r) const { return left != r.left || top != r.top || right != r.right || bottom != r.bottom; }

	RECTI operator - () const { return RECTI(-right, -bottom, -left, -top); }
	RECTI operator + (const POINT_ &p) const { return RECTI(left+p.x, top+p.y, right+p.x, bottom+p.y); }
	RECTI operator - (const POINT_ &p) const { return RECTI(left-p.x, top-p.y, right-p.x, bottom-p.y); }
	RECTI operator * (int s) const { return RECTI(left*s, top*s, right*s, bottom*s); }
	RECTI operator * (const POINT_ &s) const { return RECTI(left*s.x, top*s.y, right*s.x, bottom*s.y); }
	RECTI & operator += (const POINT_ &p) { left += p.x; top += p.y; right += p.x; bottom += p.y; return *this; }
	RECTI & operator -= (const POINT_ &p) { left -= p.x; top -= p.y; right -= p.x; bottom -= p.y; return *this; }
	RECTI & operator *= (int s) { left *= s; top *= s; right *= s; bottom *= s; return *this; }
	RECTI & operator *= (const POINT_ &s) { left *= s.x; top *= s.y; right *= s.x; bottom *= s.y; return *this; }

	bool IsValid() const { return left < right && top < bottom; }
	void Repair() { if (right < left) std::swap(left, right); if (bottom < top) std::swap(top, bottom); }

	void LeftTop(POINT_ *Out) const { Out->x = left; Out->y = top; }
	void RightBottom(POINT_ *Out) const { Out->x = right; Out->y = bottom; }

	int & operator [] (uint4 Index) { return v[Index]; }
	const int & operator [] (uint4 Index) const { return v[Index]; }

	int * GetArray() { return &left; }
	const int * GetArray() const { return &left; }

	// Powiêksza prostok¹t z ka¿dej strony o podan¹ wartoœæ
	void Extend(int d) { left -= d; top -= d; right += d; bottom += d; }
	// Oblicza i zwraca szerokoœæ
	int Width() const { return right - left; }
	// Oblicza i zwraca wysokoœæ
	int Height() const { return bottom - top; }
	// Oblicza i zwraca d³ugoœæ przek¹tnej
	float Diagonal() const { return sqrtf( (float)((right-left)*(right-left)) + (float)((bottom-top)*(bottom-top)) ); }
	// Oblicza i zwraca pole powierzchni
	int Field() const { return (right-left)*(bottom-top); }
	// Zmienia szerokoœæ na podan¹
	void SetWidth(int NewWidth) { right = left + NewWidth; }
	// Zmienia wysokoœæ na podan¹
	void SetHeight(int NewHeight) { bottom = top + NewHeight; }
	// Zmienia szerokoœæ i wysokoœæ na podan¹
	void SetSize(int NewWidth, int NewHeight) { right = left + NewWidth; bottom = top + NewHeight; }
	// Oblicza i zwraca pozycjê œrodka
	void GetCenter(POINT_ *Out) const { Out->x = (left+right)/2; Out->y = (top+bottom)/2; }
	int GetCenterX() const { return (left+right)/2; }
	int GetCenterY() const { return (top+bottom)/2; }
	// Zwraca true, jeœli prostok¹t jest zerowy
	bool Empty() const { return left == 0 && top == 0 && right == 0 && bottom == 0; }
	// Czyœci prostok¹t na zerowy
	void Clear() { left = top = right = bottom = 0; }
};

struct RECTF
{
	union
	{
		struct
		{
			float left;
			float top;
			float right;
			float bottom;
		};
		float v[4];
	};

	static const RECTF ZERO;

	RECTF() { }
	RECTF(float left, float top, float right, float bottom) : left(left), top(top), right(right), bottom(bottom) { }

	bool operator == (const RECTF &r) const { return left == r.left && top == r.top && right == r.right && bottom == r.bottom; }
	bool operator != (const RECTF &r) const { return left != r.left || top != r.top || right != r.right || bottom != r.bottom; }

	RECTF operator - () const { return RECTF(-right, -bottom, -left, -top); }
	RECTF operator + (const VEC2 &p) const { return RECTF(left+p.x, top+p.y, right+p.x, bottom+p.y); }
	RECTF operator - (const VEC2 &p) const { return RECTF(left-p.x, top-p.y, right-p.x, bottom-p.y); }
	RECTF operator * (float s) const { return RECTF(left*s, top*s, right*s, bottom*s); }
	RECTF operator * (const VEC2 &s) const { return RECTF(left*s.x, top*s.y, right*s.x, bottom*s.y); }
	RECTF & operator += (const VEC2 &p) { left += p.x; top += p.y; right += p.x; bottom += p.y; return *this; }
	RECTF & operator -= (const VEC2 &p) { left -= p.x; top -= p.y; right -= p.x; bottom -= p.y; return *this; }
	RECTF & operator *= (float s) { left *= s; top *= s; right *= s; bottom *= s; return *this; }
	RECTF & operator *= (const VEC2 &s) { left *= s.x; top *= s.y; right *= s.x; bottom *= s.y; return *this; }

	bool IsValid() const { return left < right && top < bottom; }
	void Repair() { if (right < left) std::swap(left, right); if (bottom < top) std::swap(top, bottom); }

	void LeftTop(VEC2 *Out) const { Out->x = left; Out->y = top; }
	void RightBottom(VEC2 *Out) const { Out->x = right; Out->y = bottom; }

	float & operator [] (uint4 Index) { return v[Index]; }
	const float & operator [] (uint4 Index) const { return v[Index]; }

	float * GetArray() { return &left; }
	const float * GetArray() const { return &left; }

	// Powiêksza prostok¹t z ka¿dej strony o podan¹ wartoœæ
	void Extend(float d) { left -= d; top -= d; right += d; bottom += d; }
	// Oblicza i zwraca szerokoœæ
	float Width() const { return right - left; }
	// Oblicza i zwraca wysokoœæ
	float Height() const { return bottom - top; }
	// Oblicza i zwraca d³ugoœæ przek¹tnej
	float Diagonal() const { return sqrtf( (right-left)*(right-left) + (bottom-top)*(bottom-top) ); }
	// Oblicza i zwraca pole powierzchni
	float Field() const { return (right-left)*(bottom-top); }
	// Zmienia szerokoœæ na podan¹
	void SetWidth(float NewWidth) { right = left + NewWidth; }
	// Zmienia wysokoœæ na podan¹
	void SetHeight(float NewHeight) { bottom = top + NewHeight; }
	// Zmienia szerokoœæ i wysokoœæ na podan¹
	void SetSize(float NewWidth, float NewHeight) { right = left + NewWidth; bottom = top + NewHeight; }
	// Oblicza i zwraca pozycjê œrodka
	void GetCenter(VEC2 *Out) const { Out->x = (left+right)*0.5f; Out->y = (top+bottom)*0.5f; }
	float GetCenterX() const { return (left+right)*0.5f; }
	float GetCenterY() const { return (top+bottom)*0.5f; }
	// Zwraca true, jeœli prostok¹t jest zerowy
	bool Empty() const { return left == 0.0f && top == 0.0f && right == 0.0f && bottom == 0.0f; }
	// Czyœci prostok¹t na zerowy
	void Clear() { left = top = right = bottom = 0.0f; }
};

// Konwersja do i z RECT z Win32API
template <> inline ::RECT & math_cast<::RECT, RECTI>(const RECTI &x) { return (::RECT&)(x); }
template <> inline RECTI & math_cast<RECTI, ::RECT>(const ::RECT &x) { return (RECTI&)(x); }

inline void swap(RECTI &r1, RECTI &r2)
{
	std::swap(r1.left, r2.left); std::swap(r1.top, r2.top);
	std::swap(r1.right, r2.right); std::swap(r1.bottom, r2.bottom);
}

inline void swap(RECTF &r1, RECTF &r2)
{
	std::swap(r1.left, r2.left); std::swap(r1.top, r2.top);
	std::swap(r1.right, r2.right); std::swap(r1.bottom, r2.bottom);
}

// Zwraca true, jeœli punkt nale¿y do prostok¹ta
inline bool PointInRect(const POINT_ &p, const RECTI &r)
{
	return
		(p.x >= r.left) &&
		(p.x <= r.right) &&
		(p.y >= r.top) &&
		(p.y <= r.bottom);
}
inline bool PointInRect(const VEC2 &p, const RECTF &r)
{
	return
		(p.x >= r.left) &&
		(p.x <= r.right) &&
		(p.y >= r.top) &&
		(p.y <= r.bottom);
}

// Zwraca true, jeœli prostok¹t sr zawiera siê w ca³oœci wewn¹trz r
inline bool RectInRect(const RECTI &sr, const RECTI &r)
{
	return
		(sr.left >= r.left) &&
		(sr.right <= r.right) &&
		(sr.top >= r.top) &&
		(sr.bottom <= r.bottom);
}
inline bool RectInRect(const RECTF &sr, const RECTF &r)
{
	return
		(sr.left >= r.left) &&
		(sr.right <= r.right) &&
		(sr.top >= r.top) &&
		(sr.bottom <= r.bottom);
}

// Zwraca true, jeœli podane prostok¹ty zachodz¹ na siebie
inline bool OverlapRect(const RECTI &r1, const RECTI &r2)
{
	return
		(r1.left <= r2.right) &&
		(r1.right >= r2.left) &&
		(r1.top <= r2.bottom) &&
		(r1.bottom >= r2.top);
}
inline bool OverlapRect(const RECTF &r1, const RECTF &r2)
{
	return
		(r1.left <= r2.right) &&
		(r1.right >= r2.left) &&
		(r1.top <= r2.bottom) &&
		(r1.bottom >= r2.top);
}

// Jeœli prostok¹ty posiadaj¹ czêœæ wspóln¹, zwraca j¹ przez Out i zwraca true
inline bool Intersection(RECTI *Out, const RECTI &r1, const RECTI &r2)
{
	*Out = RECTI(
		std::max(r1.left, r2.left),
		std::max(r1.top, r2.top),
		std::min(r1.right, r2.right),
		std::min(r1.bottom, r2.bottom));
	return Out->IsValid();
}
inline bool Intersection(RECTF *Out, const RECTF &r1, const RECTF &r2)
{
	*Out = RECTF(
		std::max(r1.left, r2.left),
		std::max(r1.top, r2.top),
		std::min(r1.right, r2.right),
		std::min(r1.bottom, r2.bottom));
	return Out->IsValid();
}

// Zwraca prostok¹t opisany na podanych dwóch prostok¹tach
inline void Union(RECTI *Out, const RECTI &r1, const RECTI &r2)
{
	Out->left = std::min(r1.left, r2.left);
	Out->top = std::min(r1.top, r2.top);
	Out->right = std::max(r1.right, r2.right);
	Out->bottom = std::max(r1.bottom, r2.bottom);
}
inline void Union(RECTF *Out, const RECTF &r1, const RECTF &r2)
{
	Out->left = std::min(r1.left, r2.left);
	Out->top = std::min(r1.top, r2.top);
	Out->right = std::max(r1.right, r2.right);
	Out->bottom = std::max(r1.bottom, r2.bottom);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// AXIS ALIGNED BOUNDING BOX

/*
Sk³adnia do konwersji na i z ³añcuch: p1x,p1y,p1z;p2x,p2y,p2z, np.:
"1,2,3;4,5,6"
*/

struct BOX
{
	VEC3 p1;
	VEC3 p2;

	static const BOX ZERO;

	BOX() { }
	BOX(const VEC3 &p1, const VEC3 &p2) : p1(p1), p2(p2) { }
	BOX(float p1x, float p1y, float p1z, float p2x, float p2y, float p2z) : p1(p1x, p1y, p1z), p2(p2x, p2y, p2z) { }

	bool operator == (const BOX &p) const { return p1 == p.p1 && p2 == p.p2; }
	bool operator != (const BOX &p) const { return p1 != p.p1 || p2 != p.p2; }

	BOX operator - () const { return BOX(-p2, -p1); }
	BOX operator + (const VEC3 &v) const { return BOX(p1+v, p2+v); }
	BOX operator - (const VEC3 &v) const { return BOX(p1-v, p2-v); }
	BOX operator * (float f) const { return BOX(p1*f, p2*f); }
	BOX operator * (const VEC3 &s) const { return BOX(VEC3(p1.x*s.x, p1.y*s.y, p1.z*s.z), VEC3(p2.x*s.x, p2.y*s.y, p2.z*s.z)); }
	BOX & operator += (const VEC3 &v) { p1 += v; p2 += v; return *this; }
	BOX & operator -= (const VEC3 &v) { p1 -= v; p2 -= v; return *this; }
	BOX & operator *= (float f) { p1 *= f; p2 *= f; return *this; }
	BOX & operator *= (const VEC3 &s) { p1.x *= s.x; p1.y *= s.y; p1.z *= s.z; p2.x *= s.x; p2.y *= s.y; p2.z *= s.z; return *this; }

	bool IsValid() { return Less(p1, p2); }
	void Repair() { if (p2.x < p1.x) std::swap(p1.x, p2.x); if (p2.y < p1.y) std::swap(p1.y, p2.y); if (p2.z < p1.z) std::swap(p1.z, p2.z); }

	float * GetArray() { return &p1.x; }
	const float * GetArray() const { return &p1.x; }

	float & operator [] (uint4 Index) { return GetArray()[Index]; }
	const float & operator [] (uint4 Index) const { return GetArray()[Index]; }

	// Powiêksza boks z ka¿dej strony o podan¹ wartoœæ
	void Extend(float d) { p1.x -= d; p1.y -= d; p1.z -= d; p2.x += d; p2.y += d; p2.z += d; }
	// Oblicza i zwraca rozmiary boksa w trzech wymiarach
	void GetSize(VEC3 *Out) const { Out->x = p2.x-p1.x; Out->y = p2.y-p1.y; Out->z = p2.z-p1.z; }
	// Oblicza szerokoœæ, wysokoœæ lub g³êbokoœæ
	float GetCX() const { return p2.x - p1.x; }
	float GetCY() const { return p2.y - p1.y; }
	float GetCZ() const { return p2.z - p1.z; }
	// Zmienia wielkoœæ na podan¹
	void SetSize(const VEC3 &NewSize) { p2.x = p1.x + NewSize.x; p2.y = p1.y + NewSize.y; p2.z = p1.z + NewSize.z; }
	// Oblicza i zwraca d³ugoœæ przek¹tnej
	float Diagonal() const { return sqrtf( (p2.x-p1.x)*(p2.x-p1.x) + (p2.y-p1.y)*(p2.y-p1.y) + (p2.z-p1.z)*(p2.z-p1.z) ); }
	// Oblicza i zwraca objêtoœæ
	float Volume() const { return (p2.x-p1.x)*(p2.y-p1.y)*(p2.z-p1.z); }
	// Oblicza i zwraca pozycjê œrodka
	void CalcCenter(VEC3 *Out) const { Out->x = (p1.x+p2.x)*0.5f; Out->y = (p1.y+p2.y)*0.5f; Out->z = (p1.z+p2.z)*0.5f; }
	// Zwraca true, jeœli prostok¹t jest zerowy
	bool Empty() const { return *this == ZERO; }
	// Czyœci prostok¹t na zerowy
	void Clear() { *this = ZERO; }
	// Zwraca wybrany wierzcho³ek
	void GetCorner(VEC3 *Out, int i) const
	{
		Out->x = (i & 1) ? p2.x : p1.x;
		Out->y = (i & 2) ? p2.y : p1.y;
		Out->z = (i & 4) ? p2.z : p1.z;
	}
	// Zwraca wszystkie 6 wierzcho³ków w kolejnoœci:
	// p1x, p1y, p1z; p2x, p1y, p1z; p1x, p2y, p1z; p2x, p2y, p1z
	// p1x, p1y, p2z; p2x, p1y, p2z; p1x, p2y, p2z; p2x, p2y, p2z
	// Trzeba podaæ wskaŸnik do tablicy.
	void GetAllCorners(VEC3 *Out) const
	{
		Out[0] = VEC3(p1.x, p1.y, p1.z); Out[1] = VEC3(p2.x, p1.y, p1.z); Out[2] = VEC3(p1.x, p2.y, p1.z); Out[3] = VEC3(p2.x, p2.y, p1.z);
		Out[4] = VEC3(p1.x, p1.y, p2.z); Out[5] = VEC3(p2.x, p1.y, p2.z); Out[6] = VEC3(p1.x, p2.y, p2.z); Out[7] = VEC3(p2.x, p2.y, p2.z);
	}
	// Powiêksza tak ¿eby na pewno zawiera³ podany punkt
	void AddInternalPoint(const VEC3 &p)
	{
		p1.x = std::min(p1.x, p.x); p1.y = std::min(p1.y, p.y); p1.z = std::min(p1.z, p.z);
		p2.x = std::max(p2.x, p.x); p2.y = std::max(p2.y, p.y); p2.z = std::max(p2.z, p.z);
	}
	// Powiêksza tak ¿eby na pewno zawiera³ podany boks
	// Podany boks musi byæ prawid³owy.
	void AddInternalBox(const BOX &b)
	{
		p1.x = std::min(p1.x, b.p1.x); p1.y = std::min(p1.y, b.p1.y); p1.z = std::min(p1.z, b.p1.z);
		p2.x = std::max(p2.x, b.p2.x); p2.y = std::max(p2.y, b.p2.y); p2.z = std::max(p2.z, b.p2.z);
	}
};

// Skaluje boks o podan¹ liczbê
inline void Scale(BOX *Out, const BOX &b, float s)
{
	Out->p1.x = b.p1.x * s; Out->p1.y = b.p1.y * s; Out->p1.z = b.p1.z * s;
	Out->p2.x = b.p2.x * s; Out->p2.y = b.p2.y * s; Out->p2.z = b.p2.z * s;
}

// Skaluje boks o podane 3 wymiary
inline void Scale(BOX *Out, const BOX &b, const VEC3 &s)
{
	Out->p1.x = b.p1.x * s.x; Out->p1.y = b.p1.y * s.y; Out->p1.z = b.p1.z * s.z;
	Out->p2.x = b.p2.x * s.x; Out->p2.y = b.p2.y * s.y; Out->p2.z = b.p2.z * s.z;
}


inline void swap(BOX &b1, BOX &b2)
{
	std::swap(b1.p1, b2.p1);
	std::swap(b1.p2, b2.p2);
}

// Zwraca true, jeœli punkt nale¿y do boksa
inline bool PointInBox(const VEC3 &p, const BOX &b)
{
	return
		(p.x >= b.p1.x) && (p.x <= b.p2.x) &&
		(p.y >= b.p1.y) && (p.y <= b.p2.y) &&
		(p.z >= b.p1.z) && (p.z <= b.p2.z);
}

// Zwraca true, jeœli boks sb zawiera siê w ca³oœci wewn¹trz b
inline bool BoxInBox(const BOX &sb, const BOX &b)
{
	return
		(sb.p1.x >= b.p1.x) && (sb.p2.x <= b.p2.x) &&
		(sb.p1.y >= b.p1.y) && (sb.p2.y <= b.p2.y) &&
		(sb.p1.z >= b.p1.z) && (sb.p2.z <= b.p2.z);
}

// Zwraca true, jeœli podane boksy zachodz¹ na siebie
inline bool OverlapBox(const BOX &b1, const BOX &b2)
{
	return
		(b1.p1.x <= b2.p2.x) && (b1.p2.x >= b2.p1.x) &&
		(b1.p1.y <= b2.p2.y) && (b1.p2.y >= b2.p1.y) &&
		(b1.p1.z <= b2.p2.z) && (b1.p2.z >= b2.p1.z);
}

// Jeœli boksy posiadaj¹ czêœæ wspóln¹, zwraca j¹ przez Out i zwraca true
inline bool Intersection(BOX *Out, const BOX &b1, const BOX &b2)
{
	*Out = BOX(
		VEC3(
			std::max(b1.p1.x, b2.p1.x),
			std::max(b1.p1.y, b2.p1.y),
			std::max(b1.p1.z, b2.p1.z)),
		VEC3(
			std::min(b1.p2.x, b2.p2.x),
			std::min(b1.p2.y, b2.p2.y),
			std::min(b1.p2.z, b2.p2.z)));
	return Out->IsValid();
}

// Zwraca boks opisany na podanych dwóch boksach
inline void Union(BOX *Out, const BOX &b1, const BOX &b2)
{
	*Out = BOX(
		VEC3(
			std::min(b1.p1.x, b2.p1.x),
			std::min(b1.p1.y, b2.p1.y),
			std::min(b1.p1.z, b2.p1.z)),
		VEC3(
			std::max(b1.p2.x, b2.p2.x),
			std::max(b1.p2.y, b2.p2.y),
			std::max(b1.p2.z, b2.p2.z)));
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// P£ASZCZYZNA

/*
Reprezentuje p³aszczyznê w przestrzeni 3D z jedn¹ stron¹ dodatni¹, a drug¹
ujemn¹. Opisana jest równaniem A x + B y + C z + D = 0. Wektor [A,B,C] to wektor
normalny wskazuj¹cy w kierunku dodatniej pó³przestrzeni. Jeœli p³aszczyzna jest
znormalizowana, wektor normalny jest znormalizowany, a D to odleg³oœæ
p³aszczyzny od œrodka uk³adu wspó³rzêdnych.
Reprezentacja jako string to A,R,G,B np. "1,2,3,0.2".
Jeœli u¿ywasz D3DX, jest kompatybilna przez math_cast z D3DXPLANE.
P³aszczyzna maj¹ca a=b=c=0 (czyli sta³a ZERO) jest niepoprawna, zdegenerowana.
*/

struct PLANE
{
	float a, b, c, d;

	static const PLANE ZERO;
	static const PLANE POSITIVE_X; // P³aszczyzna pionowa, równoleg³a do Y i Z, wskazuj¹ca na prawo na X
	static const PLANE POSITIVE_Y; // P³aszczyzna pozioma, równoleg³a do X i Z, wskazuj¹ca do góry na Y
	static const PLANE POSITIVE_Z; // P³aszczyzna pionowa, równoleg³a do X i Y, wskazuj¹ca w g³¹b na Z
	static const PLANE NEGATIVE_X; // P³aszczyzna pionowa, równoleg³a do Y i Z, wskazuj¹ca na lewo na X
	static const PLANE NEGATIVE_Y; // P³aszczyzna pozioma, równoleg³a do X i Z, wskazuj¹ca w dó³ na Y
	static const PLANE NEGATIVE_Z; // P³aszczyzna pionowa, równoleg³a do X i Y, wskazuj¹ca do gracza na Z

	PLANE() { }
	PLANE(float a, float b, float c, float d) : a(a), b(b), c(c), d(d) { }
	PLANE(const VEC3 &n, float d) : a(n.x), b(n.y), c(n.z), d(d) { }

	VEC3 & GetNormal() { return (VEC3&)a; }
	const VEC3 & GetNormal() const { return (VEC3&)a; }

	bool operator == (const PLANE &p) const { return a == p.a && b == p.b && c == p.c && d == p.d; }
	bool operator != (const PLANE &p) const { return a != p.a || b != p.b || c != p.c || d != p.d; }

	// Zwraca p³aszczyznê zwrócon¹ w przeciwn¹ stronê
	PLANE operator - () const { return PLANE(-a, -b, -c, -d); }

	// Skalowanie
	PLANE operator * (float s) const { return PLANE(a*s, b*s, c*s, d*s); }
	PLANE operator / (float s) const { return PLANE(a/s, b/s, c/s, d/s); }
	PLANE & operator *= (float s) { a *= s; b *= s; c *= s; d *= s; return *this; }
	PLANE & operator /= (float s) { a /= s; b /= s; c /= s; d /= s; return *this; }

	float * GetArray() { return &a; }
	const float * GetArray() const { return &a; }

	float & operator [] (uint4 Index) { return GetArray()[Index]; }
	const float & operator [] (uint4 Index) const { return GetArray()[Index]; }

	// Zwraca jakiœ punkt le¿¹cy na tej p³aszczyŸnie
	// Musi byæ znormalizowana.
	void GetMemberPoint(VEC3 *Out)
	{
		*Out = GetNormal() * -d;
	}

	// Zmienia odleg³oœæ od pocz¹tku uk³adu wspó³rzêdnych tak, ¿eby ta p³aszczyzna pozostaj¹c tak samo
	// zorientowana zosta³a przesuniêta tak, ¿eby zawieraæ w sobie podany punkt
	// Musi byæ znormalizowana (?)
	void RecalculateD(const VEC3 &p)
	{
		d = -Dot(GetNormal(), p);
	}
};

inline PLANE operator * (float s, const PLANE &p) { return PLANE(p.a*s, p.b*s, p.c*s, p.d*s); }
inline PLANE operator / (float s, const PLANE &p) { return PLANE(p.a/s, p.b/s, p.c/s, p.d/s); }

template <> inline ::D3DXPLANE & math_cast<::D3DXPLANE, PLANE>(const PLANE &x) { return (::D3DXPLANE&)(x); }
template <> inline PLANE & math_cast<PLANE, ::D3DXPLANE>(const ::D3DXPLANE &x) { return (PLANE&)(x); }

inline void swap(PLANE &p1, PLANE &p2)
{
	std::swap(p1.a, p2.a); std::swap(p1.b, p2.b);
	std::swap(p1.c, p2.c); std::swap(p1.d, p2.d);
}

// Zwraca true, jeœli podane p³aszczyzny s¹ sobie mniej wiêcej równe
inline bool PlaneEqual(const PLANE &p1, const PLANE &p2) {
	return float_equal(p1.a, p2.a) && float_equal(p1.b, p2.b) && float_equal(p1.c, p2.c) && float_equal(p1.d, p2.d);
}

// Tworzy p³aszczyznê na podstawie trzech punktów
// P³aszczyzna bêdzie zorientowana dodatnio w kierunku tej pó³przestrzeni,
// z której wierzcho³ki trójk¹ta s¹ widoczne jako zorientowane zgodnie z ruchem
// wskazówek zegara w uk³adzie LH.
inline void PointsToPlane(PLANE *Out, const VEC3 &p1, const VEC3 &p2, const VEC3 &p3) {
	Cross(&Out->GetNormal(), p2-p1, p3-p1);
	Normalize(&Out->GetNormal());
	Out->d = -Dot(p1, Out->GetNormal());
}
// Tworzy p³aszczyznê z wektora normalnego i punktu le¿¹cego na tej p³aszczyŸnie
// - Czy Normal musi byæ znormalizowany? Nie wiem.
inline void PointNormalToPlane(PLANE *Out, const VEC3 &Point, const VEC3 &Normal) {
	Out->a = Normal.x; Out->b = Normal.y; Out->c = Normal.z;
	Out->d = -Dot(Normal, Point);
}

// Skalowanie p³aszczyzny
inline void Mul(PLANE *Out, const PLANE &p, float s)
{
	Out->a = p.a * s; Out->b = p.b * s;
	Out->c = p.c * s; Out->d = p.d * s;
}
inline void Div(PLANE *Out, const PLANE &p, float s)
{
	Out->a = p.a / s; Out->b = p.b / s;
	Out->c = p.c / s; Out->d = p.d / s;
}

// Iloczyn skalarny p³aszczyzny i punktu we wspó³rzêdnych jednorodnych
// Mo¿na u¿yæ do wyznaczenia czy tak zapisany punkt le¿y na p³aszczyŸnie,
// po której stronie i jak daleko.
inline float Dot(const PLANE &p, const VEC4 &pt)
{
	return p.a*pt.x + p.b*pt.y + p.c*pt.z + p.d*pt.w;
}

// Iloczyn skalarny p³aszczyzny z punktem 3D: a*x+b*y+c*z+d*1
// Mo¿na u¿yæ do wyznaczenia czy punkt le¿y na p³aszczyŸnie, po której stronie
// i jak daleko.
inline float DotCoord(const PLANE &p, const VEC3 &pt)
{
	return p.a*pt.x + p.b*pt.y + p.c*pt.z + p.d;
}

// Iloczyn skalarny p³aszczyzny z wektorem 3D: a*x+b*y+c*z+d*0
// Przydatny do wyznaczania k¹ta miêdzy p³aszczyzn¹ (jej normaln¹) a wektorem.
inline float DotNormal(const PLANE &p, const VEC3 &v)
{
	return p.a*v.x + p.b*v.y + p.c*v.z;
}

// Normalizacja p³aszczyzny
// Po tej operacji wektor normalny p³aszczyzny jest znormalizowany.
// P³aszczyzna pozostaje po³o¿ona i zorientowana tam gdzie by³a.
inline void Normalize(PLANE *Out, const PLANE &p)
{
	float nlsq = 1.0f / Length(p.GetNormal());
	Out->a = p.a * nlsq; Out->b = p.b * nlsq;
	Out->c = p.c * nlsq; Out->d = p.d * nlsq;
}
// Normalizacja p³aszczyzny w miejscu
inline void Normalize(PLANE *p)
{
	float nlsq = 1.0f / Length(p->GetNormal());
	p->a *= nlsq; p->b *= nlsq;
	p->c *= nlsq; p->d *= nlsq;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// PROSTA 2D

/*
Reprezentuje prost¹ w przestrzeni 2D z jedn¹ stron¹ dodatni¹, a drug¹
ujemn¹. Opisana jest równaniem A x + B y + C = 0. Wektor [A,B] to wektor
normalny wskazuj¹cy w kierunku dodatniej pó³p³aszczyzny. Jeœli prosta jest
znormalizowana, wektor normalny jest znormalizowany, a C to odleg³oœæ prostej
od œrodka uk³adu wspó³rzêdnych.
Reprezentacja jako string to A,B,C, np. "2,0,2.1"
Prosta maj¹ca a = b = 0 (czyli np. sta³a ZERO) jest niepoprawna, zdegenerowana.
*/

struct LINE2D
{
	float a, b, c;

	static const LINE2D ZERO;
	static const LINE2D POSITIVE_X;
	static const LINE2D NEGATIVE_X;
	static const LINE2D POSITIVE_Y;
	static const LINE2D NEGATIVE_Y;

	LINE2D() { }
	LINE2D(float a, float b, float c) : a(a), b(b), c(c) { }
	LINE2D(const VEC2 &Normal, float c) : a(Normal.x), b(Normal.y), c(c) { }

	VEC2 & GetNormal() { return (VEC2&)a; }
	const VEC2 & GetNormal() const { return (VEC2&)a; }

	bool operator == (const LINE2D &L) const { return a == L.a && b == L.b && c == L.c; }
	bool operator != (const LINE2D &L) const { return a != L.a || b != L.b || c != L.c; }

	// Zwraca prost¹ zwrócon¹ w przeciwn¹ stronê
	LINE2D operator - () const { return LINE2D(-a, -b, -c); }

	// Skalowanie
	LINE2D operator * (float s) const { return LINE2D(a*s, b*s, c*s); }
	LINE2D operator / (float s) const { return LINE2D(a/s, b/s, c/s); }
	LINE2D & operator *= (float s) { a *= s; b *= s; c *= s; return *this; }
	LINE2D & operator /= (float s) { a /= s; b /= s; c /= s; return *this; }

	float * GetArray() { return &a; }
	const float * GetArray() const { return &a; }

	float & operator [] (uint4 Index) { return GetArray()[Index]; }
	const float & operator [] (uint4 Index) const { return GetArray()[Index]; }

	// Zwraca wektor wskazuj¹cy w kierunku prostej
	void GetTangent(VEC2 *Out) { Out->x = b; Out->y = -a; }

	// Zwraca jakiœ punkt le¿¹cy na tej prostej
	// Musi byæ znormalizowana.
	void GetMemberPoint(VEC2 *Out)
	{
		*Out = GetNormal() * -c;
	}

	// Zmienia odleg³oœæ od pocz¹tku uk³adu wspó³rzêdnych tak, ¿eby ta prosta pozostaj¹c tak samo
	// zorientowana zosta³a przesuniêta tak, ¿eby zawieraæ w sobie podany punkt
	// Musi byæ znormalizowana (?)
	void RecalculateD(const VEC2 &p)
	{
		c = -Dot(GetNormal(), p);
	}
};

inline LINE2D operator * (float s, const LINE2D &p) { return LINE2D(p.a*s, p.b*s, p.c*s); }
inline LINE2D operator / (float s, const LINE2D &p) { return LINE2D(p.a/s, p.b/s, p.c/s); }

// Zwraca true, jeœli podane proste s¹ sobie mniej wiêcej równe
inline bool LineEqual(const LINE2D &p1, const LINE2D &p2) {
	return float_equal(p1.a, p2.a) && float_equal(p1.b, p2.b) && float_equal(p1.c, p2.c);
}

inline void swap(LINE2D &p1, LINE2D &p2)
{
	std::swap(p1.a, p2.a); std::swap(p1.b, p2.b); std::swap(p1.c, p2.c);
}

// Tworzy prost¹ na podstwie dwóch punktów
// Wynikowa prosta nie jest znormalizowana.
inline void PointsToLine(LINE2D *Out, const VEC2 &p1, const VEC2 &p2) {
	Out->a = p1.y - p2.y;
	Out->b = p2.x - p1.x;
	Out->c = - Out->a * p1.x - Out->b * p1.y;
}

// Tworzy prost¹ z wektora normalnego i punktu le¿¹cego na tej prostej
inline void PointNormalToLine(LINE2D *Out, const VEC2 &Point, const VEC2 &Normal) {
	Out->a = Normal.x; Out->b = Normal.y;
	Out->c = -Dot(Normal, Point);
}

// Zwraca true, jeœli podane proste s¹ równoleg³e
inline bool LinesParallel(const LINE2D &L1, const LINE2D &L2) {
	return FLOAT_ALMOST_ZERO(L1.a*L2.b - L2.a*L1.b);
}

// Zwraca true, jeœli podane proste s¹ prostopad³e
inline bool LinesPerpedicular(const LINE2D &L1, const LINE2D &L2) {
	return FLOAT_ALMOST_ZERO(L1.a*L2.a + L1.b*L2.b);
}

// Zwraca dot z wektorów normalnych dwóch prostych
// Jeœli te proste s¹ znormalizowane, wynikiem jest cos k¹ta miêdzy tymi prostymi.
inline float Dot(const LINE2D &L1, const LINE2D &L2) {
	return L1.a*L2.a + L1.b*L2.b;
}

// Zwraca signed odleg³oœæ dwóch prostych na p³aszczyŸnie
// Proste musz¹ byæ znormalizowane.
inline float LineDistance(const LINE2D &L1, const LINE2D &L2) {
	return L1.c - L2.c;
}

// Oblicza punkt przeciecia dwóch prostych
// Jeœli proste nie przecinaj¹ siê, tylko s¹ równoleg³e lub siê pokrywan¹, to nie oblicza tylko zwraca false.
bool LinesIntersection(VEC2 *Out, const LINE2D &L1, const LINE2D &L2);

// Zwraca true, jeœli podane trzy punkty le¿¹ na jednej prostej
bool PointsColinear(const VEC2 &p1, const VEC2 &p2, const VEC2 &p3);

// Skalowanie prostej
inline void Mul(LINE2D *Out, const LINE2D &p, float s) {
	Out->a = p.a * s; Out->b = p.b * s; Out->c = p.c * s;
}
inline void Div(LINE2D *Out, const LINE2D &p, float s) {
	Out->a = p.a / s; Out->b = p.b / s; Out->c = p.c / s;
}

// Iloczyn skalarny p³aszczyzny i punktu we wspó³rzêdnych jednorodnych 2D
// Mo¿na u¿yæ do wyznaczenia czy tak zapisany punkt le¿y na p³aszczyŸnie,
// po której stronie i jak daleko.
inline float Dot(const LINE2D &p, const VEC3 &pt) {
	return p.a*pt.x + p.b*pt.y + p.c*pt.z;
}

// Iloczyn skalarny prostej 2D z punktem 2D: a*x + b*y + c*1
// Mo¿na u¿yæ do wyznaczenia czy punkt le¿y na prostej 2D, po której stronie i jak daleko.
inline float DotCoord(const LINE2D &p, const VEC2 &pt) {
	return p.a*pt.x + p.b*pt.y + p.c;
}

// Iloczyn skalarny prostej 2D z wektorem 2D: a*x + b*y + c*z + d*0
// Przydatny do wyznaczania k¹ta miêdzy prost¹ 2D (jej normaln¹) a wektorem.
inline float DotNormal(const LINE2D &p, const VEC2 &v) {
	return p.a*v.x + p.b*v.y;
}

// Normalizacja prostej 2D
// Po tej operacji wektor normalny prostej jest znormalizowany.
// Prosta pozostaje po³o¿ona i zorientowana tam gdzie by³a.
inline void Normalize(LINE2D *Out, const LINE2D &p) {
	float nlsq = 1.f / Length(p.GetNormal());
	Out->a = p.a * nlsq; Out->b = p.b * nlsq; Out->c = p.c * nlsq;
}
// Normalizacja p³aszczyzny w miejscu
inline void Normalize(LINE2D *p) {
	float nlsq = 1.f / Length(p->GetNormal());
	p->a *= nlsq; p->b *= nlsq; p->c *= nlsq;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// MACIERZ 4 x 4

/*
Elementy s¹ indeksowane [wiersz,kolumna].
Reprezentacja tekstowa to:
	a11,a12,a13,a14;a21,a22,a23,a24;a31,a32,a33,a34;a41,a42,a43,a44
*/

struct MATRIX
{
	union
	{
		struct
		{
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
			float _41, _42, _43, _44;
		};
        struct {
            float a00, a01, a02, a03;
            float a10, a11, a12, a13;
            float a20, a21, a22, a23;
            float a30, a31, a32, a33;
        };
		float m[4][4]; // [wiersz][kolumna]
	};

	static const MATRIX ZERO;
	static const MATRIX IDENTITY;

	MATRIX() { }
	MATRIX(float _11, float _12, float _13, float _14, float _21, float _22, float _23, float _24, float _31, float _32, float _33, float _34, float _41, float _42, float _43, float _44) : _11(_11), _12(_12), _13(_13), _14(_14), _21(_21), _22(_22), _23(_23), _24(_24), _31(_31), _32(_32), _33(_33), _34(_34), _41(_41), _42(_42), _43(_43), _44(_44) { }
	MATRIX(float *Array) : _11(Array[0]), _12(Array[1]), _13(Array[2]), _14(Array[3]), _21(Array[4]), _22(Array[5]), _23(Array[6]), _24(Array[7]), _31(Array[8]), _32(Array[9]), _33(Array[10]), _34(Array[11]), _41(Array[12]), _42(Array[13]), _43(Array[14]), _44(Array[15]) { }

	bool operator == (const MATRIX &m) const { return _11 == m._11 && _12 == m._12 && _13 == m._13 && _14 == m._14 && _21 == m._21 && _22 == m._22 && _23 == m._23 && _24 == m._24 && _31 == m._31 && _32 == m._32 && _33 == m._33 && _34 == m._34 && _41 == m._41 && _42 == m._42 && _43 == m._43 && _44 == m._44; }
	bool operator != (const MATRIX &m) const { return _11 != m._11 || _12 != m._12 || _13 != m._13 || _14 != m._14 || _21 != m._21 || _22 != m._22 || _23 != m._23 || _24 != m._24 || _31 != m._31 || _32 != m._32 || _33 != m._33 || _34 != m._34 || _41 != m._41 || _42 != m._42 || _43 != m._43 || _44 != m._44; }

	float & operator () (unsigned Row, unsigned Col) { return m[Row][Col]; }
	float operator () (unsigned Row, unsigned Col) const { return m[Row][Col]; }

	MATRIX operator - () const;
	MATRIX operator + (const MATRIX &m) const;
	MATRIX operator - (const MATRIX &m) const;
	MATRIX operator * (const MATRIX &m) const
	{
		return MATRIX(
			_11 * m._11 + _12 * m._21 + _13 * m._31 + _14 * m._41,
			_11 * m._12 + _12 * m._22 + _13 * m._32 + _14 * m._42,
			_11 * m._13 + _12 * m._23 + _13 * m._33 + _14 * m._43,
			_11 * m._14 + _12 * m._24 + _13 * m._34 + _14 * m._44,

			_21 * m._11 + _22 * m._21 + _23 * m._31 + _24 * m._41,
			_21 * m._12 + _22 * m._22 + _23 * m._32 + _24 * m._42,
			_21 * m._13 + _22 * m._23 + _23 * m._33 + _24 * m._43,
			_21 * m._14 + _22 * m._24 + _23 * m._34 + _24 * m._44,

			_31 * m._11 + _32 * m._21 + _33 * m._31 + _34 * m._41,
			_31 * m._12 + _32 * m._22 + _33 * m._32 + _34 * m._42,
			_31 * m._13 + _32 * m._23 + _33 * m._33 + _34 * m._43,
			_31 * m._14 + _32 * m._24 + _33 * m._34 + _34 * m._44,

			_41 * m._11 + _42 * m._21 + _43 * m._31 + _44 * m._41,
			_41 * m._12 + _42 * m._22 + _43 * m._32 + _44 * m._42,
			_41 * m._13 + _42 * m._23 + _43 * m._33 + _44 * m._43,
			_41 * m._14 + _42 * m._24 + _43 * m._34 + _44 * m._44);
	}
	MATRIX & operator += (const MATRIX &m);
	MATRIX & operator -= (const MATRIX &m);
	MATRIX & operator *= (const MATRIX &m);

	MATRIX operator * (float s) const;
	MATRIX operator / (float s) const;
	MATRIX & operator *= (float s);
	MATRIX & operator /= (float s);

	float * GetArray() { return &m[0][0]; }
	float GetArray() const { return m[0][0]; }
};

template <> inline ::D3DXMATRIX & math_cast<::D3DXMATRIX, MATRIX>(const MATRIX &x) { return (::D3DXMATRIX&)(x); }
template <> inline MATRIX & math_cast<MATRIX, ::D3DXMATRIX>(const ::D3DXMATRIX &x) { return (MATRIX&)(x); }

void MatrixToStr(string *Out, const MATRIX &m);
bool StrToMatrix(MATRIX *Out, const string &Str);

inline void swap(MATRIX &m1, MATRIX &m2)
{
	std::swap(m1._11, m2._11); std::swap(m1._12, m2._12); std::swap(m1._13, m2._13); std::swap(m1._14, m2._14);
	std::swap(m1._21, m2._21); std::swap(m1._22, m2._22); std::swap(m1._23, m2._23); std::swap(m1._24, m2._24);
	std::swap(m1._31, m2._31); std::swap(m1._32, m2._32); std::swap(m1._33, m2._33); std::swap(m1._34, m2._34);
	std::swap(m1._41, m2._41); std::swap(m1._42, m2._42); std::swap(m1._43, m2._43); std::swap(m1._44, m2._44);
}

// Zwraca true, jeœli podane macierze s¹ sobie mniej wiêcej równe
bool MatrixEqual(const MATRIX &m1, const MATRIX &m2);
// Negacja macierzy w miejscu
void Minus(MATRIX *m);
// Dodawanie, odejmowanie, mno¿enie macierzy
void Add(MATRIX *Out, const MATRIX &m1, const MATRIX &m2);
void Sub(MATRIX *Out, const MATRIX &m1, const MATRIX &m2);
void Mul(MATRIX *Out, const MATRIX &m1, const MATRIX &m2);
// Mno¿enie i dzielenie macierzy przez skalar w miejscu
void Mul(MATRIX *m, float s);
void Div(MATRIX *m, float s);

// Mno¿enie wektora 2D jako [x,y,0,1] przez macierz, wychodzi wektor 4D
void Transform(VEC4 *Out, const VEC2 &v, const MATRIX &m);
// Mno¿enie wektora 2D jako [x,y,0,1] przez macierz z olaniem wyjœciowego W
void Transform(VEC2 *Out, const VEC2 &v, const MATRIX &m);
// Mno¿enie wektora 2D jako [x,y,0,1] przez macierz i sprowadzenie z powrotem do 2D przez podzielenie przez W
void TransformCoord(VEC2 *Out, const VEC2 &v, const MATRIX &m);
// Mno¿enie wektora 2D jako [x,y,0,0] przez macierz
void TransformNormal(VEC2 *Out, const VEC2 &v, const MATRIX &m);

// Mno¿enie wektora 3D jako [X,Y,Z,1] przez macierz, wychodzi wektor 4D
void Transform(VEC4 *Out, const VEC3 &v, const MATRIX &m);
// Mno¿enie wektora 3D jako [X,Y,Z,1] przez macierz z olaniem wyjœciowego W
// Dobre do przekszta³cania wektorów 3D jeœli macierz reprezentuje przekszta³cenie liniowe.
void Transform(VEC3 *Out, const VEC3 &v, const MATRIX &m);
// Mno¿enie wektora 3D jako [X,Y,Z,1] przez macierz i sprowadzenie z powrotem do 3D przez podzielenie przez W
// Dobre do niezawodnego przekszta³cania wektorów 3D przez dowoln¹ macierz np. rzutowania.
void TransformCoord(VEC3 *Out, const VEC3 &v, const MATRIX &m);
// Mno¿enie wektora 3D jako [X,Y,Z,0] przez macierz
// Dobre do przekszta³cania wektorów (np. normalnych), nie punktów, bo nie robi translacji.
void TransformNormal(VEC3 *Out, const VEC3 &v, const MATRIX &m);

// Przekszta³ca na raz ca³¹ tablicê wektorów - w miejscu lub z tablicy wejœciowej do wyjœciowej
void TransformArray(VEC3 OutPoints[], const VEC3 InPoints[], size_t PointCount, const MATRIX &M);
void TransformArray(VEC3 InOutPoints[], size_t PointCount, const MATRIX &M);
void TransformNormalArray(VEC3 OutPoints[], const VEC3 InPoints[], size_t PointCount, const MATRIX &M);
void TransformNormalArray(VEC3 InOutPoints[], size_t PointCount, const MATRIX &M);
void TransformCoordArray(VEC3 OutPoints[], const VEC3 InPoints[], size_t PointCount, const MATRIX &M);
void TransformCoordArray(VEC3 InOutPoints[], size_t PointCount, const MATRIX &M);

// Mno¿enie wektora 4D przez macierz
// Czyli przekszta³cenie wektora we wspó³rzêdnych jednorodnych przez t¹ macierz.
void Transform(VEC4 *Out, const VEC4 &v, const MATRIX &m);

// Przekszta³ca p³aszczyznê przez macierz
// P³aszczyzna ma byæ znormalizowana.
// Macierz ma byæ odwrotna i transponowana wzglêdem normalnej macierzy.
void Transform(PLANE *Out, const PLANE &p, const MATRIX &m);
// Przekszta³ca promieñ przez macierz
void TransformRay(VEC3 *OutOrigin, VEC3 *OutDir, const VEC3 &RayOrigin, const VEC3 &RayDir, const MATRIX &m);

// Przekszta³ca AABB przez podan¹ macierz i wylicza AABB tego co wysz³o po przekszta³ceniu
void TransformBox(BOX *Out, const BOX &In, const MATRIX &M);
// Przekszta³ca AABB przez podan¹ macierz i wylicza AABB tego co wysz³o po przekszta³ceniu
// Wersja dziel¹ca przez W - mo¿na stosowaæ te¿ do macierzy projekcji.
void TransformBoxCoord(BOX *Out, const BOX &In, const MATRIX &M);

// Tworzy macierz przekszta³caj¹c¹ do lokalnego uk³adu wspó³rzêdnych wyznaczonego przez podane 3 osie
// Nast¹pi obrót, a jeœli nie s¹ znormalizowane to i skalowanie.
void AxesToMatrix(MATRIX *Out, const VEC3 &AxisX, const VEC3 &AxisY, const VEC3 &AxisZ);
// Tworzy macierz przekszta³caj¹c¹ do lokalnego uk³adu wspó³rzêdnych wyznaczonego przez podane 3 osie
// i pozycjê pocz¹tku tego uk³adu.
// Nast¹pi obrót, a jeœli osie s¹ znormalizowane to i skalowanie.
void AxesToMatrixTranslation(MATRIX *Out, const VEC3 &Origin, const VEC3 &AxisX, const VEC3 &AxisY, const VEC3 &AxisZ);

// Macierz kamery
// Eye to punkt, Forward i Up to wektory.
void LookAtLH(MATRIX *Out, const VEC3 &Eye, const VEC3 &Forward, const VEC3 &Up);
void LookAtRH(MATRIX *Out, const VEC3 &Eye, const VEC3 &Forward, const VEC3 &Up);

// Tworzy macierz identycznoœciow¹
void Identity(MATRIX *Out);
// Tworzy macierz translacji (przesuwania)
void Translation(MATRIX *Out, float x, float y, float z);
void Translation(MATRIX *Out, const VEC3 &v);
// Tworzy macierz skalowania (powiêkszania, pomniejszania, rozci¹gania)
void Scaling(MATRIX *Out, float sx, float sy, float sz);
void Scaling(MATRIX *Out, const VEC3 &sv);
void Scaling(MATRIX *Out, float s);
// Tworzy macierz skalowania wzd³u¿ podanego kierunku
// k - wspó³czynnik skalowania
// n - wektor kierunku, musi byæ znormalizowany
void ScalingAxis(MATRIX *Out, const VEC3 &n, float k);

// Macierz obrotu wokó³ danej osi
void RotationX(MATRIX *Out, float Angle);
void RotationY(MATRIX *Out, float Angle);
void RotationZ(MATRIX *Out, float Angle);
// Axis musi byæ znormalizowany.
void RotationAxisLH(MATRIX *Out, const VEC3 &Axis, float Angle);
inline void RotationAxisRH(MATRIX *Out, const VEC3 &Axis, float Angle) { RotationAxisLH(Out, Axis, -Angle); }

// Macierz obrotu wokó³ trzech osi w kolejnoœci Yaw-Pitch-Roll (ZXY) - tzw. k¹ty Eulera
// Yaw jest wokó³ osi Y. Pitch jest wokó³ osi X. Roll jest wokó³ osi Z.
// W przypadku wektora sk³adowe odpowiadaj¹ obrotom wokó³ poszczególnych osi, czyli:
//   X = Pitch, Y = Yaw, Z = Roll
// Przekszta³ca ze wsp. obiektu do œwiata (object -> interial space). [a mo¿e odwrotnie?]
// Czyli tak naprawdê robi ZXY.
void RotationYawPitchRoll(MATRIX *Out, float Yaw, float Pitch, float Roll);
void RotationYawPitchRoll(MATRIX *Out, const VEC3 &EulerAngles);
// Przekszta³ca ze wsp. œwiata do obiektu (interial -> object space). [a mo¿e odwrotnie?]
// Czyli tworzy macierz dok³adnie odwrotn¹ do tej z RotationYawPitchRoll.
void RotationYawPitchRollInv(MATRIX *Out, float Yaw, float Pitch, float Roll);
void RotationYawPitchRollInv(MATRIX *Out, const VEC3 &EulerAngles);

// Przetwarza macierz obeotu (to musi byæ czysta macierz obrotu) na k¹ty Eulera
//void RotationMatrixToEulerAngles(const MATRIX &m, float *Yaw, float *Pitch, float *Roll);

// Tworzy macierz rzutowania ortogonalnego
// Dane s¹ w uk³adzie kamery.
void OrthoLH(MATRIX *Out, float Width, float Height, float ZNear, float ZFar);
void OrthoRH(MATRIX *Out, float Width, float Height, float ZNear, float ZFar);
void OrthoOffCenterLH(MATRIX *Out, float Left, float Right, float Bottom, float Top, float ZNear, float ZFar);
void OrthoOffCenterRH(MATRIX *Out, float Left, float Right, float Bottom, float Top, float ZNear, float ZFar);
// Tworzy macierz rzutowania perspektywicznego
// Width i Height s¹ wymiarami na p³aszczyŸnie bliskiej (near).
// Left, Top, Right, Bottom prawdopodobnie te¿.
// Tworzone macierze s¹ "W-Friendly" - wektor przemno¿ony przez tak¹ macierz ma w sk³adowej W komponent Z.
void PerspectiveLH(MATRIX *Out, float Width, float Height, float ZNear, float ZFar);
void PerspectiveRH(MATRIX *Out, float Width, float Height, float ZNear, float ZFar);
void PerspectiveFovLH(MATRIX *Out, float FovY, float Aspect, float ZNear, float ZFar);
void PerspectiveFovRH(MATRIX *Out, float FovY, float Aspect, float ZNear, float ZFar);
void PerspectiveOffCenterLH(MATRIX *Out, float Left, float Right, float Bottom, float Top, float ZNear, float ZFar);
void PerspectiveOffCenterRH(MATRIX *Out, float Left, float Right, float Bottom, float Top, float ZNear, float ZFar);
// Wersja tworz¹ca macierz z Z-Far = INFINITY
void PerspectiveFovLH_Inf(MATRIX *Out, float FovY, float Aspect, float ZNear);
// Tworzy macierz sp³aszczaj¹c¹ geometriê na podan¹ p³aszczyznê w podanym kierunku padania œwiat³a
// P³aszczyzna musi byæ znormalizowana.
void MatrixShadow(MATRIX *Out, const VEC4 &Light, const PLANE &Plane);
// Tworzy macierz odbijaj¹c¹ geometriê wzglêdem podanej p³aszczyzny
// P³aszczyzna musi byæ znormalizowana.
void MatrixReflect(MATRIX *Out, const PLANE &p);

// Transpozycja macierzy na miejscu
void Transpose(MATRIX *m);
// Interpolacja liniowa macierzy
void Lerp(MATRIX *Out, const MATRIX &m1, const MATRIX &m2, float t);
// Wyznacznik macierzy
float Det(const MATRIX &m);
// Odwrotnoœæ macierzy
bool Inverse(MATRIX *Out, const MATRIX &m);


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// KWATERNION

/*
Reprezentacja jako ³añcuch to: "x,y,z,w", np.: "1,2,3.5,-10".
Kwaterniony s¹ mno¿one od drugiej strony ni¿ matematycznie, dziêki czemu mo¿na
³¹czyæ obroty które reprezentuj¹ w porz¹dku naturalnym, tak jak macierze.
*/

struct QUATERNION
{
	union
	{
		struct
		{
			float x, y, z, w;
		};
		float v[4];
	};

	static const QUATERNION ZERO;
	static const QUATERNION IDENTITY;

	QUATERNION() { }
	QUATERNION(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) { }
	QUATERNION(float *Array) : x(Array[0]), y(Array[1]), z(Array[2]), w(Array[3]) { }

	bool operator == (const QUATERNION &v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
	bool operator != (const QUATERNION &v) const { return x != v.x || y != v.y || z != v.z || w != v.w; }

	QUATERNION operator - () const { return QUATERNION(-x, -y, -z, -w); }
	QUATERNION operator + (const QUATERNION &q) const { return QUATERNION(x+q.x, y+q.y, z+q.z, w+q.w); }
	QUATERNION operator - (const QUATERNION &q) const { return QUATERNION(x-q.x, y-q.y, z-q.z, w-q.w); }
	QUATERNION operator * (const QUATERNION &q) const { return QUATERNION(w*q.x + x*q.w + y*q.z - z*q.y, w*q.y + y*q.w + z*q.x - x*q.z, w*q.z + z*q.w + x*q.y - y*q.x, w*q.w - x*q.x - y*q.y - z*q.z); }
	QUATERNION & operator += (const QUATERNION &q) { x += q.x; y += q.y; z += q.z; w += q.w; return *this; }
	QUATERNION & operator -= (const QUATERNION &q) { x -= q.x; y -= q.y; z -= q.z; w -= q.w; return *this; }
	QUATERNION & operator *= (const QUATERNION &q) { QUATERNION R; R.x = w*q.x + x*q.w + y*q.z - z*q.y; R.y = w*q.y + y*q.w + z*q.x - x*q.z; R.z = w*q.z + z*q.w + x*q.y - y*q.x; R.w = w*q.w - x*q.x - y*q.y - z*q.z; *this = R; return *this; }
	QUATERNION & operator *= (float v) { x *= v; y *= v; z *= v; w *= v; return *this; }
	QUATERNION & operator /= (float v) { x /= v; y /= v; z /= v; w /= v; return *this; }

	float & operator [] (uint4 Index) { return v[Index]; }
	const float & operator [] (uint4 Index) const { return v[Index]; }

	float * GetArray() { return &x; }
	const float * GetArray() const { return &x; }

	// Zwraca k¹t obrotu jaki reprezentuje ten kwaternion
	float CalcAngle() const { return safe_acos(w) * 2.0f; }
	// Zwraca oœ obrotu jak¹ reprezentuje ten kwaternion
	void CalcAxis(VEC3 *Out) const;
};

inline QUATERNION operator * (const QUATERNION pt, float v) { return QUATERNION(pt.x*v, pt.y*v, pt.z*v, pt.w*v); }
inline QUATERNION operator / (const QUATERNION pt, float v) { return QUATERNION(pt.x/v, pt.y/v, pt.z/v, pt.w/v); }
inline QUATERNION operator * (float v, const QUATERNION pt) { return QUATERNION(pt.x*v, pt.y*v, pt.z*v, pt.w*v); }
inline QUATERNION operator / (float v, const QUATERNION pt) { return QUATERNION(pt.x/v, pt.y/v, pt.z/v, pt.w/v); }

template <> inline ::D3DXQUATERNION & math_cast<::D3DXQUATERNION, QUATERNION>(const QUATERNION &x) { return (::D3DXQUATERNION&)(x); }
template <> inline QUATERNION & math_cast<QUATERNION, ::D3DXQUATERNION>(const ::D3DXQUATERNION &x) { return (QUATERNION&)(x); }

inline void swap(QUATERNION &v1, QUATERNION &v2)
{
	std::swap(v1.x, v2.x);
	std::swap(v1.y, v2.y);
	std::swap(v1.z, v2.z);
	std::swap(v1.w, v2.w);
}

// Zwraca true, jeœli podane kwaterniony s¹ sobie mniej wiêcej równe
inline bool QuaternionEqual(const QUATERNION &q1, const QUATERNION &q2) {
	return float_equal(q1.x, q2.x) && float_equal(q1.y, q2.y) && float_equal(q1.z, q2.z) && float_equal(q1.w, q2.w);
}

// Ujemny kwaternion
void Minus(QUATERNION *q);
inline void Minus(QUATERNION *Out, const QUATERNION &q) { *Out = q; Minus(Out); }
// Dodawanie, odejmowanie, mno¿enie kwaternionów
void Add(QUATERNION *Out, const QUATERNION &q1, const QUATERNION &q2);
void Sub(QUATERNION *Out, const QUATERNION &q1, const QUATERNION &q2);
void Mul(QUATERNION *Out, const QUATERNION &q1, const QUATERNION &q2);
// Mno¿enie i dzielenie kwaterniona przez skalar
void Mul(QUATERNION *q, float v);
void Div(QUATERNION *q, float v);
inline void Mul(QUATERNION *Out, const QUATERNION &q, float v) { *Out = q; Mul(Out, v); }
inline void Div(QUATERNION *Out, const QUATERNION &q, float v) { *Out = q; Div(Out, v); }

// Tworzy kwaternion na podstawie macierzy obrotu
void RotationMatrixToQuaternion(QUATERNION *Out, const MATRIX &RotationMatrix);
// Przekszta³ca kwaternion w macierz obrotu
void QuaternionToRotationMatrix(MATRIX *Out, const QUATERNION &q);
// Przekszta³ca punkt/wektor przez kwaternion obracaj¹c go o ten kwaternion
// > To nie jest wcale super szybkie i nie przydaje siê a¿ tak czêsto.
//   Lepiej sobie przekszta³ciæ kwaternion na macierz.
// > Out = q * p * q^-1
void QuaternionTransform(VEC3 *Out, const VEC3 &p, const QUATERNION &q);
// Konwersja k¹tów Eulera na kwaternion (który obraca ze wsp. obiektu do œwiata, czyli object -> interial space) [a mo¿e odwrotnie?]
void EulerAnglesToQuaternionO2I(QUATERNION *Out, float Yaw, float Pitch, float Roll);
// Konwersja k¹tów Eulera na kwaternion (który obraca ze wsp. œwiata do obiektu, czyli interial -> object space) [a mo¿e odwrotnie?]
void EulerAnglesToQuaternionI2O(QUATERNION *Out, float Yaw, float Pitch, float Roll);
// Konwersja kwaterniona który obraca ze wsp. obiektu do œwiata, czyli object -> interial space, na k¹ty Eulera
void QuaternionO2IToEulerAngles(float *Yaw, float *Pitch, float *Roll, const QUATERNION &q);
// Konwersja kwaterniona który obraca ze wsp. œwiata do obiektu, czyli interial -> object space, na k¹ty Eulera
void QuaternionI2OToEulerAngles(float *Yaw, float *Pitch, float *Roll, const QUATERNION &q);

// Tworzy kwaternion obracaj¹cy wokó³ osi X
void QuaternionRotationX(QUATERNION *Out, float a);
// Tworzy kwaternion obracaj¹cy wokó³ osi Y
void QuaternionRotationY(QUATERNION *Out, float a);
// Tworzy kwaternion obracaj¹cy wokó³ osi Z
void QuaternionRotationZ(QUATERNION *Out, float a);
// Tworzy kwaternion na podstawie osi obrotu i k¹ta
void AxisToQuaternion(QUATERNION *Out, const VEC3 &Axis, float Angle);

// Oblica "ró¿nicê" dwóch kwaternionów reprezentuj¹c¹ taki obrót ¿e a*Out = b
// To jest obliczane tak: Out = a^-1 * b, wiêc jeœli masz ju¿ a^-1 to nie u¿ywaj.
void QuaternionDiff(QUATERNION *Out, const QUATERNION &a, const QUATERNION &b);
// Kwaternion sprzê¿ony do danego
void Conjugate(QUATERNION *q);
inline void Conjugate(QUATERNION *Out, const QUATERNION &q) { *Out = q; Conjugate(Out); }
// Odwrotnoœæ kwaterniona
void Inverse(QUATERNION *Out, const QUATERNION &q);
// Normalizacja kwaternion
void Normalize(QUATERNION *Out, const QUATERNION &q);
void Normalize(QUATERNION *InOut);
// Iloczyn skalarny dwóch kwaternionów
float Dot(const QUATERNION &q1, const QUATERNION &q2);
// Kwadrat d³ugoœci kwaterniona
float LengthSq(const QUATERNION &q);
// D³ugoœæ kwaterniona
float Length(const QUATERNION &q);
void Log(QUATERNION *Out, const QUATERNION &q);
void Exp(QUATERNION *Out, const QUATERNION &q);
// Podnieœ kwaternion do potêgi.
// Wyk³adnik 0..1 daje interpolacje kwaterniona od identycznoœciowego do q.
void Pow(QUATERNION *InOut, float t);
inline void Pow(QUATERNION *Out, const QUATERNION &q, float t) { *Out = q; Pow(Out, t); }
// Interpolacja normalna kwaternionów
void Lerp(QUATERNION *Out, const QUATERNION &q1, const QUATERNION &q2, float t);
// Interpolacja sferyczna kwaternionów
// - Sprawdzona, daje wyniki dok³adnie tekie same jak D3DXQuaternionSlerp.
void Slerp(QUATERNION *Out, const QUATERNION &q0, const QUATERNION &q1, float t);
// NIE TESTOWANA
void Squad(QUATERNION *Out, const QUATERNION &Q1, const QUATERNION &A, const QUATERNION &B, const QUATERNION &C, float t);
// NIE TESTOWANA
void SquadSetup(QUATERNION *OutA, QUATERNION *OutB, QUATERNION *OutC, const QUATERNION &Q0, const QUATERNION &Q1, const QUATERNION &Q2, const QUATERNION &Q3);


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// FRUSTUM

struct FRUSTUM_PLANES;
struct FRUSTUM_POINTS;

/*
Frustum opisany 6 p³aszczyznami z wektorami normalnymi zwróconymi do wewn¹trz.
- Jeœli WorldViewProj jest macierz¹ Proj, otrzymane p³aszczyzny s¹ we wsp. widoku.
  Jeœli WorldViewProj jest macierz¹ View*Proj, otrzymane p³aszczyzny s¹ we wsp. œwiata.
  Jeœli WorldViewProj jest macierz¹ World*View*Proj, otrzymane p³aszczyzny s¹ we wsp. lokalnych obiektu.
*/
struct FRUSTUM_PLANES
{
	// Indeksy do tej tablicy
	// Przedrostek PLANE_ jest, bo NEAR zosta³o gdzieœ zdefiniowane jako makro - grrrrr!
	static const size_t PLANE_LEFT   = 0;
	static const size_t PLANE_RIGHT  = 1;
	static const size_t PLANE_TOP    = 2;
	static const size_t PLANE_BOTTOM = 3;
	static const size_t PLANE_NEAR   = 4;
	static const size_t PLANE_FAR    = 5;

	PLANE Planes[6];

	// Tworzy niezainicjalizowany
	FRUSTUM_PLANES() { }
	// Inicjalizuje na podstawie macierzy
	FRUSTUM_PLANES(const MATRIX &WorldViewProj) { Set(WorldViewProj); }
	// Inicjalizuje na podstawie reprezentacji punktowej
	FRUSTUM_PLANES(const FRUSTUM_POINTS &FrustumPoints) { Set(FrustumPoints); }
	// Wype³nia
	void Set(const MATRIX &WorldViewProj);
	void Set(const FRUSTUM_POINTS &FrustumPoints);
	// Normalizuje p³aszczyzny
	void Normalize();

	PLANE & operator [] (size_t Index) { return Planes[Index]; }
	const PLANE & operator [] (size_t Index) const { return Planes[Index]; }
};

/*
Frustum zapisany jako 8 wierzcho³ków
- Uzyskiwanie przez konwersjê macierzy ViewProj do FRUSTUM_PLANES i z niego
  do FRUSTUM_POINTS jest du¿o szybsze ni¿ budowanie FRUSTUM_POINTS z macierzy
  ViewProjInv - 2 (Release) - 8 (Debug) razy szybsze!
*/
struct FRUSTUM_POINTS
{
	// indeksy do tej tablicy
	// kolejno na przeciêciu p³aszczyzn:
	static const size_t NEAR_LEFT_BOTTOM  = 0;
	static const size_t NEAR_RIGHT_BOTTOM = 1;
	static const size_t NEAR_LEFT_TOP     = 2;
	static const size_t NEAR_RIGHT_TOP    = 3;
	static const size_t FAR_LEFT_BOTTOM   = 4;
	static const size_t FAR_RIGHT_BOTTOM  = 5;
	static const size_t FAR_LEFT_TOP      = 6;
	static const size_t FAR_RIGHT_TOP     = 7;

	VEC3 Points[8];

	// Tworzy niezainicjalizowany
	FRUSTUM_POINTS() { }
	// Inicjalizuje na podstawie p³aszczyzn
	FRUSTUM_POINTS(const FRUSTUM_PLANES &FrustumPlanes) { Set(FrustumPlanes); }
	// Inicjalizuje na podstawie ODWROTNOŒCI macierzy View*Projection
	FRUSTUM_POINTS(const MATRIX &WorldViewProjInv) { Set(WorldViewProjInv); }
	// Wype³nia
	void Set(const FRUSTUM_PLANES &FrustumPlanes);
	void Set(const MATRIX &WorldViewProjInv);

	VEC3 & operator [] (size_t Index) { return Points[Index]; }
	const VEC3 & operator [] (size_t Index) const { return Points[Index]; }

	// Wylicza i zwraca œrodek frustuma
	void CalcCenter(VEC3 *Out) const;
	// Oblicza najmniejszy AABB otaczaj¹cy frustum dany tymi punktami
	void CalcBoundingBox(BOX *Box) const;
	// Oblicza sferê otaczaj¹c¹ frustum
	// Mo¿e nie najlepsz¹, ale jak¹œtam... (a mo¿e to jest najlepsza? nie wiem :P)
	void CalcBoundingSphere(VEC3 *SphereCenter, float *SphereRadius) const;
};

/*
Frustum - reprezentacja radarowa.
- Bardzo szybki jest test kolizji z punktem i ze sfer¹ (PointInFrustum,
  SphereToFrustum).
- UWAGA! Jako wektor Up podawaæ prawdziwy wektor kamery do góry, tzw. RealUp,
  który jest naprawdê wzajemnie prostopad³y do Forward i Right, a nie ten który
  zawsze wynosi (0,1,0) - to by³ b³¹d którego siê naszuka³em.
*/
class FRUSTUM_RADAR
{
private:
	VEC3 m_Eye;
	VEC3 m_Forward;
	VEC3 m_Up;
	VEC3 m_Right;
	float m_RFactor;
	float m_UFactor;
	float m_RSphereFactor;
	float m_USphereFactor;
	float m_ZNear;
	float m_ZFar;

public:
	// Tworzy niezainicjalizowany
	FRUSTUM_RADAR() { }
	// Tworzy w pe³ni zainicjalizowany
	FRUSTUM_RADAR(const VEC3 &Eye, const VEC3 &Forward, const VEC3 &Up, const VEC3 &Right, float FovY, float Aspect, float ZNear, float ZFar) { Set(Eye, Forward, Up, Right, FovY, Aspect, ZNear, ZFar); }

	// Zwraca poszczególne pamiêtane pola (FovY i Aspect nie pamiêta bezpoœrednio)
	const VEC3 & GetEye() const     { return m_Eye; }
	const VEC3 & GetForward() const { return m_Forward; }
	const VEC3 & GetUp() const      { return m_Up; }
	const VEC3 & GetRight() const   { return m_Right; }
	float GetZNear() const   { return m_ZNear; }
	float GetZFar() const    { return m_ZFar; }

	// Zwraca dane pomocnicze
	float GetRFactor() const { return m_RFactor; }
	float GetUFactor() const { return m_UFactor; }
	float GetRSphereFactor() const { return m_RSphereFactor; }
	float GetUSphereFactor() const { return m_USphereFactor; }

	// Ustawia poszczególne pola
	void SetEye    (const VEC3 &Eye)     { m_Eye = Eye; }
	void SetForward(const VEC3 &Forward) { m_Forward = Forward; }
	void SetUp     (const VEC3 &Up)      { m_Up = Up; }
	void SetRight  (const VEC3 &Right)   { m_Right = Right; }
	void SetZNear(float ZNear) { m_ZNear = ZNear; }
	void SetZFor (float ZFar)  { m_ZFar = ZFar; }
	void SetFovAndAspect(float FovY, float Aspect);
	
	// Kompletnie wype³nia
	void Set(const VEC3 &Eye, const VEC3 &Forward, const VEC3 &Up, const VEC3 &Right, float FovY, float Aspect, float ZNear, float ZFar) { SetProjection(FovY, Aspect, ZNear, ZFar); SetView(Eye, Forward, Up, Right); }
	// Wype³nia jedn¹ po³ówkê danych
	void SetProjection(float FovY, float Aspect, float ZNear, float ZFar);
	// Wype³nia drug¹ po³ówkê danych
	void SetView(const VEC3 &Eye, const VEC3 &Forward, const VEC3 &Up, const VEC3 &Right);
};


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// KAPSU£A

// Objêtoœæ kapsu³y
float CapsuleVolume(const VEC3 &p1, const VEC3 &p2, float R);


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// INNE

// Funkcja wyg³adzaj¹ca zmianê zmiennej w czasie - SmoothCD.
// Nak³adki na t¹ funkcjê - szablon specjalizowany dla: float, VEC2, VEC3.
// Klasa szablonowa enkapsuluj¹ca pamiêtanie parametrów wyg³adzania.

template <typename T>
inline void SmoothCD_T(T *InOutPos, const T &Dest, T *InOutVel, float SmoothTime, float TimeDelta)
{
	assert(0 && "SmoothCD_T for invalid template type.");
}

template <>
inline void SmoothCD_T<float>(float *InOutPos, const float &Dest, float *InOutVel, float SmoothTime, float TimeDelta)
{
	SmoothCD(InOutPos, Dest, InOutVel, SmoothTime, TimeDelta);
}

template <>
inline void SmoothCD_T<VEC2>(VEC2 *InOutPos, const VEC2 &Dest, VEC2 *InOutVel, float SmoothTime, float TimeDelta)
{
	SmoothCD(&InOutPos->x, Dest.x, &InOutVel->x, SmoothTime, TimeDelta);
	SmoothCD(&InOutPos->y, Dest.y, &InOutVel->y, SmoothTime, TimeDelta);
}

template <>
inline void SmoothCD_T<VEC3>(VEC3 *InOutPos, const VEC3 &Dest, VEC3 *InOutVel, float SmoothTime, float TimeDelta)
{
	SmoothCD(&InOutPos->x, Dest.x, &InOutVel->x, SmoothTime, TimeDelta);
	SmoothCD(&InOutPos->y, Dest.y, &InOutVel->y, SmoothTime, TimeDelta);
	SmoothCD(&InOutPos->z, Dest.z, &InOutVel->z, SmoothTime, TimeDelta);
}

template <typename T>
class SmoothCD_obj
{
public:
	T Pos;
	T Dest; // Zapamiêtana ostatnia wartoœæ docelowa - mo¿e siê przydaæ.
	T Vel;
	float SmoothTime;

	// Tworzy niezainicjalizowany
	SmoothCD_obj() { }
	// Tworzy zainicjalizowany
	SmoothCD_obj(const T &Pos, float SmoothTime, const T &Vel) : Pos(Pos), Dest(Pos), Vel(Vel), SmoothTime(SmoothTime) { }
	// Ustawia parametry
	void Set(const T &Pos, const T &Vel) { this->Pos = Pos; this->Dest = Pos; this->Vel = Vel; }

	// Wykonuje krok czasowy
	void Update(const T &Dest, float TimeDelta) {
		this->Dest = Dest;
		SmoothCD_T<T>(&Pos, Dest, &Vel, SmoothTime, TimeDelta);
	}
};

// Zamiana miêdzy uk³adem wspó³rzêdnych:
// - Kartezjañskim DirectX: X w prawo, Y w górê, Z w g³¹b
// - Sferycznym: Yaw to d³ugoœæ geograficzna, Pitch to szerokoœæ geograficzna, R to promieñ wodz¹cy

// Tu mo¿na podawaæ co siê chce.
void SphericalToCartesian(VEC3 *Out, float Yaw, float Pitch, float R);

// Jako OutYaw, OutPitch, OutR mo¿na podawaæ NULL jeœli akurat nas nie interesuje - wtedy liczy szybciej.
// Nie podawaæ wektora zerowego bo wyjd¹ buraki.
// Yaw wychodzi -PI..PI
// Pitch wychodzi -PI/2..PI/2
// R wychodzi > 0
void CartesianToSpherical(float *OutYaw, float *OutPitch, float *OutR, const VEC3 &Pos);

// Zwraca wypuk³¹ otoczkê zbioru punktów 2D.
// - Na wejœciu musi byæ co najmniej jeden punkt.
// - Punkty bêd¹ u³o¿one zgodnie z ruchem wskazówek zegara, jeœli uk³ad jest X w prawo a Y w dó³.
// - Punkty powtarzaj¹ce siê s¹ chyba usuwane, tzn. wynik jest bez powtórzeñ.
// - Punkty le¿¹ce na prostej krawêdzi raz pojawiaj¹ siê w wyniku a raz nie.
void ConvexHull2D(std::vector<VEC2> *OutPoints, const VEC2 InPoints[], size_t InPointCount);


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// KOLIZJE

// Dany punkt p i promieñ (RayOrig, RayDir).
// Oblicza parametr t dla punktu na tym promieniu bêd¹cego rzutem tego punktu na ten promieñ.
// W ten sposób mo¿na obliczyæ rzut punktu na promieñ (punkt z promienia najbli¿szy danemu punktowi) jako RayOrig + RayDir*t.
// Mo¿na te¿ policzyæ najbli¿szy punkt na pó³prostej lub odcinku limituj¹c najpierw t od 0 do 1 czy od 0 do dlugoœci odcinka.
// UWAGA! Jeœli RayDir nie jest jednostkowy, wynik trzeba podzieliæ przez LengthSq(RayDir).
float ClosestPointOnLine(const VEC3 &p, const VEC3 &RayOrig, const VEC3 &RayDir);
// Zwraca odleg³oœæ punktu od prostej
// LineDir musi byæ znormalizowane!
float PointToLineDistance(const VEC3 &P, const VEC3 &LineOrig, const VEC3 &LineDir);
// Zwraca true, jeœli punkt le¿y na prostej
// LineDir musi byæ znormalizowane!
bool PointOnLine(const VEC3 &P, const VEC3 &LineOrig, const VEC3 &LineDir);
// Zwraca parametr T punktu le¿¹cego na prostej
// Punkt P musi naprawdê le¿eæ na tej prostej!
float PointToLine(const VEC3 &P, const VEC3 &LineOrig, const VEC3 &LineDir);
// Zwraca true, jeœli punkt le¿y na odcinku.
// Punkt P musi le¿eæ na prostej przechodz¹cej przez ten odcinek!
bool PointInLineSegment(const VEC3 &P, const VEC3 &SegmentBeg, const VEC3 &SegmentEnd);
// Wyznacza najbli¿szy punkt le¿¹cy na odcinku P1-P2 wzglêdem punktu P
void ClosestPointOnLineSegment(VEC3 *Out, const VEC3 &P, const VEC3 &P1, const VEC3 &P2);
// Wyznacza najbli¿szy punkt wewn¹trz prostopad³oœcianu wzglêdem punktu p.
// Jeœli p le¿y wewn¹trz tego prostopad³oœcianu, zwrócone zostaje dok³adnie p.
void ClosestPointInBox(VEC3 *Out, const BOX &Box, const VEC3 &p);
// Odleg³oœæ punktu od prostopad³oœcianu
// Jeœli p le¿y wewn¹trz tego prostopad³oœcianu, zwraca 0.
float PointToBoxDistance(const VEC3 &p, const BOX &box);
// Zwraca true, jeœli podany punkt nale¿y do sfery
bool PointInSphere(const VEC3 &p, const VEC3 &SphereCenter, float SphereRadius);
// Zwraca odleg³oœæ punktu od powierzchni sfery
// Jeœli punkt le¿y wewn¹trz sfery, zwróci odleg³oœæ ujemn¹.
float PointToSphereDistance(const VEC3 &p, const VEC3 &SphereCenter, float SphereRadius);
// Wyznacza najbli¿szy punkt na powierzchni sfery wzglêdem punktu p.
// Punkt mo¿e byæ wewn¹trz sfery - te¿ jest OK, zwróci z powierzchni sfery.
// Punkt nie mo¿e byæ dok³adnie na œrodku sfery - wtedy jest dzielenie przez zero.
void ClosestPointOnSphere(VEC3 *Out, const VEC3 &SphereCenter, float SphereRadius, const VEC3 &p);
// Zwraca true, jeœli podany punkt nale¿y do p³aszczyzny
// P³aszczyzna nie musi byæ znormalizowana.
bool PointOnPlane(const VEC3 &p, const PLANE &plane);
// Wyznacza najbli¿szy punkt na p³aszczyŸnie wzglêdem punktu p.
void ClosestPointOnPlane(VEC3 *Out, const PLANE &Plane, const VEC3 &p);
// Zwraca true, jeœli punkt nale¿y do trójk¹ta
bool PointOnTriangle(const VEC3 &point, const VEC3 &pa, const VEC3 &pb, const VEC3 &pc);
// Zwraca true, jeœli punkt nale¿y do wnêtrza podanego frustuma
bool PointInFrustum(const VEC3 &p, const FRUSTUM_PLANES &Frustum);
bool PointInFrustum(const VEC3 &p, const FRUSTUM_RADAR &Frustum);
// Dane s¹ dwie proste (RayOrig1, RayDir1) i (RayOrig2, RayDir2).
// Funkcja wyznacza parametry t1 i t2 dla punktów na tych prostych, w których te proste s¹ najbli¿ej siebie.
// Jeœli nie da siê takich znaleŸæ (proste s¹ równoleg³e), funkcja zwraca false.
// Te punkty mo¿na policzyæ ze wzorów p1=RayOrig1+t1*RayDir1 i p2=RayOrig2+t2*RayDir2.
// Proste faktycznie przecinaj¹ siê jeœli odleg³oœæ miêdzy tymi punktami wynosi ok. 0.
bool ClosestPointsOnLines(float *OutT1, float *OutT2, const VEC3 &RayOrig1, const VEC3 &RayDir1, const VEC3 &RayOrig2, const VEC3 &RayDir2);
// Kolizja promienia z prostopad³oœcianem
// Jeœli promieñ nie przecina prostopad³oœcianu, zwraca false.
// Jeœli promieñ przecina prostopad³oœcian, zwraca true i przez OutT zwraca odleg³oœæ w wielokrotnoœciach d³ugoœci RayDir.
// Jeœli promieñ przecina prostopad³oœcian od ty³u, funkcja te¿ zwraca true i zwraca OutT ujemne.
// Jeœli RayOrig jest wewn¹trz prostopad³oœcianu, funkcja zwraca true i OutT = 0.
bool RayToBox(float *OutT, const VEC3 &RayOrig, const VEC3 &RayDir, const BOX &Box);
// Wylicza kolizjê promienia ze sfer¹
// Jeœli nie ma kolizji, zwraca false.
// Jeœli jest kolizja, zwraca true i parametr T.
// Jeœli kolizja jest z ty³u promienia - wczeœniej ni¿ jego pocz¹tek - zwraca true i T ujemne.
// Jeœli pocz¹tek promienia jest wewn¹trz sfery, zwraca true i T=0
bool RayToSphere(const VEC3 &RayOrig, const VEC3 &RayDir, const VEC3 &SphereCenter, float SphereRadius, float *OutT);
// Przeciêcie promienia z p³aszczyzn¹
// OutT - zwrócony parametr dla promienia (NIE mo¿na podaæ 0)
// OutVD - zwrócona liczba mówi¹ca o zwrocie p³aszczyzny (NIE mo¿na podaæ 0)
//   OutVD > 0 oznacza, ¿e p³aszczyzna jest ty³em do promienia
// Wartoœæ zwracana - false jeœli promieñ by³ równoleg³y do p³aszczyzny i nie uda³o siê nic policzyæ.
bool RayToPlane(const VEC3 &RayOrig, const VEC3 &RayDir, const PLANE &Plane, float *OutT, float *OutVD);
// Kolizja promienia z trójk¹tem
// t - mo¿na podaæ NULL jeœli nie interesuje nas odleg³oœæ kolizji.
// BackfaceCulling - jeœli true, funkcja zawsze uwzglêdnia wy³¹cznie kolizjê z trójk¹tem zwróconym w stronê CW (jak w DirectX).
bool RayToTriangle(
	const VEC3 &RayOrig, const VEC3 &RayDir,
	const VEC3 &p0, const VEC3 &p1, const VEC3 &p2,
	bool BackfaceCulling,
	float *OutT);
// Kolizja promienia z frustumem
bool RayToFrustum(const VEC3 &RayOrig, const VEC3 &RayDir, const FRUSTUM_PLANES &Frustum, float *t_near, float *t_far);
// Zwraca true jeœli kula koliduje z prostopad³oœcianem (tak¿e jeœli jest w jego œrodku)
bool SphereToBox(const VEC3 &SphereCenter, float SphereRadius, const BOX &Box);
// Zwraca true, jeœli sfera zawiera siê w ca³oœci w prostopad³oœcianie
bool SphereInBox(const VEC3 &SphereCenter, float SphereRadius, const BOX &Box);
// Zwraca true, jeœli prostopad³oœcian zawiera siê w ca³oœci w sferze
bool BoxInSphere(const BOX &Box, const VEC3 &SphereCenter, float SphereRadius);
// Kolizja p³aszczyzny z prostopad³oœcianem
bool PlaneToBox(const PLANE &plane, const BOX &box);
// Klasyfikuje po³o¿enie prostopad³oœcianu wzglêdem p³aszczyzny
// -1 = prostopad³oœcian w ca³oœci po ujemnej stronie p³aszczyzny
//  0 = prostopad³oœcian przecina p³aszczyznê
// +1 = prostopad³oœcian w ca³oœci po dodatniej stronie p³aszczyzny
int ClassifyBoxToPlane(const PLANE &plane, const BOX &box);
// Zwraca true, jeœli trójk¹t zawiera siê w prostopad³oœcianie
bool TriangleInBox(const VEC3 &p1, const VEC3 &p2, const VEC3 &p3, const BOX &Box);
// Zwraca true, jeœli trójk¹t przecina prostopad³oœcian lub siê w nim zawiera
// OptTrianglePlane mo¿na podaæ jeœli siê ju¿ ma wyliczone, ¿eby przyspieszyæ. Nie musi byæ znormalizowane.
bool TriangleToBox(const VEC3 &p1, const VEC3 &p2, const VEC3 &p3, const BOX &Box, const PLANE *OptTrianglePlane = NULL);
// Zwraca true, jeœli podany prostopad³oœcian jest widoczny choæ trochê w bryle widzenia
// - Uwaga! W rzadkich przypadkach mo¿e stwierdziæ kolizjê chocia¿ jej nie ma.
bool BoxToFrustum_Fast(const BOX &Box, const FRUSTUM_PLANES &Frustum);
// Zwraca true, jeœli podany prostopad³oœcian jest widoczny choæ trochê w bryle widzenia
// - Test dok³adny, ale wolniejszy i wymaga te¿ reprezentacji punktowej frustuma
// - OptFrustumPoints to parametr opcjonalny - podaj jeœli masz ju¿ wyliczony
bool BoxToFrustum(const BOX &Box, const FRUSTUM_PLANES &FrustumPlanes, const FRUSTUM_POINTS *OptFrustumPoints);
bool BoxToFrustum(const BOX &Box, const FRUSTUM_RADAR &Frustum);
// Zwraca true, jeœli AABB jest w ca³oœci wewn¹trz frustuma
bool BoxInFrustum(const BOX &Box, const FRUSTUM_PLANES &Frustum);
// Zwraca true jeœli podane dwie sfery koliduj¹ ze sob¹
bool SphereToSphere(const VEC3 &Center1, float Radius1, const VEC3 &Center2, float Radius2);
// Zwraca true, jeœli sfera koliduje z p³aszczyzn¹
// P³aszczyzna chyba musi byæ znormalizowana.
bool SphereToPlane(const VEC3 &SphereCenter, float SphereRadius, const PLANE &Plane);
// Klasyfikuje po³o¿enie sfery wzglêdem p³aszczyzny
// -1 = sfera w ca³oœci po ujemnej stronie p³aszczyzny
//  0 = sfera przecina p³aszczyznê
// +1 = sfera w ca³oœci po dodatniej stronie p³aszczyzny
int ClassifySphereToPlane(const VEC3 &SphereCenter, float SphereRadius, const PLANE &Plane);
// Zwraca true, jeœli sfera koliduje z frustumem
// - Uwaga! W rzadkich przypadkach mo¿e stwierdziæ kolizjê chocia¿ jej nie ma.
bool SphereToFrustum_Fast(const VEC3 &SphereCenter, float SphereRadius, const FRUSTUM_PLANES &Frustum);
// Zwraca true, jeœli sfera koliduje z frustumem
// - Test dok³adny, ale wolniejszy i wymaga te¿ reprezentacji punktowej frustuma
// - OptFrustumPoints to parametr opcjonalny - podaj jeœli masz ju¿ wyliczony
bool SphereToFrustum(
	const VEC3 &SphereCenter, float SphereRadius,
	const FRUSTUM_PLANES &FrustumPlanes, const FRUSTUM_POINTS *OptFrustumPoints);
// - Uwaga! Wygl¹da na to, ¿e z nienznaych przyczyn ta funkcja nie zwraca true
//   tak¿e dla sfery która nie jest œciœle koliduj¹ca z frustumem, ale z pewnym
//   niewielkim marginesem b³êdu w kierunku +/- Right.
bool SphereToFrustum(const VEC3 &SphereCenter, float SphereRadius, const FRUSTUM_RADAR &Frustum);
// Zwraca true, jeœli sfera zawiera siê w ca³oœci wewn¹trz frustuma
bool SphereInFrustum(const VEC3 &SphereCenter, float SphereRadius, const FRUSTUM_PLANES &Frustum);
// Wylicza kolizjê dwóch p³aszczyzn zwracaj¹c parametry prostej tworz¹cej ich przeciêcie
// Jesli nie ma kolizji, zwraca false.
// Zwrócony kierunek promienia nie jest znormalizowany.
bool Intersect2Planes(const PLANE &Plane1, const PLANE &Plane2, VEC3 *LineOrig, VEC3 *LineDir);
// Zwraca punkt przeciêcia trzech p³aszczyzn lub false, jeœli taki punkt nie istnieje
bool Intersect3Planes(const PLANE &P1, const PLANE &P2, const PLANE &P3, VEC3 *OutP);
// Kolizja p³aszczyzny z trójk¹tem
bool PlaneToTriangle(const PLANE &Plane, const VEC3 &p0, const VEC3 &p1, const VEC3 &p2);
// Klasyfikuje po³o¿enie trójk¹ta wzglêdem p³aszczyzny
// -1 = trójk¹t w ca³oœci po ujemnej stronie p³aszczyzny
//  0 = trójk¹t przecina p³aszczyznê
// +1 = trójk¹t w ca³oœci po dodatniej stronie p³aszczyzny
int ClassifyPlaneToTriangle(const PLANE &Plane, const VEC3 &p0, const VEC3 &p1, const VEC3 &p2);
// Kolizja p³aszczyzny z frustumem (klasyfikacja)
int ClassifyFrustumToPlane(const FRUSTUM_POINTS &Frustum, const PLANE &Plane);
// Zwraca true, jeœli dwa odcinki 2D: p1-p2 i p3-p4 zachodz¹ na siebie
bool IntersectLines(const VEC2 &p1, const VEC2 &p2, const VEC2 &p3, const VEC2 &p4);
// Testuje kolizjê dwóch trójk¹tów 3D
// - Nie wiem co siê dzieje jeœli trójk¹ty s¹ zdegenerowane.
bool TriangleToTriangle(
	const VEC3 &V0,const VEC3 &V1,const VEC3 &V2,
	const VEC3 &U0,const VEC3 &U1,const VEC3 &U2);
// Testuje, czy trójk¹t le¿y w ca³oœci wewn¹trz frustuma
bool TriangleInFrustum(const VEC3 &t1, const VEC3 &t2, const VEC3 &t3, const FRUSTUM_PLANES &Frustum);
// Testuje kolizjê trójk¹ta z frustumem
// - Parametry OptTrianglePlane i OptFrustumPoints s¹ opcjonalne - podaj jeœli masz ju¿ wyliczone, aby przyspieszyæ.
bool TriangleToFrustum(
	const VEC3 &t1, const VEC3 &t2, const VEC3 &t3, const PLANE *OptTrianglePlane,
	const FRUSTUM_PLANES &FrustumPlanes, const FRUSTUM_POINTS *OptFrustumPoints);
// Testuje kolizjê miêdzy dwoma frustumami
bool FrustumToFrustum(
	const FRUSTUM_PLANES &Frustum1_Planes, const FRUSTUM_POINTS &Frustum1_Points,
	const FRUSTUM_PLANES &Frustum2_Planes, const FRUSTUM_POINTS &Frustum2_Points);
// [2D] Zwraca true, jeœli podany punkt le¿y wewn¹trz podanego wielok¹ta.
// Wielok¹t mo¿e mieæ dowolny kszta³t, tak¿e niewypuk³y, a nawet sam siê przecinaæ.
// Jego wierzcho³ki mog¹ biec w dowolnym kierunku.
bool PointInPolygon(const VEC2 &Point, const VEC2 PolygonPoints[], uint PolygonPointCount);
// [3D] Zwraca true, jeœli promieñ przecina wielok¹t i wtedy przez OutT zwraca parametr dla promienia.
// Wierzcho³ki wielok¹ta musz¹ le¿eæ w jednej p³aszczyŸnie.
// Jeœli BackfaceCulling = true, promieñ trafia tylko jeœli wchodzi z jednej strony, z tej z której
// wierzcho³ki wielok¹ta s¹ widoczne jako zorientowane w kierunku zgodnym ze wskazówkami zeraga.
bool RayToPolygon(const VEC3 &RayOrig, const VEC3 &RayDir, const VEC3 PolygonPoints[], uint PolygonPointCount, bool BackfaceCulling, float *OutT);

// Znajduje sferê otaczaj¹c¹ podany zbiór punktów
// - Nie jest to mo¿e najmniejsza sfera, ale przynajmniej ten algorytm jest prosty i szybki.
void SphereBoundingPoints(VEC3 *OutSphereCenter, float *OutSphereRadius, const VEC3 Points[], size_t PointCount);
// Liczy kolizjê poruszaj¹cej siê sfery z p³aszczyzn¹
// - P³aszczyzna musi byæ znormalizowana.
// - Jako OutT0 i OutT1 mo¿na podawaæ NULL, jeœli nas akurat nie interesuje.
// - Jeœli sfera porusza siê w drug¹ stronê, te¿ wychodzi true i T wychodz¹ ujemne.
// - Jeœli sfera koliduje z p³aszczyzn¹ i porusza siê w kierunku do niej równoleg³ym, jest T0=0, T1=MAXFLOAT i return true.
bool SweptSphereToPlane(const VEC3 &SphereCenter, float SphereRadius, const VEC3 &SphereSweepDir, const PLANE &Plane, float *OutT0, float *OutT1);
// Oblicza kolizjê poruszaj¹cej siê sfery z frustumem
// - Frustum musi byæ znormalizowany.
// - Jeœli sfera le¿y w œrodku, te¿ oczywiœcie zwraca true.
bool SweptSphereToFrustum(const VEC3 &SphereCenter, float SphereRadius, const VEC3 &SphereSweepDir, const FRUSTUM_PLANES &Frustum);
// Kolizja poruszaj¹cego siê AABB z drugim AABB.
// SweepDirBox1 nie musi byæ znormalizowany. T bêdzie wyra¿one we wielokrotnoœciach jego d³ugoœci.
// Jeœli nie zachodzi, zwraca false.
// Jeœli zachodzi, zwraca true i T jako czas pocz¹tkowy kontaktu.
// Jeœli zachodzi z ty³u, zwraca true i T ujemne.
// Jeœli zachodzi w chwili pocz¹tkowej, zwraca true i T=0.
bool SweptBoxToBox(const BOX &Box1, const BOX &Box2, const VEC3 &SweepDirBox1, float *OutT);

// Tworzy najmniejszy boks otaczaj¹cy podany zbiór punktów
void BoxBoundingPoints(BOX *box, const VEC3 points[], size_t PointCount);
// Oblicza sferê otaczaj¹c¹ box
void BoxBoundingSphere(VEC3 *SphereCenter, float *SphereRadius, const BOX &Box);
// Oblicza box otaczaj¹cy sferê
void SphereBoundingBox(BOX *Out, const VEC3 &SphereCenter, float SphereRadius);
// Wylicza AABB otaczaj¹cy podany zbiór sfer
// - SphereCount musi byæ wiêksze od 0.
void BoxBoundingSpheres(BOX *OutBox, const VEC3 SpheresCenter[], const float SpheresRadius[], size_t SphereCount);

} // namespace common


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Konwersje

template <>
struct SthToStr_obj<common::POINT_>
{
	void operator () (string *Str, const common::POINT_ &Sth)
	{
		*Str = common::Format("#,#") % Sth.x % Sth.y;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct StrToSth_obj<common::POINT_>
{
	bool operator () (common::POINT_ *Sth, const string &Str)
	{
		size_t CommaPos = Str.find(',');
		if (CommaPos == string::npos) return false;
		if (!StrToSth<int>(&Sth->x, Str.substr(0, CommaPos))) return false;
		if (!StrToSth<int>(&Sth->y, Str.substr(CommaPos+1))) return false;
		return true;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<common::VEC2>
{
	void operator () (string *Str, const common::VEC2 &Sth)
	{
		*Str = common::Format("#,#") % Sth.x % Sth.y;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct StrToSth_obj<common::VEC2>
{
	bool operator () (common::VEC2 *Sth, const string &Str)
	{
		size_t CommaPos = Str.find(',');
		if (CommaPos == string::npos) return false;
		if (!StrToSth<float>(&Sth->x, Str.substr(0, CommaPos))) return false;
		if (!StrToSth<float>(&Sth->y, Str.substr(CommaPos+1))) return false;
		return true;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<common::VEC3>
{
	void operator () (string *Str, const common::VEC3 &Sth)
	{
		*Str = common::Format("#,#,#") % Sth.x % Sth.y % Sth.z;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct StrToSth_obj<common::VEC3>
{
	bool operator () (common::VEC3 *Sth, const string &Str)
	{
		size_t CommaPos1 = Str.find(',');
		if (CommaPos1 == string::npos) return false;
		size_t CommaPos2 = Str.find(',', CommaPos1+1);
		if (CommaPos2 == string::npos) return false;
		if (!StrToSth<float>(&Sth->x, Str.substr(0, CommaPos1))) return false;
		if (!StrToSth<float>(&Sth->y, Str.substr(CommaPos1+1, CommaPos2-(CommaPos1+1)))) return false;
		if (!StrToSth<float>(&Sth->z, Str.substr(CommaPos2+1))) return false;
		return true;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<common::VEC4>
{
	void operator () (string *Str, const common::VEC4 &Sth)
	{
		*Str = common::Format("#,#,#,#") % Sth.x % Sth.y % Sth.z % Sth.w;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct StrToSth_obj<common::VEC4>
{
	bool operator () (common::VEC4 *Sth, const string &Str)
	{
		size_t CommaPos1 = Str.find(',');
		if (CommaPos1 == string::npos) return false;
		size_t CommaPos2 = Str.find(',', CommaPos1+1);
		if (CommaPos2 == string::npos) return false;
		size_t CommaPos3 = Str.find(',', CommaPos2+1);
		if (CommaPos3 == string::npos) return false;
		if (!StrToSth<float>(&Sth->x, Str.substr(0, CommaPos1))) return false;
		if (!StrToSth<float>(&Sth->y, Str.substr(CommaPos1+1, CommaPos2-(CommaPos1+1)))) return false;
		if (!StrToSth<float>(&Sth->z, Str.substr(CommaPos2+1, CommaPos3-(CommaPos2+1)))) return false;
		if (!StrToSth<float>(&Sth->w, Str.substr(CommaPos3+1))) return false;
		return true;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<common::RECTI>
{
	void operator () (string *Str, const common::RECTI &Sth)
	{
		*Str = common::Format("#,#,#,#") % Sth.left % Sth.top % Sth.right % Sth.bottom;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct StrToSth_obj<common::RECTI>
{
	bool operator () (common::RECTI *Sth, const string &Str)
	{
		size_t CommaPos1 = Str.find(',');
		if (CommaPos1 == string::npos) return false;
		size_t CommaPos2 = Str.find(',', CommaPos1+1);
		if (CommaPos2 == string::npos) return false;
		size_t CommaPos3 = Str.find(',', CommaPos2+1);
		if (CommaPos3 == string::npos) return false;
		if (!StrToSth<int>(&Sth->left,   Str.substr(0, CommaPos1))) return false;
		if (!StrToSth<int>(&Sth->top,    Str.substr(CommaPos1+1, CommaPos2-(CommaPos1+1)))) return false;
		if (!StrToSth<int>(&Sth->right,  Str.substr(CommaPos2+1, CommaPos3-(CommaPos2+1)))) return false;
		if (!StrToSth<int>(&Sth->bottom, Str.substr(CommaPos3+1))) return false;
		return true;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<common::RECTF>
{
	void operator () (string *Str, const common::RECTF &Sth)
	{
		*Str = common::Format("#,#,#,#") % Sth.left % Sth.top % Sth.right % Sth.bottom;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct StrToSth_obj<common::RECTF>
{
	bool operator () (common::RECTF *Sth, const string &Str)
	{
		size_t CommaPos1 = Str.find(',');
		if (CommaPos1 == string::npos) return false;
		size_t CommaPos2 = Str.find(',', CommaPos1+1);
		if (CommaPos2 == string::npos) return false;
		size_t CommaPos3 = Str.find(',', CommaPos2+1);
		if (CommaPos3 == string::npos) return false;
		if (!StrToSth<float>(&Sth->left,   Str.substr(0, CommaPos1))) return false;
		if (!StrToSth<float>(&Sth->top,    Str.substr(CommaPos1+1, CommaPos2-(CommaPos1+1)))) return false;
		if (!StrToSth<float>(&Sth->right,  Str.substr(CommaPos2+1, CommaPos3-(CommaPos2+1)))) return false;
		if (!StrToSth<float>(&Sth->bottom, Str.substr(CommaPos3+1))) return false;
		return true;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<common::BOX>
{
	void operator () (string *Str, const common::BOX &Sth)
	{
		*Str = common::Format("#;#") % Sth.p1 % Sth.p2;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct StrToSth_obj<common::BOX>
{
	bool operator () (common::BOX *Sth, const string &Str)
	{
		size_t SemicolonPos = Str.find(';');
		if (SemicolonPos == string::npos) return false;
		if (!StrToSth<common::VEC3>(&Sth->p1, Str.substr(0, SemicolonPos))) return false;
		if (!StrToSth<common::VEC3>(&Sth->p2, Str.substr(SemicolonPos+1))) return false;
		return true;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<common::PLANE>
{
	void operator () (string *Str, const common::PLANE &Sth)
	{
		*Str = common::Format("#,#,#,#") % Sth.a % Sth.b % Sth.c % Sth.d;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct StrToSth_obj<common::PLANE>
{
	bool operator () (common::PLANE *Sth, const string &Str)
	{
		size_t CommaPos1 = Str.find(',');
		if (CommaPos1 == string::npos) return false;
		size_t CommaPos2 = Str.find(',', CommaPos1+1);
		if (CommaPos2 == string::npos) return false;
		size_t CommaPos3 = Str.find(',', CommaPos2+1);
		if (CommaPos3 == string::npos) return false;
		if (!StrToSth<float>(&Sth->a, Str.substr(0, CommaPos1))) return false;
		if (!StrToSth<float>(&Sth->b, Str.substr(CommaPos1+1, CommaPos2-(CommaPos1+1)))) return false;
		if (!StrToSth<float>(&Sth->c, Str.substr(CommaPos2+1, CommaPos3-(CommaPos2+1)))) return false;
		if (!StrToSth<float>(&Sth->d, Str.substr(CommaPos3+1))) return false;
		return true;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<common::LINE2D>
{
	void operator () (string *Str, const common::LINE2D &Sth)
	{
		*Str = common::Format("#,#,#") % Sth.a % Sth.b % Sth.c;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct StrToSth_obj<common::LINE2D>
{
	bool operator () (common::LINE2D *Sth, const string &Str)
	{
		size_t CommaPos1 = Str.find(',');
		if (CommaPos1 == string::npos) return false;
		size_t CommaPos2 = Str.find(',', CommaPos1+1);
		if (CommaPos2 == string::npos) return false;
		if (!StrToSth<float>(&Sth->a, Str.substr(0, CommaPos1))) return false;
		if (!StrToSth<float>(&Sth->b, Str.substr(CommaPos1+1, CommaPos2-(CommaPos1+1)))) return false;
		if (!StrToSth<float>(&Sth->c, Str.substr(CommaPos2+1))) return false;
		return true;
	}
	static inline bool IsSupported() { return true; }
};

template <> struct SthToStr_obj<common::MATRIX> {
	void operator () (string *Str, const common::MATRIX &Sth) { common::MatrixToStr(Str, Sth); }
	static inline bool IsSupported() { return true; }
};
template <> struct StrToSth_obj<common::MATRIX> {
	bool operator () (common::MATRIX *Sth, const string &Str) { return common::StrToMatrix(Sth, Str); }
	static inline bool IsSupported() { return true; }
};

template <>
struct SthToStr_obj<common::QUATERNION>
{
	void operator () (string *Str, const common::QUATERNION &Sth)
	{
		*Str = common::Format("#,#,#,#") % Sth.x % Sth.y % Sth.z % Sth.w;
	}
	static inline bool IsSupported() { return true; }
};

template <>
struct StrToSth_obj<common::QUATERNION>
{
	bool operator () (common::QUATERNION *Sth, const string &Str)
	{
		size_t CommaPos1 = Str.find(',');
		if (CommaPos1 == string::npos) return false;
		size_t CommaPos2 = Str.find(',', CommaPos1+1);
		if (CommaPos2 == string::npos) return false;
		size_t CommaPos3 = Str.find(',', CommaPos2+1);
		if (CommaPos3 == string::npos) return false;
		if (!StrToSth<float>(&Sth->x, Str.substr(0, CommaPos1))) return false;
		if (!StrToSth<float>(&Sth->y, Str.substr(CommaPos1+1, CommaPos2-(CommaPos1+1)))) return false;
		if (!StrToSth<float>(&Sth->z, Str.substr(CommaPos2+1, CommaPos3-(CommaPos2+1)))) return false;
		if (!StrToSth<float>(&Sth->w, Str.substr(CommaPos3+1))) return false;
		return true;
	}
	static inline bool IsSupported() { return true; }
};
