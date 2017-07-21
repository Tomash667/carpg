#pragma once

using namespace DirectX;

//-----------------------------------------------------------------------------
struct INT2;
struct Rect;
struct VEC2;
struct VEC3;
struct VEC4;
struct BOX2D;
struct BOX;
struct OBBOX;
struct OOB;
struct MATRIX;
struct QUAT;
struct PLANE;

// for ray-mesh collision
#include "VertexData.h"

//-----------------------------------------------------------------------------
// Random numbers
//-----------------------------------------------------------------------------
// Pseudo random number generator
struct RNG
{
	unsigned long val;

	RNG() : val(1)
	{
	}

	int Rand()
	{
		val = val * 214013L + 2531011L;
		return ((val >> 16) & 0x7fff);
	}

	int RandTmp()
	{
		int tval = val * 214013L + 2531011L;
		return ((tval >> 16) & 0x7fff);
	}
};
extern RNG _RNG;

inline int Rand()
{
	return _RNG.Rand();
}
inline int Rand(int a)
{
	return _RNG.Rand() % a;
}
inline void Srand(int x)
{
	_RNG.val = x;
}
inline uint RandVal()
{
	return _RNG.val;
}
inline int RandTmp()
{
	return _RNG.RandTmp();
}
inline int MyRand(int a)
{
	return Rand(a);
}

// Random float number in range <0,1>
inline float Random()
{
	return (float)Rand() / RAND_MAX;
}

// Random number in range <0,a>
template<typename T>
inline T Random(T a)
{
	static_assert(std::is_integral<T>::value, "T must be integral");
	assert(a >= 0);
	return Rand(a + 1);
}
template<>
inline float Random(float a)
{
	return ((float)Rand() / RAND_MAX)*a;
}

// Random number in range <a,b>
template<typename T>
inline T Random(T a, T b)
{
	static_assert(std::is_integral<T>::value, "T must be integral");
	assert(b >= a);
	return Rand() % (b - a + 1) + a;
}
template<>
inline float Random(float a, float b)
{
	assert(b >= a);
	return ((float)Rand() / RAND_MAX)*(b - a) + a;
}

inline float RandomPart(int parts)
{
	return 1.f / parts * (Rand() % parts);
}

// Return normalized number in range <-val,val>
inline float RandomNormalized(float val)
{
	return (Random(-val, val) + Random(-val, val)) / 2;
}

// Return element based on chance
template<typename T>
inline T Chance(int c, T a, T b)
{
	return (Rand() % c == 0 ? a : b);
}
template<typename T>
inline T Chance(int chance_a, int chance_b, int chance_c, T a, T b, T c)
{
	int ch = Rand() % (chance_a + chance_b + chance_c);
	if(ch < chance_a)
		return a;
	else if(ch < chance_a + chance_b)
		return b;
	else
		return c;
}

//-----------------------------------------------------------------------------
// Math functions
//-----------------------------------------------------------------------------
// Clamp value to range
template<typename T>
inline T Clamp(T value, T min, T max)
{
	if(value > max)
		return max;
	else if(value < min)
		return min;
	else
		return value;
}

// Compare two floats using epsilon
inline bool Equal(float a, float b)
{
	return abs(a - b) < std::numeric_limits<float>::epsilon();
}

// Return min max value
template<typename T>
inline void MinMax(T a, T b, T& min, T& max)
{
	static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "T must be int or float");
	if(a > b)
	{
		min = b;
		max = a;
	}
	else
	{
		min = a;
		max = b;
	}
}

// Find min value from any arguments count
template<typename T, typename T2>
inline T Min(T a, T2 b)
{
	static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "T must be int or float");
	if(a > b)
		return b;
	else
		return a;
}
template<typename T, typename T2, typename... Args>
inline T Min(T a, T2 b, Args... args)
{
	static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "T must be int or float");
	if(a > b)
		return Min(b, args...);
	else
		return Min(a, args...);
}

// Find min value from any arguments count
template<typename T, typename T2>
inline T Max(T a, T2 b)
{
	static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "T must be int or float");
	if(a < b)
		return b;
	else
		return a;
}
template<typename T, typename T2, typename... Args>
inline T Max(T a, T2 b, Args... args)
{
	static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value, "T must be int or float");
	if(a < b)
		return Max(b, args...);
	else
		return Max(a, args...);
}

// Distance between two 2d points
template<typename T>
inline T Distance(T x1, T y1, T x2, T y2)
{
	T x = abs(x1 - x2);
	T y = abs(y1 - y2);
	return sqrt(x*x + y*y);
}
inline float DistanceSqrt(float x1, float y1, float x2, float y2)
{
	float x = abs(x1 - x2),
		y = abs(y1 - y2);
	return x*x + y*y;
}

// Clip value to range
inline float Clip(float f, float range = PI * 2)
{
	int n = (int)floor(f / range);
	return f - range * n;
}

// Return angle between two points
float Angle(float x1, float y1, float x2, float y2);

// Return difference between two angles
inline float AngleDiff(float a, float b)
{
	assert(a >= 0.f && a < PI * 2 && b >= 0.f && b < PI * 2);
	return min((2 * PI) - abs(a - b), abs(b - a));
}

// Return true if float is not zero
inline bool NotZero(float a)
{
	return abs(a) >= std::numeric_limits<float>::epsilon();
}

// Return true if float is zero
inline bool IsZero(float a)
{
	return abs(a) < std::numeric_limits<float>::epsilon();
}

// Return sign of value
template<typename T>
inline T Sign(T f)
{
	if(f > 0)
		return 1;
	else if(f < 0)
		return -1;
	else
		return 0;
}

// Return linear interpolation of value
inline float Lerp(float a, float b, float t)
{
	return (b - a)*t + a;
}
inline int Lerp(int a, int b, float t)
{
	return int(t*(b - a)) + a;
}

// Return shortes direction between angles
float ShortestArc(float a, float b);

// Linear interpolation between two angles
void LerpAngle(float& angle, float from, float to, float t);

// Return true if value is in range
template<typename T>
inline bool InRange(T v, T left, T right)
{
	return (v >= left && v <= right);
}
template<typename T>
inline bool InRange(T left, T a, T b, T right)
{
	return (a >= left && b >= a && b <= right);
}

// Return true if number is in other numeric type range
template<typename T>
inline bool InRange(__int64 value)
{
	return value >= std::numeric_limits<T>::min() && value <= std::numeric_limits<T>::max();
}

// Return spherical interpolation between two angles
inline float Slerp(float a, float b, float t)
{
	float angle = ShortestArc(a, b);
	return a + angle * t;
}

// Count 1 bits in value
inline int CountBits(int i)
{
	// It's write-only code. Just put a comment that you are not meant to understand or maintain this code, just worship the gods that revealed it to mankind.
	i = i - ((i >> 1) & 0x55555555);
	i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
	return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

// Return true if value is power of 2
template <class T>
inline bool IsPow2(T x)
{
	return ((x > 0) && ((x & (x - 1)) == 0));
}

// Return float infinity
inline float Inf()
{
	return std::numeric_limits<float>::infinity();
}

// Convert angle from degrees to radians
inline float ToRadians(float degrees)
{
	return degrees * PI / 180;
}

// Convert angle from radians to degrees
inline float ToDegrees(float radians)
{
	return radians * 180 / PI;
}

// Round float to int
inline int Roundi(float value)
{
	return (int)round(value);
}

// Return module
inline int Modulo(int a, int mod)
{
	if(a >= 0)
		return a % mod;
	else
		return a + mod * ((-a / mod) + 1);
}

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
// check for overflow a + b, and return value
inline bool CheckedAdd(uint a, uint b, uint& result)
{
	uint64 r = (uint64)a + b;
	if(r > std::numeric_limits<uint>::max())
		return false;
	result = (uint)r;
	return true;
}

// check for overflow a * b + c, and return value
inline bool CheckedMultiplyAdd(uint a, uint b, uint c, uint& result)
{
	uint64 r = (uint64)a * b + c;
	if(r > std::numeric_limits<uint>::max())
		return false;
	result = (uint)r;
	return true;
}

//-----------------------------------------------------------------------------
// 2D int point
//-----------------------------------------------------------------------------
struct INT2
{
	int x, y;

	INT2();
	INT2(int x, int y);
	INT2(const INT2& i);
	template<typename T, typename T2>
	INT2(T x, T2 y);
	explicit INT2(int xy);
	explicit INT2(const VEC2& v);
	explicit INT2(const VEC3& v);

	// Comparison operators
	bool operator == (const INT2& i) const;
	bool operator != (const INT2& i) const;

	// Assignment operators
	INT2& operator = (const INT2& i);
	void operator += (const INT2& i);
	void operator -= (const INT2& i);
	void operator *= (int a);
	void operator /= (int a);

	// Unary operators
	INT2 operator + () const;
	INT2 operator - () const;

	// Binary operators
	int operator ()(int shift) const;
	INT2 operator + (const INT2& i) const;
	INT2 operator - (const INT2& i) const;
	INT2 operator * (int a) const;
	INT2 operator * (float s) const;
	INT2 operator / (int a) const;
	friend INT2 operator * (int a, const INT2& i);

	// Methods
	int Clamp(int d) const;
	int Lerp(float t) const;
	int Random() const;

	// Static functions
	static int Distance(const INT2& i1, const INT2& i2);
	static INT2 Lerp(const INT2& i1, const INT2& i2, float t);
	static INT2 Max(const INT2& i1, const INT2& i2);
	static INT2 Min(const INT2& i1, const INT2& i2);
	static INT2 Random(const INT2& i1, const INT2& i2);
};

//-----------------------------------------------------------------------------
// 2D int point
//-----------------------------------------------------------------------------
struct UINT2
{
	uint x, y;
};

//-----------------------------------------------------------------------------
// Rectangle using int
//-----------------------------------------------------------------------------
struct Rect
{
	INT2 p1, p2;

	Rect();
	Rect(int x1, int y1, int x2, int y2);
	Rect(const INT2& p1, const INT2& p2);
	Rect(const Rect& box);
	explicit Rect(const BOX2D& box);
	Rect(const BOX2D& box, const INT2& pad);;

	// Comparison operators
	bool operator == (const Rect& r) const;
	bool operator != (const Rect& r) const;

	// Assignment operators
	Rect& operator = (const Rect& r);
	Rect& operator += (const INT2& p);
	Rect& operator -= (const INT2& p);
	Rect& operator *= (int d);
	Rect& operator /= (int d);

	// Unary operators
	Rect operator + () const;
	Rect operator - () const;

	// Binary operators
	Rect operator + (const INT2& p) const;
	Rect operator - (const INT2& p) const;
	Rect operator * (int d) const;
	Rect operator * (const VEC2& v) const;
	Rect operator / (int d) const;
	friend Rect operator * (int d, const Rect& r);

	INT2& LeftTop() { return p1; }
	const INT2& LeftTop() const { return p1; }
	INT2 LeftBottom() const { return INT2(Left(), Bottom()); }
	INT2 RightTop() const { return INT2(Right(), Top()); }
	INT2& RightBottom() { return p2; }
	const INT2& RightBottom() const { return p2; }
	int SizeX() const { return p2.x - p1.x; }
	int SizeY() const { return p2.y - p1.y; }
	int& Left() { return p1.x; }
	const int& Left() const { return p1.x; }
	int& Right() { return p2.x; }
	const int& Right() const { return p2.x; }
	int& Top() { return p1.y; }
	const int& Top() const { return p1.y; }
	int& Bottom() { return p2.y; }
	const int& Bottom() const { return p2.y; }

	// Methods
	bool IsInside(const INT2& pt) const;
	Rect LeftBottomPart() const;
	Rect LeftTopPart() const;
	INT2 Random() const;
	void Resize(const Rect& r);
	void Resize(const INT2& size);
	Rect RightBottomPart() const;
	Rect RightTopPart() const;
	void Set(int x1, int y1, int x2, int y2);
	void Set(const INT2& pos, const INT2& size);
	INT2 Size() const;

	// Static functions
	static Rect Create(const INT2& pos, const INT2& size);
	static Rect Intersect(const Rect& r1, const Rect& r2);
	static bool Intersect(const Rect& r1, const Rect& r2, Rect& result);

	// Constants
	static const Rect Zero;
};

//-----------------------------------------------------------------------------
// 2D float point
//-----------------------------------------------------------------------------
struct VEC2 : XMFLOAT2
{
	VEC2();
	VEC2(float x, float y);
	VEC2(const VEC2& v);
	VEC2(FXMVECTOR v);
	explicit VEC2(float xy);
	explicit VEC2(const INT2& i);
	explicit VEC2(const XMVECTORF32& v);

	operator XMVECTOR() const;
	operator float*();
	operator const float*() const;

	// Comparison operators
	bool operator == (const VEC2& v) const;
	bool operator != (const VEC2& v) const;

	// Assignment operators
	VEC2& operator = (const VEC2& v);
	VEC2& operator = (const XMVECTORF32& v);
	VEC2& operator += (const VEC2& v);
	VEC2& operator -= (const VEC2& v);
	VEC2& operator *= (float s);
	VEC2& operator /= (float s);

	// Unary operators
	VEC2 operator + () const;
	VEC2 operator - () const;

	// Binary operators
	VEC2 operator + (const VEC2& v) const;
	VEC2 operator - (const VEC2& v) const;
	VEC2 operator * (float s) const;
	VEC2 operator / (float s) const;
	friend VEC2 operator * (float s, const VEC2& v);

	// Methods
	float Clamp(float f) const;
	void Clamp(const VEC2& min, const VEC2& max);
	void Clamp(const VEC2& min, const VEC2& max, VEC2& result) const;
	VEC2 Clamped(const VEC2& min = VEC2(0, 0), const VEC2& max = VEC2(1, 1)) const;
	VEC2 Clip() const;
	void Cross(const VEC2& v, VEC2& result) const;
	VEC2 Cross(const VEC2& v) const;
	float Dot(const VEC2& v) const;
	float DotSelf() const;
	bool Equal(const VEC2& v) const;
	bool InBounds(const VEC2& bounds) const;
	float Length() const;
	float LengthSquared() const;
	VEC2& Normalize();
	void Normalize(VEC2& v) const;
	VEC2 Normalized() const;
	float Random() const;
	VEC3 XZ(float y = 0.f) const;

	// Static functions
	static float Angle(const VEC2& v1, const VEC2& v2);
	static void Barycentric(const VEC2& v1, const VEC2& v2, const VEC2& v3, float f, float g, VEC2& result);
	static VEC2 Barycentric(const VEC2& v1, const VEC2& v2, const VEC2& v3, float f, float g);
	static void CatmullRom(const VEC2& v1, const VEC2& v2, const VEC2& v3, const VEC2& v4, float t, VEC2& result);
	static VEC2 CatmullRom(const VEC2& v1, const VEC2& v2, const VEC2& v3, const VEC2& v4, float t);
	static float Distance(const VEC2& v1, const VEC2& v2);
	static float DistanceSquared(const VEC2& v1, const VEC2& v2);
	static void Hermite(const VEC2& v1, const VEC2& t1, const VEC2& v2, const VEC2& t2, float t, VEC2& result);
	static VEC2 Hermite(const VEC2& v1, const VEC2& t1, const VEC2& v2, const VEC2& t2, float t);
	static void Lerp(const VEC2& v1, const VEC2& v2, float t, VEC2& result);
	static VEC2 Lerp(const VEC2& v1, const VEC2& v2, float t);
	static float LookAtAngle(const VEC2& v1, const VEC2& v2);
	static void Max(const VEC2& v1, const VEC2& v2, VEC2& result);
	static VEC2 Max(const VEC2& v1, const VEC2& v2);
	static void Min(const VEC2& v1, const VEC2& v2, VEC2& result);
	static VEC2 Min(const VEC2& v1, const VEC2& v2);
	static void MinMax(const VEC2& v1, const VEC2& v2, VEC2& min, VEC2& max);
	static VEC2 Random(float a, float b);
	static VEC2 Random(const VEC2& v1, const VEC2& v2);
	static VEC2 RandomCirclePt(float r);
	static VEC2 RandomPoissonDiscPoint();
	static void Reflect(const VEC2& ivec, const VEC2& nvec, VEC2& result);
	static VEC2 Reflect(const VEC2& ivec, const VEC2& nvec);
	static void Refract(const VEC2& ivec, const VEC2& nvec, float refractionIndex, VEC2& result);
	static VEC2 Refract(const VEC2& ivec, const VEC2& nvec, float refractionIndex);
	static VEC2 Slerp(const VEC2& a, const VEC2& b, float t);
	static void SmoothStep(const VEC2& v1, const VEC2& v2, float t, VEC2& result);
	static VEC2 SmoothStep(const VEC2& v1, const VEC2& v2, float t);
	static void Transform(const VEC2& v, const QUAT& quat, VEC2& result);
	static VEC2 Transform(const VEC2& v, const QUAT& quat);
	static void Transform(const VEC2& v, const MATRIX& m, VEC2& result);
	static VEC2 Transform(const VEC2& v, const MATRIX& m);
	static void Transform(const VEC2* varray, size_t count, const MATRIX& m, VEC2* resultArray);
	static void Transform(const VEC2& v, const MATRIX& m, VEC4& result);
	static void Transform(const VEC2* varray, size_t count, const MATRIX& m, VEC4* resultArray);
	static void TransformNormal(const VEC2& v, const MATRIX& m, VEC2& result);
	static VEC2 TransformNormal(const VEC2& v, const MATRIX& m);
	static void TransformNormal(const VEC2* varray, size_t count, const MATRIX& m, VEC2* resultArray);

	// Constants
	static const VEC2 Zero;
	static const VEC2 One;
	static const VEC2 UnitX;
	static const VEC2 UnitY;
};

//-----------------------------------------------------------------------------
// 3D float point
//-----------------------------------------------------------------------------
struct VEC3 : XMFLOAT3
{
	VEC3();
	VEC3(float x, float y, float z);
	VEC3(const VEC3& v);
	VEC3(FXMVECTOR v);
	explicit VEC3(const XMVECTORF32& v);

	operator XMVECTOR() const;
	operator float*();
	operator const float*() const;

	// Comparison operators
	bool operator == (const VEC3& v) const;
	bool operator != (const VEC3& v) const;

	// Assignment operators
	VEC3& operator = (const VEC3& v);
	VEC3& operator = (const XMVECTORF32& v);
	VEC3& operator += (const VEC3& v);
	VEC3& operator -= (const VEC3& v);
	VEC3& operator *= (float s);
	VEC3& operator /= (float s);

	// Unary operators
	VEC3 operator + () const;
	VEC3 operator - () const;

	// Binary operators
	VEC3 operator + (const VEC3& v) const;
	VEC3 operator - (const VEC3& v) const;
	VEC3 operator * (float s) const;
	VEC3 operator / (float s) const;
	friend VEC3 operator * (float s, const VEC3& v);

	// Methods
	void Clamp(const VEC3& min, const VEC3& max);
	void Clamp(const VEC3& min, const VEC3& max, VEC3& result) const;
	VEC3 Clamped(const VEC3& min = VEC3(0, 0, 0), const VEC3& max = VEC3(1, 1, 1)) const;
	void Cross(const VEC3& V, VEC3& result) const;
	VEC3 Cross(const VEC3& V) const;
	float Dot(const VEC3& V) const;
	float Dot2d(const VEC3& v) const;
	float Dot2d() const;
	bool Equal(const VEC3& v) const;
	bool InBounds(const VEC3& bounds) const;
	bool IsPositive() const;
	float Length() const;
	float LengthSquared() const;
	VEC3& Normalize();
	void Normalize(VEC3& result) const;
	VEC3 Normalized() const;
	VEC2 XY() const;
	VEC2 XZ() const;

	// Static functions
	static float Angle2d(const VEC3& v1, const VEC3& v2);
	static void Barycentric(const VEC3& v1, const VEC3& v2, const VEC3& v3, float f, float g, VEC3& result);
	static VEC3 Barycentric(const VEC3& v1, const VEC3& v2, const VEC3& v3, float f, float g);
	static void CatmullRom(const VEC3& v1, const VEC3& v2, const VEC3& v3, const VEC3& v4, float t, VEC3& result);
	static VEC3 CatmullRom(const VEC3& v1, const VEC3& v2, const VEC3& v3, const VEC3& v4, float t);
	static float Distance(const VEC3& v1, const VEC3& v2);
	static float DistanceSquared(const VEC3& v1, const VEC3& v2);
	static float Distance2d(const VEC3& v1, const VEC3& v2);
	static void Hermite(const VEC3& v1, const VEC3& t1, const VEC3& v2, const VEC3& t2, float t, VEC3& result);
	static VEC3 Hermite(const VEC3& v1, const VEC3& t1, const VEC3& v2, const VEC3& t2, float t);
	static void Lerp(const VEC3& v1, const VEC3& v2, float t, VEC3& result);
	static VEC3 Lerp(const VEC3& v1, const VEC3& v2, float t);
	static float LookAtAngle(const VEC3& v1, const VEC3& v2);
	static void Max(const VEC3& v1, const VEC3& v2, VEC3& result);
	static VEC3 Max(const VEC3& v1, const VEC3& v2);
	static void Min(const VEC3& v1, const VEC3& v2, VEC3& result);
	static VEC3 Min(const VEC3& v1, const VEC3& v2);
	static void MinMax(const VEC3& v1, const VEC3& v2, VEC3& min, VEC3& max);
	static VEC3 Random(float a, float b);
	static VEC3 Random(const VEC3& min, const VEC3& max);
	static void Reflect(const VEC3& ivec, const VEC3& nvec, VEC3& result);
	static VEC3 Reflect(const VEC3& ivec, const VEC3& nvec);
	static void Refract(const VEC3& ivec, const VEC3& nvec, float refractionIndex, VEC3& result);
	static VEC3 Refract(const VEC3& ivec, const VEC3& nvec, float refractionIndex);
	static void SmoothStep(const VEC3& v1, const VEC3& v2, float t, VEC3& result);
	static VEC3 SmoothStep(const VEC3& v1, const VEC3& v2, float t);
	static void Transform(const VEC3& v, const QUAT& quat, VEC3& result);
	static VEC3 Transform(const VEC3& v, const QUAT& quat);
	static void Transform(const VEC3& v, const MATRIX& m, VEC3& result);
	static VEC3 Transform(const VEC3& v, const MATRIX& m);
	static void Transform(const VEC3* varray, size_t count, const MATRIX& m, VEC3* resultArray);
	static void Transform(const VEC3& v, const MATRIX& m, VEC4& result);
	static void Transform(const VEC3* varray, size_t count, const MATRIX& m, VEC4* resultArray);
	static void TransformNormal(const VEC3& v, const MATRIX& m, VEC3& result);
	static VEC3 TransformNormal(const VEC3& v, const MATRIX& m);
	static void TransformNormal(const VEC3* varray, size_t count, const MATRIX& m, VEC3* resultArray);
	static VEC3 TransformZero(const MATRIX& m);

	// Constants
	static const VEC3 Zero;
	static const VEC3 One;
	static const VEC3 UnitX;
	static const VEC3 UnitY;
	static const VEC3 UnitZ;
	static const VEC3 Up;
	static const VEC3 Down;
	static const VEC3 Right;
	static const VEC3 Left;
	static const VEC3 Forward;
	static const VEC3 Backward;
};

//-----------------------------------------------------------------------------
// 4D float point
//-----------------------------------------------------------------------------
struct VEC4 : XMFLOAT4
{
	VEC4();
	VEC4(float x, float y, float z, float w);
	VEC4(const VEC4& v);
	VEC4(const VEC3& v, float w);
	VEC4(FXMVECTOR v);
	explicit VEC4(const XMVECTORF32& v);

	operator XMVECTOR() const;
	operator float*();
	operator const float*() const;

	// Comparison operators
	bool operator == (const VEC4& v) const;
	bool operator != (const VEC4& v) const;

	// Assignment operators
	VEC4& operator = (const VEC4& v);
	VEC4& operator = (const XMVECTORF32& v);
	VEC4& operator += (const VEC4& v);
	VEC4& operator -= (const VEC4& v);
	VEC4& operator *= (float s);
	VEC4& operator /= (float s);

	// Unary operators
	VEC4 operator+ () const;
	VEC4 operator- () const;

	// Binary operators
	VEC4 operator + (const VEC4& v) const;
	VEC4 operator - (const VEC4& v) const;
	VEC4 operator * (float s) const;
	VEC4 operator / (float s) const;
	friend VEC4 operator * (float s, const VEC4& v);

	// Methods
	void Clamp(const VEC4& vmin, const VEC4& vmax);
	void Clamp(const VEC4& vmin, const VEC4& vmax, VEC4& result) const;
	VEC4 Clamped(const VEC4& min = VEC4(0, 0, 0, 0), const VEC4& max = VEC4(1, 1, 1, 1)) const;
	void Cross(const VEC4& v1, const VEC4& v2, VEC4& result) const;
	VEC4 Cross(const VEC4& v1, const VEC4& v2) const;
	float Dot(const VEC4& V) const;
	float DotSelf() const;
	bool Equal(const VEC4& v) const;
	bool InBounds(const VEC4& Bounds) const;
	float Length() const;
	float LengthSquared() const;
	VEC4& Normalize();
	void Normalize(VEC4& result) const;
	VEC4 Normalized() const;

	// Static functions
	static void Barycentric(const VEC4& v1, const VEC4& v2, const VEC4& v3, float f, float g, VEC4& result);
	static VEC4 Barycentric(const VEC4& v1, const VEC4& v2, const VEC4& v3, float f, float g);
	static void CatmullRom(const VEC4& v1, const VEC4& v2, const VEC4& v3, const VEC4& v4, float t, VEC4& result);
	static VEC4 CatmullRom(const VEC4& v1, const VEC4& v2, const VEC4& v3, const VEC4& v4, float t);
	static float Distance(const VEC4& v1, const VEC4& v2);
	static float DistanceSquared(const VEC4& v1, const VEC4& v2);
	static VEC4 FromColor(DWORD color);
	static void Hermite(const VEC4& v1, const VEC4& t1, const VEC4& v2, const VEC4& t2, float t, VEC4& result);
	static VEC4 Hermite(const VEC4& v1, const VEC4& t1, const VEC4& v2, const VEC4& t2, float t);
	static void Lerp(const VEC4& v1, const VEC4& v2, float t, VEC4& result);
	static VEC4 Lerp(const VEC4& v1, const VEC4& v2, float t);
	static void Max(const VEC4& v1, const VEC4& v2, VEC4& result);
	static VEC4 Max(const VEC4& v1, const VEC4& v2);
	static void Min(const VEC4& v1, const VEC4& v2, VEC4& result);
	static VEC4 Min(const VEC4& v1, const VEC4& v2);
	static void Reflect(const VEC4& ivec, const VEC4& nvec, VEC4& result);
	static VEC4 Reflect(const VEC4& ivec, const VEC4& nvec);
	static void Refract(const VEC4& ivec, const VEC4& nvec, float refractionIndex, VEC4& result);
	static VEC4 Refract(const VEC4& ivec, const VEC4& nvec, float refractionIndex);
	static void SmoothStep(const VEC4& v1, const VEC4& v2, float t, VEC4& result);
	static VEC4 SmoothStep(const VEC4& v1, const VEC4& v2, float t);
	static void Transform(const VEC2& v, const QUAT& quat, VEC4& result);
	static VEC4 Transform(const VEC2& v, const QUAT& quat);
	static void Transform(const VEC3& v, const QUAT& quat, VEC4& result);
	static VEC4 Transform(const VEC3& v, const QUAT& quat);
	static void Transform(const VEC4& v, const QUAT& quat, VEC4& result);
	static VEC4 Transform(const VEC4& v, const QUAT& quat);
	static void Transform(const VEC4& v, const MATRIX& m, VEC4& result);
	static VEC4 Transform(const VEC4& v, const MATRIX& m);
	static void Transform(const VEC4* varray, size_t count, const MATRIX& m, VEC4* resultArray);

	// Constants
	static const VEC4 Zero;
	static const VEC4 One;
	static const VEC4 UnitX;
	static const VEC4 UnitY;
	static const VEC4 UnitZ;
	static const VEC4 UnitW;
};

//-----------------------------------------------------------------------------
// 2d box using floats
//-----------------------------------------------------------------------------
struct BOX2D
{
	VEC2 v1, v2;

	BOX2D();
	BOX2D(float minx, float miny, float maxx, float maxy);
	BOX2D(const VEC2& v1, const VEC2& v2);
	BOX2D(const BOX2D& box);
	BOX2D(float x, float y);
	BOX2D(const BOX2D& box, float margin);
	explicit BOX2D(const VEC2& v);
	explicit BOX2D(const Rect& r);

	// Comparison operators
	bool operator == (const BOX2D& b) const;
	bool operator != (const BOX2D& b) const;

	// Assignment operators
	BOX2D& operator = (const BOX2D& b);
	BOX2D& operator += (const VEC2& v);
	BOX2D& operator -= (const VEC2& v);
	BOX2D& operator *= (float f);
	BOX2D& operator /= (float f);

	// Unary operators
	BOX2D operator + () const;
	BOX2D operator - () const;

	// Binary operators
	BOX2D operator + (const VEC2& v) const;
	BOX2D operator - (const VEC2& v) const;
	BOX2D operator * (float f) const;
	BOX2D operator / (float f) const;
	BOX2D operator / (const VEC2& v) const;
	friend BOX2D operator * (float f, const BOX2D& v);

	static BOX2D Create(const INT2& pos, const INT2& size)
	{
		BOX2D box;
		box.Set(pos, size);
		return box;
	}

	// Methods
	VEC2 GetRandomPoint() const;
	bool IsInside(const VEC2& v) const;
	bool IsInside(const VEC3& v) const;
	bool IsInside(const INT2& p) const;
	bool IsValid() const;
	VEC2 Midpoint() const;
	VEC2 Size() const;
	float SizeX() const;
	float SizeY() const;

	void Set(const INT2& pos, const INT2& size)
	{
		v1.x = (float)pos.x;
		v1.y = (float)pos.y;
		v2.x = v1.x + (float)size.x;
		v2.y = v1.y + (float)size.y;
	}

	void Move(const VEC2& pos)
	{
		VEC2 dif = pos - v1;
		v1 += dif;
		v2 += dif;
	}

	VEC3 GetRandomPos3(float y = 0.f) const
	{
		return VEC3(::Random(v1.x, v2.x), y, ::Random(v1.y, v2.y));
	}

	VEC2 LeftTop() const
	{
		return v1;
	}
	VEC2 RightBottom() const
	{
		return v2;
	}
	VEC2 RightTop() const
	{
		return VEC2(v2.x, v1.y);
	}
	VEC2 LeftBottom() const
	{
		return VEC2(v1.x, v2.y);
	}

	VEC3 LeftTop3() const
	{
		return VEC3(v1.x, v1.y, 0);
	}
	VEC3 RightTop3() const
	{
		return VEC3(v2.x, v1.y, 0);
	}
	VEC3 LeftBottom3() const
	{
		return VEC3(v1.x, v2.y, 0);
	}
	VEC3 RightBottom3() const
	{
		return VEC3(v2.x, v2.y, 0);
	}

	BOX2D LeftBottomPart() const
	{
		return BOX2D(v1.x, v1.y, v1.x + (v2.x - v1.x) / 2, v1.y + (v2.y - v1.y) / 2);
	}
	BOX2D RightBottomPart() const
	{
		return BOX2D(v1.x + (v2.x - v1.x) / 2, v1.y, v2.x, v1.y + (v2.y - v1.y) / 2);
	}
	BOX2D LeftTopPart() const
	{
		return BOX2D(v1.x, v1.y + (v2.y - v1.y) / 2, v1.x + (v2.x - v1.x) / 2, v2.y);
	}
	BOX2D RightTopPart() const
	{
		return BOX2D(v1.x + (v2.x - v1.x) / 2, v1.y + (v2.y - v1.y) / 2, v2.x, v2.y);
	}

	void ToRectangle(float& x, float& y, float& w, float& h) const
	{
		x = v1.x + (v2.x - v1.x) / 2;
		y = v1.y + (v2.y - v1.y) / 2;
		w = (v2.x - v1.x) / 2;
		h = (v2.y - v1.y) / 2;
	}

	float& Left() { return v1.x; }
	float& Right() { return v2.x; }
	float& Top() { return v1.y; }
	float& Bottom() { return v2.y; }

	float Left() const { return v1.x; }
	float Right() const { return v2.x; }
	float Top() const { return v1.y; }
	float Bottom() const { return v2.y; }
};

//-----------------------------------------------------------------------------
// 3d float box
//-----------------------------------------------------------------------------
struct BOX
{
	VEC3 v1, v2;

	BOX();
	BOX(float minx, float miny, float minz, float maxx, float maxy, float maxz);
	BOX(const VEC3& v1, const VEC3& v2);
	BOX(const BOX& box);
	BOX(float x, float y, float z);
	explicit BOX(const VEC3& v);

	// Comparison operators
	bool operator == (const BOX& b) const;
	bool operator != (const BOX& b) const;

	// Assignment operators
	BOX& operator = (const BOX& b);
	BOX& operator += (const VEC3& v);
	BOX& operator -= (const VEC3& v);
	BOX& operator *= (float f);
	BOX& operator /= (float f);

	// Unary operators
	BOX operator + () const;
	BOX operator - () const;

	// Binary operators
	BOX operator + (const VEC3& v) const;
	BOX operator - (const VEC3& v) const;
	BOX operator * (float f) const;
	BOX operator / (float f) const;
	friend BOX operator * (float f, const BOX& b);

	// Methods
	VEC3 GetRandomPoint() const;
	bool IsInside(const VEC3& v) const;
	bool IsValid() const;
	VEC3 Midpoint() const;
	VEC3 Size() const;
	float SizeX() const;
	VEC2 SizeXZ() const;
	float SizeY() const;
	float SizeZ() const;
};

//-----------------------------------------------------------------------------
// 4x4 float matrix
//-----------------------------------------------------------------------------
struct MATRIX : XMFLOAT4X4
{
	MATRIX();
	MATRIX(float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23,
		float m30, float m31, float m32, float m33);
	MATRIX(const VEC3& v1, const VEC3& v2, const VEC3& v3);
	MATRIX(const VEC4& v1, const VEC4& v2, const VEC4& v3, const VEC4& v4);
	MATRIX(const MATRIX& m);
	MATRIX(CXMMATRIX m);

	operator XMMATRIX() const;

	// Comparison operators
	bool operator == (const MATRIX& m) const;
	bool operator != (const MATRIX& m) const;

	// Assignment operators
	MATRIX& operator = (const MATRIX& m);
	MATRIX& operator += (const MATRIX& m);
	MATRIX& operator -= (const MATRIX& m);
	MATRIX& operator *= (const MATRIX& m);
	MATRIX& operator *= (float s);
	MATRIX& operator /= (float s);
	MATRIX& operator /= (const MATRIX& m);

	// Unary operators
	MATRIX operator+ () const;
	MATRIX operator- () const;

	// Binary operators
	MATRIX operator+ (const MATRIX& m) const;
	MATRIX operator- (const MATRIX& m) const;
	MATRIX operator* (const MATRIX& m) const;
	MATRIX operator* (float S) const;
	MATRIX operator/ (float S) const;
	MATRIX operator/ (const MATRIX& m) const;
	friend MATRIX operator * (float s, const MATRIX& m);

	// Methods
	bool Decompose(VEC3& scale, QUAT& rotation, VEC3& translation);
	float Determinant() const;
	float GetYaw() const;
	void Identity();
	MATRIX Inverse() const;
	void Inverse(MATRIX& result) const;
	MATRIX Transpose() const;
	void Transpose(MATRIX& result) const;

	// Static functions
	static MATRIX CreateBillboard(const VEC3& object, const VEC3& cameraPosition, const VEC3& cameraUp, const VEC3* cameraForward = nullptr);
	static MATRIX CreateConstrainedBillboard(const VEC3& object, const VEC3& cameraPosition, const VEC3& rotateAxis,
		const VEC3* cameraForward = nullptr, const VEC3* objectForward = nullptr);
	static MATRIX CreateFromAxisAngle(const VEC3& axis, float angle);
	static MATRIX CreateLookAt(const VEC3& position, const VEC3& target, const VEC3& up = VEC3(0, 1, 0));
	static MATRIX CreateOrthographic(float width, float height, float zNearPlane, float zFarPlane);
	static MATRIX CreateOrthographicOffCenter(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane);
	static MATRIX CreatePerspective(float width, float height, float nearPlane, float farPlane);
	static MATRIX CreatePerspectiveFieldOfView(float fov, float aspectRatio, float nearPlane, float farPlane);
	static MATRIX CreatePerspectiveOffCenter(float left, float right, float bottom, float top, float nearPlane, float farPlane);
	static MATRIX CreateReflection(const PLANE& plane);
	static MATRIX CreateShadow(const VEC3& lightDir, const PLANE& plane);
	static MATRIX CreateWorld(const VEC3& position, const VEC3& forward, const VEC3& up);
	static void Lerp(const MATRIX& M1, const MATRIX& M2, float t, MATRIX& result);
	static MATRIX Lerp(const MATRIX& M1, const MATRIX& M2, float t);
	static MATRIX Rotation(float yaw, float pitch, float roll);
	static MATRIX Rotation(const VEC3& v);
	static MATRIX Rotation(const QUAT& quat);
	static MATRIX RotationX(float radians);
	static MATRIX RotationY(float radians);
	static MATRIX RotationZ(float radians);
	static MATRIX Scale(const VEC3& scales);
	static MATRIX Scale(float xs, float ys, float zs);
	static MATRIX Scale(float scale);
	static void Transform(const MATRIX& M, const QUAT& rotation, MATRIX& result);
	static MATRIX Transform(const MATRIX& M, const QUAT& rotation);
	static MATRIX Transform2D(const VEC2* scaling_center, float scaling_rotation, const VEC2* scaling, const VEC2* rotation_center, float rotation, const VEC2* translation);
	static MATRIX Translation(const VEC3& position);
	static MATRIX Translation(float x, float y, float z);

	// Constants
	static const MATRIX IdentityMatrix;
};

//-----------------------------------------------------------------------------
// Quaternion
//-----------------------------------------------------------------------------
struct QUAT : public XMFLOAT4
{
	QUAT();
	QUAT(float x, float y, float z, float w);
	QUAT(const VEC3& v, float w);
	QUAT(const QUAT& q);
	QUAT(FXMVECTOR v);
	explicit QUAT(const VEC4& v);
	explicit QUAT(const XMVECTORF32& v);

	operator XMVECTOR() const;

	// Comparison operators
	bool operator == (const QUAT& q) const;
	bool operator != (const QUAT& q) const;

	// Assignment operators
	QUAT& operator = (const QUAT& q);
	QUAT& operator = (const XMVECTORF32& v);
	QUAT& operator += (const QUAT& q);
	QUAT& operator -= (const QUAT& q);
	QUAT& operator *= (const QUAT& q);
	QUAT& operator *= (float s);
	QUAT& operator /= (const QUAT& q);

	// Unary operators
	QUAT operator + () const;
	QUAT operator - () const;

	// Binary operators
	QUAT operator + (const QUAT& q) const;
	QUAT operator - (const QUAT& q) const;
	QUAT operator * (const QUAT& q) const;
	QUAT operator * (float s) const;
	QUAT operator / (const QUAT& q) const;
	friend QUAT operator * (float s, const QUAT& q);

	// Mathods
	void Conjugate();
	void Conjugate(QUAT& result) const;
	float Dot(const QUAT& q) const;
	void Inverse(QUAT& result) const;
	float Length() const;
	float LengthSquared() const;
	void Normalize();
	void Normalize(QUAT& result) const;

	// Static functions
	static void Concatenate(const QUAT& q1, const QUAT& q2, QUAT& result);
	static QUAT Concatenate(const QUAT& q1, const QUAT& q2);
	static QUAT CreateFromAxisAngle(const VEC3& axis, float angle);
	static QUAT CreateFromRotationMatrix(const MATRIX& M);
	static QUAT CreateFromYawPitchRoll(float yaw, float pitch, float roll);
	static void Lerp(const QUAT& q1, const QUAT& q2, float t, QUAT& result);
	static QUAT Lerp(const QUAT& q1, const QUAT& q2, float t);
	static void Slerp(const QUAT& q1, const QUAT& q2, float t, QUAT& result);
	static QUAT Slerp(const QUAT& q1, const QUAT& q2, float t);

	// Constants
	static const QUAT Identity;
};

//-----------------------------------------------------------------------------
// Plane
//-----------------------------------------------------------------------------
struct PLANE : public XMFLOAT4
{
	PLANE();
	PLANE(float x, float y, float z, float w);
	PLANE(const VEC3& normal, float d);
	PLANE(const VEC3& point1, const VEC3& point2, const VEC3& point3);
	PLANE(const VEC3& point, const VEC3& normal);
	PLANE(FXMVECTOR v);
	explicit PLANE(const VEC4& v);
	explicit PLANE(const XMVECTORF32& v);

	operator XMVECTOR() const;

	// Comparison operators
	bool operator == (const PLANE& p) const;
	bool operator != (const PLANE& p) const;

	// Assignment operators
	PLANE& operator = (const PLANE& p);

	// Methods
	float Dot(const VEC4& v) const;
	float DotCoordinate(const VEC3& position) const;
	float DotNormal(const VEC3& normal) const;
	void Normalize();
	void Normalize(PLANE& result) const;

	// Static functions
	static bool Intersect3Planes(const PLANE& p1, const PLANE& p2, const PLANE& p3, VEC3& result);
	static void Transform(const PLANE& plane, const MATRIX& M, PLANE& result);
	static PLANE Transform(const PLANE& plane, const MATRIX& M);
	static void Transform(const PLANE& plane, const QUAT& rotation, PLANE& result);
	static PLANE Transform(const PLANE& plane, const QUAT& rotation);
};

//-----------------------------------------------------------------------------
// Frustrum planes to check objects visible from camera
//-----------------------------------------------------------------------------
struct FrustumPlanes
{
	PLANE planes[6];

	FrustumPlanes() {}
	explicit FrustumPlanes(const MATRIX& worldViewProj) { Set(worldViewProj); }
	void Set(const MATRIX& worldViewProj);

	// Return points on edge of frustum
	void GetPoints(VEC3* points) const;
	static void GetPoints(const MATRIX& worldViewProj, VEC3* points);
	// Checks if point is inside frustum
	bool PointInFrustum(const VEC3 &p) const;
	// Checks if box collide with frustum
	// In rare cases can return true even if it's outside!
	bool BoxToFrustum(const BOX& box) const;
	bool BoxToFrustum(const BOX2D& box) const;
	// Checks if box is fully inside frustum
	bool BoxInFrustum(const BOX& box) const;
	// Checks if sphere collide with frustum
	// In rare cases can return true even if it's outside!
	bool SphereToFrustum(const VEC3& sphere_center, float sphere_radius) const;
	// Checks if sphere is fully inside frustum
	bool SphereInFrustum(const VEC3& sphere_center, float sphere_radius) const;
};

//-----------------------------------------------------------------------------
struct Pixel
{
	INT2 pt;
	byte alpha;

	Pixel(int x, int y, byte alpha = 0) : pt(x, y), alpha(alpha) {}

	static void PlotLine(int x0, int y0, int x1, int y1, float th, vector<Pixel>& pixels);
	static void PlotQuadBezierSeg(int x0, int y0, int x1, int y1, int x2, int y2, float w, float th, vector<Pixel>& pixels);
	static void PlotQuadBezier(int x0, int y0, int x1, int y1, int x2, int y2, float w, float th, vector<Pixel>& pixels);
	static void PlotCubicBezierSeg(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, float th, vector<Pixel>& pixels);
	static void PlotCubicBezier(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, float th, vector<Pixel>& pixels);
};

//-----------------------------------------------------------------------------
// Object oriented bounding box
//-----------------------------------------------------------------------------
struct OBBOX
{
	VEC3 pos;
	VEC3 size;
	MATRIX rot;
};
// inna wersja, oka¿e siê czy lepszy algorytm
struct OOB
{
	VEC3 c; // œrodek
	VEC3 u[3]; // obrót po X,Y,Z
	VEC3 e; // po³owa rozmiaru
};

//-----------------------------------------------------------------------------
// KOLIZJE
//-----------------------------------------------------------------------------
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
bool OrientedBoxToOrientedBox(const OBBOX& obox1, const OBBOX& obox2, VEC3* contact);
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
	return DistanceSqrt(cx1, cy1, cx2, cy2) < r * r;
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
bool RayToMesh(const VEC3& _ray_pos, const VEC3& _ray_dir, const VEC3& _obj_pos, float _obj_rot, VertexData* _vd, float& _dist);

//-----------------------------------------------------------------------------
// POD types
//-----------------------------------------------------------------------------
struct VEC3P
{
	float x, y, z;
};

//-----------------------------------------------------------------------------
// Kahn's algorithm
class Graph
{
public:
	struct Edge
	{
		int from;
		int to;
	};

	Graph(int vertices) : vertices(vertices)
	{
	}

	void AddEdge(int from, int to)
	{
		edges.push_back({ from, to });
	}

	bool Sort()
	{
		vector<int> S;

		for(int i = 0; i < vertices; ++i)
		{
			bool any = false;
			for(auto& e : edges)
			{
				if(e.to == i)
				{
					any = true;
					break;
				}
			}
			if(!any)
				S.push_back(i);
		}

		while(!S.empty())
		{
			int n = S.back();
			S.pop_back();
			result.push_back(n);

			for(auto it = edges.begin(), end = edges.end(); it != end; )
			{
				if(it->from == n)
				{
					int m = it->to;
					it = edges.erase(it);
					end = edges.end();

					bool any = false;
					for(auto& e : edges)
					{
						if(e.to == m)
						{
							any = true;
							break;
						}
					}
					if(!any)
						S.push_back(m);
				}
				else
					++it;
			}
		}

		// if there any edges left, graph has cycles
		return edges.empty();
	}

	vector<int>& GetResult() { return result; }

private:
	vector<int> result;
	vector<Edge> edges;
	int vertices;
};

#include "CoreMath.inl"
