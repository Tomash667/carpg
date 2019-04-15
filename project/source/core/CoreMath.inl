#pragma once

//*************************************************************************************************
//
// 2D int point
//
//*************************************************************************************************
inline Int2::Int2()
{
}

inline Int2::Int2(int x, int y) : x(x), y(y)
{
}

inline Int2::Int2(const Int2& i) : x(i.x), y(i.y)
{
}

template<typename T, typename T2>
inline Int2::Int2(T x, T2 y) : x((int)x), y((int)y)
{
}

inline Int2::Int2(int xy) : x(xy), y(xy)
{
}

inline Int2::Int2(const Vec2& v) : x(int(v.x)), y(int(v.y))
{
}

inline Int2::Int2(const Vec3& v) : x(int(v.x)), y(int(v.z))
{
}

inline bool Int2::operator == (const Int2& i) const
{
	return (x == i.x && y == i.y);
}

inline bool Int2::operator != (const Int2& i) const
{
	return (x != i.x || y != i.y);
}

inline bool Int2::operator > (const Int2& i) const
{
	return x > i.x || y > i.y;
}

inline Int2& Int2::operator = (const Int2& i)
{
	x = i.x;
	y = i.y;
	return *this;
}

inline void Int2::operator += (const Int2& i)
{
	x += i.x;
	y += i.y;
}

inline void Int2::operator -= (const Int2& i)
{
	x -= i.x;
	y -= i.y;
}

inline void Int2::operator *= (int a)
{
	x *= a;
	y *= a;
}

inline void Int2::operator /= (int a)
{
	x /= a;
	y /= a;
}

inline Int2 Int2::operator + () const
{
	return *this;
}

inline Int2 Int2::operator - () const
{
	return Int2(-x, -y);
}

inline int Int2::operator ()(int shift) const
{
	return x + y * shift;
}

inline Int2 Int2::operator + (const Int2& xy) const
{
	return Int2(x + xy.x, y + xy.y);
}

inline Int2 Int2::operator - (const Int2& xy) const
{
	return Int2(x - xy.x, y - xy.y);
}

inline Int2 Int2::operator * (const Vec2& scale) const
{
	return Int2(float(x) * scale.x, float(y) * scale.y);
}

inline Int2 Int2::operator * (int a) const
{
	return Int2(x*a, y*a);
}

inline Int2 Int2::operator * (float a) const
{
	return Int2(int(a*x), int(a*y));
}

inline Int2 Int2::operator / (int a) const
{
	return Int2(x / a, y / a);
}

inline Int2 operator * (int a, const Int2& i)
{
	return i * a;
}

//------------------------------------------------------------------------------
// Methods
//------------------------------------------------------------------------------

inline int Int2::Clamp(int d) const
{
	if(d > y)
		return y;
	else if(d < x)
		return x;
	else
		return d;
}

inline int Int2::Lerp(float t) const
{
	return int(t*(y - x)) + x;
}

inline int Int2::Random() const
{
	return ::Random(x, y);
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline int Int2::Distance(const Int2& i1, const Int2& i2)
{
	return abs(i1.x - i2.x) + abs(i1.y - i2.y);
}

inline Int2 Int2::Lerp(const Int2& i1, const Int2& i2, float t)
{
	return Int2((int)::Lerp(float(i1.x), float(i2.x), t), (int)::Lerp(float(i1.y), float(i2.y), t));
}

inline Int2 Int2::Max(const Int2& i1, const Int2& i2)
{
	return Int2(max(i1.x, i2.x), max(i1.y, i2.y));
}

inline Int2 Int2::Min(const Int2& i1, const Int2& i2)
{
	return Int2(min(i1.x, i2.x), min(i1.y, i2.y));
}

inline void Int2::MinMax(Int2& i1, Int2& i2)
{
	if(i1.x > i2.x)
		std::swap(i1.x, i2.x);
	if(i1.y > i2.y)
		std::swap(i1.y, i2.y);
}

inline void Int2::MinMax(const Int2& i1, const Int2& i2, Int2& min, Int2& max)
{
	if(i1.x < i2.x)
	{
		min.x = i1.x;
		max.x = i2.x;
	}
	else
	{
		min.x = i2.x;
		max.x = i1.x;
	}
	if(i1.y < i2.y)
	{
		min.y = i1.y;
		max.y = i2.y;
	}
	else
	{
		min.y = i2.y;
		max.y = i1.y;
	}
}

inline Int2 Int2::Random(const Int2& i1, const Int2& i2)
{
	return Int2(::Random(i1.x, i2.x), ::Random(i1.y, i2.y));
}

//*************************************************************************************************
//
// Rectangle using int
//
//*************************************************************************************************
inline Rect::Rect()
{
}

inline Rect::Rect(int x, int y) : p1(x, y), p2(x, y)
{
}

inline Rect::Rect(int x1, int y1, int x2, int y2) : p1(x1, y1), p2(x2, y2)
{
}

inline Rect::Rect(const Int2& p1, const Int2& p2) : p1(p1), p2(p2)
{
}

inline Rect::Rect(const Rect& box) : p1(box.p1), p2(box.p2)
{
}

inline Rect::Rect(const Box2d& box) : p1(box.v1), p2(box.v2)
{
}

inline Rect::Rect(const Box2d& box, const Int2& pad) : p1((int)box.v1.x + pad.x, (int)box.v1.y + pad.y), p2((int)box.v2.x - pad.x, (int)box.v2.y - pad.y)
{
}

inline bool Rect::operator == (const Rect& r) const
{
	return p1 == r.p1 && p2 == r.p2;
}

inline bool Rect::operator != (const Rect& r) const
{
	return p1 != r.p1 || p2 != r.p2;
}

inline Rect& Rect::operator = (const Rect& r)
{
	p1 = r.p1;
	p2 = r.p2;
	return *this;
}

inline Rect& Rect::operator += (const Int2& p)
{
	p1 += p;
	p2 += p;
	return *this;
}

inline Rect& Rect::operator -= (const Int2& p)
{
	p1 -= p;
	p2 -= p;
	return *this;
}

inline Rect& Rect::operator *= (int d)
{
	p1 *= d;
	p2 *= d;
	return *this;
}

inline Rect& Rect::operator /= (int d)
{
	p1 /= d;
	p2 /= d;
	return *this;
}

inline Rect Rect::operator + () const
{
	return *this;
}

inline Rect Rect::operator - () const
{
	return Rect(-p1, -p2);
}

inline Rect Rect::operator + (const Int2& p) const
{
	return Rect(p1 + p, p2 + p);
}

inline Rect Rect::operator - (const Int2& p) const
{
	return Rect(p1 - p, p2 - p);
}

inline Rect Rect::operator * (int d) const
{
	return Rect(p1 * d, p2 * d);
}

inline Rect Rect::operator * (const Vec2& v) const
{
	return Rect(int(v.x * p1.x), int(v.y * p1.y), int(v.x * p2.x), int(v.y * p2.y));
}

inline Rect Rect::operator / (int d) const
{
	return Rect(p1 / d, p2 / d);
}

inline Rect operator * (int d, const Rect& r)
{
	return r * d;
}

//------------------------------------------------------------------------------
// Methods
//------------------------------------------------------------------------------

inline bool Rect::IsInside(const Int2& pt) const
{
	return pt.x >= p1.x && pt.x <= p2.x && pt.y >= p1.y && pt.y <= p2.y;
}

inline Rect Rect::LeftBottomPart() const
{
	return Rect(Left(), MidY(), MidX(), Bottom());
}

inline Rect Rect::LeftTopPart() const
{
	return Rect(Left(), Top(), MidX(), MidY());
}

inline Int2 Rect::Random() const
{
	return Int2(::Random(p1.x, p2.x), ::Random(p1.y, p2.y));
}

inline void Rect::Resize(const Rect& r)
{
	if(r.p1.x < p1.x)
		p1.x = r.p1.x;
	if(r.p2.x > p2.x)
		p2.x = r.p2.x;
	if(r.p1.y < p1.y)
		p1.y = r.p1.y;
	if(r.p2.y > p2.y)
		p2.y = r.p2.y;
}

inline Rect Rect::RightBottomPart() const
{
	return Rect(MidX(), MidY(), Right(), Bottom());
}

inline Rect Rect::RightTopPart() const
{
	return Rect(MidX(), Top(), Right(), MidY());
}

inline void Rect::Set(const Int2& pos, const Int2& size)
{
	p1 = pos;
	p2 = pos + size;
}

inline void Rect::Set(int x1, int y1, int x2, int y2)
{
	p1.x = x1;
	p1.y = y1;
	p2.x = x2;
	p2.y = y2;
}

inline Int2 Rect::Size() const
{
	return Int2(p2.x - p1.x, p2.y - p1.y);
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline Rect Rect::Create(const Int2& pos, const Int2& size)
{
	Rect box;
	box.Set(pos, size);
	return box;
}

inline Rect Rect::Create(const Int2& pos, const Int2& size, int pad)
{
	return Rect(pos.x + pad, pos.y + pad, pos.x + size.x - pad, pos.y + size.y - pad);
}

inline Rect Rect::Intersect(const Rect& r1, const Rect& r2)
{
	Rect result;
	if(!Intersect(r1, r2, result))
		result = Rect(0, 0, 0, 0);
	return result;
}

inline bool Rect::Intersect(const Rect& r1, const Rect& r2, Rect& result)
{
	int x = max(r1.Left(), r1.Left());
	int num1 = min(r1.Right(), r2.Right());
	int y = max(r1.Top(), r2.Top());
	int num2 = min(r1.Bottom(), r2.Bottom());
	if(num1 >= x && num2 >= y)
	{
		result = Rect(x, y, num1, num2);
		return true;
	}
	else
		return false;
}

inline bool Rect::IsInside(const Int2& pos, const Int2& size, const Int2& pt)
{
	return pt.x >= pos.x
		&& pt.y >= pos.y
		&& pt.x <= pos.x + size.x
		&& pt.y <= pos.y + size.y;
}

//*************************************************************************************************
//
// 2D float point
//
//*************************************************************************************************
inline Vec2::Vec2()
{
}

inline Vec2::Vec2(float x, float y) : XMFLOAT2(x, y)
{
}

inline Vec2::Vec2(const Vec2& v) : XMFLOAT2(v.x, v.y)
{
}

inline Vec2::Vec2(FXMVECTOR v)
{
	XMStoreFloat2(this, v);
}

inline Vec2::Vec2(float xy) : XMFLOAT2(xy, xy)
{
}

inline Vec2::Vec2(const Int2& i) : XMFLOAT2(float(i.x), float(i.y))
{
}

inline Vec2::Vec2(const XMVECTORF32& v) : XMFLOAT2(v.f[0], v.f[1])
{
}

inline Vec2::operator XMVECTOR() const
{
	return XMLoadFloat2(this);
}

inline Vec2::operator float*()
{
	return &x;
}

inline Vec2::operator const float*() const
{
	return &x;
}

inline float& Vec2::operator [](int index)
{
	return ((float*)*this)[index];
}

inline const float& Vec2::operator [](int index) const
{
	return ((const float*)*this)[index];
}

inline bool Vec2::operator == (const Vec2& v) const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR v2 = XMLoadFloat2(&v);
	return XMVector2Equal(v1, v2);
}

inline bool Vec2::operator != (const Vec2& v) const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR v2 = XMLoadFloat2(&v);
	return XMVector2NotEqual(v1, v2);
}

inline bool Vec2::operator < (const Vec2& v) const
{
	if(x == v.x)
	{
		if(y == v.y)
			return false;
		else
			return y < v.y;
	}
	else
		return x < v.x;
}

inline Vec2& Vec2::operator = (const Vec2& v)
{
	x = v.x;
	y = v.y;
	return *this;
}

inline Vec2& Vec2::operator = (const XMVECTORF32& v)
{
	x = v.f[0];
	y = v.f[1];
	return *this;
}

inline Vec2& Vec2::operator += (const Vec2& v)
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR v2 = XMLoadFloat2(&v);
	XMVECTOR r = XMVectorAdd(v1, v2);
	XMStoreFloat2(this, r);
	return *this;
}

inline Vec2& Vec2::operator -= (const Vec2& v)
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR v2 = XMLoadFloat2(&v);
	XMVECTOR r = XMVectorSubtract(v1, v2);
	XMStoreFloat2(this, r);
	return *this;
}

inline Vec2& Vec2::operator *= (float s)
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR r = XMVectorScale(v1, s);
	XMStoreFloat2(this, r);
	return *this;
}

inline Vec2& Vec2::operator /= (float s)
{
	assert(s != 0.f);
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR r = XMVectorScale(v1, 1.f / s);
	XMStoreFloat2(this, r);
	return *this;
}

inline Vec2 Vec2::operator + () const
{
	return *this;
}

inline Vec2 Vec2::operator - () const
{
	return Vec2(-x, -y);
}

inline Vec2 Vec2::operator + (const Vec2& v) const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR v2 = XMLoadFloat2(&v);
	XMVECTOR r = XMVectorAdd(v1, v2);
	Vec2 result;
	XMStoreFloat2(&result, r);
	return result;
}

inline Vec2 Vec2::operator - (const Vec2& v) const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR v2 = XMLoadFloat2(&v);
	XMVECTOR r = XMVectorSubtract(v1, v2);
	Vec2 result;
	XMStoreFloat2(&result, r);
	return result;
}

inline Vec2 Vec2::operator * (const Vec2& v) const
{
	return Vec2(x * v.x, y * v.y);
}

inline Vec2 Vec2::operator * (float s) const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR r = XMVectorScale(v1, s);
	Vec2 result;
	XMStoreFloat2(&result, r);
	return result;
}

inline Vec2 Vec2::operator / (float s) const
{
	assert(s != 0.f);
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR r = XMVectorScale(v1, 1.f / s);
	Vec2 result;
	XMStoreFloat2(&result, r);
	return result;
}

inline Vec2 Vec2::operator / (const Int2& i) const
{
	return Vec2(x / i.x, y / i.y);
}

inline Vec2 operator * (float s, const Vec2& v)
{
	XMVECTOR v1 = XMLoadFloat2(&v);
	XMVECTOR r = XMVectorScale(v1, s);
	Vec2 result;
	XMStoreFloat2(&result, r);
	return result;
}

//------------------------------------------------------------------------------
// Methods
//------------------------------------------------------------------------------

inline float Vec2::Clamp(float f) const
{
	if(f > y)
		return y;
	else if(f < x)
		return x;
	else
		return f;
}

inline void Vec2::Clamp(const Vec2& min, const Vec2& max)
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR v2 = XMLoadFloat2(&min);
	XMVECTOR v3 = XMLoadFloat2(&max);
	XMVECTOR r = XMVectorClamp(v1, v2, v3);
	XMStoreFloat2(this, r);
}

inline void Vec2::Clamp(const Vec2& min, const Vec2& max, Vec2& result) const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR v2 = XMLoadFloat2(&min);
	XMVECTOR v3 = XMLoadFloat2(&max);
	XMVECTOR r = XMVectorClamp(v1, v2, v3);
	XMStoreFloat2(&result, r);
}

inline Vec2 Vec2::Clamped(const Vec2& min, const Vec2& max) const
{
	Vec2 result;
	Clamp(min, max, result);
	return result;
}

inline Vec2 Vec2::Clip() const
{
	return Vec2(::Clip(x), ::Clip(y));
}

inline void Vec2::Cross(const Vec2& v, Vec2& result) const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR v2 = XMLoadFloat2(&v);
	XMVECTOR r = XMVector2Cross(v1, v2);
	XMStoreFloat2(&result, r);
}

inline Vec2 Vec2::Cross(const Vec2& v) const
{
	Vec2 result;
	Cross(v, result);
	return result;
}

inline float Vec2::Dot(const Vec2& v) const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR v2 = XMLoadFloat2(&v);
	XMVECTOR r = XMVector2Dot(v1, v2);
	return XMVectorGetX(r);
}

inline float Vec2::DotSelf() const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR X = XMVector2Dot(v1, v1);
	return XMVectorGetX(X);
}

inline bool Vec2::Equal(const Vec2& v) const
{
	return ::Equal(x, v.x) && ::Equal(y, v.y);
}

inline bool Vec2::InBounds(const Vec2& bounds) const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR v2 = XMLoadFloat2(&bounds);
	return XMVector2InBounds(v1, v2);
}

inline float Vec2::Length() const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR r = XMVector2Length(v1);
	return XMVectorGetX(r);
}

inline float Vec2::LengthSquared() const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR r = XMVector2LengthSq(v1);
	return XMVectorGetX(r);
}

inline Vec2& Vec2::Normalize()
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR r = XMVector2Normalize(v1);
	XMStoreFloat2(this, r);
	return *this;
}

inline void Vec2::Normalize(Vec2& v) const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR r = XMVector2Normalize(v1);
	XMStoreFloat2(&v, r);
}

inline Vec2 Vec2::Normalized() const
{
	Vec2 result;
	Normalize(result);
	return result;
}

inline float Vec2::Random() const
{
	return ::Random(x, y);
}


inline Vec3 Vec2::XY(float _z) const
{
	return Vec3(x, y, _z);
}

inline Vec3 Vec2::XZ(float _y) const
{
	return Vec3(x, _y, y);
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline float Vec2::Angle(const Vec2& v1, const Vec2& v2)
{
	return ::Angle(v1.x, v1.y, v2.x, v2.y);
}

inline void Vec2::Barycentric(const Vec2& v1, const Vec2& v2, const Vec2& v3, float f, float g, Vec2& result)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR x3 = XMLoadFloat2(&v3);
	XMVECTOR X = XMVectorBaryCentric(x1, x2, x3, f, g);
	XMStoreFloat2(&result, X);
}

inline Vec2 Vec2::Barycentric(const Vec2& v1, const Vec2& v2, const Vec2& v3, float f, float g)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR x3 = XMLoadFloat2(&v3);
	XMVECTOR X = XMVectorBaryCentric(x1, x2, x3, f, g);

	Vec2 result;
	XMStoreFloat2(&result, X);
	return result;
}

inline void Vec2::CatmullRom(const Vec2& v1, const Vec2& v2, const Vec2& v3, const Vec2& v4, float t, Vec2& result)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR x3 = XMLoadFloat2(&v3);
	XMVECTOR x4 = XMLoadFloat2(&v4);
	XMVECTOR X = XMVectorCatmullRom(x1, x2, x3, x4, t);
	XMStoreFloat2(&result, X);
}

inline Vec2 Vec2::CatmullRom(const Vec2& v1, const Vec2& v2, const Vec2& v3, const Vec2& v4, float t)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR x3 = XMLoadFloat2(&v3);
	XMVECTOR x4 = XMLoadFloat2(&v4);
	XMVECTOR X = XMVectorCatmullRom(x1, x2, x3, x4, t);

	Vec2 result;
	XMStoreFloat2(&result, X);
	return result;
}

inline float Vec2::Distance(const Vec2& v1, const Vec2& v2)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR V = XMVectorSubtract(x2, x1);
	XMVECTOR X = XMVector2Length(V);
	return XMVectorGetX(X);
}

inline float Vec2::DistanceSquared(const Vec2& v1, const Vec2& v2)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR V = XMVectorSubtract(x2, x1);
	XMVECTOR X = XMVector2LengthSq(V);
	return XMVectorGetX(X);
}

inline void Vec2::Hermite(const Vec2& v1, const Vec2& t1, const Vec2& v2, const Vec2& t2, float t, Vec2& result)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&t1);
	XMVECTOR x3 = XMLoadFloat2(&v2);
	XMVECTOR x4 = XMLoadFloat2(&t2);
	XMVECTOR X = XMVectorHermite(x1, x2, x3, x4, t);
	XMStoreFloat2(&result, X);
}

inline Vec2 Vec2::Hermite(const Vec2& v1, const Vec2& t1, const Vec2& v2, const Vec2& t2, float t)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&t1);
	XMVECTOR x3 = XMLoadFloat2(&v2);
	XMVECTOR x4 = XMLoadFloat2(&t2);
	XMVECTOR X = XMVectorHermite(x1, x2, x3, x4, t);

	Vec2 result;
	XMStoreFloat2(&result, X);
	return result;
}

inline void Vec2::Lerp(const Vec2& v1, const Vec2& v2, float t, Vec2& result)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR X = XMVectorLerp(x1, x2, t);
	XMStoreFloat2(&result, X);
}

inline Vec2 Vec2::Lerp(const Vec2& v1, const Vec2& v2, float t)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR X = XMVectorLerp(x1, x2, t);

	Vec2 result;
	XMStoreFloat2(&result, X);
	return result;
}

inline float Vec2::LookAtAngle(const Vec2& v1, const Vec2& v2)
{
	return ::Clip(-Angle(v1, v2) - PI / 2);
}

inline void Vec2::Max(const Vec2& v1, const Vec2& v2, Vec2& result)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR X = XMVectorMax(x1, x2);
	XMStoreFloat2(&result, X);
}

inline Vec2 Vec2::Max(const Vec2& v1, const Vec2& v2)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR X = XMVectorMax(x1, x2);

	Vec2 result;
	XMStoreFloat2(&result, X);
	return result;
}

inline void Vec2::Min(const Vec2& v1, const Vec2& v2, Vec2& result)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR X = XMVectorMin(x1, x2);
	XMStoreFloat2(&result, X);
}

inline Vec2 Vec2::Min(const Vec2& v1, const Vec2& v2)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR X = XMVectorMin(x1, x2);

	Vec2 result;
	XMStoreFloat2(&result, X);
	return result;
}

inline void Vec2::MinMax(const Vec2& v1, const Vec2& v2, Vec2& min, Vec2& max)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR r = XMVectorMin(x1, x2);
	XMStoreFloat2(&min, r);
	r = XMVectorMax(x1, x2);
	XMStoreFloat2(&max, r);
}

inline Vec2 Vec2::Random(float a, float b)
{
	return Vec2(::Random(a, b), ::Random(a, b));
}

inline Vec2 Vec2::Random(const Vec2& vmin, const Vec2& vmax)
{
	return Vec2(::Random(vmin.x, vmax.x), ::Random(vmin.y, vmax.y));
}

inline Vec2 Vec2::RandomCirclePt(float r)
{
	float a = ::Random(),
		b = ::Random();
	if(b < a)
		std::swap(a, b);
	return Vec2(b*r*cos(2 * PI*a / b), b*r*sin(2 * PI*a / b));
}

inline void Vec2::Reflect(const Vec2& ivec, const Vec2& nvec, Vec2& result)
{
	XMVECTOR i = XMLoadFloat2(&ivec);
	XMVECTOR n = XMLoadFloat2(&nvec);
	XMVECTOR X = XMVector2Reflect(i, n);
	XMStoreFloat2(&result, X);
}

inline Vec2 Vec2::Reflect(const Vec2& ivec, const Vec2& nvec)
{
	XMVECTOR i = XMLoadFloat2(&ivec);
	XMVECTOR n = XMLoadFloat2(&nvec);
	XMVECTOR X = XMVector2Reflect(i, n);

	Vec2 result;
	XMStoreFloat2(&result, X);
	return result;
}

inline void Vec2::Refract(const Vec2& ivec, const Vec2& nvec, float refractionIndex, Vec2& result)
{
	XMVECTOR i = XMLoadFloat2(&ivec);
	XMVECTOR n = XMLoadFloat2(&nvec);
	XMVECTOR X = XMVector2Refract(i, n, refractionIndex);
	XMStoreFloat2(&result, X);
}

inline Vec2 Vec2::Refract(const Vec2& ivec, const Vec2& nvec, float refractionIndex)
{
	XMVECTOR i = XMLoadFloat2(&ivec);
	XMVECTOR n = XMLoadFloat2(&nvec);
	XMVECTOR X = XMVector2Refract(i, n, refractionIndex);

	Vec2 result;
	XMStoreFloat2(&result, X);
	return result;
}

inline Vec2 Vec2::Slerp(const Vec2& a, const Vec2& b, float t)
{
	return Vec2(::Slerp(a.x, b.x, t), ::Slerp(a.y, b.y, t));
}

inline void Vec2::SmoothStep(const Vec2& v1, const Vec2& v2, float t, Vec2& result)
{
	t = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t);  // Clamp value to 0 to 1
	t = t * t*(3.f - 2.f*t);
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR X = XMVectorLerp(x1, x2, t);
	XMStoreFloat2(&result, X);
}

inline Vec2 Vec2::SmoothStep(const Vec2& v1, const Vec2& v2, float t)
{
	t = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t);  // Clamp value to 0 to 1
	t = t * t*(3.f - 2.f*t);
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR X = XMVectorLerp(x1, x2, t);

	Vec2 result;
	XMStoreFloat2(&result, X);
	return result;
}

inline void Vec2::Transform(const Vec2& v, const Quat& quat, Vec2& result)
{
	XMVECTOR v1 = XMLoadFloat2(&v);
	XMVECTOR q = XMLoadFloat4(&quat);
	XMVECTOR X = XMVector3Rotate(v1, q);
	XMStoreFloat2(&result, X);
}

inline Vec2 Vec2::Transform(const Vec2& v, const Quat& quat)
{
	XMVECTOR v1 = XMLoadFloat2(&v);
	XMVECTOR q = XMLoadFloat4(&quat);
	XMVECTOR X = XMVector3Rotate(v1, q);

	Vec2 result;
	XMStoreFloat2(&result, X);
	return result;
}

inline void Vec2::Transform(const Vec2& v, const Matrix& m, Vec2& result)
{
	XMVECTOR v1 = XMLoadFloat2(&v);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector2TransformCoord(v1, M);
	XMStoreFloat2(&result, X);
}

inline Vec2 Vec2::Transform(const Vec2& v, const Matrix& m)
{
	XMVECTOR v1 = XMLoadFloat2(&v);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector2TransformCoord(v1, M);

	Vec2 result;
	XMStoreFloat2(&result, X);
	return result;
}

inline void Vec2::Transform(const Vec2* varray, size_t count, const Matrix& m, Vec2* resultArray)
{
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVector2TransformCoordStream(resultArray, sizeof(XMFLOAT2), varray, sizeof(XMFLOAT2), count, M);
}

inline void Vec2::Transform(const Vec2& v, const Matrix& m, Vec4& result)
{
	XMVECTOR v1 = XMLoadFloat2(&v);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector2Transform(v1, M);
	XMStoreFloat4(&result, X);
}

inline void Vec2::Transform(const Vec2* varray, size_t count, const Matrix& m, Vec4* resultArray)
{
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVector2TransformStream(resultArray, sizeof(XMFLOAT4), varray, sizeof(XMFLOAT2), count, M);
}

inline void Vec2::TransformNormal(const Vec2& v, const Matrix& m, Vec2& result)
{
	XMVECTOR v1 = XMLoadFloat2(&v);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector2TransformNormal(v1, M);
	XMStoreFloat2(&result, X);
}

inline Vec2 Vec2::TransformNormal(const Vec2& v, const Matrix& m)
{
	XMVECTOR v1 = XMLoadFloat2(&v);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector2TransformNormal(v1, M);

	Vec2 result;
	XMStoreFloat2(&result, X);
	return result;
}

inline void Vec2::TransformNormal(const Vec2* varray, size_t count, const Matrix& m, Vec2* resultArray)
{
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVector2TransformNormalStream(resultArray, sizeof(XMFLOAT2), varray, sizeof(XMFLOAT2), count, M);
}

//*************************************************************************************************
//
// 3D float point
//
//*************************************************************************************************
inline Vec3::Vec3()
{
}

inline Vec3::Vec3(float x, float y, float z) : XMFLOAT3(x, y, z)
{
}

inline Vec3::Vec3(const Vec3& v) : XMFLOAT3(v.x, v.y, v.z)
{
}

inline Vec3::Vec3(FXMVECTOR v)
{
	XMStoreFloat3(this, v);
}

inline Vec3::Vec3(const XMVECTORF32& v) : XMFLOAT3(v.f[0], v.f[1], v.f[2])
{
}

inline Vec3::Vec3(const float* f) : XMFLOAT3(f[0], f[1], f[2])
{
}

inline Vec3::operator XMVECTOR() const
{
	return XMLoadFloat3(this);
}

inline Vec3::operator float*()
{
	return &x;
}

inline Vec3::operator const float*() const
{
	return &x;
}

inline float& Vec3::operator [](int index)
{
	return ((float*)*this)[index];
}

inline const float& Vec3::operator [](int index) const
{
	return ((const float*)*this)[index];
}

inline bool Vec3::operator == (const Vec3& v) const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR v2 = XMLoadFloat3(&v);
	return XMVector3Equal(v1, v2);
}

inline bool Vec3::operator != (const Vec3& v) const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR v2 = XMLoadFloat3(&v);
	return XMVector3NotEqual(v1, v2);
}

inline bool Vec3::operator < (const Vec3& v) const
{
	if(x == v.x)
	{
		if(y == v.y)
		{
			if(z == v.z)
				return false;
			else
				return z < v.z;
		}
		else
			return y < v.y;
	}
	else
		return x < v.x;
}

inline Vec3& Vec3::operator = (const Vec3& v)
{
	x = v.x;
	y = v.y;
	z = v.z;
	return *this;
}

inline Vec3& Vec3::operator = (const XMVECTORF32& v)
{
	x = v.f[0];
	y = v.f[1];
	z = v.f[2];
	return *this;
}

inline Vec3& Vec3::operator += (const Vec3& v)
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR v2 = XMLoadFloat3(&v);
	XMVECTOR r = XMVectorAdd(v1, v2);
	XMStoreFloat3(this, r);
	return *this;
}

inline Vec3& Vec3::operator -= (const Vec3& v)
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR v2 = XMLoadFloat3(&v);
	XMVECTOR r = XMVectorSubtract(v1, v2);
	XMStoreFloat3(this, r);
	return *this;
}

inline Vec3& Vec3::operator *= (float s)
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR r = XMVectorScale(v1, s);
	XMStoreFloat3(this, r);
	return *this;
}

inline Vec3& Vec3::operator /= (float s)
{
	assert(s != 0.f);
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR r = XMVectorScale(v1, 1.f / s);
	XMStoreFloat3(this, r);
	return *this;
}

inline Vec3 Vec3::operator + () const
{
	return *this;
}

inline Vec3 Vec3::operator - () const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR r = XMVectorNegate(v1);
	Vec3 result;
	XMStoreFloat3(&result, r);
	return result;
}

inline Vec3 Vec3::operator + (const Vec3& v) const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR v2 = XMLoadFloat3(&v);
	XMVECTOR r = XMVectorAdd(v1, v2);
	Vec3 result;
	XMStoreFloat3(&result, r);
	return result;
}

inline Vec3 Vec3::operator - (const Vec3& v) const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR v2 = XMLoadFloat3(&v);
	XMVECTOR r = XMVectorSubtract(v1, v2);
	Vec3 result;
	XMStoreFloat3(&result, r);
	return result;
}

inline Vec3 Vec3::operator * (float s) const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR r = XMVectorScale(v1, s);
	Vec3 result;
	XMStoreFloat3(&result, r);
	return result;
}

inline Vec3 Vec3::operator / (float s) const
{
	assert(s != 0.f);
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR r = XMVectorScale(v1, 1.f / s);
	Vec3 result;
	XMStoreFloat3(&result, r);
	return result;
}

inline Vec3 operator * (float s, const Vec3& v)
{
	return v * s;
}

//------------------------------------------------------------------------------
// Methods
//------------------------------------------------------------------------------

inline void Vec3::Clamp(const Vec3& vmin, const Vec3& vmax)
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR v2 = XMLoadFloat3(&vmin);
	XMVECTOR v3 = XMLoadFloat3(&vmax);
	XMVECTOR X = XMVectorClamp(v1, v2, v3);
	XMStoreFloat3(this, X);
}

inline void Vec3::Clamp(const Vec3& vmin, const Vec3& vmax, Vec3& result) const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR v2 = XMLoadFloat3(&vmin);
	XMVECTOR v3 = XMLoadFloat3(&vmax);
	XMVECTOR X = XMVectorClamp(v1, v2, v3);
	XMStoreFloat3(&result, X);
}

inline Vec3 Vec3::Clamped(const Vec3& min, const Vec3& max) const
{
	Vec3 result;
	Clamp(min, max, result);
	return result;
}

inline void Vec3::Cross(const Vec3& V, Vec3& result) const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR v2 = XMLoadFloat3(&V);
	XMVECTOR R = XMVector3Cross(v1, v2);
	XMStoreFloat3(&result, R);
}

inline Vec3 Vec3::Cross(const Vec3& V) const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR v2 = XMLoadFloat3(&V);
	XMVECTOR R = XMVector3Cross(v1, v2);

	Vec3 result;
	XMStoreFloat3(&result, R);
	return result;
}

inline float Vec3::Dot(const Vec3& V) const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR v2 = XMLoadFloat3(&V);
	XMVECTOR X = XMVector3Dot(v1, v2);
	return XMVectorGetX(X);
}

inline float Vec3::Dot2d(const Vec3& v) const
{
	return (x*v.x + z * v.z);
}

inline float Vec3::Dot2d() const
{
	return x * x + z * z;
}

inline bool Vec3::Equal(const Vec3& v) const
{
	return ::Equal(x, v.x) && ::Equal(y, v.y) && ::Equal(z, v.z);
}

inline bool Vec3::InBounds(const Vec3& Bounds) const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR v2 = XMLoadFloat3(&Bounds);
	return XMVector3InBounds(v1, v2);
}

inline bool Vec3::IsPositive() const
{
	return x >= 0.f && y >= 0.f && z >= 0.f;
}

inline float Vec3::Length() const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR X = XMVector3Length(v1);
	return XMVectorGetX(X);
}

inline float Vec3::LengthSquared() const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR X = XMVector3LengthSq(v1);
	return XMVectorGetX(X);
}

inline Vec3 Vec3::ModX(float value) const
{
	return Vec3(x + value, y, z);
}

inline Vec3 Vec3::ModY(float value) const
{
	return Vec3(x, y + value, z);
}

inline Vec3 Vec3::ModZ(float value) const
{
	return Vec3(x, y, z + value);
}

inline Vec3& Vec3::Normalize()
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR X = XMVector3Normalize(v1);
	XMStoreFloat3(this, X);
	return *this;
}

inline void Vec3::Normalize(Vec3& result) const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR X = XMVector3Normalize(v1);
	XMStoreFloat3(&result, X);
}

inline Vec3 Vec3::Normalized() const
{
	Vec3 result;
	Normalize(result);
	return result;
}

inline Vec2 Vec3::XY() const
{
	return Vec2(x, y);
}

inline Vec2 Vec3::XZ() const
{
	return Vec2(x, z);
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline float Vec3::Angle2d(const Vec3& v1, const Vec3& v2)
{
	return ::Angle(v1.x, v1.z, v2.x, v2.z);
}

inline void Vec3::Barycentric(const Vec3& v1, const Vec3& v2, const Vec3& v3, float f, float g, Vec3& result)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR x3 = XMLoadFloat3(&v3);
	XMVECTOR X = XMVectorBaryCentric(x1, x2, x3, f, g);
	XMStoreFloat3(&result, X);
}

inline Vec3 Vec3::Barycentric(const Vec3& v1, const Vec3& v2, const Vec3& v3, float f, float g)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR x3 = XMLoadFloat3(&v3);
	XMVECTOR X = XMVectorBaryCentric(x1, x2, x3, f, g);

	Vec3 result;
	XMStoreFloat3(&result, X);
	return result;
}

inline void Vec3::CatmullRom(const Vec3& v1, const Vec3& v2, const Vec3& v3, const Vec3& v4, float t, Vec3& result)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR x3 = XMLoadFloat3(&v3);
	XMVECTOR x4 = XMLoadFloat3(&v4);
	XMVECTOR X = XMVectorCatmullRom(x1, x2, x3, x4, t);
	XMStoreFloat3(&result, X);
}

inline Vec3 Vec3::CatmullRom(const Vec3& v1, const Vec3& v2, const Vec3& v3, const Vec3& v4, float t)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR x3 = XMLoadFloat3(&v3);
	XMVECTOR x4 = XMLoadFloat3(&v4);
	XMVECTOR X = XMVectorCatmullRom(x1, x2, x3, x4, t);

	Vec3 result;
	XMStoreFloat3(&result, X);
	return result;
}

inline float Vec3::Distance(const Vec3& v1, const Vec3& v2)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR V = XMVectorSubtract(x2, x1);
	XMVECTOR X = XMVector3Length(V);
	return XMVectorGetX(X);
}

inline float Vec3::DistanceSquared(const Vec3& v1, const Vec3& v2)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR V = XMVectorSubtract(x2, x1);
	XMVECTOR X = XMVector3LengthSq(V);
	return XMVectorGetX(X);
}

inline float Vec3::Distance2d(const Vec3& v1, const Vec3& v2)
{
	float x = abs(v1.x - v2.x),
		z = abs(v1.z - v2.z);
	return sqrt(x*x + z * z);
}

inline void Vec3::Hermite(const Vec3& v1, const Vec3& t1, const Vec3& v2, const Vec3& t2, float t, Vec3& result)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&t1);
	XMVECTOR x3 = XMLoadFloat3(&v2);
	XMVECTOR x4 = XMLoadFloat3(&t2);
	XMVECTOR X = XMVectorHermite(x1, x2, x3, x4, t);
	XMStoreFloat3(&result, X);
}

inline Vec3 Vec3::Hermite(const Vec3& v1, const Vec3& t1, const Vec3& v2, const Vec3& t2, float t)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&t1);
	XMVECTOR x3 = XMLoadFloat3(&v2);
	XMVECTOR x4 = XMLoadFloat3(&t2);
	XMVECTOR X = XMVectorHermite(x1, x2, x3, x4, t);

	Vec3 result;
	XMStoreFloat3(&result, X);
	return result;
}

inline void Vec3::Lerp(const Vec3& v1, const Vec3& v2, float t, Vec3& result)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR X = XMVectorLerp(x1, x2, t);
	XMStoreFloat3(&result, X);
}

inline Vec3 Vec3::Lerp(const Vec3& v1, const Vec3& v2, float t)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR X = XMVectorLerp(x1, x2, t);

	Vec3 result;
	XMStoreFloat3(&result, X);
	return result;
}

inline float Vec3::LookAtAngle(const Vec3& v1, const Vec3& v2)
{
	return Clip(-Angle2d(v1, v2) - PI / 2);
}

inline void Vec3::Max(const Vec3& v1, const Vec3& v2, Vec3& result)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR X = XMVectorMax(x1, x2);
	XMStoreFloat3(&result, X);
}

inline Vec3 Vec3::Max(const Vec3& v1, const Vec3& v2)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR X = XMVectorMax(x1, x2);

	Vec3 result;
	XMStoreFloat3(&result, X);
	return result;
}

inline void Vec3::Min(const Vec3& v1, const Vec3& v2, Vec3& result)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR X = XMVectorMin(x1, x2);
	XMStoreFloat3(&result, X);
}

inline Vec3 Vec3::Min(const Vec3& v1, const Vec3& v2)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR X = XMVectorMin(x1, x2);

	Vec3 result;
	XMStoreFloat3(&result, X);
	return result;
}

inline void Vec3::MinMax(const Vec3& v1, const Vec3& v2, Vec3& min, Vec3& max)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR r = XMVectorMin(x1, x2);
	XMStoreFloat3(&min, r);
	r = XMVectorMax(x1, x2);
	XMStoreFloat3(&max, r);
}

inline Vec3 Vec3::Random(float a, float b)
{
	return Vec3(::Random(a, b), ::Random(a, b), ::Random(a, b));
}

inline Vec3 Vec3::Random(const Vec3& min, const Vec3& max)
{
	return Vec3(::Random(min.x, max.x), ::Random(min.y, max.y), ::Random(min.z, max.z));
}

inline void Vec3::Reflect(const Vec3& ivec, const Vec3& nvec, Vec3& result)
{
	XMVECTOR i = XMLoadFloat3(&ivec);
	XMVECTOR n = XMLoadFloat3(&nvec);
	XMVECTOR X = XMVector3Reflect(i, n);
	XMStoreFloat3(&result, X);
}

inline Vec3 Vec3::Reflect(const Vec3& ivec, const Vec3& nvec)
{
	XMVECTOR i = XMLoadFloat3(&ivec);
	XMVECTOR n = XMLoadFloat3(&nvec);
	XMVECTOR X = XMVector3Reflect(i, n);

	Vec3 result;
	XMStoreFloat3(&result, X);
	return result;
}

inline void Vec3::Refract(const Vec3& ivec, const Vec3& nvec, float refractionIndex, Vec3& result)
{
	XMVECTOR i = XMLoadFloat3(&ivec);
	XMVECTOR n = XMLoadFloat3(&nvec);
	XMVECTOR X = XMVector3Refract(i, n, refractionIndex);
	XMStoreFloat3(&result, X);
}

inline Vec3 Vec3::Refract(const Vec3& ivec, const Vec3& nvec, float refractionIndex)
{
	XMVECTOR i = XMLoadFloat3(&ivec);
	XMVECTOR n = XMLoadFloat3(&nvec);
	XMVECTOR X = XMVector3Refract(i, n, refractionIndex);

	Vec3 result;
	XMStoreFloat3(&result, X);
	return result;
}

inline void Vec3::SmoothStep(const Vec3& v1, const Vec3& v2, float t, Vec3& result)
{
	t = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t);  // Clamp value to 0 to 1
	t = t * t*(3.f - 2.f*t);
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR X = XMVectorLerp(x1, x2, t);
	XMStoreFloat3(&result, X);
}

inline Vec3 Vec3::SmoothStep(const Vec3& v1, const Vec3& v2, float t)
{
	t = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t);  // Clamp value to 0 to 1
	t = t * t*(3.f - 2.f*t);
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR X = XMVectorLerp(x1, x2, t);

	Vec3 result;
	XMStoreFloat3(&result, X);
	return result;
}

inline void Vec3::Transform(const Vec3& v, const Quat& quat, Vec3& result)
{
	XMVECTOR v1 = XMLoadFloat3(&v);
	XMVECTOR q = XMLoadFloat4(&quat);
	XMVECTOR X = XMVector3Rotate(v1, q);
	XMStoreFloat3(&result, X);
}

inline Vec3 Vec3::Transform(const Vec3& v, const Quat& quat)
{
	XMVECTOR v1 = XMLoadFloat3(&v);
	XMVECTOR q = XMLoadFloat4(&quat);
	XMVECTOR X = XMVector3Rotate(v1, q);

	Vec3 result;
	XMStoreFloat3(&result, X);
	return result;
}

inline void Vec3::Transform(const Vec3& v, const Matrix& m, Vec3& result)
{
	XMVECTOR v1 = XMLoadFloat3(&v);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector3TransformCoord(v1, M);
	XMStoreFloat3(&result, X);
}

inline Vec3 Vec3::Transform(const Vec3& v, const Matrix& m)
{
	XMVECTOR v1 = XMLoadFloat3(&v);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector3TransformCoord(v1, M);

	Vec3 result;
	XMStoreFloat3(&result, X);
	return result;
}

inline void Vec3::Transform(const Vec3* varray, size_t count, const Matrix& m, Vec3* resultArray)
{
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVector3TransformCoordStream(resultArray, sizeof(XMFLOAT3), varray, sizeof(XMFLOAT3), count, M);
}

inline void Vec3::Transform(const Vec3& v, const Matrix& m, Vec4& result)
{
	XMVECTOR v1 = XMLoadFloat3(&v);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector3Transform(v1, M);
	XMStoreFloat4(&result, X);
}

inline void Vec3::Transform(const Vec3* varray, size_t count, const Matrix& m, Vec4* resultArray)
{
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVector3TransformStream(resultArray, sizeof(XMFLOAT4), varray, sizeof(XMFLOAT3), count, M);
}

inline void Vec3::TransformNormal(const Vec3& v, const Matrix& m, Vec3& result)
{
	XMVECTOR v1 = XMLoadFloat3(&v);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector3TransformNormal(v1, M);
	XMStoreFloat3(&result, X);
}

inline Vec3 Vec3::TransformNormal(const Vec3& v, const Matrix& m)
{
	XMVECTOR v1 = XMLoadFloat3(&v);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector3TransformNormal(v1, M);

	Vec3 result;
	XMStoreFloat3(&result, X);
	return result;
}

inline void Vec3::TransformNormal(const Vec3* varray, size_t count, const Matrix& m, Vec3* resultArray)
{
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVector3TransformNormalStream(resultArray, sizeof(XMFLOAT3), varray, sizeof(XMFLOAT3), count, M);
}

inline Vec3 Vec3::TransformZero(const Matrix& m)
{
	XMVECTOR v1 = XMLoadFloat3(&Vec3::Zero);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector3TransformCoord(v1, M);

	Vec3 result;
	XMStoreFloat3(&result, X);
	return result;
}

//*************************************************************************************************
//
// 4D float point
//
//*************************************************************************************************
inline Vec4::Vec4()
{
}

inline constexpr Vec4::Vec4(float x, float y, float z, float w) : XMFLOAT4(x, y, z, w)
{
}

inline Vec4::Vec4(const Vec4& v) : XMFLOAT4(v.x, v.y, v.z, v.w)
{
}

inline Vec4::Vec4(const Vec3& v, float w) : XMFLOAT4(v.x, v.y, v.z, w)
{
}

inline Vec4::Vec4(FXMVECTOR v)
{
	XMStoreFloat4(this, v);
}

inline Vec4::Vec4(const XMVECTORF32& v) : XMFLOAT4(v.f[0], v.f[1], v.f[2], v.f[3])
{
}

inline Vec4::operator XMVECTOR() const
{
	return XMLoadFloat4(this);
}

inline Vec4::operator float*()
{
	return &x;
}

inline Vec4::operator const float*() const
{
	return &x;
}

inline float& Vec4::operator [](int index)
{
	return ((float*)*this)[index];
}

inline const float& Vec4::operator [](int index) const
{
	return ((const float*)*this)[index];
}

inline bool Vec4::operator == (const Vec4& v) const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR v2 = XMLoadFloat4(&v);
	return XMVector4Equal(v1, v2);
}

inline bool Vec4::operator != (const Vec4& v) const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR v2 = XMLoadFloat4(&v);
	return XMVector4NotEqual(v1, v2);
}

inline bool Vec4::operator < (const Vec4& v) const
{
	if(x == v.x)
	{
		if(y == v.y)
		{
			if(z == v.z)
			{
				if(w == v.w)
					return false;
				else
					return w < v.w;
			}
			else
				return z < v.z;
		}
		else
			return y < v.y;
	}
	else
		return x < v.x;
}

inline Vec4& Vec4::operator = (const Vec4& v)
{
	x = v.x;
	y = v.y;
	z = v.z;
	w = v.w;
	return *this;
}

inline Vec4& Vec4::operator = (const XMVECTORF32& v)
{
	x = v.f[0];
	y = v.f[1];
	z = v.f[2];
	w = v.f[3];
	return *this;
}

inline Vec4& Vec4::operator += (const Vec4& v)
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR v2 = XMLoadFloat4(&v);
	XMVECTOR r = XMVectorAdd(v1, v2);
	XMStoreFloat4(this, r);
	return *this;
}

inline Vec4& Vec4::operator -= (const Vec4& v)
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR v2 = XMLoadFloat4(&v);
	XMVECTOR r = XMVectorSubtract(v1, v2);
	XMStoreFloat4(this, r);
	return *this;
}

inline Vec4& Vec4::operator *= (float s)
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR r = XMVectorScale(v1, s);
	XMStoreFloat4(this, r);
	return *this;
}

inline Vec4& Vec4::operator /= (float s)
{
	assert(s != 0.f);
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR r = XMVectorScale(v1, 1.f / s);
	XMStoreFloat4(this, r);
	return *this;
}

inline Vec4 Vec4::operator + () const
{
	return *this;
}

inline Vec4 Vec4::operator - () const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR r = XMVectorNegate(v1);
	Vec4 result;
	XMStoreFloat4(&result, r);
	return r;
}

inline Vec4 Vec4::operator + (const Vec4& v) const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR v2 = XMLoadFloat4(&v);
	XMVECTOR r = XMVectorAdd(v1, v2);
	Vec4 result;
	XMStoreFloat4(&result, r);
	return result;
}

inline Vec4 Vec4::operator - (const Vec4& v) const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR v2 = XMLoadFloat4(&v);
	XMVECTOR r = XMVectorSubtract(v1, v2);
	Vec4 result;
	XMStoreFloat4(&result, r);
	return result;
}

inline Vec4 Vec4::operator * (float s) const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR r = XMVectorScale(v1, s);
	Vec4 result;
	XMStoreFloat4(&result, r);
	return result;
}

inline Vec4 Vec4::operator / (float s) const
{
	assert(s != 0.f);
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR r = XMVectorScale(v1, 1.f / s);
	Vec4 result;
	XMStoreFloat4(&result, r);
	return result;
}

inline Vec4 operator * (float s, const Vec4& v)
{
	return v * s;
}

//------------------------------------------------------------------------------
// Methods
//------------------------------------------------------------------------------

inline void Vec4::Clamp(const Vec4& vmin, const Vec4& vmax)
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR v2 = XMLoadFloat4(&vmin);
	XMVECTOR v3 = XMLoadFloat4(&vmax);
	XMVECTOR X = XMVectorClamp(v1, v2, v3);
	XMStoreFloat4(this, X);
}

inline void Vec4::Clamp(const Vec4& vmin, const Vec4& vmax, Vec4& result) const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR v2 = XMLoadFloat4(&vmin);
	XMVECTOR v3 = XMLoadFloat4(&vmax);
	XMVECTOR X = XMVectorClamp(v1, v2, v3);
	XMStoreFloat4(&result, X);
}

inline Vec4 Vec4::Clamped(const Vec4& min, const Vec4& max) const
{
	Vec4 result;
	Clamp(min, max, result);
	return result;
}

inline void Vec4::Cross(const Vec4& v1, const Vec4& v2, Vec4& result) const
{
	XMVECTOR x1 = XMLoadFloat4(this);
	XMVECTOR x2 = XMLoadFloat4(&v1);
	XMVECTOR x3 = XMLoadFloat4(&v2);
	XMVECTOR R = XMVector4Cross(x1, x2, x3);
	XMStoreFloat4(&result, R);
}

inline Vec4 Vec4::Cross(const Vec4& v1, const Vec4& v2) const
{
	XMVECTOR x1 = XMLoadFloat4(this);
	XMVECTOR x2 = XMLoadFloat4(&v1);
	XMVECTOR x3 = XMLoadFloat4(&v2);
	XMVECTOR R = XMVector4Cross(x1, x2, x3);

	Vec4 result;
	XMStoreFloat4(&result, R);
	return result;
}

inline float Vec4::Dot(const Vec4& V) const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR v2 = XMLoadFloat4(&V);
	XMVECTOR X = XMVector4Dot(v1, v2);
	return XMVectorGetX(X);
}

inline float Vec4::DotSelf() const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR X = XMVector4Dot(v1, v1);
	return XMVectorGetX(X);
}

inline bool Vec4::Equal(const Vec4& v) const
{
	return ::Equal(x, v.x) && ::Equal(y, v.y) && ::Equal(z, v.z) && ::Equal(w, v.w);
}

inline bool Vec4::InBounds(const Vec4& Bounds) const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR v2 = XMLoadFloat4(&Bounds);
	return XMVector4InBounds(v1, v2);
}

inline float Vec4::Length() const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR X = XMVector4Length(v1);
	return XMVectorGetX(X);
}

inline float Vec4::LengthSquared() const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR X = XMVector4LengthSq(v1);
	return XMVectorGetX(X);
}

inline Vec4& Vec4::Normalize()
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR X = XMVector4Normalize(v1);
	XMStoreFloat4(this, X);
	return *this;
}

inline void Vec4::Normalize(Vec4& result) const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR X = XMVector4Normalize(v1);
	XMStoreFloat4(&result, X);
}

inline Vec4 Vec4::Normalized() const
{
	Vec4 result;
	Normalize(result);
	return result;
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline void Vec4::Barycentric(const Vec4& v1, const Vec4& v2, const Vec4& v3, float f, float g, Vec4& result)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR x3 = XMLoadFloat4(&v3);
	XMVECTOR X = XMVectorBaryCentric(x1, x2, x3, f, g);
	XMStoreFloat4(&result, X);
}

inline Vec4 Vec4::Barycentric(const Vec4& v1, const Vec4& v2, const Vec4& v3, float f, float g)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR x3 = XMLoadFloat4(&v3);
	XMVECTOR X = XMVectorBaryCentric(x1, x2, x3, f, g);

	Vec4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline void Vec4::CatmullRom(const Vec4& v1, const Vec4& v2, const Vec4& v3, const Vec4& v4, float t, Vec4& result)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR x3 = XMLoadFloat4(&v3);
	XMVECTOR x4 = XMLoadFloat4(&v4);
	XMVECTOR X = XMVectorCatmullRom(x1, x2, x3, x4, t);
	XMStoreFloat4(&result, X);
}

inline Vec4 Vec4::CatmullRom(const Vec4& v1, const Vec4& v2, const Vec4& v3, const Vec4& v4, float t)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR x3 = XMLoadFloat4(&v3);
	XMVECTOR x4 = XMLoadFloat4(&v4);
	XMVECTOR X = XMVectorCatmullRom(x1, x2, x3, x4, t);

	Vec4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline float Vec4::Distance(const Vec4& v1, const Vec4& v2)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR V = XMVectorSubtract(x2, x1);
	XMVECTOR X = XMVector4Length(V);
	return XMVectorGetX(X);
}

inline float Vec4::DistanceSquared(const Vec4& v1, const Vec4& v2)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR V = XMVectorSubtract(x2, x1);
	XMVECTOR X = XMVector4LengthSq(V);
	return XMVectorGetX(X);
}

inline void Vec4::Hermite(const Vec4& v1, const Vec4& t1, const Vec4& v2, const Vec4& t2, float t, Vec4& result)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&t1);
	XMVECTOR x3 = XMLoadFloat4(&v2);
	XMVECTOR x4 = XMLoadFloat4(&t2);
	XMVECTOR X = XMVectorHermite(x1, x2, x3, x4, t);
	XMStoreFloat4(&result, X);
}

inline Vec4 Vec4::Hermite(const Vec4& v1, const Vec4& t1, const Vec4& v2, const Vec4& t2, float t)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&t1);
	XMVECTOR x3 = XMLoadFloat4(&v2);
	XMVECTOR x4 = XMLoadFloat4(&t2);
	XMVECTOR X = XMVectorHermite(x1, x2, x3, x4, t);

	Vec4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline void Vec4::Lerp(const Vec4& v1, const Vec4& v2, float t, Vec4& result)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR X = XMVectorLerp(x1, x2, t);
	XMStoreFloat4(&result, X);
}

inline Vec4 Vec4::Lerp(const Vec4& v1, const Vec4& v2, float t)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR X = XMVectorLerp(x1, x2, t);

	Vec4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline void Vec4::Max(const Vec4& v1, const Vec4& v2, Vec4& result)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR X = XMVectorMax(x1, x2);
	XMStoreFloat4(&result, X);
}

inline Vec4 Vec4::Max(const Vec4& v1, const Vec4& v2)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR X = XMVectorMax(x1, x2);

	Vec4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline void Vec4::Min(const Vec4& v1, const Vec4& v2, Vec4& result)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR X = XMVectorMin(x1, x2);
	XMStoreFloat4(&result, X);
}

inline Vec4 Vec4::Min(const Vec4& v1, const Vec4& v2)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR X = XMVectorMin(x1, x2);

	Vec4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline void Vec4::Reflect(const Vec4& ivec, const Vec4& nvec, Vec4& result)
{
	XMVECTOR i = XMLoadFloat4(&ivec);
	XMVECTOR n = XMLoadFloat4(&nvec);
	XMVECTOR X = XMVector4Reflect(i, n);
	XMStoreFloat4(&result, X);
}

inline Vec4 Vec4::Reflect(const Vec4& ivec, const Vec4& nvec)
{
	XMVECTOR i = XMLoadFloat4(&ivec);
	XMVECTOR n = XMLoadFloat4(&nvec);
	XMVECTOR X = XMVector4Reflect(i, n);

	Vec4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline void Vec4::Refract(const Vec4& ivec, const Vec4& nvec, float refractionIndex, Vec4& result)
{
	XMVECTOR i = XMLoadFloat4(&ivec);
	XMVECTOR n = XMLoadFloat4(&nvec);
	XMVECTOR X = XMVector4Refract(i, n, refractionIndex);
	XMStoreFloat4(&result, X);
}

inline Vec4 Vec4::Refract(const Vec4& ivec, const Vec4& nvec, float refractionIndex)
{
	XMVECTOR i = XMLoadFloat4(&ivec);
	XMVECTOR n = XMLoadFloat4(&nvec);
	XMVECTOR X = XMVector4Refract(i, n, refractionIndex);

	Vec4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline void Vec4::SmoothStep(const Vec4& v1, const Vec4& v2, float t, Vec4& result)
{
	t = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t);  // Clamp value to 0 to 1
	t = t * t*(3.f - 2.f*t);
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR X = XMVectorLerp(x1, x2, t);
	XMStoreFloat4(&result, X);
}

inline Vec4 Vec4::SmoothStep(const Vec4& v1, const Vec4& v2, float t)
{
	t = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t);  // Clamp value to 0 to 1
	t = t * t*(3.f - 2.f*t);
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR X = XMVectorLerp(x1, x2, t);

	Vec4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline void Vec4::Transform(const Vec2& v, const Quat& quat, Vec4& result)
{
	XMVECTOR v1 = XMLoadFloat2(&v);
	XMVECTOR q = XMLoadFloat4(&quat);
	XMVECTOR X = XMVector3Rotate(v1, q);
	X = XMVectorSelect(g_XMIdentityR3, X, g_XMSelect1110); // result.w = 1.f
	XMStoreFloat4(&result, X);
}

inline Vec4 Vec4::Transform(const Vec2& v, const Quat& quat)
{
	XMVECTOR v1 = XMLoadFloat2(&v);
	XMVECTOR q = XMLoadFloat4(&quat);
	XMVECTOR X = XMVector3Rotate(v1, q);
	X = XMVectorSelect(g_XMIdentityR3, X, g_XMSelect1110); // result.w = 1.f

	Vec4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline void Vec4::Transform(const Vec3& v, const Quat& quat, Vec4& result)
{
	XMVECTOR v1 = XMLoadFloat3(&v);
	XMVECTOR q = XMLoadFloat4(&quat);
	XMVECTOR X = XMVector3Rotate(v1, q);
	X = XMVectorSelect(g_XMIdentityR3, X, g_XMSelect1110); // result.w = 1.f
	XMStoreFloat4(&result, X);
}

inline Vec4 Vec4::Transform(const Vec3& v, const Quat& quat)
{
	XMVECTOR v1 = XMLoadFloat3(&v);
	XMVECTOR q = XMLoadFloat4(&quat);
	XMVECTOR X = XMVector3Rotate(v1, q);
	X = XMVectorSelect(g_XMIdentityR3, X, g_XMSelect1110); // result.w = 1.f

	Vec4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline void Vec4::Transform(const Vec4& v, const Quat& quat, Vec4& result)
{
	XMVECTOR v1 = XMLoadFloat4(&v);
	XMVECTOR q = XMLoadFloat4(&quat);
	XMVECTOR X = XMVector3Rotate(v1, q);
	X = XMVectorSelect(v1, X, g_XMSelect1110); // result.w = v.w
	XMStoreFloat4(&result, X);
}

inline Vec4 Vec4::Transform(const Vec4& v, const Quat& quat)
{
	XMVECTOR v1 = XMLoadFloat4(&v);
	XMVECTOR q = XMLoadFloat4(&quat);
	XMVECTOR X = XMVector3Rotate(v1, q);
	X = XMVectorSelect(v1, X, g_XMSelect1110); // result.w = v.w

	Vec4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline void Vec4::Transform(const Vec4& v, const Matrix& m, Vec4& result)
{
	XMVECTOR v1 = XMLoadFloat4(&v);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector4Transform(v1, M);
	XMStoreFloat4(&result, X);
}

inline Vec4 Vec4::Transform(const Vec4& v, const Matrix& m)
{
	XMVECTOR v1 = XMLoadFloat4(&v);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector4Transform(v1, M);

	Vec4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline void Vec4::Transform(const Vec4* varray, size_t count, const Matrix& m, Vec4* resultArray)
{
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVector4TransformStream(resultArray, sizeof(XMFLOAT4), varray, sizeof(XMFLOAT4), count, M);
}

//*************************************************************************************************
//
// 2d box using floats
//
//*************************************************************************************************
inline Box2d::Box2d()
{
}

inline Box2d::Box2d(float minx, float miny, float maxx, float maxy) : v1(minx, miny), v2(maxx, maxy)
{
}

inline Box2d::Box2d(const Vec2& v1, const Vec2& v2) : v1(v1), v2(v2)
{
}

inline Box2d::Box2d(const Box2d& box) : v1(box.v1), v2(box.v2)
{
}

inline Box2d::Box2d(float x, float y) : v1(x, y), v2(x, y)
{
}

inline Box2d::Box2d(const Box2d& box, float margin) : v1(box.v1.x - margin, box.v1.y - margin), v2(box.v2.x + margin, box.v2.y + margin)
{
}

inline Box2d::Box2d(const Vec2& v) : v1(v), v2(v)
{
}

inline Box2d::Box2d(const Rect& r) : v1(Vec2(r.p1)), v2(Vec2(r.p2))
{
}

inline bool Box2d::operator == (const Box2d& b) const
{
	return v1 == b.v1 && v2 == b.v2;
}

inline bool Box2d::operator != (const Box2d& b) const
{
	return v1 != b.v1 || v2 != b.v2;
}

inline Box2d& Box2d::operator = (const Box2d& b)
{
	v1 = b.v1;
	v2 = b.v2;
	return *this;
}

inline Box2d& Box2d::operator += (const Vec2& v)
{
	v1 += v;
	v2 += v;
	return *this;
}

inline Box2d& Box2d::operator -= (const Vec2& v)
{
	v1 -= v;
	v2 -= v;
	return *this;
}

inline Box2d& Box2d::operator *= (float f)
{
	v1 *= f;
	v2 *= f;
	return *this;
}

inline Box2d& Box2d::operator /= (float f)
{
	v1 /= f;
	v2 /= f;
	return *this;
}

inline Box2d Box2d::operator + () const
{
	return *this;
}

inline Box2d Box2d::operator - () const
{
	return Box2d(-v1, -v2);
}

inline Box2d Box2d::operator + (const Vec2& v) const
{
	return Box2d(v1 + v, v2 + v);
}

inline Box2d Box2d::operator - (const Vec2& v) const
{
	return Box2d(v1 - v, v2 - v);
}

inline Box2d Box2d::operator * (float f) const
{
	return Box2d(v1 * f, v2 * f);
}

inline Box2d Box2d::operator / (float f) const
{
	return Box2d(v1 / f, v2 / f);
}

inline Box2d Box2d::operator / (const Vec2& v) const
{
	return Box2d(v1.x / v.x, v1.y / v.y, v2.x / v.x, v2.y / v.y);
}

inline Box2d operator * (float f, const Box2d& b)
{
	return b * f;
}

//------------------------------------------------------------------------------
// Methods
//------------------------------------------------------------------------------

inline Vec2 Box2d::GetRandomPoint() const
{
	return Vec2(::Random(v1.x, v2.x), ::Random(v1.y, v2.y));
}

inline Vec2 Box2d::GetRandomPoint(float offset) const
{
	assert(SizeX() >= offset * 2);
	return Vec2(::Random(v1.x + offset, v2.x - offset), ::Random(v1.y + offset, v2.y - offset));
}

inline Box2d Box2d::Intersect(const Box2d& b) const
{
	return Intersect(*this, b);
}

inline bool Box2d::IsInside(const Vec2& v) const
{
	return v.x >= v1.x && v.y >= v1.y && v.x <= v2.x && v.y <= v2.y;
}

inline bool Box2d::IsInside(const Vec3& v) const
{
	return v.x >= v1.x && v.z >= v1.y && v.x <= v2.x && v.z <= v2.y;
}

inline bool Box2d::IsInside(const Int2& p) const
{
	float x = (float)p.x;
	float y = (float)p.y;
	return x >= v1.x && y >= v1.y && x <= v2.x && y <= v2.y;
}

inline bool Box2d::IsValid() const
{
	return v1.x <= v2.x && v1.y <= v2.y;
}

inline Vec2 Box2d::Midpoint() const
{
	return v1 + (v2 - v1) / 2;
}

inline float Box2d::SizeX() const
{
	return abs(v2.x - v1.x);
}

inline float Box2d::SizeY() const
{
	return abs(v2.y - v1.y);
}

inline Vec2 Box2d::Size() const
{
	return Vec2(SizeX(), SizeY());
}

inline Box Box2d::ToBoxXZ(float y1, float y2) const
{
	return Box(v1.x, y1, v1.y, v2.x, y2, v2.y);
}

inline bool Box2d::Intersect(const Box2d& a, const Box2d& b, Box2d& result)
{
	float x = max(a.Left(), b.Left());
	float x2 = min(a.Right(), b.Right());
	float y = max(a.Top(), b.Top());
	float y2 = min(a.Bottom(), b.Bottom());
	if(x2 >= x && y2 >= y)
	{
		result = Box2d(x, y, x2, y2);
		return true;
	}
	else
		return false;
}

inline Box2d Box2d::Intersect(const Box2d& a, const Box2d& b)
{
	Box2d result;
	if(!Intersect(a, b, result))
		result = Box2d::Zero;
	return result;
}

//*************************************************************************************************
//
// 3d box using floats
//
//*************************************************************************************************
inline Box::Box()
{
}

inline Box::Box(float minx, float miny, float minz, float maxx, float maxy, float maxz) : v1(minx, miny, minz), v2(maxx, maxy, maxz)
{
}

inline Box::Box(const Vec3& v1, const Vec3& v2) : v1(v1), v2(v2)
{
}

inline Box::Box(const Box& box) : v1(box.v1), v2(box.v2)
{
}

inline Box::Box(float x, float y, float z) : v1(x, y, z), v2(x, y, z)
{
}

inline Box::Box(const Vec3& v) : v1(v), v2(v)
{
}

inline bool Box::operator == (const Box& b) const
{
	return v1 == b.v1 && v2 == b.v2;
}

inline bool Box::operator != (const Box& b) const
{
	return v1 != b.v1 || v2 != b.v2;
}

inline Box& Box::operator = (const Box& b)
{
	v1 = b.v1;
	v2 = b.v2;
	return *this;
}

inline Box& Box::operator += (const Vec3& v)
{
	v1 += v;
	v2 += v;
	return *this;
}

inline Box& Box::operator -= (const Vec3& v)
{
	v1 -= v;
	v2 -= v;
	return *this;
}

inline Box& Box::operator *= (float f)
{
	v1 *= f;
	v2 *= f;
	return *this;
}

inline Box& Box::operator /= (float f)
{
	v1 /= f;
	v2 /= f;
	return *this;
}

inline Box Box::operator + () const
{
	return *this;
}

inline Box Box::operator - () const
{
	return Box(-v1, -v2);
}

inline Box Box::operator + (const Vec3& v) const
{
	return Box(v1 + v, v2 + v);
}

inline Box Box::operator - (const Vec3& v) const
{
	return Box(v1 - v, v2 - v);
}

inline Box Box::operator * (float f) const
{
	return Box(v1 * f, v2 * f);
}

inline Box Box::operator / (float f) const
{
	return Box(v1 / f, v2 / f);
}

inline Box operator * (float f, const Box& b)
{
	return b * f;
}

//------------------------------------------------------------------------------
// Methods
//------------------------------------------------------------------------------

inline void Box::AddPoint(const Vec3& v)
{
	v1.x = min(v1.x, v.x);
	v1.y = min(v1.y, v.y);
	v1.z = min(v1.z, v.z);
	v2.x = max(v2.x, v.x);
	v2.y = max(v2.y, v.y);
	v2.z = max(v2.z, v.z);
}

inline Vec3 Box::GetRandomPoint() const
{
	return Vec3(::Random(v1.x, v2.x), ::Random(v1.y, v2.y), ::Random(v1.z, v2.z));
}

inline bool Box::IsInside(const Vec3& v) const
{
	return v.x >= v1.x && v.x <= v2.x && v.y >= v1.y && v.y <= v2.y && v.z >= v1.z && v.z <= v2.z;
}

inline bool Box::IsValid() const
{
	return v1.x <= v2.x && v1.y <= v2.y && v1.z <= v2.z;
}

inline Vec3 Box::Midpoint() const
{
	return v1 + (v2 - v1) / 2;
}

inline Vec3 Box::Size() const
{
	return Vec3(SizeX(), SizeY(), SizeZ());
}

inline float Box::SizeX() const
{
	return abs(v2.x - v1.x);
}

inline Vec2 Box::SizeXZ() const
{
	return Vec2(SizeX(), SizeZ());
}

inline float Box::SizeY() const
{
	return abs(v2.y - v1.y);
}

inline float Box::SizeZ() const
{
	return abs(v2.z - v1.z);
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline Box Box::Create(const Vec3& pos, const Vec3& size)
{
	return Box(pos, pos + size);
}

inline Box Box::CreateXZ(const Box2d& box2d, float y1, float y2)
{
	return Box(box2d.v1.x, y1, box2d.v1.y, box2d.v2.x, y2, box2d.v2.y);
}

//*************************************************************************************************
//
// 4x4 float matrix
//
//*************************************************************************************************
inline Matrix::Matrix()
{
}

inline Matrix::Matrix(
	float m00, float m01, float m02, float m03,
	float m10, float m11, float m12, float m13,
	float m20, float m21, float m22, float m23,
	float m30, float m31, float m32, float m33) :
	XMFLOAT4X4(
	m00, m01, m02, m03,
	m10, m11, m12, m13,
	m20, m21, m22, m23,
	m30, m31, m32, m33)
{
}

inline Matrix::Matrix(const Vec3& v1, const Vec3& v2, const Vec3& v3) : XMFLOAT4X4(
	v1.x, v1.y, v1.z, 0,
	v2.x, v2.y, v2.z, 0,
	v3.x, v3.y, v3.z, 0,
	0, 0, 0, 1.f)
{
}

inline Matrix::Matrix(const Vec4& v1, const Vec4& v2, const Vec4& v3, const Vec4& v4) : XMFLOAT4X4(
	v1.x, v1.y, v1.z, v1.w,
	v2.x, v2.y, v2.z, v2.w,
	v3.x, v3.y, v3.z, v3.w,
	v4.x, v4.y, v4.z, v4.w)
{
}

inline Matrix::Matrix(const Matrix& m) : XMFLOAT4X4(
	m._11, m._12, m._13, m._14,
	m._21, m._22, m._23, m._24,
	m._31, m._32, m._33, m._34,
	m._41, m._42, m._43, m._44)
{
}

inline Matrix::Matrix(CXMMATRIX m)
{
	XMStoreFloat4x4(this, m);
}

inline Matrix::operator XMMATRIX() const
{
	return XMLoadFloat4x4(this);
}

inline bool Matrix::operator == (const Matrix& M) const
{
	XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
	XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
	XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
	XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

	XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._11));
	XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._21));
	XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._31));
	XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._41));

	return (XMVector4Equal(x1, y1)
		&& XMVector4Equal(x2, y2)
		&& XMVector4Equal(x3, y3)
		&& XMVector4Equal(x4, y4)) != 0;
}

inline bool Matrix::operator != (const Matrix& M) const
{
	XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
	XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
	XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
	XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

	XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._11));
	XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._21));
	XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._31));
	XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._41));

	return (XMVector4NotEqual(x1, y1)
		|| XMVector4NotEqual(x2, y2)
		|| XMVector4NotEqual(x3, y3)
		|| XMVector4NotEqual(x4, y4)) != 0;
}

inline Matrix& Matrix::operator = (const Matrix& m)
{
	memcpy_s(this, sizeof(float) * 16, &m, sizeof(float) * 16);
	return *this;
}

inline Matrix& Matrix::operator += (const Matrix& M)
{
	XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
	XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
	XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
	XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

	XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._11));
	XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._21));
	XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._31));
	XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._41));

	x1 = XMVectorAdd(x1, y1);
	x2 = XMVectorAdd(x2, y2);
	x3 = XMVectorAdd(x3, y3);
	x4 = XMVectorAdd(x4, y4);

	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_11), x1);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_21), x2);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_31), x3);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_41), x4);
	return *this;
}

inline Matrix& Matrix::operator -= (const Matrix& M)
{
	XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
	XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
	XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
	XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

	XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._11));
	XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._21));
	XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._31));
	XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._41));

	x1 = XMVectorSubtract(x1, y1);
	x2 = XMVectorSubtract(x2, y2);
	x3 = XMVectorSubtract(x3, y3);
	x4 = XMVectorSubtract(x4, y4);

	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_11), x1);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_21), x2);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_31), x3);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_41), x4);
	return *this;
}

inline Matrix& Matrix::operator *= (const Matrix& M)
{
	XMMATRIX M1 = XMLoadFloat4x4(this);
	XMMATRIX M2 = XMLoadFloat4x4(&M);
	XMMATRIX X = XMMatrixMultiply(M1, M2);
	XMStoreFloat4x4(this, X);
	return *this;
}

inline Matrix& Matrix::operator *= (float S)
{
	XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
	XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
	XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
	XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

	x1 = XMVectorScale(x1, S);
	x2 = XMVectorScale(x2, S);
	x3 = XMVectorScale(x3, S);
	x4 = XMVectorScale(x4, S);

	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_11), x1);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_21), x2);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_31), x3);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_41), x4);
	return *this;
}

inline Matrix& Matrix::operator /= (float S)
{
	assert(S != 0.f);
	XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
	XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
	XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
	XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

	float rs = 1.f / S;

	x1 = XMVectorScale(x1, rs);
	x2 = XMVectorScale(x2, rs);
	x3 = XMVectorScale(x3, rs);
	x4 = XMVectorScale(x4, rs);

	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_11), x1);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_21), x2);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_31), x3);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_41), x4);
	return *this;
}

inline Matrix& Matrix::operator /= (const Matrix& M)
{
	XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
	XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
	XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
	XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

	XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._11));
	XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._21));
	XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._31));
	XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._41));

	x1 = XMVectorDivide(x1, y1);
	x2 = XMVectorDivide(x2, y2);
	x3 = XMVectorDivide(x3, y3);
	x4 = XMVectorDivide(x4, y4);

	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_11), x1);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_21), x2);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_31), x3);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&_41), x4);
	return *this;
}

inline Matrix Matrix::operator + () const
{
	return *this;
}

inline Matrix Matrix::operator - () const
{
	XMVECTOR v1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
	XMVECTOR v2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
	XMVECTOR v3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
	XMVECTOR v4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

	v1 = XMVectorNegate(v1);
	v2 = XMVectorNegate(v2);
	v3 = XMVectorNegate(v3);
	v4 = XMVectorNegate(v4);

	Matrix R;
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), v1);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), v2);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), v3);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), v4);
	return R;
}

inline Matrix Matrix::operator + (const Matrix& m) const
{
	XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
	XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
	XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
	XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

	XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&m._11));
	XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&m._21));
	XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&m._31));
	XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&m._41));

	x1 = XMVectorAdd(x1, y1);
	x2 = XMVectorAdd(x2, y2);
	x3 = XMVectorAdd(x3, y3);
	x4 = XMVectorAdd(x4, y4);

	Matrix R;
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
	return R;
}

inline Matrix Matrix::operator - (const Matrix& m) const
{
	XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
	XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
	XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
	XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

	XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&m._11));
	XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&m._21));
	XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&m._31));
	XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&m._41));

	x1 = XMVectorSubtract(x1, y1);
	x2 = XMVectorSubtract(x2, y2);
	x3 = XMVectorSubtract(x3, y3);
	x4 = XMVectorSubtract(x4, y4);

	Matrix R;
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
	return R;
}

inline Matrix Matrix::operator * (const Matrix& m) const
{
	XMMATRIX m1 = XMLoadFloat4x4(this);
	XMMATRIX m2 = XMLoadFloat4x4(&m);
	XMMATRIX X = XMMatrixMultiply(m1, m2);

	Matrix R;
	XMStoreFloat4x4(&R, X);
	return R;
}

inline Matrix Matrix::operator * (float S) const
{
	XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
	XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
	XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
	XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

	x1 = XMVectorScale(x1, S);
	x2 = XMVectorScale(x2, S);
	x3 = XMVectorScale(x3, S);
	x4 = XMVectorScale(x4, S);

	Matrix R;
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
	return R;
}

inline Matrix Matrix::operator / (float S) const
{
	assert(S != 0.f);

	XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
	XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
	XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
	XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

	float rs = 1.f / S;

	x1 = XMVectorScale(x1, rs);
	x2 = XMVectorScale(x2, rs);
	x3 = XMVectorScale(x3, rs);
	x4 = XMVectorScale(x4, rs);

	Matrix R;
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
	return R;
}

inline Matrix Matrix::operator / (const Matrix& m) const
{
	XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
	XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
	XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
	XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

	XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&m._11));
	XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&m._21));
	XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&m._31));
	XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&m._41));

	x1 = XMVectorDivide(x1, y1);
	x2 = XMVectorDivide(x2, y2);
	x3 = XMVectorDivide(x3, y3);
	x4 = XMVectorDivide(x4, y4);

	Matrix R;
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
	return R;
}

inline Matrix operator * (float S, const Matrix& M)
{
	XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._11));
	XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._21));
	XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._31));
	XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._41));

	x1 = XMVectorScale(x1, S);
	x2 = XMVectorScale(x2, S);
	x3 = XMVectorScale(x3, S);
	x4 = XMVectorScale(x4, S);

	Matrix R;
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
	return R;
}

//------------------------------------------------------------------------------
// Methods
//------------------------------------------------------------------------------

inline bool Matrix::Decompose(Vec3& scale, Quat& rotation, Vec3& translation)
{
	XMVECTOR s, r, t;

	if(!XMMatrixDecompose(&s, &r, &t, *this))
		return false;

	XMStoreFloat3(&scale, s);
	XMStoreFloat4(&rotation, r);
	XMStoreFloat3(&translation, t);

	return true;
}

inline float Matrix::Determinant() const
{
	XMMATRIX M = XMLoadFloat4x4(this);
	return XMVectorGetX(XMMatrixDeterminant(M));
}

inline float Matrix::GetYaw() const
{
	if(_21 > 0.998f || _21 < -0.998f)
		return atan2(_13, _33);
	else
		return atan2(-_31, _11);
}

inline void Matrix::Identity()
{
	*this = IdentityMatrix;
}

inline Matrix Matrix::Inverse() const
{
	XMMATRIX M = XMLoadFloat4x4(this);
	Matrix R;
	XMVECTOR det;
	XMStoreFloat4x4(&R, XMMatrixInverse(&det, M));
	return R;
}

inline void Matrix::Inverse(Matrix& result) const
{
	XMMATRIX M = XMLoadFloat4x4(this);
	XMVECTOR det;
	XMStoreFloat4x4(&result, XMMatrixInverse(&det, M));
}

inline Matrix Matrix::Transpose() const
{
	XMMATRIX M = XMLoadFloat4x4(this);
	Matrix R;
	XMStoreFloat4x4(&R, XMMatrixTranspose(M));
	return R;
}

inline void Matrix::Transpose(Matrix& result) const
{
	XMMATRIX M = XMLoadFloat4x4(this);
	XMStoreFloat4x4(&result, XMMatrixTranspose(M));
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline Matrix Matrix::CreateBillboard(const Vec3& object, const Vec3& cameraPosition, const Vec3& cameraUp, const Vec3* cameraForward)
{
	XMVECTOR O = XMLoadFloat3(&object);
	XMVECTOR C = XMLoadFloat3(&cameraPosition);
	XMVECTOR Z = XMVectorSubtract(O, C);

	XMVECTOR N = XMVector3LengthSq(Z);
	if(XMVector3Less(N, g_XMEpsilon))
	{
		if(cameraForward)
		{
			XMVECTOR F = XMLoadFloat3(cameraForward);
			Z = XMVectorNegate(F);
		}
		else
			Z = g_XMNegIdentityR2;
	}
	else
	{
		Z = XMVector3Normalize(Z);
	}

	XMVECTOR up = XMLoadFloat3(&cameraUp);
	XMVECTOR X = XMVector3Cross(up, Z);
	X = XMVector3Normalize(X);

	XMVECTOR Y = XMVector3Cross(Z, X);

	XMMATRIX M;
	M.r[0] = X;
	M.r[1] = Y;
	M.r[2] = Z;
	M.r[3] = XMVectorSetW(O, 1.f);

	Matrix R;
	XMStoreFloat4x4(&R, M);
	return R;
}

inline Matrix Matrix::CreateConstrainedBillboard(const Vec3& object, const Vec3& cameraPosition, const Vec3& rotateAxis,
	const Vec3* cameraForward, const Vec3* objectForward)
{
	static const XMVECTORF32 s_minAngle = { 0.99825467075f, 0.99825467075f, 0.99825467075f, 0.99825467075f }; // 1.0 - XMConvertToRadians( 0.1f );

	XMVECTOR O = XMLoadFloat3(&object);
	XMVECTOR C = XMLoadFloat3(&cameraPosition);
	XMVECTOR faceDir = XMVectorSubtract(O, C);

	XMVECTOR N = XMVector3LengthSq(faceDir);
	if(XMVector3Less(N, g_XMEpsilon))
	{
		if(cameraForward)
		{
			XMVECTOR F = XMLoadFloat3(cameraForward);
			faceDir = XMVectorNegate(F);
		}
		else
			faceDir = g_XMNegIdentityR2;
	}
	else
	{
		faceDir = XMVector3Normalize(faceDir);
	}

	XMVECTOR Y = XMLoadFloat3(&rotateAxis);
	XMVECTOR X, Z;

	XMVECTOR dot = XMVectorAbs(XMVector3Dot(Y, faceDir));
	if(XMVector3Greater(dot, s_minAngle))
	{
		if(objectForward)
		{
			Z = XMLoadFloat3(objectForward);
			dot = XMVectorAbs(XMVector3Dot(Y, Z));
			if(XMVector3Greater(dot, s_minAngle))
			{
				dot = XMVectorAbs(XMVector3Dot(Y, g_XMNegIdentityR2));
				Z = (XMVector3Greater(dot, s_minAngle)) ? g_XMIdentityR0 : g_XMNegIdentityR2;
			}
		}
		else
		{
			dot = XMVectorAbs(XMVector3Dot(Y, g_XMNegIdentityR2));
			Z = (XMVector3Greater(dot, s_minAngle)) ? g_XMIdentityR0 : g_XMNegIdentityR2;
		}

		X = XMVector3Cross(Y, Z);
		X = XMVector3Normalize(X);

		Z = XMVector3Cross(X, Y);
		Z = XMVector3Normalize(Z);
	}
	else
	{
		X = XMVector3Cross(Y, faceDir);
		X = XMVector3Normalize(X);

		Z = XMVector3Cross(X, Y);
		Z = XMVector3Normalize(Z);
	}

	XMMATRIX M;
	M.r[0] = X;
	M.r[1] = Y;
	M.r[2] = Z;
	M.r[3] = XMVectorSetW(O, 1.f);

	Matrix R;
	XMStoreFloat4x4(&R, M);
	return R;
}

inline Matrix Matrix::CreateFromAxes(const Vec3& axisX, const Vec3& axisY, const Vec3& axisZ)
{
	return Matrix(
		axisX.x, axisX.y, axisX.z, 0.0f,
		axisY.x, axisY.y, axisY.z, 0.0f,
		axisZ.x, axisZ.y, axisZ.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);
}

inline Matrix Matrix::CreateFromAxisAngle(const Vec3& axis, float angle)
{
	Matrix R;
	XMVECTOR a = XMLoadFloat3(&axis);
	XMStoreFloat4x4(&R, XMMatrixRotationAxis(a, angle));
	return R;
}

inline Matrix Matrix::CreatePerspectiveFieldOfView(float fov, float aspectRatio, float nearPlane, float farPlane)
{
	Matrix R;
	XMStoreFloat4x4(&R, XMMatrixPerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane));
	return R;
}

inline Matrix Matrix::CreatePerspective(float width, float height, float nearPlane, float farPlane)
{
	Matrix R;
	XMStoreFloat4x4(&R, XMMatrixPerspectiveLH(width, height, nearPlane, farPlane));
	return R;
}

inline Matrix Matrix::CreatePerspectiveOffCenter(float left, float right, float bottom, float top, float nearPlane, float farPlane)
{
	Matrix R;
	XMStoreFloat4x4(&R, XMMatrixPerspectiveOffCenterLH(left, right, bottom, top, nearPlane, farPlane));
	return R;
}

inline Matrix Matrix::CreateOrthographic(float width, float height, float zNearPlane, float zFarPlane)
{
	Matrix R;
	XMStoreFloat4x4(&R, XMMatrixOrthographicLH(width, height, zNearPlane, zFarPlane));
	return R;
}

inline Matrix Matrix::CreateOrthographicOffCenter(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane)
{
	Matrix R;
	XMStoreFloat4x4(&R, XMMatrixOrthographicOffCenterLH(left, right, bottom, top, zNearPlane, zFarPlane));
	return R;
}

inline Matrix Matrix::CreateLookAt(const Vec3& eye, const Vec3& target, const Vec3& up)
{
	Matrix R;
	XMVECTOR eyev = XMLoadFloat3(&eye);
	XMVECTOR targetv = XMLoadFloat3(&target);
	XMVECTOR upv = XMLoadFloat3(&up);
	XMStoreFloat4x4(&R, XMMatrixLookAtLH(eyev, targetv, upv));
	return R;
}

inline Matrix Matrix::CreateWorld(const Vec3& position, const Vec3& forward, const Vec3& up)
{
	XMVECTOR zaxis = XMVector3Normalize(XMVectorNegate(XMLoadFloat3(&forward)));
	XMVECTOR yaxis = XMLoadFloat3(&up);
	XMVECTOR xaxis = XMVector3Normalize(XMVector3Cross(yaxis, zaxis));
	yaxis = XMVector3Cross(zaxis, xaxis);

	Matrix R;
	XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&R._11), xaxis);
	XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&R._21), yaxis);
	XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&R._31), zaxis);
	R._14 = R._24 = R._34 = 0.f;
	R._41 = position.x; R._42 = position.y; R._43 = position.z;
	R._44 = 1.f;
	return R;
}

inline Matrix Matrix::CreateShadow(const Vec3& lightDir, const Plane& plane)
{
	Matrix R;
	XMVECTOR light = XMLoadFloat3(&lightDir);
	XMVECTOR planev = XMLoadFloat4(&plane);
	XMStoreFloat4x4(&R, XMMatrixShadow(planev, light));
	return R;
}

inline Matrix Matrix::CreateReflection(const Plane& plane)
{
	Matrix R;
	XMVECTOR planev = XMLoadFloat4(&plane);
	XMStoreFloat4x4(&R, XMMatrixReflect(planev));
	return R;
}

inline void Matrix::Lerp(const Matrix& M1, const Matrix& M2, float t, Matrix& result)
{
	XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._11));
	XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._21));
	XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._31));
	XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._41));

	XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._11));
	XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._21));
	XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._31));
	XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._41));

	x1 = XMVectorLerp(x1, y1, t);
	x2 = XMVectorLerp(x2, y2, t);
	x3 = XMVectorLerp(x3, y3, t);
	x4 = XMVectorLerp(x4, y4, t);

	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._11), x1);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._21), x2);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._31), x3);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._41), x4);
}

inline Matrix Matrix::Lerp(const Matrix& M1, const Matrix& M2, float t)
{
	XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._11));
	XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._21));
	XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._31));
	XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M1._41));

	XMVECTOR y1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._11));
	XMVECTOR y2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._21));
	XMVECTOR y3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._31));
	XMVECTOR y4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M2._41));

	x1 = XMVectorLerp(x1, y1, t);
	x2 = XMVectorLerp(x2, y2, t);
	x3 = XMVectorLerp(x3, y3, t);
	x4 = XMVectorLerp(x4, y4, t);

	Matrix result;
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._11), x1);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._21), x2);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._31), x3);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._41), x4);
	return result;
}

inline Matrix Matrix::Rotation(float yaw, float pitch, float roll)
{
	Matrix R;
	XMStoreFloat4x4(&R, XMMatrixRotationRollPitchYaw(pitch, yaw, roll));
	return R;
}

inline Matrix Matrix::Rotation(const Vec3& v)
{
	return Rotation(v.y, v.x, v.z);
}

inline Matrix Matrix::Rotation(const Quat& rotation)
{
	Matrix R;
	XMVECTOR quatv = XMLoadFloat4(&rotation);
	XMStoreFloat4x4(&R, XMMatrixRotationQuaternion(quatv));
	return R;
}

inline Matrix Matrix::RotationX(float radians)
{
	Matrix R;
	XMStoreFloat4x4(&R, XMMatrixRotationX(radians));
	return R;
}

inline Matrix Matrix::RotationY(float radians)
{
	Matrix R;
	XMStoreFloat4x4(&R, XMMatrixRotationY(radians));
	return R;
}

inline Matrix Matrix::RotationZ(float radians)
{
	Matrix R;
	XMStoreFloat4x4(&R, XMMatrixRotationZ(radians));
	return R;
}

inline Matrix Matrix::Scale(const Vec3& scales)
{
	Matrix R;
	XMStoreFloat4x4(&R, XMMatrixScaling(scales.x, scales.y, scales.z));
	return R;
}

inline Matrix Matrix::Scale(float xs, float ys, float zs)
{
	Matrix R;
	XMStoreFloat4x4(&R, XMMatrixScaling(xs, ys, zs));
	return R;
}

inline Matrix Matrix::Scale(float scale)
{
	Matrix R;
	XMStoreFloat4x4(&R, XMMatrixScaling(scale, scale, scale));
	return R;
}

inline void Matrix::Transform(const Matrix& M, const Quat& rotation, Matrix& result)
{
	XMVECTOR quatv = XMLoadFloat4(&rotation);

	XMMATRIX M0 = XMLoadFloat4x4(&M);
	XMMATRIX M1 = XMMatrixRotationQuaternion(quatv);

	XMStoreFloat4x4(&result, XMMatrixMultiply(M0, M1));
}

inline Matrix Matrix::Transform(const Matrix& M, const Quat& rotation)
{
	XMVECTOR quatv = XMLoadFloat4(&rotation);

	XMMATRIX M0 = XMLoadFloat4x4(&M);
	XMMATRIX M1 = XMMatrixRotationQuaternion(quatv);

	Matrix result;
	XMStoreFloat4x4(&result, XMMatrixMultiply(M0, M1));
	return result;
}

inline Matrix Matrix::Transform2D(const Vec2* scaling_center, float scaling_rotation, const Vec2* scaling, const Vec2* rotation_center, float rotation, const Vec2* translation)
{
	XMVECTOR m_scaling_center = scaling_center ? *scaling_center : Vec2(0, 0),
		m_scaling = scaling ? *scaling : Vec2(1, 1),
		m_rotation_center = rotation_center ? *rotation_center : Vec2(0, 0),
		m_translation = translation ? *translation : Vec2(0, 0);
	return XMMatrixTransformation2D(m_scaling_center, scaling_rotation, m_scaling, m_rotation_center, rotation, m_translation);
}

inline Matrix Matrix::Translation(const Vec3& position)
{
	Matrix R;
	XMStoreFloat4x4(&R, XMMatrixTranslation(position.x, position.y, position.z));
	return R;
}

inline Matrix Matrix::Translation(float x, float y, float z)
{
	Matrix R;
	XMStoreFloat4x4(&R, XMMatrixTranslation(x, y, z));
	return R;
}

//*************************************************************************************************
//
// Quaternion
//
//*************************************************************************************************
inline Quat::Quat()
{
}

inline Quat::Quat(float x, float y, float z, float w) : XMFLOAT4(x, y, z, w)
{
}

inline Quat::Quat(const Vec3& v, float w) : XMFLOAT4(v.x, v.y, v.z, w)
{
}

inline Quat::Quat(const Quat& q) : XMFLOAT4(q.x, q.y, q.z, q.w)
{
}

inline Quat::Quat(FXMVECTOR v)
{
	XMStoreFloat4(this, v);
}

inline Quat::Quat(const Vec4& v) : XMFLOAT4(v.x, v.y, v.z, v.w)
{
}

inline Quat::Quat(const XMVECTORF32& v) : XMFLOAT4(v.f[0], v.f[1], v.f[2], v.f[3])
{
}

inline Quat::operator XMVECTOR() const
{
	return XMLoadFloat4(this);
}

inline bool Quat::operator == (const Quat& q) const
{
	XMVECTOR q1 = XMLoadFloat4(this);
	XMVECTOR q2 = XMLoadFloat4(&q);
	return XMQuaternionEqual(q1, q2);
}

inline bool Quat::operator != (const Quat& q) const
{
	XMVECTOR q1 = XMLoadFloat4(this);
	XMVECTOR q2 = XMLoadFloat4(&q);
	return XMQuaternionNotEqual(q1, q2);
}

inline Quat& Quat::operator = (const Quat& q)
{
	x = q.x;
	y = q.y;
	z = q.z;
	w = q.w;
	return *this;
}

inline Quat& Quat::operator = (const XMVECTORF32& v)
{
	x = v.f[0];
	y = v.f[1];
	z = v.f[2];
	w = v.f[3];
	return *this;
}

inline Quat& Quat::operator += (const Quat& q)
{
	XMVECTOR q1 = XMLoadFloat4(this);
	XMVECTOR q2 = XMLoadFloat4(&q);
	XMStoreFloat4(this, XMVectorAdd(q1, q2));
	return *this;
}

inline Quat& Quat::operator -= (const Quat& q)
{
	XMVECTOR q1 = XMLoadFloat4(this);
	XMVECTOR q2 = XMLoadFloat4(&q);
	XMStoreFloat4(this, XMVectorSubtract(q1, q2));
	return *this;
}

inline Quat& Quat::operator *= (const Quat& q)
{
	XMVECTOR q1 = XMLoadFloat4(this);
	XMVECTOR q2 = XMLoadFloat4(&q);
	XMStoreFloat4(this, XMQuaternionMultiply(q1, q2));
	return *this;
}

inline Quat& Quat::operator *= (float S)
{
	XMVECTOR q = XMLoadFloat4(this);
	XMStoreFloat4(this, XMVectorScale(q, S));
	return *this;
}

inline Quat& Quat::operator /= (const Quat& q)
{
	XMVECTOR q1 = XMLoadFloat4(this);
	XMVECTOR q2 = XMLoadFloat4(&q);
	q2 = XMQuaternionInverse(q2);
	XMStoreFloat4(this, XMQuaternionMultiply(q1, q2));
	return *this;
}

inline Quat Quat::operator + () const
{
	return *this;
}

inline Quat Quat::operator - () const
{
	XMVECTOR q = XMLoadFloat4(this);

	Quat R;
	XMStoreFloat4(&R, XMVectorNegate(q));
	return R;
}

inline Quat Quat::operator + (const Quat& q) const
{
	XMVECTOR q1 = XMLoadFloat4(this);
	XMVECTOR q2 = XMLoadFloat4(&q);

	Quat R;
	XMStoreFloat4(&R, XMVectorAdd(q1, q2));
	return R;
}

inline Quat Quat::operator - (const Quat& q) const
{
	XMVECTOR q1 = XMLoadFloat4(this);
	XMVECTOR q2 = XMLoadFloat4(&q);

	Quat R;
	XMStoreFloat4(&R, XMVectorSubtract(q1, q2));
	return R;
}

inline Quat Quat::operator * (const Quat& q) const
{
	XMVECTOR q1 = XMLoadFloat4(this);
	XMVECTOR q2 = XMLoadFloat4(&q);

	Quat R;
	XMStoreFloat4(&R, XMQuaternionMultiply(q1, q2));
	return R;
}

inline Quat Quat::operator * (float s) const
{
	XMVECTOR q = XMLoadFloat4(this);

	Quat R;
	XMStoreFloat4(&R, XMVectorScale(q, s));
	return R;
}

inline Quat Quat::operator / (const Quat& q) const
{
	XMVECTOR q1 = XMLoadFloat4(this);
	XMVECTOR q2 = XMLoadFloat4(&q);
	q2 = XMQuaternionInverse(q2);

	Quat R;
	XMStoreFloat4(&R, XMQuaternionMultiply(q1, q2));
	return R;
}

inline Quat operator* (float s, const Quat& q)
{
	XMVECTOR q1 = XMLoadFloat4(&q);

	Quat R;
	XMStoreFloat4(&R, XMVectorScale(q1, s));
	return R;
}

//------------------------------------------------------------------------------
// Methods
//------------------------------------------------------------------------------

inline void Quat::Conjugate()
{
	XMVECTOR q = XMLoadFloat4(this);
	XMStoreFloat4(this, XMQuaternionConjugate(q));
}

inline void Quat::Conjugate(Quat& result) const
{
	XMVECTOR q = XMLoadFloat4(this);
	XMStoreFloat4(&result, XMQuaternionConjugate(q));
}

inline float Quat::Dot(const Quat& q) const
{
	XMVECTOR q1 = XMLoadFloat4(this);
	XMVECTOR q2 = XMLoadFloat4(&q);
	return XMVectorGetX(XMQuaternionDot(q1, q2));
}

inline void Quat::Inverse(Quat& result) const
{
	XMVECTOR q = XMLoadFloat4(this);
	XMStoreFloat4(&result, XMQuaternionInverse(q));
}

inline float Quat::Length() const
{
	XMVECTOR q = XMLoadFloat4(this);
	return XMVectorGetX(XMQuaternionLength(q));
}

inline float Quat::LengthSquared() const
{
	XMVECTOR q = XMLoadFloat4(this);
	return XMVectorGetX(XMQuaternionLengthSq(q));
}

inline void Quat::Normalize()
{
	XMVECTOR q = XMLoadFloat4(this);
	XMStoreFloat4(this, XMQuaternionNormalize(q));
}

inline void Quat::Normalize(Quat& result) const
{
	XMVECTOR q = XMLoadFloat4(this);
	XMStoreFloat4(&result, XMQuaternionNormalize(q));
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline void Quat::Concatenate(const Quat& q1, const Quat& q2, Quat& result)
{
	XMVECTOR Q0 = XMLoadFloat4(&q1);
	XMVECTOR Q1 = XMLoadFloat4(&q2);
	XMStoreFloat4(&result, XMQuaternionMultiply(Q1, Q0));
}

inline Quat Quat::Concatenate(const Quat& q1, const Quat& q2)
{
	XMVECTOR Q0 = XMLoadFloat4(&q1);
	XMVECTOR Q1 = XMLoadFloat4(&q2);

	Quat result;
	XMStoreFloat4(&result, XMQuaternionMultiply(Q1, Q0));
	return result;
}

inline Quat Quat::CreateFromAxisAngle(const Vec3& axis, float angle)
{
	XMVECTOR a = XMLoadFloat3(&axis);

	Quat R;
	XMStoreFloat4(&R, XMQuaternionRotationAxis(a, angle));
	return R;
}

inline Quat Quat::CreateFromRotationMatrix(const Matrix& M)
{
	XMMATRIX M0 = XMLoadFloat4x4(&M);

	Quat R;
	XMStoreFloat4(&R, XMQuaternionRotationMatrix(M0));
	return R;
}

inline Quat Quat::CreateFromYawPitchRoll(float yaw, float pitch, float roll)
{
	Quat R;
	XMStoreFloat4(&R, XMQuaternionRotationRollPitchYaw(pitch, yaw, roll));
	return R;
}

inline void Quat::Lerp(const Quat& q1, const Quat& q2, float t, Quat& result)
{
	XMVECTOR Q0 = XMLoadFloat4(&q1);
	XMVECTOR Q1 = XMLoadFloat4(&q2);

	XMVECTOR dot = XMVector4Dot(Q0, Q1);

	XMVECTOR R;
	if(XMVector4GreaterOrEqual(dot, XMVectorZero()))
	{
		R = XMVectorLerp(Q0, Q1, t);
	}
	else
	{
		XMVECTOR tv = XMVectorReplicate(t);
		XMVECTOR t1v = XMVectorReplicate(1.f - t);
		XMVECTOR X0 = XMVectorMultiply(Q0, t1v);
		XMVECTOR X1 = XMVectorMultiply(Q1, tv);
		R = XMVectorSubtract(X0, X1);
	}

	XMStoreFloat4(&result, XMQuaternionNormalize(R));
}

inline Quat Quat::Lerp(const Quat& q1, const Quat& q2, float t)
{
	XMVECTOR Q0 = XMLoadFloat4(&q1);
	XMVECTOR Q1 = XMLoadFloat4(&q2);

	XMVECTOR dot = XMVector4Dot(Q0, Q1);

	XMVECTOR R;
	if(XMVector4GreaterOrEqual(dot, XMVectorZero()))
	{
		R = XMVectorLerp(Q0, Q1, t);
	}
	else
	{
		XMVECTOR tv = XMVectorReplicate(t);
		XMVECTOR t1v = XMVectorReplicate(1.f - t);
		XMVECTOR X0 = XMVectorMultiply(Q0, t1v);
		XMVECTOR X1 = XMVectorMultiply(Q1, tv);
		R = XMVectorSubtract(X0, X1);
	}

	Quat result;
	XMStoreFloat4(&result, XMQuaternionNormalize(R));
	return result;
}

inline void Quat::Slerp(const Quat& q1, const Quat& q2, float t, Quat& result)
{
	XMVECTOR Q0 = XMLoadFloat4(&q1);
	XMVECTOR Q1 = XMLoadFloat4(&q2);
	XMStoreFloat4(&result, XMQuaternionSlerp(Q0, Q1, t));
}

inline Quat Quat::Slerp(const Quat& q1, const Quat& q2, float t)
{
	XMVECTOR Q0 = XMLoadFloat4(&q1);
	XMVECTOR Q1 = XMLoadFloat4(&q2);

	Quat result;
	XMStoreFloat4(&result, XMQuaternionSlerp(Q0, Q1, t));
	return result;
}

//*************************************************************************************************
//
// Quaternion
//
//*************************************************************************************************
inline Plane::Plane()
{
}

inline Plane::Plane(float x, float y, float z, float w) : XMFLOAT4(x, y, z, w)
{
}

inline Plane::Plane(const Vec3& normal, float d) : XMFLOAT4(normal.x, normal.y, normal.z, d)
{
}

inline Plane::Plane(const Vec3& point1, const Vec3& point2, const Vec3& point3)
{
	XMVECTOR P0 = XMLoadFloat3(&point1);
	XMVECTOR P1 = XMLoadFloat3(&point2);
	XMVECTOR P2 = XMLoadFloat3(&point3);
	XMStoreFloat4(this, XMPlaneFromPoints(P0, P1, P2));
}

inline Plane::Plane(const Vec3& point, const Vec3& normal)
{
	XMVECTOR P = XMLoadFloat3(&point);
	XMVECTOR N = XMLoadFloat3(&normal);
	XMStoreFloat4(this, XMPlaneFromPointNormal(P, N));
}

inline Plane::Plane(FXMVECTOR v)
{
	XMStoreFloat4(this, v);
}

inline Plane::Plane(const Vec4& v) : XMFLOAT4(v.x, v.y, v.z, v.w)
{
}

inline Plane::Plane(const XMVECTORF32& v) : XMFLOAT4(v.f[0], v.f[1], v.f[2], v.f[3])
{
}

inline Plane::operator XMVECTOR() const
{
	return XMLoadFloat4(this);
}

inline bool Plane::operator == (const Plane& p) const
{
	XMVECTOR p1 = XMLoadFloat4(this);
	XMVECTOR p2 = XMLoadFloat4(&p);
	return XMPlaneEqual(p1, p2);
}

inline bool Plane::operator != (const Plane& p) const
{
	XMVECTOR p1 = XMLoadFloat4(this);
	XMVECTOR p2 = XMLoadFloat4(&p);
	return XMPlaneNotEqual(p1, p2);
}

inline Plane& Plane::operator = (const Plane& p)
{
	x = p.x;
	y = p.y;
	z = p.z;
	w = p.w;
	return *this;
}

//------------------------------------------------------------------------------
// Methods
//------------------------------------------------------------------------------

inline void Plane::Normalize()
{
	XMVECTOR p = XMLoadFloat4(this);
	XMStoreFloat4(this, XMPlaneNormalize(p));
}

inline void Plane::Normalize(Plane& result) const
{
	XMVECTOR p = XMLoadFloat4(this);
	XMStoreFloat4(&result, XMPlaneNormalize(p));
}

inline float Plane::Dot(const Vec4& v) const
{
	XMVECTOR p = XMLoadFloat4(this);
	XMVECTOR v0 = XMLoadFloat4(&v);
	return XMVectorGetX(XMPlaneDot(p, v0));
}

inline float Plane::DotCoordinate(const Vec3& position) const
{
	XMVECTOR p = XMLoadFloat4(this);
	XMVECTOR v0 = XMLoadFloat3(&position);
	return XMVectorGetX(XMPlaneDotCoord(p, v0));
}

inline float Plane::DotNormal(const Vec3& normal) const
{
	XMVECTOR p = XMLoadFloat4(this);
	XMVECTOR n0 = XMLoadFloat3(&normal);
	return XMVectorGetX(XMPlaneDotNormal(p, n0));
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline void Plane::Transform(const Plane& plane, const Matrix& M, Plane& result)
{
	XMVECTOR p = XMLoadFloat4(&plane);
	XMMATRIX m0 = XMLoadFloat4x4(&M);
	XMStoreFloat4(&result, XMPlaneTransform(p, m0));
}

inline Plane Plane::Transform(const Plane& plane, const Matrix& M)
{
	XMVECTOR p = XMLoadFloat4(&plane);
	XMMATRIX m0 = XMLoadFloat4x4(&M);

	Plane result;
	XMStoreFloat4(&result, XMPlaneTransform(p, m0));
	return result;
}

inline void Plane::Transform(const Plane& plane, const Quat& rotation, Plane& result)
{
	XMVECTOR p = XMLoadFloat4(&plane);
	XMVECTOR q = XMLoadFloat4(&rotation);
	XMVECTOR X = XMVector3Rotate(p, q);
	X = XMVectorSelect(p, X, g_XMSelect1110); // result.d = plane.d
	XMStoreFloat4(&result, X);
}

inline Plane Plane::Transform(const Plane& plane, const Quat& rotation)
{
	XMVECTOR p = XMLoadFloat4(&plane);
	XMVECTOR q = XMLoadFloat4(&rotation);
	XMVECTOR X = XMVector3Rotate(p, q);
	X = XMVectorSelect(p, X, g_XMSelect1110); // result.d = plane.d

	Plane result;
	XMStoreFloat4(&result, X);
	return result;
}
