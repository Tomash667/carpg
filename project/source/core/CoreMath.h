#pragma once

using namespace DirectX;

//-----------------------------------------------------------------------------
// Declarations
struct Int2;
struct Rect;
struct Vec2;
struct Vec3;
struct Vec4;
struct Box2d;
struct Box;
struct Obbox;
struct Oob;
struct Matrix;
struct Quat;
struct Plane;

//-----------------------------------------------------------------------------
// Global constants
const float PI = 3.14159265358979323846f;
const float SQRT_2 = 1.41421356237f;
const float G = 9.8105f;
const float MAX_ANGLE = PI * 2 - FLT_EPSILON;

//-----------------------------------------------------------------------------
// Random numbers
//-----------------------------------------------------------------------------
namespace internal
{
	extern std::minstd_rand rng;
};

void Srand();
inline void Srand(uint seed)
{
	internal::rng.seed(seed);
}
int RandVal();
inline int Rand()
{
	return internal::rng();
}
inline int Rand(int a)
{
	return Rand() % a;
}
inline int MyRand(int a)
{
	return Rand(a);
}

// Random float number in range <0,1>
inline float Random()
{
	return (float)Rand() / internal::rng.max();
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
	return ((float)Rand() / internal::rng.max())*a;
}

inline int RandomNormal(int a)
{
	assert(a > 0);
	std::binomial_distribution<> r(a);
	return r(internal::rng);
}
inline int RandomNormal(int a, int b)
{
	if(a == b)
		return a;
	else
	{
		assert(b > a);
		std::binomial_distribution<> r(b - a);
		return a + r(internal::rng);
	}
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
	return ((float)Rand() / internal::rng.max())*(b - a) + a;
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

template<typename T>
T GetRandomWeight(const vector<pair<T, uint>>& v, uint total)
{
	uint sum = 0;
	uint j = Rand() % total;
	for(auto& e : v)
	{
		sum += e.second;
		if(j < sum)
			return e.first;
	}
	assert(0);
	return v[0].first;
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
	return sqrt(x*x + y * y);
}
inline float DistanceSqrt(float x1, float y1, float x2, float y2)
{
	float x = abs(x1 - x2),
		y = abs(y1 - y2);
	return x * x + y * y;
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

inline bool AlmostZero(float f)
{
	return (absolute_cast<unsigned>(f) & 0x7f800000L) == 0;
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
template<typename T>
inline T Lerp(T a, T b, float t)
{
	return T(t*(b - a)) + a;
}

// Return shortes direction between angles
float ShortestArc(float a, float b);

// Linear interpolation between two angles
void LerpAngle(float& angle, float from, float to, float t);

void AdjustAngle(float& angle, float expected, float max_diff);

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
	return Clip(a + angle * t);
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

// Round up to next highest power of 2
template<typename T>
inline T NextPow2(T x)
{
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x++;
	return x;
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
struct Int2
{
	int x, y;

	Int2();
	Int2(int x, int y);
	Int2(const Int2& i);
	template<typename T, typename T2>
	Int2(T x, T2 y);
	explicit Int2(int xy);
	explicit Int2(const Vec2& v);
	explicit Int2(const Vec3& v);

	// Comparison operators
	bool operator == (const Int2& i) const;
	bool operator != (const Int2& i) const;
	bool operator > (const Int2& i) const;

	// Assignment operators
	Int2& operator = (const Int2& i);
	void operator += (const Int2& i);
	void operator -= (const Int2& i);
	void operator *= (int a);
	void operator /= (int a);

	// Unary operators
	Int2 operator + () const;
	Int2 operator - () const;

	// Binary operators
	int operator ()(int shift) const;
	Int2 operator + (const Int2& i) const;
	Int2 operator - (const Int2& i) const;
	Int2 operator * (const Vec2& scale) const;
	Int2 operator * (int a) const;
	Int2 operator * (float s) const;
	Int2 operator / (int a) const;
	friend Int2 operator * (int a, const Int2& i);

	// Methods
	int Clamp(int d) const;
	int Lerp(float t) const;
	int Random() const;
	Int2 YX() const { return Int2(y, x); }

	// Static functions
	static int Distance(const Int2& i1, const Int2& i2);
	static Int2 Lerp(const Int2& i1, const Int2& i2, float t);
	static Int2 Max(const Int2& i1, const Int2& i2);
	static Int2 Min(const Int2& i1, const Int2& i2);
	static void MinMax(Int2& i1, Int2& i2);
	static void MinMax(const Int2& i1, const Int2& i2, Int2& min, Int2& max);
	static Int2 Random(const Int2& i1, const Int2& i2);

	// Constants
	static const Int2 Zero;
	static const Int2 MaxValue;
};

//-----------------------------------------------------------------------------
// Rectangle using int
//-----------------------------------------------------------------------------
struct Rect
{
	Int2 p1, p2;

	Rect();
	Rect(int x, int y);
	Rect(int x1, int y1, int x2, int y2);
	Rect(const Int2& p1, const Int2& p2);
	Rect(const Rect& box);
	explicit Rect(const Box2d& box);
	Rect(const Box2d& box, const Int2& pad);

	// Comparison operators
	bool operator == (const Rect& r) const;
	bool operator != (const Rect& r) const;

	// Assignment operators
	Rect& operator = (const Rect& r);
	Rect& operator += (const Int2& p);
	Rect& operator -= (const Int2& p);
	Rect& operator *= (int d);
	Rect& operator /= (int d);

	// Unary operators
	Rect operator + () const;
	Rect operator - () const;

	// Binary operators
	Rect operator + (const Int2& p) const;
	Rect operator - (const Int2& p) const;
	Rect operator * (int d) const;
	Rect operator * (const Vec2& v) const;
	Rect operator / (int d) const;
	friend Rect operator * (int d, const Rect& r);

	Int2& LeftTop() { return p1; }
	const Int2& LeftTop() const { return p1; }
	Int2 LeftBottom() const { return Int2(Left(), Bottom()); }
	Int2 RightTop() const { return Int2(Right(), Top()); }
	Int2& RightBottom() { return p2; }
	const Int2& RightBottom() const { return p2; }
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
	int MidX() const { return (p2.x - p1.x) / 2 + p1.x; }
	int MidY() const { return (p2.y - p1.y) / 2 + p1.y; }

	// Methods
	bool IsInside(const Int2& pt) const;
	Rect LeftBottomPart() const;
	Rect LeftTopPart() const;
	Int2 Random() const;
	void Resize(const Rect& r);
	Rect RightBottomPart() const;
	Rect RightTopPart() const;
	void Set(int x1, int y1, int x2, int y2);
	void Set(const Int2& pos, const Int2& size);
	Int2 Size() const;

	// Static functions
	static Rect Create(const Int2& pos, const Int2& size);
	static Rect Create(const Int2& pos, const Int2& size, int pad);
	static Rect Intersect(const Rect& r1, const Rect& r2);
	static bool Intersect(const Rect& r1, const Rect& r2, Rect& result);
	static bool IsInside(const Int2& pos, const Int2& size, const Int2& pt);

	// Constants
	static const Rect Zero;
};

//-----------------------------------------------------------------------------
// 2D float point
//-----------------------------------------------------------------------------
struct Vec2 : XMFLOAT2
{
	Vec2();
	Vec2(float x, float y);
	Vec2(const Vec2& v);
	Vec2(FXMVECTOR v);
	explicit Vec2(float xy);
	explicit Vec2(const Int2& i);
	explicit Vec2(const XMVECTORF32& v);

	operator XMVECTOR() const;
	explicit operator float*();
	explicit operator const float*() const;
	float& operator [](int index);
	const float& operator [](int index) const;

	// Comparison operators
	bool operator == (const Vec2& v) const;
	bool operator != (const Vec2& v) const;
	bool operator < (const Vec2& v) const;

	// Assignment operators
	Vec2& operator = (const Vec2& v);
	Vec2& operator = (const XMVECTORF32& v);
	Vec2& operator += (const Vec2& v);
	Vec2& operator -= (const Vec2& v);
	Vec2& operator *= (float s);
	Vec2& operator /= (float s);

	// Unary operators
	Vec2 operator + () const;
	Vec2 operator - () const;

	// Binary operators
	Vec2 operator + (const Vec2& v) const;
	Vec2 operator - (const Vec2& v) const;
	Vec2 operator * (const Vec2& v) const;
	Vec2 operator * (float s) const;
	Vec2 operator / (float s) const;
	Vec2 operator / (const Int2& i) const;
	friend Vec2 operator * (float s, const Vec2& v);

	// Methods
	float Clamp(float f) const;
	void Clamp(const Vec2& min, const Vec2& max);
	void Clamp(const Vec2& min, const Vec2& max, Vec2& result) const;
	Vec2 Clamped(const Vec2& min = Vec2(0, 0), const Vec2& max = Vec2(1, 1)) const;
	Vec2 Clip() const;
	void Cross(const Vec2& v, Vec2& result) const;
	Vec2 Cross(const Vec2& v) const;
	float Dot(const Vec2& v) const;
	float DotSelf() const;
	bool Equal(const Vec2& v) const;
	bool InBounds(const Vec2& bounds) const;
	float Length() const;
	float LengthSquared() const;
	float Lerp(float t) const { return ::Lerp(x, y, t); }
	Vec2& Normalize();
	void Normalize(Vec2& v) const;
	Vec2 Normalized() const;
	float Random() const;
	void Swap() { std::swap(x, y); }
	Vec3 XY(float z = 0.f) const;
	Vec3 XZ(float y = 0.f) const;

	// Static functions
	static float Angle(const Vec2& v1, const Vec2& v2);
	static void Barycentric(const Vec2& v1, const Vec2& v2, const Vec2& v3, float f, float g, Vec2& result);
	static Vec2 Barycentric(const Vec2& v1, const Vec2& v2, const Vec2& v3, float f, float g);
	static void CatmullRom(const Vec2& v1, const Vec2& v2, const Vec2& v3, const Vec2& v4, float t, Vec2& result);
	static Vec2 CatmullRom(const Vec2& v1, const Vec2& v2, const Vec2& v3, const Vec2& v4, float t);
	static float Distance(const Vec2& v1, const Vec2& v2);
	static float DistanceSquared(const Vec2& v1, const Vec2& v2);
	static void Hermite(const Vec2& v1, const Vec2& t1, const Vec2& v2, const Vec2& t2, float t, Vec2& result);
	static Vec2 Hermite(const Vec2& v1, const Vec2& t1, const Vec2& v2, const Vec2& t2, float t);
	static void Lerp(const Vec2& v1, const Vec2& v2, float t, Vec2& result);
	static Vec2 Lerp(const Vec2& v1, const Vec2& v2, float t);
	static float LookAtAngle(const Vec2& v1, const Vec2& v2);
	static void Max(const Vec2& v1, const Vec2& v2, Vec2& result);
	static Vec2 Max(const Vec2& v1, const Vec2& v2);
	static void Min(const Vec2& v1, const Vec2& v2, Vec2& result);
	static Vec2 Min(const Vec2& v1, const Vec2& v2);
	static void MinMax(const Vec2& v1, const Vec2& v2, Vec2& min, Vec2& max);
	static Vec2 Random(float a, float b);
	static Vec2 Random(const Vec2& v1, const Vec2& v2);
	static Vec2 RandomCirclePt(float r);
	static Vec2 RandomPoissonDiscPoint();
	static void Reflect(const Vec2& ivec, const Vec2& nvec, Vec2& result);
	static Vec2 Reflect(const Vec2& ivec, const Vec2& nvec);
	static void Refract(const Vec2& ivec, const Vec2& nvec, float refractionIndex, Vec2& result);
	static Vec2 Refract(const Vec2& ivec, const Vec2& nvec, float refractionIndex);
	static Vec2 Slerp(const Vec2& a, const Vec2& b, float t);
	static void SmoothStep(const Vec2& v1, const Vec2& v2, float t, Vec2& result);
	static Vec2 SmoothStep(const Vec2& v1, const Vec2& v2, float t);
	static void Transform(const Vec2& v, const Quat& quat, Vec2& result);
	static Vec2 Transform(const Vec2& v, const Quat& quat);
	static void Transform(const Vec2& v, const Matrix& m, Vec2& result);
	static Vec2 Transform(const Vec2& v, const Matrix& m);
	static void Transform(const Vec2* varray, size_t count, const Matrix& m, Vec2* resultArray);
	static void Transform(const Vec2& v, const Matrix& m, Vec4& result);
	static void Transform(const Vec2* varray, size_t count, const Matrix& m, Vec4* resultArray);
	static void TransformNormal(const Vec2& v, const Matrix& m, Vec2& result);
	static Vec2 TransformNormal(const Vec2& v, const Matrix& m);
	static void TransformNormal(const Vec2* varray, size_t count, const Matrix& m, Vec2* resultArray);

	// Constants
	static const Vec2 Zero;
	static const Vec2 One;
	static const Vec2 UnitX;
	static const Vec2 UnitY;
};

//-----------------------------------------------------------------------------
// 3D float point
//-----------------------------------------------------------------------------
struct Vec3 : XMFLOAT3
{
	Vec3();
	Vec3(float x, float y, float z);
	Vec3(const Vec3& v);
	Vec3(FXMVECTOR v);
	explicit Vec3(const XMVECTORF32& v);
	explicit Vec3(const float* f);

	operator XMVECTOR() const;
	explicit operator float*();
	explicit operator const float*() const;
	float& operator [](int index);
	const float& operator [](int index) const;

	// Comparison operators
	bool operator == (const Vec3& v) const;
	bool operator != (const Vec3& v) const;
	bool operator < (const Vec3& v) const;

	// Assignment operators
	Vec3& operator = (const Vec3& v);
	Vec3& operator = (const XMVECTORF32& v);
	Vec3& operator += (const Vec3& v);
	Vec3& operator -= (const Vec3& v);
	Vec3& operator *= (float s);
	Vec3& operator /= (float s);

	// Unary operators
	Vec3 operator + () const;
	Vec3 operator - () const;

	// Binary operators
	Vec3 operator + (const Vec3& v) const;
	Vec3 operator - (const Vec3& v) const;
	Vec3 operator * (float s) const;
	Vec3 operator / (float s) const;
	friend Vec3 operator * (float s, const Vec3& v);

	// Methods
	void Clamp(const Vec3& min, const Vec3& max);
	void Clamp(const Vec3& min, const Vec3& max, Vec3& result) const;
	Vec3 Clamped(const Vec3& min = Vec3(0, 0, 0), const Vec3& max = Vec3(1, 1, 1)) const;
	void Cross(const Vec3& V, Vec3& result) const;
	Vec3 Cross(const Vec3& V) const;
	float Dot(const Vec3& V) const;
	float Dot2d(const Vec3& v) const;
	float Dot2d() const;
	bool Equal(const Vec3& v) const;
	bool InBounds(const Vec3& bounds) const;
	bool IsPositive() const;
	float Length() const;
	float LengthSquared() const;
	Vec3 ModX(float value) const;
	Vec3 ModY(float value) const;
	Vec3 ModZ(float value) const;
	Vec3& Normalize();
	void Normalize(Vec3& result) const;
	Vec3 Normalized() const;
	Vec2 XY() const;
	Vec2 XZ() const;

	// Static functions
	static float Angle2d(const Vec3& v1, const Vec3& v2);
	static void Barycentric(const Vec3& v1, const Vec3& v2, const Vec3& v3, float f, float g, Vec3& result);
	static Vec3 Barycentric(const Vec3& v1, const Vec3& v2, const Vec3& v3, float f, float g);
	static void CatmullRom(const Vec3& v1, const Vec3& v2, const Vec3& v3, const Vec3& v4, float t, Vec3& result);
	static Vec3 CatmullRom(const Vec3& v1, const Vec3& v2, const Vec3& v3, const Vec3& v4, float t);
	static float Distance(const Vec3& v1, const Vec3& v2);
	static float DistanceSquared(const Vec3& v1, const Vec3& v2);
	static float Distance2d(const Vec3& v1, const Vec3& v2);
	static void Hermite(const Vec3& v1, const Vec3& t1, const Vec3& v2, const Vec3& t2, float t, Vec3& result);
	static Vec3 Hermite(const Vec3& v1, const Vec3& t1, const Vec3& v2, const Vec3& t2, float t);
	static void Lerp(const Vec3& v1, const Vec3& v2, float t, Vec3& result);
	static Vec3 Lerp(const Vec3& v1, const Vec3& v2, float t);
	static float LookAtAngle(const Vec3& v1, const Vec3& v2);
	static void Max(const Vec3& v1, const Vec3& v2, Vec3& result);
	static Vec3 Max(const Vec3& v1, const Vec3& v2);
	static void Min(const Vec3& v1, const Vec3& v2, Vec3& result);
	static Vec3 Min(const Vec3& v1, const Vec3& v2);
	static void MinMax(const Vec3& v1, const Vec3& v2, Vec3& min, Vec3& max);
	static Vec3 Random(float a, float b);
	static Vec3 Random(const Vec3& min, const Vec3& max);
	static void Reflect(const Vec3& ivec, const Vec3& nvec, Vec3& result);
	static Vec3 Reflect(const Vec3& ivec, const Vec3& nvec);
	static void Refract(const Vec3& ivec, const Vec3& nvec, float refractionIndex, Vec3& result);
	static Vec3 Refract(const Vec3& ivec, const Vec3& nvec, float refractionIndex);
	static void SmoothStep(const Vec3& v1, const Vec3& v2, float t, Vec3& result);
	static Vec3 SmoothStep(const Vec3& v1, const Vec3& v2, float t);
	static void Transform(const Vec3& v, const Quat& quat, Vec3& result);
	static Vec3 Transform(const Vec3& v, const Quat& quat);
	static void Transform(const Vec3& v, const Matrix& m, Vec3& result);
	static Vec3 Transform(const Vec3& v, const Matrix& m);
	static void Transform(const Vec3* varray, size_t count, const Matrix& m, Vec3* resultArray);
	static void Transform(const Vec3& v, const Matrix& m, Vec4& result);
	static void Transform(const Vec3* varray, size_t count, const Matrix& m, Vec4* resultArray);
	static void TransformNormal(const Vec3& v, const Matrix& m, Vec3& result);
	static Vec3 TransformNormal(const Vec3& v, const Matrix& m);
	static void TransformNormal(const Vec3* varray, size_t count, const Matrix& m, Vec3* resultArray);
	static Vec3 TransformZero(const Matrix& m);

	// Constants
	static const Vec3 Zero;
	static const Vec3 One;
	static const Vec3 UnitX;
	static const Vec3 UnitY;
	static const Vec3 UnitZ;
	static const Vec3 Up;
	static const Vec3 Down;
	static const Vec3 Right;
	static const Vec3 Left;
	static const Vec3 Forward;
	static const Vec3 Backward;
};

//-----------------------------------------------------------------------------
// 4D float point
//-----------------------------------------------------------------------------
struct Vec4 : XMFLOAT4
{
	Vec4();
	constexpr Vec4(float x, float y, float z, float w);
	Vec4(const Vec4& v);
	Vec4(const Vec3& v, float w);
	Vec4(FXMVECTOR v);
	explicit Vec4(const XMVECTORF32& v);

	operator XMVECTOR() const;
	explicit operator float*();
	explicit operator const float*() const;
	float& operator [](int index);
	const float& operator [](int index) const;

	// Comparison operators
	bool operator == (const Vec4& v) const;
	bool operator != (const Vec4& v) const;
	bool operator < (const Vec4& v) const;

	// Assignment operators
	Vec4& operator = (const Vec4& v);
	Vec4& operator = (const XMVECTORF32& v);
	Vec4& operator += (const Vec4& v);
	Vec4& operator -= (const Vec4& v);
	Vec4& operator *= (float s);
	Vec4& operator /= (float s);

	// Unary operators
	Vec4 operator+ () const;
	Vec4 operator- () const;

	// Binary operators
	Vec4 operator + (const Vec4& v) const;
	Vec4 operator - (const Vec4& v) const;
	Vec4 operator * (float s) const;
	Vec4 operator / (float s) const;
	friend Vec4 operator * (float s, const Vec4& v);

	// Methods
	void Clamp(const Vec4& vmin, const Vec4& vmax);
	void Clamp(const Vec4& vmin, const Vec4& vmax, Vec4& result) const;
	Vec4 Clamped(const Vec4& min = Vec4(0, 0, 0, 0), const Vec4& max = Vec4(1, 1, 1, 1)) const;
	void Cross(const Vec4& v1, const Vec4& v2, Vec4& result) const;
	Vec4 Cross(const Vec4& v1, const Vec4& v2) const;
	float Dot(const Vec4& V) const;
	float DotSelf() const;
	bool Equal(const Vec4& v) const;
	bool InBounds(const Vec4& Bounds) const;
	float Length() const;
	float LengthSquared() const;
	Vec4& Normalize();
	void Normalize(Vec4& result) const;
	Vec4 Normalized() const;

	// Static functions
	static void Barycentric(const Vec4& v1, const Vec4& v2, const Vec4& v3, float f, float g, Vec4& result);
	static Vec4 Barycentric(const Vec4& v1, const Vec4& v2, const Vec4& v3, float f, float g);
	static void CatmullRom(const Vec4& v1, const Vec4& v2, const Vec4& v3, const Vec4& v4, float t, Vec4& result);
	static Vec4 CatmullRom(const Vec4& v1, const Vec4& v2, const Vec4& v3, const Vec4& v4, float t);
	static float Distance(const Vec4& v1, const Vec4& v2);
	static float DistanceSquared(const Vec4& v1, const Vec4& v2);
	static void Hermite(const Vec4& v1, const Vec4& t1, const Vec4& v2, const Vec4& t2, float t, Vec4& result);
	static Vec4 Hermite(const Vec4& v1, const Vec4& t1, const Vec4& v2, const Vec4& t2, float t);
	static void Lerp(const Vec4& v1, const Vec4& v2, float t, Vec4& result);
	static Vec4 Lerp(const Vec4& v1, const Vec4& v2, float t);
	static void Max(const Vec4& v1, const Vec4& v2, Vec4& result);
	static Vec4 Max(const Vec4& v1, const Vec4& v2);
	static void Min(const Vec4& v1, const Vec4& v2, Vec4& result);
	static Vec4 Min(const Vec4& v1, const Vec4& v2);
	static void Reflect(const Vec4& ivec, const Vec4& nvec, Vec4& result);
	static Vec4 Reflect(const Vec4& ivec, const Vec4& nvec);
	static void Refract(const Vec4& ivec, const Vec4& nvec, float refractionIndex, Vec4& result);
	static Vec4 Refract(const Vec4& ivec, const Vec4& nvec, float refractionIndex);
	static void SmoothStep(const Vec4& v1, const Vec4& v2, float t, Vec4& result);
	static Vec4 SmoothStep(const Vec4& v1, const Vec4& v2, float t);
	static void Transform(const Vec2& v, const Quat& quat, Vec4& result);
	static Vec4 Transform(const Vec2& v, const Quat& quat);
	static void Transform(const Vec3& v, const Quat& quat, Vec4& result);
	static Vec4 Transform(const Vec3& v, const Quat& quat);
	static void Transform(const Vec4& v, const Quat& quat, Vec4& result);
	static Vec4 Transform(const Vec4& v, const Quat& quat);
	static void Transform(const Vec4& v, const Matrix& m, Vec4& result);
	static Vec4 Transform(const Vec4& v, const Matrix& m);
	static void Transform(const Vec4* varray, size_t count, const Matrix& m, Vec4* resultArray);

	// Constants
	static const Vec4 Zero;
	static const Vec4 One;
	static const Vec4 UnitX;
	static const Vec4 UnitY;
	static const Vec4 UnitZ;
	static const Vec4 UnitW;
};

//-----------------------------------------------------------------------------
// 2d box using floats
//-----------------------------------------------------------------------------
struct Box2d
{
	Vec2 v1, v2;

	Box2d();
	Box2d(float minx, float miny, float maxx, float maxy);
	Box2d(const Vec2& v1, const Vec2& v2);
	Box2d(const Box2d& box);
	Box2d(float x, float y);
	Box2d(const Box2d& box, float margin);
	explicit Box2d(const Vec2& v);
	explicit Box2d(const Rect& r);

	// Comparison operators
	bool operator == (const Box2d& b) const;
	bool operator != (const Box2d& b) const;

	// Assignment operators
	Box2d& operator = (const Box2d& b);
	Box2d& operator += (const Vec2& v);
	Box2d& operator -= (const Vec2& v);
	Box2d& operator *= (float f);
	Box2d& operator /= (float f);

	// Unary operators
	Box2d operator + () const;
	Box2d operator - () const;

	// Binary operators
	Box2d operator + (const Vec2& v) const;
	Box2d operator - (const Vec2& v) const;
	Box2d operator * (float f) const;
	Box2d operator / (float f) const;
	Box2d operator / (const Vec2& v) const;
	friend Box2d operator * (float f, const Box2d& v);

	static Box2d Create(const Int2& pos, const Int2& size)
	{
		Box2d box;
		box.Set(pos, size);
		return box;
	}
	static Box2d Create(const Vec2& pos, const Vec2& size)
	{
		return Box2d(pos.x, pos.y, pos.x + size.x, pos.y + size.y);
	}

	// Methods
	void AddMargin(float margin)
	{
		v1.x += margin;
		v1.y += margin;
		v2.x -= margin;
		v2.y -= margin;
	}
	Vec2 GetRandomPoint() const;
	Vec2 GetRandomPoint(float offset) const;
	Box2d Intersect(const Box2d& b) const;
	bool IsInside(const Vec2& v) const;
	bool IsInside(const Vec3& v) const;
	bool IsInside(const Int2& p) const;
	bool IsFullyInside(const Vec2& v, float r) const;
	bool IsValid() const;
	Vec2 Midpoint() const;
	Vec2 Size() const;
	float SizeX() const;
	float SizeY() const;

	void Set(const Int2& pos, const Int2& size)
	{
		v1.x = (float)pos.x;
		v1.y = (float)pos.y;
		v2.x = v1.x + (float)size.x;
		v2.y = v1.y + (float)size.y;
	}

	void Move(const Vec2& pos)
	{
		Vec2 dif = pos - v1;
		v1 += dif;
		v2 += dif;
	}

	Vec3 GetRandomPos3(float y = 0.f) const
	{
		return Vec3(::Random(v1.x, v2.x), y, ::Random(v1.y, v2.y));
	}

	Vec2 LeftTop() const
	{
		return v1;
	}
	Vec2 RightBottom() const
	{
		return v2;
	}
	Vec2 RightTop() const
	{
		return Vec2(v2.x, v1.y);
	}
	Vec2 LeftBottom() const
	{
		return Vec2(v1.x, v2.y);
	}

	Vec3 LeftTop3() const
	{
		return Vec3(v1.x, v1.y, 0);
	}
	Vec3 RightTop3() const
	{
		return Vec3(v2.x, v1.y, 0);
	}
	Vec3 LeftBottom3() const
	{
		return Vec3(v1.x, v2.y, 0);
	}
	Vec3 RightBottom3() const
	{
		return Vec3(v2.x, v2.y, 0);
	}

	Box2d LeftBottomPart() const
	{
		return Box2d(Left(), MidY(), MidX(), Bottom());
	}
	Box2d RightBottomPart() const
	{
		return Box2d(MidX(), MidY(), Right(), Bottom());
	}
	Box2d LeftTopPart() const
	{
		return Box2d(Left(), Top(), MidX(), MidY());
	}
	Box2d RightTopPart() const
	{
		return Box2d(MidX(), Top(), Right(), MidY());
	}

	void ToRectangle(float& x, float& y, float& w, float& h) const
	{
		x = v1.x + (v2.x - v1.x) / 2;
		y = v1.y + (v2.y - v1.y) / 2;
		w = (v2.x - v1.x) / 2;
		h = (v2.y - v1.y) / 2;
	}
	Box ToBoxXZ(float y1, float y2) const;

	float& Left() { return v1.x; }
	float& Right() { return v2.x; }
	float& Top() { return v1.y; }
	float& Bottom() { return v2.y; }

	float Left() const { return v1.x; }
	float Right() const { return v2.x; }
	float Top() const { return v1.y; }
	float Bottom() const { return v2.y; }
	float MidX() const { return (v2.x - v1.x) / 2 + v1.x; }
	float MidY() const { return (v2.y - v1.y) / 2 + v1.y; }

	// Static functions
	static bool Intersect(const Box2d& a, const Box2d& b, Box2d& result);
	static Box2d Intersect(const Box2d& a, const Box2d& b);

	// Constants
	static const Box2d Zero;
	static const Box2d Unit;
};

//-----------------------------------------------------------------------------
// 3d float box
//-----------------------------------------------------------------------------
struct Box
{
	Vec3 v1, v2;

	Box();
	Box(float minx, float miny, float minz, float maxx, float maxy, float maxz);
	Box(const Vec3& v1, const Vec3& v2);
	Box(const Box& box);
	Box(float x, float y, float z);
	explicit Box(const Vec3& v);

	// Comparison operators
	bool operator == (const Box& b) const;
	bool operator != (const Box& b) const;

	// Assignment operators
	Box& operator = (const Box& b);
	Box& operator += (const Vec3& v);
	Box& operator -= (const Vec3& v);
	Box& operator *= (float f);
	Box& operator /= (float f);

	// Unary operators
	Box operator + () const;
	Box operator - () const;

	// Binary operators
	Box operator + (const Vec3& v) const;
	Box operator - (const Vec3& v) const;
	Box operator * (float f) const;
	Box operator / (float f) const;
	friend Box operator * (float f, const Box& b);

	// Methods
	void AddPoint(const Vec3& v);
	Vec3 GetRandomPoint() const;
	bool IsInside(const Vec3& v) const;
	bool IsValid() const;
	Vec3 Midpoint() const;
	Vec3 Size() const;
	float SizeX() const;
	Vec2 SizeXZ() const;
	float SizeY() const;
	float SizeZ() const;

	// Static functions
	static Box Create(const Vec3& pos, const Vec3& size);
	static Box CreateXZ(const Box2d& box2d, float y1, float y2);
};

//-----------------------------------------------------------------------------
// 4x4 float matrix
//-----------------------------------------------------------------------------
struct Matrix : XMFLOAT4X4
{
	Matrix();
	Matrix(float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23,
		float m30, float m31, float m32, float m33);
	Matrix(const Vec3& v1, const Vec3& v2, const Vec3& v3);
	Matrix(const Vec4& v1, const Vec4& v2, const Vec4& v3, const Vec4& v4);
	Matrix(const Matrix& m);
	Matrix(CXMMATRIX m);

	operator XMMATRIX() const;

	// Comparison operators
	bool operator == (const Matrix& m) const;
	bool operator != (const Matrix& m) const;

	// Assignment operators
	Matrix& operator = (const Matrix& m);
	Matrix& operator += (const Matrix& m);
	Matrix& operator -= (const Matrix& m);
	Matrix& operator *= (const Matrix& m);
	Matrix& operator *= (float s);
	Matrix& operator /= (float s);
	Matrix& operator /= (const Matrix& m);

	// Unary operators
	Matrix operator+ () const;
	Matrix operator- () const;

	// Binary operators
	Matrix operator+ (const Matrix& m) const;
	Matrix operator- (const Matrix& m) const;
	Matrix operator* (const Matrix& m) const;
	Matrix operator* (float S) const;
	Matrix operator/ (float S) const;
	Matrix operator/ (const Matrix& m) const;
	friend Matrix operator * (float s, const Matrix& m);

	// Methods
	bool Decompose(Vec3& scale, Quat& rotation, Vec3& translation);
	float Determinant() const;
	float GetYaw() const;
	void Identity();
	Matrix Inverse() const;
	void Inverse(Matrix& result) const;
	Matrix Transpose() const;
	void Transpose(Matrix& result) const;

	// Static functions
	static Matrix CreateBillboard(const Vec3& object, const Vec3& cameraPosition, const Vec3& cameraUp, const Vec3* cameraForward = nullptr);
	static Matrix CreateConstrainedBillboard(const Vec3& object, const Vec3& cameraPosition, const Vec3& rotateAxis,
		const Vec3* cameraForward = nullptr, const Vec3* objectForward = nullptr);
	static Matrix CreateFromAxes(const Vec3& axisX, const Vec3& axisY, const Vec3& axisZ);
	static Matrix CreateFromAxisAngle(const Vec3& axis, float angle);
	static Matrix CreateLookAt(const Vec3& position, const Vec3& target, const Vec3& up = Vec3(0, 1, 0));
	static Matrix CreateOrthographic(float width, float height, float zNearPlane, float zFarPlane);
	static Matrix CreateOrthographicOffCenter(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane);
	static Matrix CreatePerspective(float width, float height, float nearPlane, float farPlane);
	static Matrix CreatePerspectiveFieldOfView(float fov, float aspectRatio, float nearPlane, float farPlane);
	static Matrix CreatePerspectiveOffCenter(float left, float right, float bottom, float top, float nearPlane, float farPlane);
	static Matrix CreateReflection(const Plane& plane);
	static Matrix CreateShadow(const Vec3& lightDir, const Plane& plane);
	static Matrix CreateWorld(const Vec3& position, const Vec3& forward, const Vec3& up);
	static void Lerp(const Matrix& M1, const Matrix& M2, float t, Matrix& result);
	static Matrix Lerp(const Matrix& M1, const Matrix& M2, float t);
	static Matrix Rotation(float yaw, float pitch, float roll);
	static Matrix Rotation(const Vec3& v);
	static Matrix Rotation(const Quat& quat);
	static Matrix RotationX(float radians);
	static Matrix RotationY(float radians);
	static Matrix RotationZ(float radians);
	static Matrix Scale(const Vec3& scales);
	static Matrix Scale(float xs, float ys, float zs);
	static Matrix Scale(float scale);
	static void Transform(const Matrix& M, const Quat& rotation, Matrix& result);
	static Matrix Transform(const Matrix& M, const Quat& rotation);
	static Matrix Transform2D(const Vec2* scaling_center, float scaling_rotation, const Vec2* scaling, const Vec2* rotation_center, float rotation, const Vec2* translation);
	static Matrix Translation(const Vec3& position);
	static Matrix Translation(float x, float y, float z);

	// Constants
	static const Matrix IdentityMatrix;
};

//-----------------------------------------------------------------------------
// Quaternion
//-----------------------------------------------------------------------------
struct Quat : public XMFLOAT4
{
	Quat();
	Quat(float x, float y, float z, float w);
	Quat(const Vec3& v, float w);
	Quat(const Quat& q);
	Quat(FXMVECTOR v);
	explicit Quat(const Vec4& v);
	explicit Quat(const XMVECTORF32& v);

	operator XMVECTOR() const;

	// Comparison operators
	bool operator == (const Quat& q) const;
	bool operator != (const Quat& q) const;

	// Assignment operators
	Quat& operator = (const Quat& q);
	Quat& operator = (const XMVECTORF32& v);
	Quat& operator += (const Quat& q);
	Quat& operator -= (const Quat& q);
	Quat& operator *= (const Quat& q);
	Quat& operator *= (float s);
	Quat& operator /= (const Quat& q);

	// Unary operators
	Quat operator + () const;
	Quat operator - () const;

	// Binary operators
	Quat operator + (const Quat& q) const;
	Quat operator - (const Quat& q) const;
	Quat operator * (const Quat& q) const;
	Quat operator * (float s) const;
	Quat operator / (const Quat& q) const;
	friend Quat operator * (float s, const Quat& q);

	// Mathods
	void Conjugate();
	void Conjugate(Quat& result) const;
	float Dot(const Quat& q) const;
	void Inverse(Quat& result) const;
	float Length() const;
	float LengthSquared() const;
	void Normalize();
	void Normalize(Quat& result) const;

	// Static functions
	static void Concatenate(const Quat& q1, const Quat& q2, Quat& result);
	static Quat Concatenate(const Quat& q1, const Quat& q2);
	static Quat CreateFromAxisAngle(const Vec3& axis, float angle);
	static Quat CreateFromRotationMatrix(const Matrix& M);
	static Quat CreateFromYawPitchRoll(float yaw, float pitch, float roll);
	static void Lerp(const Quat& q1, const Quat& q2, float t, Quat& result);
	static Quat Lerp(const Quat& q1, const Quat& q2, float t);
	static void Slerp(const Quat& q1, const Quat& q2, float t, Quat& result);
	static Quat Slerp(const Quat& q1, const Quat& q2, float t);

	// Constants
	static const Quat Identity;
};

//-----------------------------------------------------------------------------
// Plane
//-----------------------------------------------------------------------------
struct Plane : public XMFLOAT4
{
	Plane();
	Plane(float x, float y, float z, float w);
	Plane(const Vec3& normal, float d);
	Plane(const Vec3& point1, const Vec3& point2, const Vec3& point3);
	Plane(const Vec3& point, const Vec3& normal);
	Plane(FXMVECTOR v);
	explicit Plane(const Vec4& v);
	explicit Plane(const XMVECTORF32& v);

	operator XMVECTOR() const;

	// Comparison operators
	bool operator == (const Plane& p) const;
	bool operator != (const Plane& p) const;

	// Assignment operators
	Plane& operator = (const Plane& p);

	// Methods
	float Dot(const Vec4& v) const;
	float DotCoordinate(const Vec3& position) const;
	float DotNormal(const Vec3& normal) const;
	void Normalize();
	void Normalize(Plane& result) const;

	// Static functions
	static bool Intersect3Planes(const Plane& p1, const Plane& p2, const Plane& p3, Vec3& result);
	static void Transform(const Plane& plane, const Matrix& M, Plane& result);
	static Plane Transform(const Plane& plane, const Matrix& M);
	static void Transform(const Plane& plane, const Quat& rotation, Plane& result);
	static Plane Transform(const Plane& plane, const Quat& rotation);
};

//-----------------------------------------------------------------------------
// Frustrum planes to check objects visible from camera
//-----------------------------------------------------------------------------
struct FrustumPlanes
{
	Plane planes[6];

	FrustumPlanes() {}
	explicit FrustumPlanes(const Matrix& worldViewProj) { Set(worldViewProj); }
	void Set(const Matrix& worldViewProj);

	// Return points on edge of frustum
	void GetPoints(Vec3* points) const;
	static void GetPoints(const Matrix& worldViewProj, Vec3* points);
	// Checks if point is inside frustum
	bool PointInFrustum(const Vec3 &p) const;
	// Checks if box collide with frustum
	// In rare cases can return true even if it's outside!
	bool BoxToFrustum(const Box& box) const;
	bool BoxToFrustum(const Box2d& box) const;
	// Checks if box is fully inside frustum
	bool BoxInFrustum(const Box& box) const;
	// Checks if sphere collide with frustum
	// In rare cases can return true even if it's outside!
	bool SphereToFrustum(const Vec3& sphere_center, float sphere_radius) const;
	// Checks if sphere is fully inside frustum
	bool SphereInFrustum(const Vec3& sphere_center, float sphere_radius) const;
};

//-----------------------------------------------------------------------------
struct Pixel
{
	Int2 pt;
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
struct Obbox
{
	Vec3 pos;
	Vec3 size;
	Matrix rot;
};
// inna wersja, oka¿e siê czy lepszy algorytm
struct Oob
{
	Vec3 c; // œrodek
	Vec3 u[3]; // obrót po X,Y,Z
	Vec3 e; // po³owa rozmiaru
};

//-----------------------------------------------------------------------------
// POD types
//-----------------------------------------------------------------------------
namespace POD
{
	struct Uint2
	{
		uint x, y;
	};
	struct Vec3
	{
		float x, y, z;
	};
}

//-----------------------------------------------------------------------------
// Collisions and functions
//-----------------------------------------------------------------------------
bool RayToBox(const Vec3& ray_pos, const Vec3& ray_dir, const Box& box, float* out_t);
bool RayToPlane(const Vec3& ray_pos, const Vec3& ray_dir, const Plane& plane, float* out_t);
bool RayToSphere(const Vec3& ray_pos, const Vec3& ray_dir, const Vec3& center, float radius, float& dist);
bool RayToTriangle(const Vec3& ray_pos, const Vec3& ray_dir, const Vec3& v1, const Vec3& v2, const Vec3& v3, float& dist);
bool RectangleToRectangle(float x1, float y1, float x2, float y2, float a1, float b1, float a2, float b2);
bool CircleToRectangle(float circlex, float circley, float radius, float rectx, float recty, float w, float h);
bool LineToLine(const Vec2& start1, const Vec2& end1, const Vec2& start2, const Vec2& end2, float* t = nullptr);
bool LineToRectangle(const Vec2& start, const Vec2& end, const Vec2& rect_pos, const Vec2& rect_pos2, float* t = nullptr);
inline bool LineToRectangle(const Vec3& start, const Vec3& end, const Vec2& rect_pos, const Vec2& rect_pos2, float* t = nullptr)
{
	return LineToRectangle(Vec2(start.x, start.z), Vec2(end.x, end.z), rect_pos, rect_pos2, t);
}
inline bool LineToRectangleSize(const Vec2& start, const Vec2& end, const Vec2& rect_pos, const Vec2& rect_size, float* t = nullptr)
{
	return LineToRectangle(start, end, rect_pos - rect_size, rect_pos + rect_size, t);
}
bool BoxToBox(const Box& box1, const Box& box2);
bool OrientedBoxToOrientedBox(const Obbox& obox1, const Obbox& obox2, Vec3* contact);
bool CircleToRotatedRectangle(float cx, float cy, float radius, float rx, float ry, float w, float h, float rot);
struct RotRect
{
	Vec2 center, size;
	float rot;
};
bool RotatedRectanglesCollision(const RotRect& r1, const RotRect& r2);
inline bool CircleToCircle(float cx1, float cy1, float r1, float cx2, float cy2, float r2)
{
	float r = (r1 + r2);
	return DistanceSqrt(cx1, cy1, cx2, cy2) < r * r;
}
bool SphereToBox(const Vec3& pos, float radius, const Box& box);
int RayToCylinder(const Vec3& ray_A, const Vec3& ray_B, const Vec3& cylinder_P, const Vec3& cylinder_Q, float radius, float& t);
bool OOBToOOB(const Oob& a, const Oob& b);
float DistanceRectangleToPoint(const Vec2& pos, const Vec2& size, const Vec2& pt);
float PointLineDistance(float x0, float y0, float x1, float y1, float x2, float y2);
float GetClosestPointOnLineSegment(const Vec2& A, const Vec2& B, const Vec2& P, Vec2& result);
inline float ClosestPointOnLine(const Vec3& p, const Vec3& ray_pos, const Vec3& ray_dir)
{
	return ray_dir.Dot(p - ray_pos);
}

//-----------------------------------------------------------------------------
#include "CoreMath.inl"
