#pragma once

//-----------------------------------------------------------------------------
// Sta³e
const float PI = 3.14159265358979323846f;
const float SQRT_2 = 1.41421356237f;
const float G = 9.8105f;
const float MAX_ANGLE = PI - FLT_EPSILON;

//-----------------------------------------------------------------------------
// Bufor
extern char BUF[256];

//-----------------------------------------------------------------------------
// Kolory DWORD
#define BLACK 0xFF000000
#define RED   0xFFFF0000
#define GREEN 0xFF00FF00
#define BLUE  0xFF0000FF
#define WHITE 0xFFFFFFFF

#define COLOR_RGB(r,g,b) D3DCOLOR_XRGB(r,g,b)
#define COLOR_RGBA(r,g,b,a) D3DCOLOR_ARGB(a,r,g,b)

//-----------------------------------------------------------------------------
// Makra
#undef NULL
#define BIT(bit) (1<<(bit))
#define IS_SET(flaga,bit) (((flaga) & (bit)) != 0)
#define IS_CLEAR(flaga,bit) (((flaga) & (bit)) == 0)
#define IS_ALL_SET(flaga,bity) (((flaga) & (bity)) == (bity))
#define SET_BIT(flaga,bit) ((flaga) |= (bit))
#define CLEAR_BIT(flaga,bit) ((flaga) &= ~(bit))
#define SET_BIT_VALUE(flaga,bit,wartos) { if(wartos) SET_BIT(flaga,bit); else CLEAR_BIT(flaga,bit); }
#define COPY_BIT(flaga,flaga2,bit) { if(((flaga2) & (bit)) != 0) SET_BIT(flaga,bit); else CLEAR_BIT(flaga,bit); }
#define FLT10(x) (float(int((x)*10))/10)
#define OR2_EQ(var,val1,val2) (((var) == (val1)) || ((var) == (val2)))
#define OR3_EQ(var,val1,val2,val3) (((var) == (val1)) || ((var) == (val2)) || ((var) == (val3)))
// makro na rozmiar tablicy
template <typename T, size_t N>
char(&_ArraySizeHelper(T(&array)[N]))[N];
#define countof( array ) (sizeof( _ArraySizeHelper( array ) ))
#define random_string(ss) ((cstring)((ss)[rand2()%countof(ss)]))
#ifndef STRING
#	define _STRING(str) #str
#	define STRING(str) _STRING(str)
#endif
#define _JOIN(a,b) a##b
#define JOIN(a,b) _JOIN(a,b)
#define FLT_(x, m) (float(int(x*m))/m)
#define FLT_1(x) FLT_(x, 10)
#define FLT_2(x) FLT_(x, 100)

//-----------------------------------------------------------------------------
// Debugowanie
#ifdef _DEBUG
#	ifndef NO_DIRECT_X
extern HRESULT _d_hr;
#		define V(x) assert(SUCCEEDED(_d_hr = (x)))
#	endif
#	define DEBUG_DO(x) (x)
#	define C(x) assert(x)
#else
#	ifndef NO_DIRECT_X
#		define V(x) (x)
#	endif
#	define DEBUG_DO(x)
#	define C(x) x
#endif
#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC2__ __FILE__ "("__STR1__(__LINE__)") : "
#ifndef _DEBUG
#	define FIXME __pragma(message(__LOC2__ "error: FIXME in release build!"))
#else
#	define FIXME
#endif

//-----------------------------------------------------------------------------
// Typy zmiennych
typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int uint;
typedef __int64 int64;
typedef unsigned __int64 uint64;
typedef const char* cstring;

//-----------------------------------------------------------------------------
// Inne typy
#ifndef COMMON_ONLY
typedef FMOD::Sound* SOUND;
#endif
struct Animesh;
struct AnimeshInstance;

//-----------------------------------------------------------------------------
// Delegates
#include "FastFunc.h"

template<typename T>
using delegate = ssvu::FastFunc<T>;
typedef delegate<void()> VoidDelegate;
typedef delegate<void()> VoidF;
typedef delegate<void(cstring)> PrintMsgFunc;

//-----------------------------------------------------------------------------
// Typy zmiennych directx
#ifndef NO_DIRECT_X
typedef ID3DXFont* FONT;
typedef LPDIRECT3DINDEXBUFFER9 IB;
typedef D3DXMATRIX MATRIX;
typedef IDirect3DTexture9* TEX;
typedef IDirect3DSurface9* SURFACE;
typedef D3DXQUATERNION QUAT;
typedef LPDIRECT3DVERTEXBUFFER9 VB;
typedef D3DXVECTOR2 VEC2;
typedef D3DXVECTOR3 VEC3;
typedef D3DXVECTOR4 VEC4;
#endif

//-----------------------------------------------------------------------------
// funkcja do zwalniania obiektów directx
template<typename T>
inline void SafeRelease(T& x)
{
	if(x)
	{
		x->Release();
		x = nullptr;
	}
}

//-----------------------------------------------------------------------------
// Generator liczb pseudolosowych (z MSVC CRT)
struct RNG
{
	unsigned long val;

	RNG() : val(1)
	{
	}

	inline int rand2()
	{
		val = val * 214013L + 2531011L;
		return ((val >> 16) & 0x7fff);
	}

	inline int rand2_tmp()
	{
		int tval = val * 214013L + 2531011L;
		return ((tval >> 16) & 0x7fff);
	}
};
extern RNG _RNG;

inline int rand2()
{
	return _RNG.rand2();
}
inline int rand2(int a)
{
	assert(a > 0);
	return _RNG.rand2() % a;
}
inline void srand2(int x)
{
	_RNG.val = x;
}
inline uint rand_r2()
{
	return _RNG.val;
}
inline int rand2_tmp()
{
	return _RNG.rand2_tmp();
}
// u¿ywane dla random_shuffle
inline int myrand(int n)
{
	return rand2() % n;
}

//-----------------------------------------------------------------------------
// Funkcje matematyczne
template<typename T>
inline void MinMax(T a, T b, T& _min, T& _max)
{
	if(a > b)
	{
		_min = b;
		_max = a;
	}
	else
	{
		_min = a;
		_max = b;
	}
}

template<typename T>
inline T clamp(T f, T _min, T _max)
{
	if(f > _max)
		return _max;
	else if(f < _min)
		return _min;
	else
		return f;
}

// losowa liczba z przedzia³u <0,1>
inline float random()
{
	return (float)rand2() / RAND_MAX;
}

// losowa liczba z przedzia³u <0,a>
template<typename T>
inline T random(T a)
{
	assert(a >= 0);
	return rand2(a + 1);
}
template<>
inline float random(float a)
{
	return ((float)rand2() / RAND_MAX)*a;
}
// losowa liczba z przedzia³u <a,b>
template<typename T>
inline T random(T a, T b)
{
	assert(b >= a);
	return rand2() % (b - a + 1) + a;
}
template<>
inline float random(float a, float b)
{
	assert(b >= a);
	return ((float)rand2() / RAND_MAX)*(b - a) + a;
}

inline float random_part(int parts)
{
	return 1.f / parts * (rand2() % parts);
}

inline float random_normalized(float val)
{
	return (random(-val, val) + random(-val, val)) / 2;
}

// losowa liczba z przedzia³u <a,b>, sprawdza czy nie s¹ równe
// czy to jest potrzebne?
template<typename T>
inline T random2(T a, T b)
{
	if(a == b)
		return a;
	else
		return random(a, b);
}

template<typename T>
inline T Chance(int c, T a, T b)
{
	return (rand2() % c == 0 ? a : b);
}

template<typename T>
inline T Chance3(int chance_a, int chance_b, int chance_c, T a, T b, T c)
{
	int ch = rand2() % (chance_a + chance_b + chance_c);
	if(ch < chance_a)
		return a;
	else if(ch < chance_a + chance_b)
		return b;
	else
		return c;
}

// minimum z 3 argumentów
template<typename T>
inline T min3(T a, T b, T c)
{
	return min(a, min(b, c));
}

// dystans pomiêdzy punktami 2d
template<typename T>
inline T distance(T x1, T y1, T x2, T y2)
{
	T x = abs(x1 - x2);
	T y = abs(y1 - y2);
	return sqrt(x*x + y*y);
}

inline float distance_sqrt(float x1, float y1, float x2, float y2)
{
	float x = abs(x1 - x2),
		y = abs(y1 - y2);
	return x*x + y*y;
}

inline float clip(float f, float range = PI * 2)
{
	int n = (int)floor(f / range);
	return f - range * n;
}

#ifndef NO_DIRECT_X
inline VEC2 clip(const VEC2& v)
{
	return VEC2(clip(v.x), clip(v.y));
}
#endif

float angle(float x1, float y1, float x2, float y2);

inline float angle_dif(float a, float b)
{
	assert(a >= 0.f && a < PI * 2 && b >= 0.f && b < PI * 2);
	return min((2 * PI) - abs(a - b), abs(b - a));
}

inline bool equal(float a, float b)
{
	return abs(a - b) < std::numeric_limits<float>::epsilon();
}

inline bool equal(float a, float b, float e)
{
	return abs(a - b) < e;
}

inline bool not_zero(float a)
{
	return abs(a) >= std::numeric_limits<float>::epsilon();
}

inline bool is_zero(float a)
{
	return abs(a) < std::numeric_limits<float>::epsilon();
}

template<typename T>
inline T sign(T f)
{
	if(f > 0)
		return 1;
	else if(f < 0)
		return -1;
	else
		return 0;
}

inline float lerp(float a, float b, float t)
{
	return (b - a)*t + a;
}

inline int lerp(int a, int b, float t)
{
	return int(t*(b - a)) + a;
}

// W któr¹ stronê trzeba siê obróciæ ¿eby by³o najszybciej
float shortestArc(float a, float b);

// Interpolacja k¹tów
void lerp_angle(float& angle, float from, float to, float t);

template<typename T>
inline bool in_range(T v, T left, T right)
{
	return (v >= left && v <= right);
}

template<typename T>
inline bool in_range2(T left, T a, T b, T right)
{
	return (a >= left && b >= a && b <= right);
}

inline float slerp(float a, float b, float t)
{
	float angle = shortestArc(a, b);
	return a + angle * t;
}

#ifndef NO_DIRECT_X
inline VEC2 slerp(const VEC2& a, const VEC2& b, float t)
{
	return VEC2(slerp(a.x, b.x, t), slerp(a.y, b.y, t));
}
#endif

inline int count_bits(int i)
{
	// It's write-only code. Just put a comment that you are not meant to understand or maintain this code, just worship the gods that revealed it to mankind.
	i = i - ((i >> 1) & 0x55555555);
	i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
	return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

inline float inf()
{
	return std::numeric_limits<float>::infinity();
}

inline float ToRadians(float degrees)
{
	return degrees * PI / 180;
}

inline float ToDegrees(float radians)
{
	return radians * 180 / PI;
}

inline bool is_pow2(int x)
{
	return ((x > 0) && ((x & (x - 1)) == 0));
}

inline int roundi(float value)
{
	return (int)round(value);
}

//-----------------------------------------------------------------------------
// Punkt na liczbach ca³kowitych
//-----------------------------------------------------------------------------
struct INT2
{
	int x, y;

	inline INT2() {}
	template<typename T, typename T2>
	inline INT2(T _x, T2 _y) : x(int(_x)), y(int(_y)) {}
	inline explicit INT2(int v) : x(v), y(v) {}
	inline INT2(int x, int y) : x(x), y(y) {}
#ifndef NO_DIRECT_X
	inline explicit INT2(const VEC2& v) : x(int(v.x)), y(int(v.y)) {}
	inline explicit INT2(const VEC3& v) : x(int(v.x)), y(int(v.z)) {}
#endif

	inline int operator ()(int _shift) const
	{
		return x + y * _shift;
	}

	inline bool operator == (const INT2& i) const
	{
		return (x == i.x && y == i.y);
	}

	inline bool operator != (const INT2& i) const
	{
		return (x != i.x || y != i.y);
	}

	inline INT2 operator + (const INT2& xy) const
	{
		return INT2(x + xy.x, y + xy.y);
	}

	inline INT2 operator - (const INT2& xy) const
	{
		return INT2(x - xy.x, y - xy.y);
	}

	inline INT2 operator * (int a) const
	{
		return INT2(x*a, y*a);
	}

	inline INT2 operator * (float a) const
	{
		return INT2(int(a*x), int(a*y));
	}

	inline INT2 operator / (int a) const
	{
		return INT2(x / a, y / a);
	}

	inline void operator += (const INT2& xy)
	{
		x += xy.x;
		y += xy.y;
	}

	inline void operator -= (const INT2& xy)
	{
		x -= xy.x;
		y -= xy.y;
	}

	inline void operator *= (int a)
	{
		x *= a;
		y *= a;
	}

	inline void operator /= (int a)
	{
		x /= a;
		y /= a;
	}

	inline int lerp(float t) const
	{
		return int(t*(y - x)) + x;
	}

	inline int random() const
	{
		if(x == y)
			return x;
		else
			return ::random(x, y);
	}
};

// INT2 pod
struct INT2P
{
	int x, y;
};

inline int random(const INT2& a)
{
	return random(a.x, a.y);
}

inline int random2(const INT2& xy)
{
	return random2(xy.x, xy.y);
}

inline INT2 random(const INT2& a, const INT2& b)
{
	return INT2(random(a.x, b.x), random(a.y, b.y));
}

inline int clamp(int f, const INT2& i)
{
	if(f > i.y)
		return i.y;
	else if(f < i.x)
		return i.x;
	else
		return f;
}

inline INT2 lerp(const INT2& i1, const INT2& i2, float t)
{
	return INT2((int)lerp(float(i1.x), float(i2.x), t), (int)lerp(float(i1.y), float(i2.y), t));
}

inline INT2 Min(const INT2& i1, const INT2& i2)
{
	return INT2(min(i1.x, i2.x), min(i1.y, i2.y));
}

inline INT2 Max(const INT2& i1, const INT2& i2)
{
	return INT2(max(i1.x, i2.x), max(i1.y, i2.y));
}

inline int distance(const INT2& pt1, const INT2& pt2)
{
	return abs(pt1.x - pt2.x) + abs(pt1.y - pt2.y);
}

inline float distance_float(const INT2& pt1, const INT2& pt2)
{
	return abs(float(pt1.x) - float(pt2.x)) + abs(float(pt1.y) - float(pt2.y));
}

//-----------------------------------------------------------------------------
// Punkt na liczbach ca³kowitych dodatnich
//-----------------------------------------------------------------------------
struct UINT2
{
	uint x, y;

	UINT2() {}
	UINT2(uint _x, uint _y) : x(_x), y(_y) {}
	UINT2(const UINT2& u) : x(u.x), y(u.y) {}
};

// UINT2 pod
struct UINT2P
{
	uint x, y;
};

//-----------------------------------------------------------------------------
// Prostok¹t
//-----------------------------------------------------------------------------
struct Rect
{
	int minx, miny, maxx, maxy;

	Rect() {}
	Rect(int minx, int miny, int maxx, int maxy) : minx(minx), miny(miny), maxx(maxx), maxy(maxy)
	{
		assert(minx <= maxx && miny <= maxy);
	}
};

//-----------------------------------------------------------------------------
// Funkcje VEC2
//-----------------------------------------------------------------------------
struct VEC2P
{
	float x, y;
};

#ifndef NO_DIRECT_X
inline float random(const VEC2& v)
{
	return random(v.x, v.y);
}

inline float random2(const VEC2& v)
{
	return random2(v.x, v.y);
}

inline VEC2 random_circle_pt(float r)
{
	float a = random(),
		b = random();
	if(b < a)
		std::swap(a, b);
	return VEC2(b*r*cos(2 * PI*a / b), b*r*sin(2 * PI*a / b));
}

inline void MinMax(const VEC2& a, const VEC2& b, VEC2& _min, VEC2& _max)
{
	MinMax(a.x, b.x, _min.x, _max.x);
	MinMax(a.y, b.y, _min.y, _max.y);
}

inline VEC2 clamp(const VEC2& v, const VEC2& _min, const VEC2& _max)
{
	return VEC2(clamp(v.x, _min.x, _max.x), clamp(v.y, _min.y, _max.y));
}

inline float clamp(float f, const VEC2& v)
{
	if(f > v.y)
		return v.y;
	else if(f < v.x)
		return v.x;
	else
		return f;
}

inline VEC2 clamp(const VEC2& v)
{
	return VEC2(clamp(v.x, 0.f, 1.f),
		clamp(v.y, 0.f, 1.f));
}

inline VEC2 random(const VEC2& vmin, const VEC2& vmax)
{
	return VEC2(random(vmin.x, vmax.x), random(vmin.y, vmax.y));
}

inline VEC2 random_VEC2(float a, float b)
{
	return VEC2(random(a, b), random(a, b));
}

inline float distance(const VEC2& p1, const VEC2& p2)
{
	float x = abs(p1.x - p2.x);
	float y = abs(p1.y - p2.y);
	return sqrt(x*x + y*y);
}

inline float distance_sqrt(const VEC2& p1, const VEC2& p2)
{
	float x = abs(p1.x - p2.x);
	float y = abs(p1.y - p2.y);
	return x*x + y*y;
}

inline float angle(const VEC2& v1, const VEC2& v2)
{
	return angle(v1.x, v1.y, v2.x, v2.y);
}

// zwraca k¹t taki jak w system.txt
inline float lookat_angle(const VEC2& v1, const VEC2& v2)
{
	return clip(-angle(v1, v2) - PI / 2);
}

inline bool equal(const VEC2& v1, const VEC2& v2)
{
	return equal(v1.x, v2.x) && equal(v1.y, v2.y);
}

inline VEC2 lerp(const VEC2& v1, const VEC2& v2, float t)
{
	VEC2 out;
	D3DXVec2Lerp(&out, &v1, &v2, t);
	return out;
}

inline void Min(const VEC2& v1, const VEC2& v2, VEC2& out)
{
	out.x = min(v1.x, v2.x);
	out.y = min(v1.y, v2.y);
}

inline void Max(const VEC2& v1, const VEC2& v2, VEC2& out)
{
	out.x = max(v1.x, v2.x);
	out.y = max(v1.y, v2.y);
}
#endif

//-----------------------------------------------------------------------------
// Funkcje VEC3
//-----------------------------------------------------------------------------
// VEC3 POD
struct VEC3P
{
	float x, y, z;

#ifndef NO_DIRECT_X
	inline void Build(const VEC3& v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
	}

	inline operator VEC3 () const
	{
		return VEC3(x, y, z);
	}
#endif
};

#ifndef NO_DIRECT_X
inline void MinMax(const VEC3& a, const VEC3& b, VEC3& _min, VEC3& _max)
{
	MinMax(a.x, b.x, _min.x, _max.x);
	MinMax(a.y, b.y, _min.y, _max.y);
	MinMax(a.z, b.z, _min.z, _max.z);
}

inline VEC3 clamp(const VEC3& v, const VEC3& _min, const VEC3& _max)
{
	return VEC3(clamp(v.x, _min.x, _max.x), clamp(v.y, _min.y, _max.y), clamp(v.z, _min.z, _max.z));
}

inline VEC3 clamp(const VEC3& v)
{
	return VEC3(clamp(v.x, 0.f, 1.f),
		clamp(v.y, 0.f, 1.f),
		clamp(v.z, 0.f, 1.f));
}

inline VEC3 random(const VEC3& vmin, const VEC3& vmax)
{
	return VEC3(random(vmin.x, vmax.x), random(vmin.y, vmax.y), random(vmin.z, vmax.z));
}

inline VEC3 random_VEC3(float a, float b)
{
	return VEC3(random(a, b), random(a, b), random(a, b));
}

inline float distance2d(const VEC3& v1, const VEC3& v2)
{
	float x = abs(v1.x - v2.x),
		z = abs(v1.z - v2.z);
	return sqrt(x*x + z*z);
}

// dystans pomiêdzy punktami 3d
inline float distance(const VEC3& v1, const VEC3& v2)
{
	float x = abs(v1.x - v2.x),
		y = abs(v1.y - v2.y),
		z = abs(v1.z - v2.z);
	return sqrt(x*x + y*y + z*z);
}

inline float distance_sqrt(const VEC3& v1, const VEC3& v2)
{
	float x = abs(v1.x - v2.x),
		y = abs(v1.y - v2.y),
		z = abs(v1.z - v2.z);
	return x*x + y*y + z*z;
}

inline float angle2d(const VEC3& v1, const VEC3& v2)
{
	return angle(v1.x, v1.z, v2.x, v2.z);
}

inline float lookat_angle(const VEC3& v1, const VEC3& v2)
{
	return clip(-angle2d(v1, v2) - PI / 2);
}

inline bool equal(const VEC3& v1, const VEC3& v2)
{
	return equal(v1.x, v2.x) && equal(v1.y, v2.y) && equal(v1.z, v2.z);
}

inline VEC3 lerp(const VEC3& v1, const VEC3& v2, float t)
{
	VEC3 out;
	D3DXVec3Lerp(&out, &v1, &v2, t);
	return out;
}

inline void Min(const VEC3& v1, const VEC3& v2, VEC3& out)
{
	out.x = min(v1.x, v2.x);
	out.y = min(v1.y, v2.y);
	out.z = min(v1.z, v2.z);
}

inline void Max(const VEC3& v1, const VEC3& v2, VEC3& out)
{
	out.x = max(v1.x, v2.x);
	out.y = max(v1.y, v2.y);
	out.z = max(v1.z, v2.z);
}

inline VEC3 VEC3_x0y(const VEC2& _v, float y = 0.f)
{
	return VEC3(_v.x, y, _v.y);
}

inline VEC2 ToVEC2(const VEC3& v)
{
	return VEC2(v.x, v.z);
}

//-----------------------------------------------------------------------------
// Funkcje VEC4
//-----------------------------------------------------------------------------
inline VEC4 clamp(const VEC4& v)
{
	return VEC4(clamp(v.x, 0.f, 1.f),
		clamp(v.y, 0.f, 1.f),
		clamp(v.z, 0.f, 1.f),
		clamp(v.w, 0.f, 1.f));
}

inline bool equal(const VEC4& v1, const VEC4& v2)
{
	return equal(v1.x, v2.x) && equal(v1.y, v2.y) && equal(v1.z, v2.z) && equal(v1.w, v2.w);
}

#endif

//-----------------------------------------------------------------------------
// Prostok¹t na int
//-----------------------------------------------------------------------------
struct IBOX2D
{
	INT2 p1, p2;

	IBOX2D() {}
	IBOX2D(int x1, int y1, int x2, int y2) : p1(x1, y1), p2(x2, y2) {}

	inline void Set(int x1, int y1, int x2, int y2)
	{
		p1.x = x1;
		p1.y = y1;
		p2.x = x2;
		p2.y = y2;
	}

	inline INT2 Random() const
	{
		return INT2(random(p1.x, p2.x), random(p1.y, p2.y));
	}

	inline IBOX2D LeftBottomPart() const
	{
		return IBOX2D(p1.x, p1.y, p1.x + (p2.x - p1.x) / 2, p1.y + (p2.y - p1.y) / 2);
	}
	inline IBOX2D RightBottomPart() const
	{
		return IBOX2D(p1.x + (p2.x - p1.x) / 2, p1.y, p2.x, p1.y + (p2.y - p1.y) / 2);
	}
	inline IBOX2D LeftTopPart() const
	{
		return IBOX2D(p1.x, p1.y + (p2.y - p1.y) / 2, p1.x + (p2.x - p1.x) / 2, p2.y);
	}
	inline IBOX2D RightTopPart() const
	{
		return IBOX2D(p1.x + (p2.x - p1.x) / 2, p1.y + (p2.y - p1.y) / 2, p2.x, p2.y);
	}
};

//-----------------------------------------------------------------------------
// Prostok¹t na float
//-----------------------------------------------------------------------------
#ifndef NO_DIRECT_X
struct BOX2D
{
	VEC2 v1, v2;

	BOX2D() {}
	BOX2D(float _minx, float _miny, float _maxx, float _maxy) : v1(_minx, _miny), v2(_maxx, _maxy)
	{
		assert(v1.x <= v2.x && v1.y <= v2.y);
	}
	BOX2D(const VEC2& _v1, const VEC2& _v2) : v1(_v1), v2(_v2)
	{
		assert(v1.x <= v2.x && v1.y <= v2.y);
	}
	BOX2D(const BOX2D& _box) : v1(_box.v1), v2(_box.v2)
	{
		// assert fires on resize
		//assert(v1.x <= v2.x && v1.y <= v2.y);
	}
	BOX2D(float _x, float _y) : v1(_x, _y), v2(_x, _y)
	{
	}
	explicit BOX2D(const VEC2& v) : v1(v), v2(v)
	{
	}
	BOX2D(const BOX2D& _box, float margin) : v1(_box.v1.x - margin, _box.v1.y - margin), v2(_box.v2.x + margin, _box.v2.y + margin)
	{
	}

	inline VEC2 Midpoint() const
	{
		return v1 + (v2 - v1) / 2;
	}

	inline bool IsInside(const VEC3& pos) const
	{
		return pos.x >= v1.x && pos.x <= v2.x && pos.z >= v1.y && pos.z <= v2.y;
	}

	inline float SizeX() const { return abs(v2.x - v1.x); }
	inline float SizeY() const { return abs(v2.y - v1.y); }
	inline VEC2 Size() const { return VEC2(SizeX(), SizeY()); }

	inline bool IsValid() const
	{
		return v1.x <= v2.x && v1.y <= v2.y;
	}

	inline VEC2 GetRandomPos() const
	{
		return VEC2(random(v1.x, v2.x), random(v1.y, v2.y));
	}
	inline VEC3 GetRandomPos3(float y = 0.f) const
	{
		return VEC3(random(v1.x, v2.x), y, random(v1.y, v2.y));
	}

	inline VEC2 LeftTop() const
	{
		return v1;
	}
	inline VEC2 RightBottom() const
	{
		return v2;
	}
	inline VEC2 RightTop() const
	{
		return VEC2(v2.x, v1.y);
	}
	inline VEC2 LeftBottom() const
	{
		return VEC2(v1.x, v2.y);
	}

	inline BOX2D LeftBottomPart() const
	{
		return BOX2D(v1.x, v1.y, v1.x + (v2.x - v1.x) / 2, v1.y + (v2.y - v1.y) / 2);
	}
	inline BOX2D RightBottomPart() const
	{
		return BOX2D(v1.x + (v2.x - v1.x) / 2, v1.y, v2.x, v1.y + (v2.y - v1.y) / 2);
	}
	inline BOX2D LeftTopPart() const
	{
		return BOX2D(v1.x, v1.y + (v2.y - v1.y) / 2, v1.x + (v2.x - v1.x) / 2, v2.y);
	}
	inline BOX2D RightTopPart() const
	{
		return BOX2D(v1.x + (v2.x - v1.x) / 2, v1.y + (v2.y - v1.y) / 2, v2.x, v2.y);
	}

	inline void ToRectangle(float& x, float& y, float& w, float& h) const
	{
		x = v1.x + (v2.x - v1.x) / 2;
		y = v1.y + (v2.y - v1.y) / 2;
		w = (v2.x - v1.x) / 2;
		h = (v2.y - v1.y) / 2;
	}
};
#endif

//-----------------------------------------------------------------------------
// Szeœcian opisany dwoma punktami
//-----------------------------------------------------------------------------
#ifndef NO_DIRECT_X
struct BOX
{
	VEC3 v1, v2;

	BOX() {}
	BOX(float _minx, float _miny, float _minz, float _maxx, float _maxy, float _maxz) : v1(_minx, _miny, _minz), v2(_maxx, _maxy, _maxz)
	{
		assert(v1.x <= v2.x && v1.y <= v2.y && v1.z <= v2.z);
	}
	BOX(const VEC3& _v1, const VEC3& _v2) : v1(_v1), v2(_v2)
	{
		assert(v1.x <= v2.x && v1.y <= v2.y && v1.z <= v2.z);
	}
	BOX(const BOX& _box) : v1(_box.v1), v2(_box.v2)
	{
		// assert fires on resize
		//assert(v1.x <= v2.x && v1.y <= v2.y && v1.z <= v2.z);
	}
	BOX(float _x, float _y, float _z) : v1(_x, _y, _z), v2(_x, _y, _z)
	{
	}
	explicit BOX(const VEC3& v) : v1(v), v2(v)
	{
	}

	inline void Create(const VEC3& _v1, const VEC3& _v2)
	{
		MinMax(_v1, _v2, v1, v2);
	}

	inline VEC3 Midpoint() const
	{
		return v1 + (v2 - v1) / 2;
	}

	inline float SizeX() const { return abs(v2.x - v1.x); }
	inline float SizeY() const { return abs(v2.y - v1.y); }
	inline float SizeZ() const { return abs(v2.z - v1.z); }
	inline VEC3 Size() const { return VEC3(SizeX(), SizeY(), SizeZ()); }
	inline VEC2 SizeXZ() const { return VEC2(SizeX(), SizeZ()); }

	inline bool IsValid() const
	{
		return v1.x <= v2.x && v1.y <= v2.y && v1.z <= v2.z;
	}

	inline bool IsInside(const VEC3& v) const
	{
		return v.x >= v1.x && v.x <= v2.x && v.y >= v1.y && v.y <= v2.y && v.z >= v1.z && v.z <= v2.z;
	}
	
	inline VEC3 GetRandomPos() const
	{
		return VEC3(random(v1.x, v2.x), random(v1.y, v2.y), random(v1.z, v2.z));
	}
};

void CreateAABBOX(BOX& out_box, const MATRIX& mat);

//-----------------------------------------------------------------------------
// obrócony bounding box
//-----------------------------------------------------------------------------
struct OOBBOX
{
	VEC3 pos;
	VEC3 size;
	MATRIX rot;
};

// inna wersja, okarze siê czy lepszy algorymt
struct OOB
{
	VEC3 c; // œrodek
	VEC3 u[3]; // obrót po X,Y,Z
	VEC3 e; // po³owa rozmiaru
};

//-----------------------------------------------------------------------------
// dodatkowe funkcje matematyczne directx
// mo¿na zrobiæ to u¿ywaj¹c innych ale tak jest krócej
//-----------------------------------------------------------------------------
inline MATRIX* D3DXMatrixTranslation(MATRIX* mat, const VEC3& pos)
{
	return D3DXMatrixTranslation(mat, pos.x, pos.y, pos.z);
}

inline MATRIX* D3DXMatrixScaling(MATRIX* mat, const VEC3& scale)
{
	return D3DXMatrixScaling(mat, scale.x, scale.y, scale.z);
}

inline MATRIX* D3DXMatrixScaling(MATRIX* mat, float scale)
{
	return D3DXMatrixScaling(mat, scale, scale, scale);
}

inline MATRIX* D3DXMatrixRotation(MATRIX* mat, const VEC3& rot)
{
	return D3DXMatrixRotationYawPitchRoll(mat, rot.y, rot.x, rot.z);
}

inline void D3DXVec2Normalize(VEC2& v)
{
	D3DXVec2Normalize(&v, &v);
}

inline void D3DXVec3Normalize(VEC3* v)
{
	D3DXVec3Normalize(v, v);
}

inline VEC3* D3DXVec3Transform(VEC3* out, const VEC3* in, const MATRIX* mat)
{
	D3DXVECTOR4 v;
	D3DXVec3Transform(&v, in, mat);
	out->x = v.x;
	out->y = v.y;
	out->z = v.z;
	return out;
}

inline void VEC3Multiply(VEC3& out, const VEC3& v1, const VEC3& v2)
{
	out.x = v1.x * v2.x;
	out.y = v1.y * v2.y;
	out.z = v1.z * v2.z;
}

inline void D3DXQuaternionRotation(QUAT& q, const VEC3& rot)
{
	D3DXQuaternionRotationYawPitchRoll(&q, rot.y, rot.x, rot.z);
}

// !! ta funkcja zak³ada okreœlon¹ kolejnoœæ wykonywania obrotów (chyba YXZ), w blenderze domyœlnie jest XYZ ale mo¿na zmieniæ
// nie u¿ywaæ, u¿ywaæ quaternion xD
inline float MatrixGetYaw(const MATRIX& m)
{
	if(m._21 > 0.998f || m._21 < -0.998f)
		return atan2(m._13, m._33);
	else
		return atan2(-m._31, m._11);
}
#endif

namespace core
{
	namespace io
	{
		// Delete directory.
		bool DeleteDirectory(cstring dir);
		// Check if directory exists.
		bool DirectoryExists(cstring dir);
		// Check if file exists.
		bool FileExists(cstring filename);
		// Find files matching pattern, return false from func to stop.
		bool FindFiles(cstring pattern, const std::function<bool (const WIN32_FIND_DATA&)>& func, bool exclude_special = true);
	}
}

//-----------------------------------------------------------------------------
// Pozosta³e funkcje
//-----------------------------------------------------------------------------
cstring Format(cstring fmt, ...);
cstring FormatList(cstring fmt, va_list lis);
cstring Upper(cstring str);
void Crypt(char* inp, uint inplen, cstring key, uint keylen);
void SplitText(char* buf, vector<cstring>& lines);
bool Unescape(const string& str_in, uint pos, uint length, string& str_out);
inline bool Unescape(const string& str_in, string& str_out)
{
	return Unescape(str_in, 0u, str_in.length(), str_out);
}
bool StringInString(cstring s1, cstring s2);
cstring Escape(cstring str);
string* ToString(const wchar_t* str);

// return index of character in cstring
inline int strchr_index(cstring chrs, char c)
{
	int index = 0;
	do
	{
		if(*chrs == c)
			return index;
		++index;
		++chrs;
	} while(*chrs);

	return -1;
}

//-----------------------------------------------------------------------------
// Funkcje obs³uguj¹ce vector
//-----------------------------------------------------------------------------
// Usuwanie elementów wektora
template<typename T>
inline void DeleteElements(vector<T>& v)
{
	for(vector<T>::iterator it = v.begin(), end = v.end(); it != end; ++it)
		delete *it;
	v.clear();
}

template<typename T>
inline void DeleteElements(vector<T>* v)
{
	DeleteElements(*v);
}

template<typename T>
inline void DeleteElementsChecked(vector<T>& v)
{
	for(T& item : v)
	{
		assert(item);
		delete item;
	}
	v.clear();
}

// usuñ pojedyñczy element z wektora, kolejnoœæ nie jest wa¿na
template<typename T>
inline void DeleteElement(vector<T>& v, T& e)
{
	for(typename vector<T>::iterator it = v.begin(), end = v.end(); it != end; ++it)
	{
		if(e == *it)
		{
			delete *it;
			std::iter_swap(it, end - 1);
			v.pop_back();
			return;
		}
	}

	assert(0 && "Nie znaleziono elementu do usuniecia!");
}

template<typename T>
void DeleteElement(vector<T>* v, T& e)
{
	DeleteElement(*v, e);
}

template<typename T>
inline bool DeleteElementTry(vector<T>& v, T& e)
{
	for(typename vector<T>::iterator it = v.begin(), end = v.end(); it != end; ++it)
	{
		if(e == *it)
		{
			delete *it;
			std::iter_swap(it, end - 1);
			v.pop_back();
			return true;
		}
	}

	return false;
}

template<typename T>
inline void RemoveElement(vector<T>& v, const T& e)
{
	for(typename vector<T>::iterator it = v.begin(), end = v.end(); it != end; ++it)
	{
		if(e == *it)
		{
			std::iter_swap(it, end - 1);
			v.pop_back();
			return;
		}
	}

	assert(0 && "Nie znaleziono elementu do wyrzucenia!");
}

template<typename T>
inline void RemoveElement(vector<T>* v, const T& e)
{
	RemoveElement(*v, e);
}

template<typename T>
inline void RemoveElementOrder(vector<T>& v, const T& e)
{
	for(typename vector<T>::iterator it = v.begin(), end = v.end(); it != end; ++it)
	{
		if(e == *it)
		{
			v.erase(it);
			return;
		}
	}

	assert(0 && "Nie znaleziono elementu do wyrzucenia!");
}

template<typename T>
inline void RemoveElementOrder(vector<T>* v, const T& e)
{
	RemoveElementOrder(*v, e);
}

template<typename T>
inline bool RemoveElementTry(vector<T>& v, const T& e)
{
	for(typename vector<T>::iterator it = v.begin(), end = v.end(); it != end; ++it)
	{
		if(e == *it)
		{
			std::iter_swap(it, end - 1);
			v.pop_back();
			return true;
		}
	}

	return false;
}

template<typename T>
inline bool RemoveElementTry(vector<T>* v, const T& e)
{
	return RemoveElementTry(*v, e);
}

template<typename T>
inline void RemoveElementIndex(vector<T>& v, int index)
{
	std::iter_swap(v.begin() + index, v.end() - 1);
	v.pop_back();
}

template<typename T>
inline void RemoveElementIndexOrder(vector<T>& v, int index)
{
	v.erase(v.begin() + index);
}

template<typename T>
inline bool is_null(const T ptr)
{
	return !ptr;
}

template<typename T>
inline void RemoveNullElements(vector<T>& v)
{
	auto it = std::remove_if(v.begin(), v.end(), is_null<T>);
	auto end = v.end();
	if(it != end)
		v.erase(it, end);
}

template<typename T>
inline void RemoveNullElements(vector<T>* v)
{
	assert(v);
	RemoveNullElements(*v);
}

template<typename T, typename T2>
inline void RemoveElements(vector<T>& v, T2 pred)
{
	auto it = std::remove_if(v.begin(), v.end(), pred);
	auto end = v.end();
	if(it != end)
		v.erase(it, end);
}

template<typename T, typename T2>
inline void RemoveElements(vector<T>* v, T2 pred)
{
	assert(v);
	RemoveElements(*v, pred);
}

template<typename T>
inline T& Add1(vector<T>& v)
{
	v.resize(v.size() + 1);
	return v.back();
}

template<typename T>
inline T& Add1(vector<T>* v)
{
	v->resize(v->size() + 1);
	return v->back();
}

template<typename T>
inline T& Add1(list<T>& v)
{
	v.resize(v.size() + 1);
	return v.back();
}

// zwraca losowy element tablicy
template<typename T>
T& random_item(vector<T>& v)
{
	return v[rand2() % v.size()];
}

template<typename T>
T random_item_pop(vector<T>& v)
{
	uint index = rand2() % v.size();
	T item = v[index];
	v.erase(v.begin() + index);
	return item;
}

template<class T>
inline T RandomItem(std::initializer_list<T> cont)
{
	int index = rand2() % cont.size();
	auto it = cont.begin();
	std::advance(it, index);
	return *it;
}

template<typename T>
inline bool IsInside(const vector<T>& v, const T& elem)
{
	for(vector<T>::const_iterator it = v.begin(), end = v.end(); it != end; ++it)
	{
		if(*it == elem)
			return true;
	}

	return false;
}

template<typename T>
inline bool IsInside(const vector<T>* v, const T& elem)
{
	return IsInside(*v, elem);
}

//-----------------------------------------------------------------------------
// Funkcje do ³atwiejszej edycji bufora
//-----------------------------------------------------------------------------
template<typename T, typename T2>
inline T* ptr_shift(T2* ptr, uint shift)
{
	return ((T*)(((byte*)ptr) + shift));
}

template<typename T>
void* ptr_shift(T* ptr, uint shift)
{
	return (((byte*)ptr) + shift);
}

template<typename T, typename T2>
inline T& ptr_shiftd(T2* ptr, uint shift)
{
	return *((T*)(((byte*)ptr) + shift));
}
//-----------------------------------------------------------------------------
// Loggery
//-----------------------------------------------------------------------------
// interfejs logera
struct Logger
{
	enum LOG_LEVEL
	{
		L_INFO,
		L_WARN,
		L_ERROR,
		L_FATAL
	};

	virtual ~Logger() {}
	void GetTime(tm& time);

	virtual void Log(cstring text, LOG_LEVEL level) = 0;
	virtual void Log(cstring text, LOG_LEVEL level, const tm& time) = 0;
	virtual void Flush() = 0;

	inline void Info(cstring text) { Log(text, L_INFO); }
	inline void Warn(cstring text) { Log(text, L_WARN); }
	inline void Error(cstring text) { Log(text, L_ERROR); }
	inline void Fatal(cstring text) { Log(text, L_FATAL); }
};

// pusty loger, nic nie robi
struct NullLogger : public Logger
{
	NullLogger() {}
	void Log(cstring text, LOG_LEVEL level) {}
	void Log(cstring text, LOG_LEVEL, const tm& time) {}
	void Flush() {}
};

// loger do konsoli
struct ConsoleLogger : public Logger
{
	~ConsoleLogger();
	void Log(cstring text, LOG_LEVEL level);
	void Log(cstring text, LOG_LEVEL level, const tm& time);
	void Flush() {}
};

// loger do pliku txt
struct TextLogger : public Logger
{
	std::ofstream out;
	string path;

	explicit TextLogger(cstring filename);
	~TextLogger();
	void Log(cstring text, LOG_LEVEL level);
	void Log(cstring text, LOG_LEVEL level, const tm& time);
	void Flush();
};

// loger do kilku innych logerów
struct MultiLogger : public Logger
{
	vector<Logger*> loggers;

	~MultiLogger();
	void Log(cstring text, LOG_LEVEL level);
	void Log(cstring text, LOG_LEVEL level, const tm& time);
	void Flush();
};

// loger który przechowuje informacje przed wybraniem okreœlonego logera
struct PreLogger : public Logger
{
private:
	struct Prelog
	{
		string str;
		LOG_LEVEL level;
		tm time;
	};

	vector<Prelog*> prelogs;
	bool flush;

public:
	PreLogger() : flush(false) {}
	void Apply(Logger* logger);
	void Clear();
	void Log(cstring text, LOG_LEVEL level);
	void Log(cstring text, LOG_LEVEL level, const tm& time);
	void Flush();
};

extern Logger* logger;

// u³atwienia
#ifdef ERROR
#	undef ERROR
#endif

#define LOG(msg) logger->Info(msg)
#define INFO(msg) logger->Info(msg)
#define WARN(msg) logger->Warn(msg)
#define ERROR(msg) logger->Error(msg)
#define FATAL(msg) logger->Fatal(msg)

//-----------------------------------------------------------------------------
// KOLIZJE
//-----------------------------------------------------------------------------
#ifndef NO_DIRECT_X
// promieñ - AABOX
bool RayToBox(const VEC3 &RayOrig, const VEC3 &RayDir, const BOX &Box, float *OutT);
// promieñ - p³aszczyzna
bool RayToPlane(const VEC3 &RayOrig, const VEC3 &RayDir, const D3DXPLANE &Plane, float *OutT);
// promieñ - sfera
bool RayToSphere(const VEC3& ray_pos, const VEC3& ray_dir, const VEC3& center, float radius, float& dist);
// promieñ - trójk¹t
bool RayToTriangle(const VEC3& ray_pos, const VEC3& ray_dir, const VEC3& v1, const VEC3& v2, const VEC3& v3, float& dist);
// prostok¹t - prostok¹t
bool RectangleToRectangle(float x1, float y1, float x2, float y2, float a1, float b1, float a2, float b2);
// okr¹g - prostok¹t
bool CircleToRectangle(float circlex, float circley, float radius, float rectx, float recty, float w, float h);
// odcinek - odcinek (2d)
bool LineToLine(const VEC2& start1, const VEC2& end1, const VEC2& start2, const VEC2& end2, float* t = nullptr);
// odcinek - prostok¹t
bool LineToRectangle(const VEC2& start, const VEC2& end, const VEC2& rect_pos, const VEC2& rect_pos2, float* t = nullptr);
inline bool LineToRectangle(const VEC3& start, const VEC3& end, const VEC2& rect_pos, const VEC2& rect_pos2, float* t = nullptr)
{
	return LineToRectangle(VEC2(start.x, start.z), VEC2(end.x, end.z), rect_pos, rect_pos2, t);
}
inline bool LineToRectangleSize(const VEC2& start, const VEC2& end, const VEC2& rect_pos, const VEC2& rect_size, float* t = nullptr)
{
	return LineToRectangle(start, end, rect_pos - rect_size, rect_pos + rect_size, t);
}
// szeœcian - szeœcian
bool BoxToBox(const BOX& box1, const BOX& box2);
// obrócony szeœcian - obrócony szeœcian
// punkt kontaktu jest opcjonalny (jest to uœredniony wynik z maksymalnie 4 kontaktów)
bool OrientedBoxToOrientedBox(const OOBBOX& obox1, const OOBBOX& obox2, VEC3* contact);
// kolizja ko³o - obrócony prostok¹t
bool CircleToRotatedRectangle(float cx, float cy, float radius, float rx, float ry, float w, float h, float rot);
// kolizja dwóch obróconych prostok¹tów (mo¿na by zrobiæ optymalizacje ¿e jeden tylko jest obrócony ale nie wiem jak :3)
struct RotRect
{
	VEC2 center, size;
	float rot;
};
bool RotatedRectanglesCollision(const RotRect& r1, const RotRect& r2);
inline bool CircleToCircle(float cx1, float cy1, float r1, float cx2, float cy2, float r2)
{
	float r = (r1 + r2);
	return distance_sqrt(cx1, cy1, cx2, cy2) < r * r;
}
bool SphereToBox(const VEC3& pos, float radius, const BOX& box);
// kolizja promienia (A->B) z cylindrem (P->Q, promieñ R)
int RayToCylinder(const VEC3& ray_A, const VEC3& ray_B, const VEC3& cylinder_P, const VEC3& cylinder_Q, float radius, float& t);
// kolizja OOB z OOB
bool OOBToOOB(const OOB& a, const OOB& b);
// odleg³oœæ punktu od prostok¹ta
float DistanceRectangleToPoint(const VEC2& pos, const VEC2& size, const VEC2& pt);
// x0,y0 - point
float PointLineDistance(float x0, float y0, float x1, float y1, float x2, float y2);
float GetClosestPointOnLineSegment(const VEC2& A, const VEC2& B, const VEC2& P, VEC2& result);

//-----------------------------------------------------------------------------
// struktura do sprawdzania czy obiekt jest widoczny na ekranie
// kod z TFQE
//-----------------------------------------------------------------------------
struct FrustumPlanes
{
	D3DXPLANE Planes[6];

	FrustumPlanes() {}
	explicit FrustumPlanes(const MATRIX &WorldViewProj) { Set(WorldViewProj); }
	void Set(const MATRIX &WorldViewProj);

	// zwraca punkty na krawêdziach frustuma
	void GetPoints(VEC3* points) const;
	static void GetPoints(const MATRIX& WorldViewProj, VEC3* points);
	/*void GetFrustumBox(BOX& box) const;
	static void GetFrustumBox(const MATRIX& WorldViewProj,BOX& box);
	void GetMidpoint(VEC3& midpoint) const;*/

	// funkcje sprawdzaj¹ce
	// Zwraca true, jeœli punkt nale¿y do wnêtrza podanego frustuma
	bool PointInFrustum(const VEC3 &p) const;

	// Zwraca true, jeœli podany prostopad³oœcian jest widoczny choæ trochê w bryle widzenia
	// Uwaga! W rzadkich przypadkach mo¿e stwierdziæ kolizjê chocia¿ jej nie ma.
	bool BoxToFrustum(const BOX &Box) const;
	// 0 - poza, 1 - poza zasiêgiem, 2 - w zasiêgu
	int BoxToFrustum2(const BOX& box) const;

	bool BoxToFrustum(const BOX2D& box) const;

	// Zwraca true, jeœli AABB jest w ca³oœci wewn¹trz frustuma
	bool BoxInFrustum(const BOX &Box) const;

	// Zwraca true, jeœli sfera koliduje z frustumem
	// Uwaga! W rzadkich przypadkach mo¿e stwierdziæ kolizjê chocia¿ jej nie ma.
	bool SphereToFrustum(const VEC3 &SphereCenter, float SphereRadius) const;

	// Zwraca true, jeœli sfera zawiera siê w ca³oœci wewn¹trz frustuma
	bool SphereInFrustum(const VEC3 &SphereCenter, float SphereRadius) const;
};
#endif

//-----------------------------------------------------------------------------
// Funkcje dla bullet physics
//-----------------------------------------------------------------------------
#ifndef COMMON_ONLY
inline VEC2 ToVEC2(const btVector3& v)
{
	return VEC2(v.x(), v.z());
}

inline VEC3 ToVEC3(const btVector3& v)
{
	return VEC3(v.x(), v.y(), v.z());
}

inline btVector3 ToVector3(const VEC2& v)
{
	return btVector3(v.x, 0, v.y);
}

inline btVector3 ToVector3(const VEC3& v)
{
	return btVector3(v.x, v.y, v.z);
}

// Function for calculating rotation around point for physic nodes
// from: http://bulletphysics.org/Bullet/phpBB3/viewtopic.php?t=5182
inline void RotateGlobalSpace(btTransform& out, const btTransform& T, const btMatrix3x3& rotationMatrixToApplyBeforeTGlobalSpace,
	const btVector3& centerOfRotationRelativeToTLocalSpace)
{
	// Note:  - centerOfRotationRelativeToTLocalSpace = TRelativeToCenterOfRotationLocalSpace (LocalSpace is relative to the T.basis())
	// Distance between the center of rotation and T in global space
	const btVector3 TRelativeToTheCenterOfRotationGlobalSpace = T.getBasis() * (-centerOfRotationRelativeToTLocalSpace);
	// Absolute position of the center of rotation = Absolute position of T + PositionOfTheCenterOfRotationRelativeToT
	const btVector3 centerOfRotationAbsolute = T.getOrigin() - TRelativeToTheCenterOfRotationGlobalSpace;
	out = btTransform(rotationMatrixToApplyBeforeTGlobalSpace*T.getBasis(),
		centerOfRotationAbsolute + rotationMatrixToApplyBeforeTGlobalSpace * TRelativeToTheCenterOfRotationGlobalSpace);
}

inline void RotateGlobalSpace(btTransform& out, const btTransform& T, const btQuaternion& rotationToApplyBeforeTGlobalSpace,
	const btVector3& centerOfRotationRelativeToTLocalSpace)
{
	RotateGlobalSpace(out, T, btMatrix3x3(rotationToApplyBeforeTGlobalSpace), centerOfRotationRelativeToTLocalSpace);
}
#endif

//-----------------------------------------------------------------------------
// Funkcje zapisuj¹ce/wczytuj¹ce z pliku
//-----------------------------------------------------------------------------
extern DWORD tmp;

template<typename T>
inline void WriteString(HANDLE file, const string& s)
{
	assert(s.length() <= std::numeric_limits<T>::max());
	T len = (T)s.length();
	WriteFile(file, &len, sizeof(len), &tmp, nullptr);
	if(len)
		WriteFile(file, s.c_str(), len, &tmp, nullptr);
}

template<typename T>
inline void ReadString(HANDLE file, string& s)
{
	T len;
	ReadFile(file, &len, sizeof(len), &tmp, nullptr);
	if(len)
	{
		s.resize(len);
		ReadFile(file, (char*)s.c_str(), len, &tmp, nullptr);
	}
	else
		s.clear();
}

inline void WriteString1(HANDLE file, const string& s)
{
	WriteString<byte>(file, s);
}

inline void WriteString2(HANDLE file, const string& s)
{
	WriteString<word>(file, s);
}

inline void ReadString1(HANDLE file, string& s)
{
	ReadString<byte>(file, s);
}

inline void ReadString1(HANDLE file)
{
	byte len;
	ReadFile(file, &len, sizeof(len), &tmp, nullptr);
	if(len == 0)
		BUF[0] = 0;
	else
	{
		ReadFile(file, BUF, len, &tmp, nullptr);
		BUF[len] = 0;
	}
}

inline void ReadString2(HANDLE file, string& s)
{
	ReadString<word>(file, s);
}

template<typename COUNT_TYPE, typename STRING_SIZE_TYPE>
inline void WriteStringArray(HANDLE file, vector<string>& strings)
{
	COUNT_TYPE ile = (COUNT_TYPE)strings.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	for(vector<string>::iterator it = strings.begin(), end = strings.end(); it != end; ++it)
		WriteString<STRING_SIZE_TYPE>(file, *it);
}

template<typename COUNT_TYPE, typename STRING_SIZE_TYPE>
inline void ReadStringArray(HANDLE file, vector<string>& strings)
{
	COUNT_TYPE ile;
	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	strings.resize(ile);
	for(vector<string>::iterator it = strings.begin(), end = strings.end(); it != end; ++it)
		ReadString<STRING_SIZE_TYPE>(file, *it);
}

//-----------------------------------------------------------------------------
// kontener u¿ywany na tymczasowe obiekty które s¹ potrzebne od czasu do czasu
//-----------------------------------------------------------------------------
//#define CHECK_POOL_LEAK
#if defined(CHECK_POOL_LEAK) && !defined(_DEBUG)
#	pragma message("Warning: Disabling CHECK_POOL_LEAK in release mode.")
#	undef CHECK_POOL_LEAK
#endif
template<typename T>
struct ObjectPool
{
	~ObjectPool()
	{
		DeleteElements(pool);
	}

	inline T* Get()
	{
		T* t;
		if(pool.empty())
			t = new T;
		else
		{
			t = pool.back();
			pool.pop_back();
		}
		__if_exists(T::OnGet)
		{
			t->OnGet();
		}
		return t;
	}

	inline void Free(T* t)
	{
		assert(t);
		__if_exists(T::OnFree)
		{
			t->OnFree();
		}
#ifdef CHECK_POOL_LEAK
		delete t;
#else
		pool.push_back(t);
#endif
	}

	inline void Free(vector<T*>& elems)
	{
		if(elems.empty())
			return;

		__if_exists(T::OnFree)
		{
			for(T* e : elems)
			{
				assert(e);
				e->OnFree();
			}
		}

#ifdef CHECK_POOL_LEAK
		DeleteElementsChecked(elems);
#else
	#ifdef _DEBUG
		for(T* e : elems)
			assert(e);
	#endif
		pool.insert(pool.end(), elems.begin(), elems.end());
		elems.clear();
#endif
	}

	inline void SafeFree(vector<T*>& elems)
	{
		for(T* e : elems)
		{
			if(e)
			{
				__if_exists(T::OnFree)
				{
					e->OnFree();
				}
#ifdef CHECK_POOL_LEAK
				delete e;
#else
				pool.push_back(e);
#endif
			}
		}
		elems.clear();
	}

private:
	vector<T*> pool;
};

template<typename T>
class ObjectPoolProxy
{
public:
	static inline T* Get() { return GetPool().Get(); }
	static inline void Free(T* t) { GetPool().Free(t); }
	static inline void Free(vector<T*>& ts) { GetPool().Free(ts); }
	static inline void SafeFree(vector <T*>& ts) { GetPool().SafeFree(ts); }
	inline virtual void Free() { Free((T*)this); }

private:
	inline static ObjectPool<T>& GetPool() { static ObjectPool<T> pool; return pool; }
};

// tymczasowe stringi
extern ObjectPool<string> StringPool;
extern ObjectPool<vector<void*> > VectorPool;

//-----------------------------------------------------------------------------
// Lokalny string który wykorzystuje StringPool
//-----------------------------------------------------------------------------
struct LocalString
{
	LocalString()
	{
		s = StringPool.Get();
		s->clear();
	}

	LocalString(cstring str)
	{
		assert(str);
		s = StringPool.Get();
		*s = str;
	}

	LocalString(cstring str, cstring str_to)
	{
		s = StringPool.Get();
		uint len = str_to - str;
		s->resize(len);
		memcpy((char*)s->data(), str, len);
	}

	LocalString(const string& str)
	{
		s = StringPool.Get();
		*s = str;
	}

	~LocalString()
	{
		StringPool.Free(s);
	}

	inline void operator = (cstring str)
	{
		*s = str;
	}

	inline void operator = (const string& str)
	{
		*s = str;
	}

	inline char at_back(uint offset) const
	{
		assert(offset < s->size());
		return s->at(s->size() - 1 - offset);
	}

	inline void pop(uint count)
	{
		assert(s->size() > count);
		s->resize(s->size() - count);
	}

	inline void operator += (cstring str)
	{
		*s += str;
	}

	inline void operator += (const string& str)
	{
		*s += str;
	}

	inline void operator += (char c)
	{
		*s += c;
	}

	inline operator cstring() const
	{
		return s->c_str();
	}

	inline string& get_ref()
	{
		return *s;
	}

	inline string* get_ptr()
	{
		return s;
	}

	inline string* operator -> ()
	{
		return s;
	}

	inline const string* operator -> () const
	{
		return s;
	}

	inline bool operator == (cstring str) const
	{
		return *s == str;
	}

	inline bool operator == (const string& str) const
	{
		return *s == str;
	}

	inline bool operator == (const LocalString& str) const
	{
		return *s == *str.s;
	}

	inline bool operator != (cstring str) const
	{
		return *s != str;
	}

	inline bool operator != (const string& str) const
	{
		return *s != str;
	}

	inline bool operator != (const LocalString& str) const
	{
		return *s != *str.s;
	}

	inline bool empty() const
	{
		return s->empty();
	}

	inline cstring c_str() const
	{
		return s->c_str();
	}

	inline void clear()
	{
		s->clear();
	}

private:
	string* s;
};

//-----------------------------------------------------------------------------
// Lokalny wektor przechowuj¹cy wskaŸniki
//-----------------------------------------------------------------------------
template<typename T>
struct LocalVector
{
	static_assert(sizeof(T) == sizeof(void*), "LocalVector element must be pointer or have sizeof pointer.");

	LocalVector()
	{
		v = (vector<T>*)VectorPool.Get();
		v->clear();
	}

	explicit LocalVector(vector<T>& v2)
	{
		v = (vector<T>*)VectorPool.Get();
		*v = v2;
	}

	~LocalVector()
	{
		VectorPool.Free((vector<void*>*)v);
	}

	inline operator vector<T>& ()
	{
		return *v;
	}

	inline vector<T>* operator -> ()
	{
		return v;
	}

	inline const vector<T>* operator -> () const
	{
		return v;
	}

	inline T& operator [] (int n)
	{
		return v->at(n);
	}

	inline void Shuffle()
	{
		std::random_shuffle(v->begin(), v->end(), myrand);
	}

private:
	vector<T>* v;
};

template<typename T>
struct LocalVector2
{
	typedef vector<T> Vector;
	typedef Vector* VectorPtr;
	typedef typename Vector::iterator Iter;

	static_assert(sizeof(T) == sizeof(void*), "LocalVector2 element must be pointer or have sizeof pointer.");

	LocalVector2()
	{
		v = (VectorPtr)VectorPool.Get();
		v->clear();
	}

	explicit LocalVector2(Vector& v2)
	{
		v = (VectorPtr*)VectorPool.Get();
		*v = v2;
	}

	~LocalVector2()
	{
		VectorPool.Free((vector<void*>*)v);
	}

	inline void push_back(T e)
	{
		v->push_back(e);
	}

	inline bool empty() const
	{
		return v->empty();
	}

	inline Iter begin()
	{
		return v->begin();
	}

	inline Iter end()
	{
		return v->end();
	}

	inline uint size() const
	{
		return v->size();
	}

	T& random_item()
	{
		return v->at(rand2() % v->size());
	}

	T& operator [] (int n)
	{
		return v->at(n);
	}

	const T& operator [] (int n) const
	{
		return v->at(n);
	}

	inline vector<T>& Get()
	{
		return *v;
	}

	inline void clear()
	{
		v->clear();
	}

private:
	VectorPtr v;
};

template<typename T>
inline T& random_item(LocalVector2<T>& v)
{
	return v.random_item();
}

template<typename T>
inline bool in_range(__int64 value)
{
	return value >= std::numeric_limits<T>::min() && value <= std::numeric_limits<T>::max();
}

namespace TextHelper
{
	// parse string to number, return 0-broken, 1-int, 2-float
	int ToNumber(cstring s, __int64& i, float& f);
	bool ToInt(cstring s, int& result);
	bool ToUint(cstring s, uint& result);
	bool ToFloat(cstring s, float& result);
	bool ToBool(cstring s, bool& result);
}

// gdy trzeba coœ na chwilê wczytaæ to mo¿na u¿ywaæ tego stringa
extern string g_tmp_string;

bool LoadFileToString(cstring path, string& str);

struct Trimmer
{
	bool done;
	inline Trimmer() : done(false) {}
	inline bool operator () (char c) const
	{
		if(done)
			return false;
		return c == ' ';
	}
};

// trim from start
inline string& ltrim(string& str)
{
	str.erase(str.begin(), find_if(str.begin(), str.end(), [](char& ch)->bool { return !isspace(ch); }));
	return str;
}

// trim from end
inline string& rtrim(string& str)
{
	str.erase(find_if(str.rbegin(), str.rend(), [](char& ch)->bool { return !isspace(ch); }).base(), str.end());
	return str;
}

// trim from both ends
inline string& trim(string& str)
{
	return ltrim(rtrim(str));
}

#include "Tokenizer.h"

//-----------------------------------------------------------------------------
class FileReader
{
public:
	FileReader() : file(INVALID_HANDLE_VALUE), own_handle(false)
	{
	}

	explicit FileReader(HANDLE file) : file(file), own_handle(false)
	{
	}

	explicit FileReader(cstring filename) : own_handle(true)
	{
		Open(filename);
	}

	~FileReader()
	{
		if(own_handle && file != INVALID_HANDLE_VALUE)
		{
			CloseHandle(file);
			file = INVALID_HANDLE_VALUE;
		}
	}

	inline bool Open(cstring filename)
	{
		assert(filename);
		file = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		own_handle = true;
		return (file != INVALID_HANDLE_VALUE);
	}

	inline bool IsOpen() const
	{
		return file != INVALID_HANDLE_VALUE;
	}

	inline bool Read(void* ptr, uint size)
	{
		ReadFile(file, ptr, size, &tmp, nullptr);
		return size == tmp;
	}

	template<typename T>
	inline bool operator >> (T& a)
	{
		return Read(&a, sizeof(a));
	}

	template<typename T>
	inline T Read()
	{
		T a;
		Read(&a, sizeof(T));
		return a;
	}

	template<typename T>
	inline bool Read(T& a)
	{
		return Read(&a, sizeof(a));
	}

	template<typename T, typename T2>
	inline bool ReadCasted(T2& a)
	{
		T b;
		if(!Read<T>(b))
			return false;
		a = (T2)b;
		return true;
	}

	inline bool ReadStringBUF()
	{
		byte len = Read<byte>();
		if(len == 0)
		{
			BUF[0] = 0;
			return true;
		}
		else
		{
			BUF[len] = 0;
			return Read(BUF, len);
		}
	}

	template<typename T>
	inline void Skip()
	{
		SetFilePointer(file, sizeof(T), nullptr, FILE_CURRENT);
	}

	inline void Skip(int bytes)
	{
		SetFilePointer(file, bytes, nullptr, FILE_CURRENT);
	}

	inline bool ReadString1(string& s)
	{
		byte len;
		if(!Read(len))
			return false;
		s.resize(len);
		return Read((char*)s.c_str(), len);
	}

	inline bool ReadString2(string& s)
	{
		word len;
		if(!Read(len))
			return false;
		s.resize(len);
		return Read((char*)s.c_str(), len);
	}

	inline bool operator >> (string& s)
	{
		return ReadString1(s);
	}

	inline operator bool() const
	{
		return file != INVALID_HANDLE_VALUE;
	}

	inline void ReadToString(string& s)
	{
		DWORD size = GetFileSize(file, nullptr);
		s.resize(size);
		ReadFile(file, (char*)s.c_str(), size, &tmp, nullptr);
		assert(size == tmp);
	}

	template<typename T>
	inline void ReadVector1(vector<T>& v)
	{
		byte count;
		Read(count);
		v.resize(count);
		if(count)
			Read(&v[0], sizeof(T)*count);
	}

	template<typename T>
	inline void ReadVector2(vector<T>& v)
	{
		word count;
		Read(count);
		v.resize(count);
		if(count)
			Read(&v[0], sizeof(T)*count);
	}

	inline uint GetSize() const
	{
		return GetFileSize(file, nullptr);
	}

	HANDLE file;
	bool own_handle;
};

//-----------------------------------------------------------------------------
class FileWriter
{
public:
	FileWriter() : file(INVALID_HANDLE_VALUE), own_handle(true)
	{
	}

	explicit FileWriter(HANDLE file) : file(file), own_handle(false)
	{
	}

	explicit FileWriter(cstring filename) : own_handle(true)
	{
		Open(filename);
	}

	~FileWriter()
	{
		if(own_handle && file != INVALID_HANDLE_VALUE)
		{
			CloseHandle(file);
			file = INVALID_HANDLE_VALUE;
		}
	}

	inline bool Open(cstring filename)
	{
		assert(filename);
		file = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		return (file != INVALID_HANDLE_VALUE);
	}

	inline bool IsOpen() const
	{
		return file != INVALID_HANDLE_VALUE;
	}

	inline void Write(const void* ptr, uint size)
	{
		WriteFile(file, ptr, size, &tmp, nullptr);
		assert(size == tmp);
	}

	template<typename T>
	inline void operator << (const T& a)
	{
		Write(&a, sizeof(a));
	}

	template<typename T>
	inline void Write(const T& a)
	{
		Write(&a, sizeof(a));
	}

	template<typename T, typename T2>
	inline void WriteCasted(const T2& a)
	{
		Write(&a, sizeof(T));
	}

	inline void WriteString1(const string& s)
	{
		int length = s.length();
		assert(length < 256);
		WriteCasted<byte>(length);
		if(length)
			Write(s.c_str(), length);
	}

	inline void WriteString1(cstring str)
	{
		assert(str);
		int length = strlen(str);
		assert(length < 256);
		WriteCasted<byte>(length);
		if(length)
			Write(str, length);
	}

	inline void WriteString2(const string& s)
	{
		int length = s.length();
		assert(length < 256 * 256);
		WriteCasted<word>(length);
		if(length)
			Write(s.c_str(), length);
	}

	inline void WriteString2(cstring str)
	{
		assert(str);
		int length = strlen(str);
		assert(length < 256 * 256);
		Write<word>(length);
		if(length)
			Write(str, length);
	}

	inline void operator << (const string& s)
	{
		WriteString1(s);
	}

	inline void operator << (cstring str)
	{
		assert(str);
		WriteString1(str);
	}

	inline operator bool() const
	{
		return file != INVALID_HANDLE_VALUE;
	}

	inline void Write0()
	{
		WriteCasted<byte>(0);
	}

	template<typename T>
	inline void WriteVector1(const vector<T>& v)
	{
		WriteCasted<byte>(v.size());
		if(!v.empty())
			Write(&v[0], sizeof(T)*v.size());
	}

	template<typename T>
	inline void WriteVector2(const vector<T>& v)
	{
		WriteCasted<word>(v.size());
		if(!v.empty())
			Write(&v[0], sizeof(T)*v.size());
	}

	inline void Flush()
	{
		FlushFileBuffers(file);
	}

	inline uint GetSize() const
	{
		return GetFileSize(file, nullptr);
	}

	HANDLE file;
	bool own_handle;
};

//-----------------------------------------------------------------------------
class CriticalSection
{
	friend class StartCriticalSection;
public:
	inline CriticalSection() : valid(false)
	{
	}
	inline void Create(uint spin_count = 50)
	{
		if(!valid)
		{
			InitializeCriticalSectionAndSpinCount(&cs, 50);
			valid = true;
		}
	}
	inline void Free()
	{
		if(valid)
		{
			DeleteCriticalSection(&cs);
			valid = false;
		}
	}
	inline void Enter()
	{
		assert(valid);
		EnterCriticalSection(&cs);
	}
	inline void Leave()
	{
		assert(valid);
		LeaveCriticalSection(&cs);
	}
private:
	CRITICAL_SECTION cs;
	bool valid;
};

//-----------------------------------------------------------------------------
class StartCriticalSection
{
public:
	inline StartCriticalSection(CRITICAL_SECTION& _cs) : cs(_cs)
	{
		EnterCriticalSection(&cs);
	}
	inline StartCriticalSection(CriticalSection& _cs) : cs(_cs.cs)
	{
		assert(_cs.valid);
		EnterCriticalSection(&cs);
	}
	inline ~StartCriticalSection()
	{
		LeaveCriticalSection(&cs);
	}
private:
	CRITICAL_SECTION& cs;
};

//-----------------------------------------------------------------------------
#ifndef COMMON_ONLY
#include "Timer.h"

class Profiler
{
public:
	struct Entry;

	Profiler();
	~Profiler();

	void Start();
	void End();
	void Push(cstring name);
	void Pop();
	inline bool IsStarted() const
	{
		return started;
	}
	inline const string& GetString() const
	{
		return str;
	}
	void Clear();

private:
	void Print(Entry* e, int tab);

	bool started, enabled;
	Timer timer;
	Entry* e, *prev_e;
	vector<Entry*> stac;
	string str;
	int frames;
};

class ProfilerBlock
{
public:
	explicit ProfilerBlock(cstring name);
	~ProfilerBlock();

private:
	bool on;
};

#define PROFILER_BLOCK(name) ProfilerBlock JOIN(_block,__COUNTER__)##(name)

extern Profiler g_profiler;
#endif

//-----------------------------------------------------------------------------
struct Pixel
{
	INT2 pt;
	byte alpha;

	Pixel(int x, int y, byte alpha = 0) : pt(x, y), alpha(alpha) {}
};

void PlotLine(int x0, int y0, int x1, int y1, float th, vector<Pixel>& pixels);
void PlotQuadBezierSeg(int x0, int y0, int x1, int y1, int x2, int y2, float w, float th, vector<Pixel>& pixels);
void PlotQuadBezier(int x0, int y0, int x1, int y1, int x2, int y2, float w, float th, vector<Pixel>& pixels);
void PlotCubicBezierSeg(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, float th, vector<Pixel>& pixels);
void PlotCubicBezier(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, float th, vector<Pixel>& pixels);

//-----------------------------------------------------------------------------
#ifndef NO_DIRECT_X
inline void ColorToVec(DWORD c, VEC4& v)
{
	v.x = float((c & 0xFF0000) >> 16) / 255;
	v.y = float((c & 0xFF00) >> 8) / 255;
	v.z = float(c & 0xFF) / 255;
	v.w = float((c & 0xFF000000) >> 24) / 255;
}
#endif

template<typename T, class Pred>
inline void Join(const vector<T>& v, string& s, cstring separator, Pred pred)
{
	if(v.empty())
		return;
	if(v.size() == 1)
		s += pred(v.front());
	else
	{
		for(vector<T>::const_iterator it = v.begin(), end = v.end() - 1; it != end; ++it)
		{
			s += pred(*it);
			s += separator;
		}
		s += pred(*(v.end() - 1));
	}
}

//-----------------------------------------------------------------------------
inline void Split3(int val, int& a, int& b, int& c)
{
	a = (val & 0xFF);
	b = ((val & 0xFF00) >> 8);
	c = ((val & 0xFF0000) >> 16);
}

inline int Join3(int a, int b, int c)
{
	return (a & 0xFF) | ((b & 0xFF) << 8) | ((c & 0xFF) << 16);
}

//-----------------------------------------------------------------------------
#ifndef NO_DIRECT_X
extern const VEC2 POISSON_DISC_2D[];
extern const int poisson_disc_count;
#endif

//-----------------------------------------------------------------------------
template<typename T>
inline bool In(T val, const std::initializer_list<T>& lis)
{
	for(T t : lis)
	{
		if(t == val)
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
struct AnyString
{
	inline AnyString(cstring s) : s(s)
	{
		assert(s);
		assert(strlen(s) > 0);
	}
	inline AnyString(const string& str) : s(str.c_str())
	{
		assert(!str.empty());
	}

	cstring s;
};

//-----------------------------------------------------------------------------
struct AnyStringNull
{
	inline AnyStringNull(cstring s) : s(s)
	{
		if(s)
			assert(strlen(s) > 0);
	}
	inline AnyStringNull(const string& str) : s(str.c_str())
	{
		assert(!str.empty());
	}

	cstring s;
};

//-----------------------------------------------------------------------------
template<typename T>
class SafeVector
{
public:
	SafeVector()
	{
		cs.Create();
	}

	~SafeVector()
	{
		cs.Free();
	}

	inline void Push(T& e)
	{
		StartCriticalSection section(cs);
		v.push_back(e);
	}

	inline T Pop()
	{
		StartCriticalSection section(cs);
		T e = v.back();
		v.pop_back();
		return e;
	}

	inline bool Any()
	{
		return !v.empty();
	}

private:
	vector<T> v;
	CriticalSection cs;
};

//-----------------------------------------------------------------------------
template<typename T>
class Ptr
{
public:
	Ptr() : ptr(nullptr) {}
	~Ptr() { delete ptr; }
	inline T* operator -> () { return ptr; }
	inline void Ensure()
	{
		if(!ptr)
			ptr = new T;
	}
	inline T* Pin()
	{
		T* t = ptr;
		ptr = nullptr;
		return t;
	}

private:
	T* ptr;
};

//-----------------------------------------------------------------------------
struct CstringComparer
{
	inline bool operator() (cstring s1, cstring s2) const
	{
		return _stricmp(s1, s2) > 0;
	}
};

//-----------------------------------------------------------------------------
// In debug it uses dynamic_cast and asserts if cast is valid
// In release it uses C style cast
template<typename T, typename T2>
inline T checked_cast(T2& a)
{
#ifdef _DEBUG
	T b = dynamic_cast<T>(a);
	assert(b);
#else
	T b = (T)a;
#endif
	return b;
}

//-----------------------------------------------------------------------------
// Offset cast
template<typename T>
inline T& offset_cast(void* data, uint offset)
{
	byte* b = ((byte*)data) + offset;
	return *(T*)b;
}

//-----------------------------------------------------------------------------
// Loop over list and erase elements that returned true
template<typename T, typename Action>
inline void LoopAndRemove(vector<T>& items, Action action)
{
	items.erase(std::remove_if(items.begin(), items.end(), action), items.end());
}

template<typename T>
struct WeightPair
{
	T item;
	int weight;

	WeightPair(T& item, int weight) : item(item), weight(weight) {}
};

template<typename T>
inline T& RandomItemWeight(vector<WeightPair<T>>& items, int max_weight)
{
	int a = rand2() % max_weight, b = 0;
	for(auto& item : items)
	{
		b += item.weight;
		if(a < b)
			return item.item;
	}
	// if it gets here max_count is wrong, return random item
	return random_item(items).item;
}

extern ObjectPool<vector<byte>> BufPool;

template<typename T>
class LocalVector3
{
public:
	struct iterator : std::iterator<std::input_iterator_tag, T>
	{
		friend class LocalVector3;

		inline T& operator * ()
		{
			return v->at(offset);
		}

		inline bool operator == (const iterator& it) const
		{
			assert(it.v == v);
			return offset == it.offset;
		}

		inline bool operator != (const iterator& it) const
		{
			assert(it.v == v);
			return offset != it.offset;
		}

		inline bool operator < (const iterator& it) const
		{
			assert(it.v == v);
			return offset < it.offset;
		}

		inline iterator& operator ++ ()
		{
			++offset;
			return *this;
		}

		inline iterator operator ++ (int)
		{
			iterator it(v, offset);
			++offset;
			return it;
		}

		inline iterator& operator -- ()
		{
			--offset;
			return *this;
		}

		inline iterator& operator + (uint count)
		{
			offset += count;
			return *this;
		}

		inline iterator& operator - (uint count)
		{
			offset -= count;
			return *this;
		}

		inline int operator - (const iterator& it) const
		{
			assert(it.v == v);
			return offset - it.offset;
		}

	private:
		iterator(LocalVector3* v, uint offset) : v(v), offset(offset) {}

		LocalVector3* v;
		uint offset;
	};

	LocalVector3()
	{
		buf = BufPool.Get();
	}

	~LocalVector3()
	{
		buf->clear();
		BufPool.Free(buf);
	}

	inline T& at(uint offset)
	{
		assert(offset < size());
		return ((T*)buf->data())[offset];
	}

	inline T& back()
	{
		assert(!empty());
		return ((T*)buf->data())[size() - 1];
	}

	inline iterator begin()
	{
		return iterator(this, 0);
	}

	inline bool empty() const
	{
		return buf->empty();
	}

	inline iterator end()
	{
		return iterator(this, size());
	}

	void push_back(T& item)
	{
		uint s = buf->size();
		buf->resize(buf->size() + sizeof(T));
		memcpy(buf->data() + s, &item, sizeof(T));
	}

	inline uint size() const
	{
		return buf->size() / sizeof(T);
	}

	template<typename Pred>
	void sort(Pred pred)
	{
		std::sort((T*)buf->data(), (T*)(buf->data() + buf->size()), pred);
	}

private:
	vector<byte>* buf;
};

// check for overflow a + b, and return value
inline bool checked_add(uint a, uint b, uint& result)
{
	uint64 r = (uint64)a + b;
	if(r > std::numeric_limits<uint>::max())
		return false;
	result = (uint)r;
	return true;
}

// check for overflow a * b + c, and return value
inline bool checked_multiply_add(uint a, uint b, uint c, uint& result)
{
	uint64 r = (uint64)a * b + c;
	if(r > std::numeric_limits<uint>::max())
		return false;
	result = (uint)r;
	return true;
}

template<typename T>
inline int GetIndex(const vector<T>& items, const T& item)
{
	int index = 0;
	for(const T& it : items)
	{
		if(it == item)
			return index;
		++index;
	}
	return -1;
}

template<typename T, typename Pred>
inline int GetIndex(const vector<T>& items, Pred pred)
{
	int index = 0;
	for(const T& it : items)
	{
		if(pred(it))
			return index;
		++index;
	}
	return -1;
}

inline char strchrs(cstring s, cstring chrs)
{
	assert(s && chrs);

	while(true)
	{
		char c = *s++;
		if(c == 0)
			return 0;
		cstring ch = chrs;
		while(true)
		{
			char c2 = *ch++;
			if(c2 == 0)
				break;
			if(c == c2)
				return c;
		}
	}
}

inline char strchr2(char c, cstring chrs)
{
	assert(chrs);

	while(true)
	{
		char c2 = *chrs++;
		if(c2 == 0)
			return 0;
		if(c == c2)
			return c;
	}
}
