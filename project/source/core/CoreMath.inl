#pragma once

//*************************************************************************************************
//
// 2D int point
//
//*************************************************************************************************
inline INT2::INT2()
{
}

inline INT2::INT2(int x, int y) : x(x), y(y)
{
}

inline INT2::INT2(const INT2& i) : x(i.x), y(i.y)
{
}

template<typename T, typename T2>
inline INT2::INT2(T x, T2 y) : x((int)x), y((int)y)
{
}

inline INT2::INT2(int xy) : x(xy), y(xy)
{
}

inline INT2::INT2(const VEC2& v) : x(int(v.x)), y(int(v.y))
{
}

inline INT2::INT2(const VEC3& v) : x(int(v.x)), y(int(v.z))
{
}

inline bool INT2::operator == (const INT2& i) const
{
	return (x == i.x && y == i.y);
}

inline bool INT2::operator != (const INT2& i) const
{
	return (x != i.x || y != i.y);
}

inline INT2& INT2::operator = (const INT2& i)
{
	x = i.x;
	y = i.y;
	return *this;
}

inline void INT2::operator += (const INT2& i)
{
	x += i.x;
	y += i.y;
}

inline void INT2::operator -= (const INT2& i)
{
	x -= i.x;
	y -= i.y;
}

inline void INT2::operator *= (int a)
{
	x *= a;
	y *= a;
}

inline void INT2::operator /= (int a)
{
	x /= a;
	y /= a;
}

inline INT2 INT2::operator + () const
{
	return *this;
}

inline INT2 INT2::operator - () const
{
	return INT2(-x, -y);
}

inline int INT2::operator ()(int shift) const
{
	return x + y * shift;
}

inline INT2 INT2::operator + (const INT2& xy) const
{
	return INT2(x + xy.x, y + xy.y);
}

inline INT2 INT2::operator - (const INT2& xy) const
{
	return INT2(x - xy.x, y - xy.y);
}

inline INT2 INT2::operator * (int a) const
{
	return INT2(x*a, y*a);
}

inline INT2 INT2::operator * (float a) const
{
	return INT2(int(a*x), int(a*y));
}

inline INT2 INT2::operator / (int a) const
{
	return INT2(x / a, y / a);
}

inline INT2 operator * (int a, const INT2& i)
{
	return i * a;
}

//------------------------------------------------------------------------------
// Methods
//------------------------------------------------------------------------------

inline int INT2::Clamp(int d) const
{
	if(d > y)
		return y;
	else if(d < x)
		return x;
	else
		return d;
}

inline int INT2::Lerp(float t) const
{
	return int(t*(y - x)) + x;
}

inline int INT2::Random() const
{
	return ::Random(x, y);
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline int INT2::Distance(const INT2& i1, const INT2& i2)
{
	return abs(i1.x - i2.x) + abs(i1.y - i2.y);
}

inline INT2 INT2::Lerp(const INT2& i1, const INT2& i2, float t)
{
	return INT2((int)::Lerp(float(i1.x), float(i2.x), t), (int)::Lerp(float(i1.y), float(i2.y), t));
}

inline INT2 INT2::Max(const INT2& i1, const INT2& i2)
{
	return INT2(max(i1.x, i2.x), max(i1.y, i2.y));
}

inline INT2 INT2::Min(const INT2& i1, const INT2& i2)
{
	return INT2(min(i1.x, i2.x), min(i1.y, i2.y));
}

inline INT2 INT2::Random(const INT2& i1, const INT2& i2)
{
	return INT2(::Random(i1.x, i2.x), ::Random(i1.y, i2.y));
}

//*************************************************************************************************
//
// Rectangle using int
//
//*************************************************************************************************
inline Rect::Rect()
{
}

inline Rect::Rect(int x1, int y1, int x2, int y2) : p1(x1, y1), p2(x2, y2)
{
}

inline Rect::Rect(const INT2& p1, const INT2& p2) : p1(p1), p2(p2)
{
}

inline Rect::Rect(const Rect& box) : p1(box.p1), p2(box.p2)
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

inline Rect& Rect::operator += (const INT2& p)
{
	p1 += p;
	p2 += p;
	return *this;
}

inline Rect& Rect::operator -= (const INT2& p)
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

inline Rect Rect::operator + (const INT2& p) const
{
	return Rect(p1 + p, p2 + p);
}

inline Rect Rect::operator - (const INT2& p) const
{
	return Rect(p1 - p, p2 - p);
}

inline Rect Rect::operator * (int d) const
{
	return Rect(p1 * d, p2 * d);
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

inline Rect Rect::LeftBottomPart() const
{
	return Rect(p1.x, p1.y, p1.x + (p2.x - p1.x) / 2, p1.y + (p2.y - p1.y) / 2);
}

inline Rect Rect::LeftTopPart() const
{
	return Rect(p1.x, p1.y + (p2.y - p1.y) / 2, p1.x + (p2.x - p1.x) / 2, p2.y);
}

inline INT2 Rect::Random() const
{
	return INT2(::Random(p1.x, p2.x), ::Random(p1.y, p2.y));
}

inline Rect Rect::RightBottomPart() const
{
	return Rect(p1.x + (p2.x - p1.x) / 2, p1.y, p2.x, p1.y + (p2.y - p1.y) / 2);
}

inline Rect Rect::RightTopPart() const
{
	return Rect(p1.x + (p2.x - p1.x) / 2, p1.y + (p2.y - p1.y) / 2, p2.x, p2.y);
}

inline void Rect::Set(const INT2& pos, const INT2& size)
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

inline INT2 Rect::Size() const
{
	return INT2(p2.x - p1.x, p2.y - p1.y);
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline Rect Rect::Create(const INT2& pos, const INT2& size)
{
	Rect box;
	box.Set(pos, size);
	return box;
}

//*************************************************************************************************
//
// 2D float point
//
//*************************************************************************************************
inline VEC2::VEC2()
{
}

inline VEC2::VEC2(float x, float y) : XMFLOAT2(x, y)
{
}

inline VEC2::VEC2(const VEC2& v) : XMFLOAT2(v.x, v.y)
{
}

inline VEC2::VEC2(FXMVECTOR v)
{
	XMStoreFloat2(this, v);
}

inline VEC2::VEC2(float xy) : XMFLOAT2(xy, xy)
{
}

inline VEC2::VEC2(const INT2& i) : XMFLOAT2(float(i.x), float(i.y))
{
}

inline VEC2::VEC2(const XMVECTORF32& v) : XMFLOAT2(v.f[0], v.f[1])
{
}

inline VEC2::operator XMVECTOR() const
{
	return XMLoadFloat2(this);
}

inline VEC2::operator float*()
{
	return &x;
}

inline VEC2::operator const float*() const
{
	return &x;
}

inline bool VEC2::operator == (const VEC2& v) const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR v2 = XMLoadFloat2(&v);
	return XMVector2Equal(v1, v2);
}

inline bool VEC2::operator != (const VEC2& v) const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR v2 = XMLoadFloat2(&v);
	return XMVector2NotEqual(v1, v2);
}

inline VEC2& VEC2::operator = (const VEC2& v)
{
	x = v.x;
	y = v.y;
	return *this;
}

inline VEC2& VEC2::operator = (const XMVECTORF32& v)
{
	x = v.f[0];
	y = v.f[1];
	return *this;
}

inline VEC2& VEC2::operator += (const VEC2& v)
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR v2 = XMLoadFloat2(&v);
	XMVECTOR r = XMVectorAdd(v1, v2);
	XMStoreFloat2(this, r);
	return *this;
}

inline VEC2& VEC2::operator -= (const VEC2& v)
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR v2 = XMLoadFloat2(&v);
	XMVECTOR r = XMVectorSubtract(v1, v2);
	XMStoreFloat2(this, r);
	return *this;
}

inline VEC2& VEC2::operator *= (float s)
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR r = XMVectorScale(v1, s);
	XMStoreFloat2(this, r);
	return *this;
}

inline VEC2& VEC2::operator /= (float s)
{
	assert(s != 0.f);
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR r = XMVectorScale(v1, 1.f / s);
	XMStoreFloat2(this, r);
	return *this;
}

inline VEC2 VEC2::operator + () const
{
	return *this;
}

inline VEC2 VEC2::operator - () const
{
	return VEC2(-x, -y);
}

inline VEC2 VEC2::operator + (const VEC2& v) const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR v2 = XMLoadFloat2(&v);
	XMVECTOR r = XMVectorAdd(v1, v2);
	VEC2 result;
	XMStoreFloat2(&result, r);
	return result;
}

inline VEC2 VEC2::operator - (const VEC2& v) const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR v2 = XMLoadFloat2(&v);
	XMVECTOR r = XMVectorSubtract(v1, v2);
	VEC2 result;
	XMStoreFloat2(&result, r);
	return result;
}

inline VEC2 VEC2::operator * (float s) const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR r = XMVectorScale(v1, s);
	VEC2 result;
	XMStoreFloat2(&result, r);
	return result;
}

inline VEC2 VEC2::operator / (float s) const
{
	assert(s != 0.f);
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR r = XMVectorScale(v1, 1.f / s);
	VEC2 result;
	XMStoreFloat2(&result, r);
	return result;
}

inline VEC2 operator * (float s, const VEC2& v)
{
	XMVECTOR v1 = XMLoadFloat2(&v);
	XMVECTOR r = XMVectorScale(v1, s);
	VEC2 result;
	XMStoreFloat2(&result, r);
	return result;
}

//------------------------------------------------------------------------------
// Methods
//------------------------------------------------------------------------------

inline float VEC2::Clamp(float f) const
{
	if(f > y)
		return y;
	else if(f < x)
		return x;
	else
		return f;
}

inline void VEC2::Clamp(const VEC2& min, const VEC2& max)
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR v2 = XMLoadFloat2(&min);
	XMVECTOR v3 = XMLoadFloat2(&max);
	XMVECTOR r = XMVectorClamp(v1, v2, v3);
	XMStoreFloat2(this, r);
}

inline void VEC2::Clamp(const VEC2& min, const VEC2& max, VEC2& result) const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR v2 = XMLoadFloat2(&min);
	XMVECTOR v3 = XMLoadFloat2(&max);
	XMVECTOR r = XMVectorClamp(v1, v2, v3);
	XMStoreFloat2(&result, r);
}

inline VEC2 VEC2::Clamped(const VEC2& min, const VEC2& max) const
{
	VEC2 result;
	Clamp(min, max, result);
	return result;
}

inline VEC2 VEC2::Clip() const
{
	return VEC2(::Clip(x), ::Clip(y));
}

inline void VEC2::Cross(const VEC2& v, VEC2& result) const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR v2 = XMLoadFloat2(&v);
	XMVECTOR r = XMVector2Cross(v1, v2);
	XMStoreFloat2(&result, r);
}

inline VEC2 VEC2::Cross(const VEC2& v) const
{
	VEC2 result;
	Cross(v, result);
	return result;
}

inline float VEC2::Dot(const VEC2& v) const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR v2 = XMLoadFloat2(&v);
	XMVECTOR r = XMVector2Dot(v1, v2);
	return XMVectorGetX(r);
}

inline float VEC3::DotSelf() const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR X = XMVector2Dot(v1, v1);
	return XMVectorGetX(X);
}

inline bool VEC2::Equal(const VEC2& v) const
{
	return ::Equal(x, v.x) && ::Equal(y, v.y);
}

inline bool VEC2::InBounds(const VEC2& bounds) const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR v2 = XMLoadFloat2(&bounds);
	return XMVector2InBounds(v1, v2);
}

inline float VEC2::Length() const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR r = XMVector2Length(v1);
	return XMVectorGetX(r);
}

inline float VEC2::LengthSquared() const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR r = XMVector2LengthSq(v1);
	return XMVectorGetX(r);
}

inline VEC2& VEC2::Normalize()
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR r = XMVector2Normalize(v1);
	XMStoreFloat2(this, r);
	return *this;
}

inline void VEC2::Normalize(VEC2& v) const
{
	XMVECTOR v1 = XMLoadFloat2(this);
	XMVECTOR r = XMVector2Normalize(v1);
	XMStoreFloat2(&v, r);
}

inline VEC2 VEC2::Normalized() const
{
	VEC2 result;
	Normalize(result);
	return result;
}

inline float VEC2::Random() const
{
	return ::Random(x, y);
}

inline VEC3 VEC2::XZ(float _y) const
{
	return VEC3(x, _y, y);
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline float VEC2::Angle(const VEC2& v1, const VEC2& v2)
{
	return ::Angle(v1.x, v1.y, v2.x, v2.y);
}

inline void VEC2::Barycentric(const VEC2& v1, const VEC2& v2, const VEC2& v3, float f, float g, VEC2& result)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR x3 = XMLoadFloat2(&v3);
	XMVECTOR X = XMVectorBaryCentric(x1, x2, x3, f, g);
	XMStoreFloat2(&result, X);
}

inline VEC2 VEC2::Barycentric(const VEC2& v1, const VEC2& v2, const VEC2& v3, float f, float g)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR x3 = XMLoadFloat2(&v3);
	XMVECTOR X = XMVectorBaryCentric(x1, x2, x3, f, g);

	VEC2 result;
	XMStoreFloat2(&result, X);
	return result;
}

inline void VEC2::CatmullRom(const VEC2& v1, const VEC2& v2, const VEC2& v3, const VEC2& v4, float t, VEC2& result)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR x3 = XMLoadFloat2(&v3);
	XMVECTOR x4 = XMLoadFloat2(&v4);
	XMVECTOR X = XMVectorCatmullRom(x1, x2, x3, x4, t);
	XMStoreFloat2(&result, X);
}

inline VEC2 VEC2::CatmullRom(const VEC2& v1, const VEC2& v2, const VEC2& v3, const VEC2& v4, float t)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR x3 = XMLoadFloat2(&v3);
	XMVECTOR x4 = XMLoadFloat2(&v4);
	XMVECTOR X = XMVectorCatmullRom(x1, x2, x3, x4, t);

	VEC2 result;
	XMStoreFloat2(&result, X);
	return result;
}

inline float VEC2::Distance(const VEC2& v1, const VEC2& v2)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR V = XMVectorSubtract(x2, x1);
	XMVECTOR X = XMVector2Length(V);
	return XMVectorGetX(X);
}

inline float VEC2::DistanceSquared(const VEC2& v1, const VEC2& v2)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR V = XMVectorSubtract(x2, x1);
	XMVECTOR X = XMVector2LengthSq(V);
	return XMVectorGetX(X);
}

inline void VEC2::Hermite(const VEC2& v1, const VEC2& t1, const VEC2& v2, const VEC2& t2, float t, VEC2& result)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&t1);
	XMVECTOR x3 = XMLoadFloat2(&v2);
	XMVECTOR x4 = XMLoadFloat2(&t2);
	XMVECTOR X = XMVectorHermite(x1, x2, x3, x4, t);
	XMStoreFloat2(&result, X);
}

inline VEC2 VEC2::Hermite(const VEC2& v1, const VEC2& t1, const VEC2& v2, const VEC2& t2, float t)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&t1);
	XMVECTOR x3 = XMLoadFloat2(&v2);
	XMVECTOR x4 = XMLoadFloat2(&t2);
	XMVECTOR X = XMVectorHermite(x1, x2, x3, x4, t);

	VEC2 result;
	XMStoreFloat2(&result, X);
	return result;
}

inline void VEC2::Lerp(const VEC2& v1, const VEC2& v2, float t, VEC2& result)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR X = XMVectorLerp(x1, x2, t);
	XMStoreFloat2(&result, X);
}

inline VEC2 VEC2::Lerp(const VEC2& v1, const VEC2& v2, float t)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR X = XMVectorLerp(x1, x2, t);

	VEC2 result;
	XMStoreFloat2(&result, X);
	return result;
}

inline float VEC2::LookAtAngle(const VEC2& v1, const VEC2& v2)
{
	return ::Clip(-Angle(v1, v2) - PI / 2);
}

inline void VEC2::Max(const VEC2& v1, const VEC2& v2, VEC2& result)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR X = XMVectorMax(x1, x2);
	XMStoreFloat2(&result, X);
}

inline VEC2 VEC2::Max(const VEC2& v1, const VEC2& v2)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR X = XMVectorMax(x1, x2);

	VEC2 result;
	XMStoreFloat2(&result, X);
	return result;
}

inline void VEC2::Min(const VEC2& v1, const VEC2& v2, VEC2& result)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR X = XMVectorMin(x1, x2);
	XMStoreFloat2(&result, X);
}

inline VEC2 VEC2::Min(const VEC2& v1, const VEC2& v2)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR X = XMVectorMin(x1, x2);

	VEC2 result;
	XMStoreFloat2(&result, X);
	return result;
}

inline void VEC2::MinMax(const VEC2& v1, const VEC2& v2, VEC2& min, VEC2& max)
{
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR r = XMVectorMin(x1, x2);
	XMStoreFloat2(&min, r);
	r = XMVectorMax(x1, x2);
	XMStoreFloat2(&max, r);
}

inline VEC2 VEC2::Random(float a, float b)
{
	return VEC2(::Random(a, b), ::Random(a, b));
}

inline VEC2 VEC2::Random(const VEC2& vmin, const VEC2& vmax)
{
	return VEC2(::Random(vmin.x, vmax.x), ::Random(vmin.y, vmax.y));
}

inline VEC2 VEC2::RandomCirclePt(float r)
{
	float a = ::Random(),
		b = ::Random();
	if(b < a)
		std::swap(a, b);
	return VEC2(b*r*cos(2 * PI*a / b), b*r*sin(2 * PI*a / b));
}

inline void VEC2::Reflect(const VEC2& ivec, const VEC2& nvec, VEC2& result)
{
	XMVECTOR i = XMLoadFloat2(&ivec);
	XMVECTOR n = XMLoadFloat2(&nvec);
	XMVECTOR X = XMVector2Reflect(i, n);
	XMStoreFloat2(&result, X);
}

inline VEC2 VEC2::Reflect(const VEC2& ivec, const VEC2& nvec)
{
	XMVECTOR i = XMLoadFloat2(&ivec);
	XMVECTOR n = XMLoadFloat2(&nvec);
	XMVECTOR X = XMVector2Reflect(i, n);

	VEC2 result;
	XMStoreFloat2(&result, X);
	return result;
}

inline void VEC2::Refract(const VEC2& ivec, const VEC2& nvec, float refractionIndex, VEC2& result)
{
	XMVECTOR i = XMLoadFloat2(&ivec);
	XMVECTOR n = XMLoadFloat2(&nvec);
	XMVECTOR X = XMVector2Refract(i, n, refractionIndex);
	XMStoreFloat2(&result, X);
}

inline VEC2 VEC2::Refract(const VEC2& ivec, const VEC2& nvec, float refractionIndex)
{
	XMVECTOR i = XMLoadFloat2(&ivec);
	XMVECTOR n = XMLoadFloat2(&nvec);
	XMVECTOR X = XMVector2Refract(i, n, refractionIndex);

	VEC2 result;
	XMStoreFloat2(&result, X);
	return result;
}

inline VEC2 VEC2::Slerp(const VEC2& a, const VEC2& b, float t)
{
	return VEC2(::Slerp(a.x, b.x, t), ::Slerp(a.y, b.y, t));
}

inline void VEC2::SmoothStep(const VEC2& v1, const VEC2& v2, float t, VEC2& result)
{
	t = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t);  // Clamp value to 0 to 1
	t = t*t*(3.f - 2.f*t);
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR X = XMVectorLerp(x1, x2, t);
	XMStoreFloat2(&result, X);
}

inline VEC2 VEC2::SmoothStep(const VEC2& v1, const VEC2& v2, float t)
{
	t = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t);  // Clamp value to 0 to 1
	t = t*t*(3.f - 2.f*t);
	XMVECTOR x1 = XMLoadFloat2(&v1);
	XMVECTOR x2 = XMLoadFloat2(&v2);
	XMVECTOR X = XMVectorLerp(x1, x2, t);

	VEC2 result;
	XMStoreFloat2(&result, X);
	return result;
}

inline void VEC2::Transform(const VEC2& v, const QUAT& quat, VEC2& result)
{
	XMVECTOR v1 = XMLoadFloat2(&v);
	XMVECTOR q = XMLoadFloat4(&quat);
	XMVECTOR X = XMVector3Rotate(v1, q);
	XMStoreFloat2(&result, X);
}

inline VEC2 VEC2::Transform(const VEC2& v, const QUAT& quat)
{
	XMVECTOR v1 = XMLoadFloat2(&v);
	XMVECTOR q = XMLoadFloat4(&quat);
	XMVECTOR X = XMVector3Rotate(v1, q);

	VEC2 result;
	XMStoreFloat2(&result, X);
	return result;
}

inline void VEC2::Transform(const VEC2& v, const MATRIX& m, VEC2& result)
{
	XMVECTOR v1 = XMLoadFloat2(&v);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector2TransformCoord(v1, M);
	XMStoreFloat2(&result, X);
}

inline VEC2 VEC2::Transform(const VEC2& v, const MATRIX& m)
{
	XMVECTOR v1 = XMLoadFloat2(&v);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector2TransformCoord(v1, M);

	VEC2 result;
	XMStoreFloat2(&result, X);
	return result;
}

inline void VEC2::Transform(const VEC2* varray, size_t count, const MATRIX& m, VEC2* resultArray)
{
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVector2TransformCoordStream(resultArray, sizeof(XMFLOAT2), varray, sizeof(XMFLOAT2), count, M);
}

inline void VEC2::Transform(const VEC2& v, const MATRIX& m, VEC4& result)
{
	XMVECTOR v1 = XMLoadFloat2(&v);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector2Transform(v1, M);
	XMStoreFloat4(&result, X);
}

inline void VEC2::Transform(const VEC2* varray, size_t count, const MATRIX& m, VEC4* resultArray)
{
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVector2TransformStream(resultArray, sizeof(XMFLOAT4), varray, sizeof(XMFLOAT2), count, M);
}

inline void VEC2::TransformNormal(const VEC2& v, const MATRIX& m, VEC2& result)
{
	XMVECTOR v1 = XMLoadFloat2(&v);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector2TransformNormal(v1, M);
	XMStoreFloat2(&result, X);
}

inline VEC2 VEC2::TransformNormal(const VEC2& v, const MATRIX& m)
{
	XMVECTOR v1 = XMLoadFloat2(&v);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector2TransformNormal(v1, M);

	VEC2 result;
	XMStoreFloat2(&result, X);
	return result;
}

inline void VEC2::TransformNormal(const VEC2* varray, size_t count, const MATRIX& m, VEC2* resultArray)
{
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVector2TransformNormalStream(resultArray, sizeof(XMFLOAT2), varray, sizeof(XMFLOAT2), count, M);
}

//*************************************************************************************************
//
// 3D float point
//
//*************************************************************************************************
inline VEC3::VEC3()
{
}

inline VEC3::VEC3(float x, float y, float z) : XMFLOAT3(x, y, z)
{
}

inline VEC3::VEC3(const VEC3& v) : XMFLOAT3(v.x, v.y, v.z)
{
}

inline VEC3::VEC3(FXMVECTOR v)
{
	XMStoreFloat3(this, v);
}

inline VEC3::VEC3(const XMVECTORF32& v) : XMFLOAT3(v.f[0], v.f[1], v.f[2])
{
}

inline VEC3::operator XMVECTOR() const
{
	return XMLoadFloat3(this);
}

inline VEC3::operator float*()
{
	return &x;
}

inline VEC3::operator const float*() const
{
	return &x;
}

inline bool VEC3::operator == (const VEC3& v) const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR v2 = XMLoadFloat3(&v);
	return XMVector3Equal(v1, v2);
}

inline bool VEC3::operator != (const VEC3& v) const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR v2 = XMLoadFloat3(&v);
	return XMVector3NotEqual(v1, v2);
}

inline VEC3& VEC3::operator = (const VEC3& v)
{
	x = v.x;
	y = v.y;
	z = v.z;
	return *this;
}

inline VEC3& VEC3::operator = (const XMVECTORF32& v)
{
	x = v.f[0];
	y = v.f[1];
	z = v.f[2];
	return *this;
}

inline VEC3& VEC3::operator += (const VEC3& v)
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR v2 = XMLoadFloat3(&v);
	XMVECTOR r = XMVectorAdd(v1, v2);
	XMStoreFloat3(this, r);
	return *this;
}

inline VEC3& VEC3::operator -= (const VEC3& v)
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR v2 = XMLoadFloat3(&v);
	XMVECTOR r = XMVectorSubtract(v1, v2);
	XMStoreFloat3(this, r);
	return *this;
}

inline VEC3& VEC3::operator *= (float s)
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR r = XMVectorScale(v1, s);
	XMStoreFloat3(this, r);
	return *this;
}

inline VEC3& VEC3::operator /= (float s)
{
	assert(s != 0.f);
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR r = XMVectorScale(v1, 1.f / s);
	XMStoreFloat3(this, r);
	return *this;
}

inline VEC3 VEC3::operator + () const
{
	return *this;
}

inline VEC3 VEC3::operator - () const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR r = XMVectorNegate(v1);
	VEC3 result;
	XMStoreFloat3(&result, r);
	return result;
}

inline VEC3 VEC3::operator + (const VEC3& v) const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR v2 = XMLoadFloat3(&v);
	XMVECTOR r = XMVectorAdd(v1, v2);
	VEC3 result;
	XMStoreFloat3(&result, r);
	return result;
}

inline VEC3 VEC3::operator - (const VEC3& v) const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR v2 = XMLoadFloat3(&v);
	XMVECTOR r = XMVectorSubtract(v1, v2);
	VEC3 result;
	XMStoreFloat3(&result, r);
	return result;
}

inline VEC3 VEC3::operator * (float s) const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR r = XMVectorScale(v1, s);
	VEC3 result;
	XMStoreFloat3(&result, r);
	return result;
}

inline VEC3 VEC3::operator / (float s) const
{
	assert(s != 0.f);
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR r = XMVectorScale(v1, 1.f / s);
	VEC3 result;
	XMStoreFloat3(&result, r);
	return result;
}

inline VEC3 operator * (float s, const VEC3& v)
{
	return v * s;
}

//------------------------------------------------------------------------------
// Methods
//------------------------------------------------------------------------------

inline void VEC3::Clamp(const VEC3& vmin, const VEC3& vmax)
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR v2 = XMLoadFloat3(&vmin);
	XMVECTOR v3 = XMLoadFloat3(&vmax);
	XMVECTOR X = XMVectorClamp(v1, v2, v3);
	XMStoreFloat3(this, X);
}

inline void VEC3::Clamp(const VEC3& vmin, const VEC3& vmax, VEC3& result) const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR v2 = XMLoadFloat3(&vmin);
	XMVECTOR v3 = XMLoadFloat3(&vmax);
	XMVECTOR X = XMVectorClamp(v1, v2, v3);
	XMStoreFloat3(&result, X);
}

inline VEC3 VEC3::Clamped(const VEC3& min, const VEC3& max) const
{
	VEC3 result;
	Clamp(min, max, result);
	return result;
}

inline void VEC3::Cross(const VEC3& V, VEC3& result) const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR v2 = XMLoadFloat3(&V);
	XMVECTOR R = XMVector3Cross(v1, v2);
	XMStoreFloat3(&result, R);
}

inline VEC3 VEC3::Cross(const VEC3& V) const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR v2 = XMLoadFloat3(&V);
	XMVECTOR R = XMVector3Cross(v1, v2);

	VEC3 result;
	XMStoreFloat3(&result, R);
	return result;
}

inline float VEC3::Dot(const VEC3& V) const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR v2 = XMLoadFloat3(&V);
	XMVECTOR X = XMVector3Dot(v1, v2);
	return XMVectorGetX(X);
}

inline float VEC3::DotSelf() const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR X = XMVector3Dot(v1, v1);
	return XMVectorGetX(X);
}

inline bool VEC3::Equal(const VEC3& v) const
{
	return ::Equal(x, v.x) && ::Equal(y, v.y) && ::Equal(z, v.z);
}

inline bool VEC3::InBounds(const VEC3& Bounds) const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR v2 = XMLoadFloat3(&Bounds);
	return XMVector3InBounds(v1, v2);
}

inline bool VEC3::IsPositive() const
{
	return x >= 0.f && y >= 0.f && z >= 0.f;
}

inline float VEC3::Length() const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR X = XMVector3Length(v1);
	return XMVectorGetX(X);
}

inline float VEC3::LengthSquared() const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR X = XMVector3LengthSq(v1);
	return XMVectorGetX(X);
}

inline VEC3& VEC3::Normalize()
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR X = XMVector3Normalize(v1);
	XMStoreFloat3(this, X);
	return *this;
}

inline void VEC3::Normalize(VEC3& result) const
{
	XMVECTOR v1 = XMLoadFloat3(this);
	XMVECTOR X = XMVector3Normalize(v1);
	XMStoreFloat3(&result, X);
}

inline VEC3 VEC3::Normalized() const
{
	VEC3 result;
	Normalize(result);
	return result;
}

inline VEC2 VEC3::XY() const
{
	return VEC2(x, y);
}

inline VEC2 VEC3::XZ() const
{
	return VEC2(x, z);
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline float VEC3::Angle2d(const VEC3& v1, const VEC3& v2)
{
	return ::Angle(v1.x, v1.z, v2.x, v2.z);
}

inline void VEC3::Barycentric(const VEC3& v1, const VEC3& v2, const VEC3& v3, float f, float g, VEC3& result)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR x3 = XMLoadFloat3(&v3);
	XMVECTOR X = XMVectorBaryCentric(x1, x2, x3, f, g);
	XMStoreFloat3(&result, X);
}

inline VEC3 VEC3::Barycentric(const VEC3& v1, const VEC3& v2, const VEC3& v3, float f, float g)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR x3 = XMLoadFloat3(&v3);
	XMVECTOR X = XMVectorBaryCentric(x1, x2, x3, f, g);

	VEC3 result;
	XMStoreFloat3(&result, X);
	return result;
}

inline void VEC3::CatmullRom(const VEC3& v1, const VEC3& v2, const VEC3& v3, const VEC3& v4, float t, VEC3& result)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR x3 = XMLoadFloat3(&v3);
	XMVECTOR x4 = XMLoadFloat3(&v4);
	XMVECTOR X = XMVectorCatmullRom(x1, x2, x3, x4, t);
	XMStoreFloat3(&result, X);
}

inline VEC3 VEC3::CatmullRom(const VEC3& v1, const VEC3& v2, const VEC3& v3, const VEC3& v4, float t)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR x3 = XMLoadFloat3(&v3);
	XMVECTOR x4 = XMLoadFloat3(&v4);
	XMVECTOR X = XMVectorCatmullRom(x1, x2, x3, x4, t);

	VEC3 result;
	XMStoreFloat3(&result, X);
	return result;
}

inline float VEC3::Distance(const VEC3& v1, const VEC3& v2)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR V = XMVectorSubtract(x2, x1);
	XMVECTOR X = XMVector3Length(V);
	return XMVectorGetX(X);
}

inline float VEC3::DistanceSquared(const VEC3& v1, const VEC3& v2)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR V = XMVectorSubtract(x2, x1);
	XMVECTOR X = XMVector3LengthSq(V);
	return XMVectorGetX(X);
}

inline float VEC3::Distance2d(const VEC3& v1, const VEC3& v2)
{
	float x = abs(v1.x - v2.x),
		z = abs(v1.z - v2.z);
	return sqrt(x*x + z*z);
}

inline void VEC3::Hermite(const VEC3& v1, const VEC3& t1, const VEC3& v2, const VEC3& t2, float t, VEC3& result)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&t1);
	XMVECTOR x3 = XMLoadFloat3(&v2);
	XMVECTOR x4 = XMLoadFloat3(&t2);
	XMVECTOR X = XMVectorHermite(x1, x2, x3, x4, t);
	XMStoreFloat3(&result, X);
}

inline VEC3 VEC3::Hermite(const VEC3& v1, const VEC3& t1, const VEC3& v2, const VEC3& t2, float t)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&t1);
	XMVECTOR x3 = XMLoadFloat3(&v2);
	XMVECTOR x4 = XMLoadFloat3(&t2);
	XMVECTOR X = XMVectorHermite(x1, x2, x3, x4, t);

	VEC3 result;
	XMStoreFloat3(&result, X);
	return result;
}

inline void VEC3::Lerp(const VEC3& v1, const VEC3& v2, float t, VEC3& result)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR X = XMVectorLerp(x1, x2, t);
	XMStoreFloat3(&result, X);
}

inline VEC3 VEC3::Lerp(const VEC3& v1, const VEC3& v2, float t)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR X = XMVectorLerp(x1, x2, t);

	VEC3 result;
	XMStoreFloat3(&result, X);
	return result;
}

inline float VEC3::LookAtAngle(const VEC3& v1, const VEC3& v2)
{
	return Clip(-Angle2d(v1, v2) - PI / 2);
}

inline void VEC3::Max(const VEC3& v1, const VEC3& v2, VEC3& result)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR X = XMVectorMax(x1, x2);
	XMStoreFloat3(&result, X);
}

inline VEC3 VEC3::Max(const VEC3& v1, const VEC3& v2)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR X = XMVectorMax(x1, x2);

	VEC3 result;
	XMStoreFloat3(&result, X);
	return result;
}

inline void VEC3::Min(const VEC3& v1, const VEC3& v2, VEC3& result)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR X = XMVectorMin(x1, x2);
	XMStoreFloat3(&result, X);
}

inline VEC3 VEC3::Min(const VEC3& v1, const VEC3& v2)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR X = XMVectorMin(x1, x2);

	VEC3 result;
	XMStoreFloat3(&result, X);
	return result;
}

inline void VEC3::MinMax(const VEC3& v1, const VEC3& v2, VEC3& min, VEC3& max)
{
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR r = XMVectorMin(x1, x2);
	XMStoreFloat3(&min, r);
	r = XMVectorMax(x1, x2);
	XMStoreFloat3(&max, r);
}

inline VEC3 VEC3::Random(float a, float b)
{
	return VEC3(::Random(a, b), ::Random(a, b), ::Random(a, b));
}

inline VEC3 VEC3::Random(const VEC3& min, const VEC3& max)
{
	return VEC3(::Random(min.x, max.x), ::Random(min.y, max.y), ::Random(min.z, max.z));
}

inline void VEC3::Reflect(const VEC3& ivec, const VEC3& nvec, VEC3& result)
{
	XMVECTOR i = XMLoadFloat3(&ivec);
	XMVECTOR n = XMLoadFloat3(&nvec);
	XMVECTOR X = XMVector3Reflect(i, n);
	XMStoreFloat3(&result, X);
}

inline VEC3 VEC3::Reflect(const VEC3& ivec, const VEC3& nvec)
{
	XMVECTOR i = XMLoadFloat3(&ivec);
	XMVECTOR n = XMLoadFloat3(&nvec);
	XMVECTOR X = XMVector3Reflect(i, n);

	VEC3 result;
	XMStoreFloat3(&result, X);
	return result;
}

inline void VEC3::Refract(const VEC3& ivec, const VEC3& nvec, float refractionIndex, VEC3& result)
{
	XMVECTOR i = XMLoadFloat3(&ivec);
	XMVECTOR n = XMLoadFloat3(&nvec);
	XMVECTOR X = XMVector3Refract(i, n, refractionIndex);
	XMStoreFloat3(&result, X);
}

inline VEC3 VEC3::Refract(const VEC3& ivec, const VEC3& nvec, float refractionIndex)
{
	XMVECTOR i = XMLoadFloat3(&ivec);
	XMVECTOR n = XMLoadFloat3(&nvec);
	XMVECTOR X = XMVector3Refract(i, n, refractionIndex);

	VEC3 result;
	XMStoreFloat3(&result, X);
	return result;
}

inline void VEC3::SmoothStep(const VEC3& v1, const VEC3& v2, float t, VEC3& result)
{
	t = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t);  // Clamp value to 0 to 1
	t = t*t*(3.f - 2.f*t);
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR X = XMVectorLerp(x1, x2, t);
	XMStoreFloat3(&result, X);
}

inline VEC3 VEC3::SmoothStep(const VEC3& v1, const VEC3& v2, float t)
{
	t = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t);  // Clamp value to 0 to 1
	t = t*t*(3.f - 2.f*t);
	XMVECTOR x1 = XMLoadFloat3(&v1);
	XMVECTOR x2 = XMLoadFloat3(&v2);
	XMVECTOR X = XMVectorLerp(x1, x2, t);

	VEC3 result;
	XMStoreFloat3(&result, X);
	return result;
}

inline void VEC3::Transform(const VEC3& v, const QUAT& quat, VEC3& result)
{
	XMVECTOR v1 = XMLoadFloat3(&v);
	XMVECTOR q = XMLoadFloat4(&quat);
	XMVECTOR X = XMVector3Rotate(v1, q);
	XMStoreFloat3(&result, X);
}

inline VEC3 VEC3::Transform(const VEC3& v, const QUAT& quat)
{
	XMVECTOR v1 = XMLoadFloat3(&v);
	XMVECTOR q = XMLoadFloat4(&quat);
	XMVECTOR X = XMVector3Rotate(v1, q);

	VEC3 result;
	XMStoreFloat3(&result, X);
	return result;
}

inline void VEC3::Transform(const VEC3& v, const MATRIX& m, VEC3& result)
{
	XMVECTOR v1 = XMLoadFloat3(&v);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector3TransformCoord(v1, M);
	XMStoreFloat3(&result, X);
}

inline VEC3 VEC3::Transform(const VEC3& v, const MATRIX& m)
{
	XMVECTOR v1 = XMLoadFloat3(&v);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector3TransformCoord(v1, M);

	VEC3 result;
	XMStoreFloat3(&result, X);
	return result;
}

inline void VEC3::Transform(const VEC3* varray, size_t count, const MATRIX& m, VEC3* resultArray)
{
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVector3TransformCoordStream(resultArray, sizeof(XMFLOAT3), varray, sizeof(XMFLOAT3), count, M);
}

inline void VEC3::Transform(const VEC3& v, const MATRIX& m, VEC4& result)
{
	XMVECTOR v1 = XMLoadFloat3(&v);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector3Transform(v1, M);
	XMStoreFloat4(&result, X);
}

inline void VEC3::Transform(const VEC3* varray, size_t count, const MATRIX& m, VEC4* resultArray)
{
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVector3TransformStream(resultArray, sizeof(XMFLOAT4), varray, sizeof(XMFLOAT3), count, M);
}

inline void VEC3::TransformNormal(const VEC3& v, const MATRIX& m, VEC3& result)
{
	XMVECTOR v1 = XMLoadFloat3(&v);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector3TransformNormal(v1, M);
	XMStoreFloat3(&result, X);
}

inline VEC3 VEC3::TransformNormal(const VEC3& v, const MATRIX& m)
{
	XMVECTOR v1 = XMLoadFloat3(&v);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector3TransformNormal(v1, M);

	VEC3 result;
	XMStoreFloat3(&result, X);
	return result;
}

inline void VEC3::TransformNormal(const VEC3* varray, size_t count, const MATRIX& m, VEC3* resultArray)
{
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVector3TransformNormalStream(resultArray, sizeof(XMFLOAT3), varray, sizeof(XMFLOAT3), count, M);
}

inline VEC3 VEC3::TransformZero(const MATRIX& m)
{
	XMVECTOR v1 = XMLoadFloat3(&VEC3::Zero);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector3TransformCoord(v1, M);

	VEC3 result;
	XMStoreFloat3(&result, X);
	return result;
}

//*************************************************************************************************
//
// 4D float point
//
//*************************************************************************************************
inline VEC4::VEC4()
{
}

inline VEC4::VEC4(float x, float y, float z, float w) : XMFLOAT4(x, y, z, w)
{
}

inline VEC4::VEC4(const VEC4& v) : XMFLOAT4(v.x, v.y, v.z, v.w)
{
}

inline VEC4::VEC4(const VEC3& v, float w) : XMFLOAT4(v.x, v.y, v.z, w)
{
}

inline VEC4::VEC4(FXMVECTOR v)
{
	XMStoreFloat4(this, v);
}

inline VEC4::VEC4(const XMVECTORF32& v) : XMFLOAT4(v.f[0], v.f[1], v.f[2], v.f[3])
{
}

inline VEC4::operator XMVECTOR() const
{
	return XMLoadFloat4(this);
}

inline VEC4::operator float*()
{
	return &x;
}

inline VEC4::operator const float*() const
{
	return &x;
}

inline bool VEC4::operator == (const VEC4& v) const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR v2 = XMLoadFloat4(&v);
	return XMVector4Equal(v1, v2);
}

inline bool VEC4::operator != (const VEC4& v) const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR v2 = XMLoadFloat4(&v);
	return XMVector4NotEqual(v1, v2);
}

inline VEC4& VEC4::operator = (const VEC4& v)
{
	x = v.x;
	y = v.y;
	z = v.z;
	w = v.w;
	return *this;
}

inline VEC4& VEC4::operator = (const XMVECTORF32& v)
{
	x = v.f[0];
	y = v.f[1];
	z = v.f[2];
	w = v.f[3];
	return *this;
}

inline VEC4& VEC4::operator += (const VEC4& v)
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR v2 = XMLoadFloat4(&v);
	XMVECTOR r = XMVectorAdd(v1, v2);
	XMStoreFloat4(this, r);
	return *this;
}

inline VEC4& VEC4::operator -= (const VEC4& v)
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR v2 = XMLoadFloat4(&v);
	XMVECTOR r = XMVectorSubtract(v1, v2);
	XMStoreFloat4(this, r);
	return *this;
}

inline VEC4& VEC4::operator *= (float s)
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR r = XMVectorScale(v1, s);
	XMStoreFloat4(this, r);
	return *this;
}

inline VEC4& VEC4::operator /= (float s)
{
	assert(s != 0.f);
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR r = XMVectorScale(v1, 1.f / s);
	XMStoreFloat4(this, r);
	return *this;
}

inline VEC4 VEC4::operator + () const
{
	return *this;
}

inline VEC4 VEC4::operator - () const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR r = XMVectorNegate(v1);
	VEC4 result;
	XMStoreFloat4(&result, r);
	return r;
}

inline VEC4 VEC4::operator + (const VEC4& v) const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR v2 = XMLoadFloat4(&v);
	XMVECTOR r = XMVectorAdd(v1, v2);
	VEC4 result;
	XMStoreFloat4(&result, r);
	return result;
}

inline VEC4 VEC4::operator - (const VEC4& v) const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR v2 = XMLoadFloat4(&v);
	XMVECTOR r = XMVectorSubtract(v1, v2);
	VEC4 result;
	XMStoreFloat4(&result, r);
	return result;
}

inline VEC4 VEC4::operator * (float s) const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR r = XMVectorScale(v1, s);
	VEC4 result;
	XMStoreFloat4(&result, r);
	return result;
}

inline VEC4 VEC4::operator / (float s) const
{
	assert(s != 0.f);
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR r = XMVectorScale(v1, 1.f / s);
	VEC4 result;
	XMStoreFloat4(&result, r);
	return result;
}

inline VEC4 operator * (float s, const VEC4& v)
{
	return v * s;
}

//------------------------------------------------------------------------------
// Methods
//------------------------------------------------------------------------------

inline void VEC4::Clamp(const VEC4& vmin, const VEC4& vmax)
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR v2 = XMLoadFloat4(&vmin);
	XMVECTOR v3 = XMLoadFloat4(&vmax);
	XMVECTOR X = XMVectorClamp(v1, v2, v3);
	XMStoreFloat4(this, X);
}

inline void VEC4::Clamp(const VEC4& vmin, const VEC4& vmax, VEC4& result) const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR v2 = XMLoadFloat4(&vmin);
	XMVECTOR v3 = XMLoadFloat4(&vmax);
	XMVECTOR X = XMVectorClamp(v1, v2, v3);
	XMStoreFloat4(&result, X);
}

inline VEC4 VEC4::Clamped(const VEC4& min, const VEC4& max) const
{
	VEC4 result;
	Clamp(min, max, result);
	return result;
}

inline void VEC4::Cross(const VEC4& v1, const VEC4& v2, VEC4& result) const
{
	XMVECTOR x1 = XMLoadFloat4(this);
	XMVECTOR x2 = XMLoadFloat4(&v1);
	XMVECTOR x3 = XMLoadFloat4(&v2);
	XMVECTOR R = XMVector4Cross(x1, x2, x3);
	XMStoreFloat4(&result, R);
}

inline VEC4 VEC4::Cross(const VEC4& v1, const VEC4& v2) const
{
	XMVECTOR x1 = XMLoadFloat4(this);
	XMVECTOR x2 = XMLoadFloat4(&v1);
	XMVECTOR x3 = XMLoadFloat4(&v2);
	XMVECTOR R = XMVector4Cross(x1, x2, x3);

	VEC4 result;
	XMStoreFloat4(&result, R);
	return result;
}

inline float VEC4::Dot(const VEC4& V) const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR v2 = XMLoadFloat4(&V);
	XMVECTOR X = XMVector4Dot(v1, v2);
	return XMVectorGetX(X);
}

inline float VEC4::DotSelf() const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR X = XMVector4Dot(v1, v1);
	return XMVectorGetX(X);
}

inline bool VEC4::Equal(const VEC4& v) const
{
	return ::Equal(x, v.x) && ::Equal(y, v.y) && ::Equal(z, v.z) && ::Equal(w, v.w);
}

inline bool VEC4::InBounds(const VEC4& Bounds) const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR v2 = XMLoadFloat4(&Bounds);
	return XMVector4InBounds(v1, v2);
}

inline float VEC4::Length() const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR X = XMVector4Length(v1);
	return XMVectorGetX(X);
}

inline float VEC4::LengthSquared() const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR X = XMVector4LengthSq(v1);
	return XMVectorGetX(X);
}

inline VEC4& VEC4::Normalize()
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR X = XMVector4Normalize(v1);
	XMStoreFloat4(this, X);
	return *this;
}

inline void VEC4::Normalize(VEC4& result) const
{
	XMVECTOR v1 = XMLoadFloat4(this);
	XMVECTOR X = XMVector4Normalize(v1);
	XMStoreFloat4(&result, X);
}

inline VEC4 VEC4::Normalized() const
{
	VEC4 result;
	Normalize(result);
	return result;
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline void VEC4::Barycentric(const VEC4& v1, const VEC4& v2, const VEC4& v3, float f, float g, VEC4& result)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR x3 = XMLoadFloat4(&v3);
	XMVECTOR X = XMVectorBaryCentric(x1, x2, x3, f, g);
	XMStoreFloat4(&result, X);
}

inline VEC4 VEC4::Barycentric(const VEC4& v1, const VEC4& v2, const VEC4& v3, float f, float g)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR x3 = XMLoadFloat4(&v3);
	XMVECTOR X = XMVectorBaryCentric(x1, x2, x3, f, g);

	VEC4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline void VEC4::CatmullRom(const VEC4& v1, const VEC4& v2, const VEC4& v3, const VEC4& v4, float t, VEC4& result)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR x3 = XMLoadFloat4(&v3);
	XMVECTOR x4 = XMLoadFloat4(&v4);
	XMVECTOR X = XMVectorCatmullRom(x1, x2, x3, x4, t);
	XMStoreFloat4(&result, X);
}

inline VEC4 VEC4::CatmullRom(const VEC4& v1, const VEC4& v2, const VEC4& v3, const VEC4& v4, float t)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR x3 = XMLoadFloat4(&v3);
	XMVECTOR x4 = XMLoadFloat4(&v4);
	XMVECTOR X = XMVectorCatmullRom(x1, x2, x3, x4, t);

	VEC4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline float VEC4::Distance(const VEC4& v1, const VEC4& v2)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR V = XMVectorSubtract(x2, x1);
	XMVECTOR X = XMVector4Length(V);
	return XMVectorGetX(X);
}

inline float VEC4::DistanceSquared(const VEC4& v1, const VEC4& v2)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR V = XMVectorSubtract(x2, x1);
	XMVECTOR X = XMVector4LengthSq(V);
	return XMVectorGetX(X);
}

inline VEC4 VEC4::FromColor(DWORD color)
{
	VEC4 v;
	v.x = float((color & 0xFF0000) >> 16) / 255;
	v.y = float((color & 0xFF00) >> 8) / 255;
	v.z = float(color & 0xFF) / 255;
	v.w = float((color & 0xFF000000) >> 24) / 255;
	return v;
}

inline void VEC4::Hermite(const VEC4& v1, const VEC4& t1, const VEC4& v2, const VEC4& t2, float t, VEC4& result)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&t1);
	XMVECTOR x3 = XMLoadFloat4(&v2);
	XMVECTOR x4 = XMLoadFloat4(&t2);
	XMVECTOR X = XMVectorHermite(x1, x2, x3, x4, t);
	XMStoreFloat4(&result, X);
}

inline VEC4 VEC4::Hermite(const VEC4& v1, const VEC4& t1, const VEC4& v2, const VEC4& t2, float t)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&t1);
	XMVECTOR x3 = XMLoadFloat4(&v2);
	XMVECTOR x4 = XMLoadFloat4(&t2);
	XMVECTOR X = XMVectorHermite(x1, x2, x3, x4, t);

	VEC4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline void VEC4::Lerp(const VEC4& v1, const VEC4& v2, float t, VEC4& result)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR X = XMVectorLerp(x1, x2, t);
	XMStoreFloat4(&result, X);
}

inline VEC4 VEC4::Lerp(const VEC4& v1, const VEC4& v2, float t)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR X = XMVectorLerp(x1, x2, t);

	VEC4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline void VEC4::Max(const VEC4& v1, const VEC4& v2, VEC4& result)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR X = XMVectorMax(x1, x2);
	XMStoreFloat4(&result, X);
}

inline VEC4 VEC4::Max(const VEC4& v1, const VEC4& v2)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR X = XMVectorMax(x1, x2);

	VEC4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline void VEC4::Min(const VEC4& v1, const VEC4& v2, VEC4& result)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR X = XMVectorMin(x1, x2);
	XMStoreFloat4(&result, X);
}

inline VEC4 VEC4::Min(const VEC4& v1, const VEC4& v2)
{
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR X = XMVectorMin(x1, x2);

	VEC4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline void VEC4::Reflect(const VEC4& ivec, const VEC4& nvec, VEC4& result)
{
	XMVECTOR i = XMLoadFloat4(&ivec);
	XMVECTOR n = XMLoadFloat4(&nvec);
	XMVECTOR X = XMVector4Reflect(i, n);
	XMStoreFloat4(&result, X);
}

inline VEC4 VEC4::Reflect(const VEC4& ivec, const VEC4& nvec)
{
	XMVECTOR i = XMLoadFloat4(&ivec);
	XMVECTOR n = XMLoadFloat4(&nvec);
	XMVECTOR X = XMVector4Reflect(i, n);

	VEC4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline void VEC4::Refract(const VEC4& ivec, const VEC4& nvec, float refractionIndex, VEC4& result)
{
	XMVECTOR i = XMLoadFloat4(&ivec);
	XMVECTOR n = XMLoadFloat4(&nvec);
	XMVECTOR X = XMVector4Refract(i, n, refractionIndex);
	XMStoreFloat4(&result, X);
}

inline VEC4 VEC4::Refract(const VEC4& ivec, const VEC4& nvec, float refractionIndex)
{
	XMVECTOR i = XMLoadFloat4(&ivec);
	XMVECTOR n = XMLoadFloat4(&nvec);
	XMVECTOR X = XMVector4Refract(i, n, refractionIndex);

	VEC4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline void VEC4::SmoothStep(const VEC4& v1, const VEC4& v2, float t, VEC4& result)
{
	t = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t);  // Clamp value to 0 to 1
	t = t*t*(3.f - 2.f*t);
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR X = XMVectorLerp(x1, x2, t);
	XMStoreFloat4(&result, X);
}

inline VEC4 VEC4::SmoothStep(const VEC4& v1, const VEC4& v2, float t)
{
	t = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t);  // Clamp value to 0 to 1
	t = t*t*(3.f - 2.f*t);
	XMVECTOR x1 = XMLoadFloat4(&v1);
	XMVECTOR x2 = XMLoadFloat4(&v2);
	XMVECTOR X = XMVectorLerp(x1, x2, t);

	VEC4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline void VEC4::Transform(const VEC2& v, const QUAT& quat, VEC4& result)
{
	XMVECTOR v1 = XMLoadFloat2(&v);
	XMVECTOR q = XMLoadFloat4(&quat);
	XMVECTOR X = XMVector3Rotate(v1, q);
	X = XMVectorSelect(g_XMIdentityR3, X, g_XMSelect1110); // result.w = 1.f
	XMStoreFloat4(&result, X);
}

inline VEC4 VEC4::Transform(const VEC2& v, const QUAT& quat)
{
	XMVECTOR v1 = XMLoadFloat2(&v);
	XMVECTOR q = XMLoadFloat4(&quat);
	XMVECTOR X = XMVector3Rotate(v1, q);
	X = XMVectorSelect(g_XMIdentityR3, X, g_XMSelect1110); // result.w = 1.f

	VEC4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline void VEC4::Transform(const VEC3& v, const QUAT& quat, VEC4& result)
{
	XMVECTOR v1 = XMLoadFloat3(&v);
	XMVECTOR q = XMLoadFloat4(&quat);
	XMVECTOR X = XMVector3Rotate(v1, q);
	X = XMVectorSelect(g_XMIdentityR3, X, g_XMSelect1110); // result.w = 1.f
	XMStoreFloat4(&result, X);
}

inline VEC4 VEC4::Transform(const VEC3& v, const QUAT& quat)
{
	XMVECTOR v1 = XMLoadFloat3(&v);
	XMVECTOR q = XMLoadFloat4(&quat);
	XMVECTOR X = XMVector3Rotate(v1, q);
	X = XMVectorSelect(g_XMIdentityR3, X, g_XMSelect1110); // result.w = 1.f

	VEC4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline void VEC4::Transform(const VEC4& v, const QUAT& quat, VEC4& result)
{
	XMVECTOR v1 = XMLoadFloat4(&v);
	XMVECTOR q = XMLoadFloat4(&quat);
	XMVECTOR X = XMVector3Rotate(v1, q);
	X = XMVectorSelect(v1, X, g_XMSelect1110); // result.w = v.w
	XMStoreFloat4(&result, X);
}

inline VEC4 VEC4::Transform(const VEC4& v, const QUAT& quat)
{
	XMVECTOR v1 = XMLoadFloat4(&v);
	XMVECTOR q = XMLoadFloat4(&quat);
	XMVECTOR X = XMVector3Rotate(v1, q);
	X = XMVectorSelect(v1, X, g_XMSelect1110); // result.w = v.w

	VEC4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline void VEC4::Transform(const VEC4& v, const MATRIX& m, VEC4& result)
{
	XMVECTOR v1 = XMLoadFloat4(&v);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector4Transform(v1, M);
	XMStoreFloat4(&result, X);
}

inline VEC4 VEC4::Transform(const VEC4& v, const MATRIX& m)
{
	XMVECTOR v1 = XMLoadFloat4(&v);
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVECTOR X = XMVector4Transform(v1, M);

	VEC4 result;
	XMStoreFloat4(&result, X);
	return result;
}

inline void VEC4::Transform(const VEC4* varray, size_t count, const MATRIX& m, VEC4* resultArray)
{
	XMMATRIX M = XMLoadFloat4x4(&m);
	XMVector4TransformStream(resultArray, sizeof(XMFLOAT4), varray, sizeof(XMFLOAT4), count, M);
}

//*************************************************************************************************
//
// 2d box using floats
//
//*************************************************************************************************
inline BOX2D::BOX2D()
{
}

inline BOX2D::BOX2D(float minx, float miny, float maxx, float maxy) : v1(minx, miny), v2(maxx, maxy)
{
}

inline BOX2D::BOX2D(const VEC2& v1, const VEC2& v2) : v1(v1), v2(v2)
{
}

inline BOX2D::BOX2D(const BOX2D& box) : v1(box.v1), v2(box.v2)
{
}

inline BOX2D::BOX2D(float x, float y) : v1(x, y), v2(x, y)
{
}

inline BOX2D::BOX2D(const BOX2D& box, float margin) : v1(box.v1.x - margin, box.v1.y - margin), v2(box.v2.x + margin, box.v2.y + margin)
{
}

inline BOX2D::BOX2D(const VEC2& v) : v1(v), v2(v)
{
}

inline BOX2D::BOX2D(const Rect& r) : v1(VEC2(r.p1)), v2(VEC2(r.p2))
{
}

inline bool BOX2D::operator == (const BOX2D& b) const
{
	return v1 == b.v1 && v2 == b.v2;
}

inline bool BOX2D::operator != (const BOX2D& b) const
{
	return v1 != b.v1 || v2 != b.v2;
}

inline BOX2D& BOX2D::operator = (const BOX2D& b)
{
	v1 = b.v1;
	v2 = b.v2;
	return *this;
}

inline BOX2D& BOX2D::operator += (const VEC2& v)
{
	v1 += v;
	v2 += v;
	return *this;
}

inline BOX2D& BOX2D::operator -= (const VEC2& v)
{
	v1 == v;
	v2 -= v;
	return *this;
}

inline BOX2D& BOX2D::operator *= (float f)
{
	v1 *= f;
	v2 *= f;
	return *this;
}

inline BOX2D& BOX2D::operator /= (float f)
{
	v1 /= f;
	v2 /= f;
	return *this;
}

inline BOX2D BOX2D::operator + () const
{
	return *this;
}

inline BOX2D BOX2D::operator - () const
{
	return BOX2D(-v1, -v2);
}

inline BOX2D BOX2D::operator + (const VEC2& v) const
{
	return BOX2D(v1 + v, v2 + v);
}

inline BOX2D BOX2D::operator - (const VEC2& v) const
{
	return BOX2D(v1 - v, v2 - v);
}

inline BOX2D BOX2D::operator * (float f) const
{
	return BOX2D(v1 * f, v2 * f);
}

inline BOX2D BOX2D::operator / (float f) const
{
	return BOX2D(v1 / f, v2 / f);
}

inline BOX2D operator * (float f, const BOX2D& b)
{
	return b * f;
}

//------------------------------------------------------------------------------
// Methods
//------------------------------------------------------------------------------

inline VEC2 BOX2D::GetRandomPoint() const
{
	return VEC2(::Random(v1.x, v2.x), ::Random(v1.y, v2.y));
}

inline bool BOX2D::IsInside(const VEC2& v) const
{
	return v.x >= v1.x && v.y >= v1.y && v.x <= v2.x && v.y <= v2.y;
}

inline bool BOX2D::IsInside(const VEC3& v) const
{
	return v.x >= v1.x && v.z >= v1.y && v.x <= v2.x && v.z <= v2.y;
}

inline bool BOX2D::IsInside(const INT2& p) const
{
	float x = (float)p.x;
	float y = (float)p.y;
	return x >= v1.x && y >= v1.y && x <= v2.x && y <= v2.y;
}

inline bool BOX2D::IsValid() const
{
	return v1.x <= v2.x && v1.y <= v2.y;
}

inline VEC2 BOX2D::Midpoint() const
{
	return v1 + (v2 - v1) / 2;
}

inline float BOX2D::SizeX() const
{
	return abs(v2.x - v1.x);
}

inline float BOX2D::SizeY() const
{
	return abs(v2.y - v1.y);
}

inline VEC2 BOX2D::Size() const
{
	return VEC2(SizeX(), SizeY());
}

//*************************************************************************************************
//
// 3d box using floats
//
//*************************************************************************************************
inline BOX::BOX()
{
}

inline BOX::BOX(float minx, float miny, float minz, float maxx, float maxy, float maxz) : v1(minx, miny, minz), v2(maxx, maxy, maxz)
{
}

inline BOX::BOX(const VEC3& v1, const VEC3& v2) : v1(v1), v2(v2)
{
}

inline BOX::BOX(const BOX& box) : v1(box.v1), v2(box.v2)
{
}

inline BOX::BOX(float x, float y, float z) : v1(x, y, z), v2(x, y, z)
{
}

inline BOX::BOX(const VEC3& v) : v1(v), v2(v)
{
}

inline bool BOX::operator == (const BOX& b) const
{
	return v1 == b.v1 && v2 == b.v2;
}

inline bool BOX::operator != (const BOX& b) const
{
	return v1 != b.v1 || v2 != b.v2;
}

inline BOX& BOX::operator = (const BOX& b)
{
	v1 = b.v1;
	v2 = b.v2;
	return *this;
}

inline BOX& BOX::operator += (const VEC3& v)
{
	v1 += v;
	v2 += v;
	return *this;
}

inline BOX& BOX::operator -= (const VEC3& v)
{
	v1 == v;
	v2 -= v;
	return *this;
}

inline BOX& BOX::operator *= (float f)
{
	v1 *= f;
	v2 *= f;
	return *this;
}

inline BOX& BOX::operator /= (float f)
{
	v1 /= f;
	v2 /= f;
	return *this;
}

inline BOX BOX::operator + () const
{
	return *this;
}

inline BOX BOX::operator - () const
{
	return BOX(-v1, -v2);
}

inline BOX BOX::operator + (const VEC3& v) const
{
	return BOX(v1 + v, v2 + v);
}

inline BOX BOX::operator - (const VEC3& v) const
{
	return BOX(v1 - v, v2 - v);
}

inline BOX BOX::operator * (float f) const
{
	return BOX(v1 * f, v2 * f);
}

inline BOX BOX::operator / (float f) const
{
	return BOX(v1 / f, v2 / f);
}

inline BOX operator * (float f, const BOX& b)
{
	return b * f;
}

//------------------------------------------------------------------------------
// Methods
//------------------------------------------------------------------------------

inline VEC3 BOX::GetRandomPoint() const
{
	return VEC3(::Random(v1.x, v2.x), ::Random(v1.y, v2.y), ::Random(v1.z, v2.z));
}

inline bool BOX::IsInside(const VEC3& v) const
{
	return v.x >= v1.x && v.x <= v2.x && v.y >= v1.y && v.y <= v2.y && v.z >= v1.z && v.z <= v2.z;
}

inline bool BOX::IsValid() const
{
	return v1.x <= v2.x && v1.y <= v2.y && v1.z <= v2.z;
}

inline VEC3 BOX::Midpoint() const
{
	return v1 + (v2 - v1) / 2;
}

inline VEC3 BOX::Size() const
{
	return VEC3(SizeX(), SizeY(), SizeZ());
}

inline float BOX::SizeX() const
{
	return abs(v2.x - v1.x);
}

inline VEC2 BOX::SizeXZ() const
{
	return VEC2(SizeX(), SizeZ());
}

inline float BOX::SizeY() const
{
	return abs(v2.y - v1.y);
}

inline float BOX::SizeZ() const
{
	return abs(v2.z - v1.z);
}

//*************************************************************************************************
//
// 4x4 float matrix
//
//*************************************************************************************************
inline MATRIX::MATRIX()
{
}

inline MATRIX::MATRIX(
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

inline MATRIX::MATRIX(const VEC3& v1, const VEC3& v2, const VEC3& v3) : XMFLOAT4X4(
	v1.x, v1.y, v1.z, 0,
	v2.x, v2.y, v2.z, 0,
	v3.x, v3.y, v3.z, 0,
	0, 0, 0, 1.f)
{
}

inline MATRIX::MATRIX(const VEC4& v1, const VEC4& v2, const VEC4& v3, const VEC4& v4) : XMFLOAT4X4(
	v1.x, v1.y, v1.z, v1.w,
	v2.x, v2.y, v2.z, v2.w,
	v3.x, v3.y, v3.z, v3.w,
	v4.x, v4.y, v4.z, v4.w)
{
}

inline MATRIX::MATRIX(const MATRIX& m) : XMFLOAT4X4(
	m._11, m._12, m._13, m._14,
	m._21, m._22, m._23, m._24,
	m._31, m._32, m._33, m._34,
	m._41, m._42, m._43, m._44)
{
}

inline MATRIX::MATRIX(CXMMATRIX m)
{
	XMStoreFloat4x4(this, m);
}

inline MATRIX::operator XMMATRIX() const
{
	return XMLoadFloat4x4(this);
}

inline bool MATRIX::operator == (const MATRIX& M) const
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

inline bool MATRIX::operator != (const MATRIX& M) const
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

inline MATRIX& MATRIX::operator = (const MATRIX& m)
{
	memcpy_s(this, sizeof(float) * 16, &m, sizeof(float) * 16);
	return *this;
}

inline MATRIX& MATRIX::operator += (const MATRIX& M)
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

inline MATRIX& MATRIX::operator -= (const MATRIX& M)
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

inline MATRIX& MATRIX::operator *= (const MATRIX& M)
{
	XMMATRIX M1 = XMLoadFloat4x4(this);
	XMMATRIX M2 = XMLoadFloat4x4(&M);
	XMMATRIX X = XMMatrixMultiply(M1, M2);
	XMStoreFloat4x4(this, X);
	return *this;
}

inline MATRIX& MATRIX::operator *= (float S)
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

inline MATRIX& MATRIX::operator /= (float S)
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

inline MATRIX& MATRIX::operator /= (const MATRIX& M)
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

inline MATRIX MATRIX::operator + () const
{
	return *this;
}

inline MATRIX MATRIX::operator - () const
{
	XMVECTOR v1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
	XMVECTOR v2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
	XMVECTOR v3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
	XMVECTOR v4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

	v1 = XMVectorNegate(v1);
	v2 = XMVectorNegate(v2);
	v3 = XMVectorNegate(v3);
	v4 = XMVectorNegate(v4);

	MATRIX R;
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), v1);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), v2);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), v3);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), v4);
	return R;
}

inline MATRIX MATRIX::operator + (const MATRIX& m) const
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

	MATRIX R;
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
	return R;
}

inline MATRIX MATRIX::operator - (const MATRIX& m) const
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

	MATRIX R;
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
	return R;
}

inline MATRIX MATRIX::operator * (const MATRIX& m) const
{
	XMMATRIX m1 = XMLoadFloat4x4(this);
	XMMATRIX m2 = XMLoadFloat4x4(&m);
	XMMATRIX X = XMMatrixMultiply(m1, m2);

	MATRIX R;
	XMStoreFloat4x4(&R, X);
	return R;
}

inline MATRIX MATRIX::operator * (float S) const
{
	XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_11));
	XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_21));
	XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_31));
	XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&_41));

	x1 = XMVectorScale(x1, S);
	x2 = XMVectorScale(x2, S);
	x3 = XMVectorScale(x3, S);
	x4 = XMVectorScale(x4, S);

	MATRIX R;
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
	return R;
}

inline MATRIX MATRIX::operator / (float S) const
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

	MATRIX R;
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
	return R;
}

inline MATRIX MATRIX::operator / (const MATRIX& m) const
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

	MATRIX R;
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
	return R;
}

inline MATRIX operator * (float S, const MATRIX& M)
{
	XMVECTOR x1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._11));
	XMVECTOR x2 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._21));
	XMVECTOR x3 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._31));
	XMVECTOR x4 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&M._41));

	x1 = XMVectorScale(x1, S);
	x2 = XMVectorScale(x2, S);
	x3 = XMVectorScale(x3, S);
	x4 = XMVectorScale(x4, S);

	MATRIX R;
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._11), x1);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._21), x2);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._31), x3);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&R._41), x4);
	return R;
}

//------------------------------------------------------------------------------
// Methods
//------------------------------------------------------------------------------

inline bool MATRIX::Decompose(VEC3& scale, QUAT& rotation, VEC3& translation)
{
	XMVECTOR s, r, t;

	if(!XMMatrixDecompose(&s, &r, &t, *this))
		return false;

	XMStoreFloat3(&scale, s);
	XMStoreFloat4(&rotation, r);
	XMStoreFloat3(&translation, t);

	return true;
}

inline float MATRIX::Determinant() const
{
	XMMATRIX M = XMLoadFloat4x4(this);
	return XMVectorGetX(XMMatrixDeterminant(M));
}

inline float MATRIX::GetYaw() const
{
	if(_21 > 0.998f || _21 < -0.998f)
		return atan2(_13, _33);
	else
		return atan2(-_31, _11);
}

inline void MATRIX::Identity()
{
	*this = IdentityMatrix;
}

inline MATRIX MATRIX::Inverse() const
{
	XMMATRIX M = XMLoadFloat4x4(this);
	MATRIX R;
	XMVECTOR det;
	XMStoreFloat4x4(&R, XMMatrixInverse(&det, M));
	return R;
}

inline void MATRIX::Inverse(MATRIX& result) const
{
	XMMATRIX M = XMLoadFloat4x4(this);
	XMVECTOR det;
	XMStoreFloat4x4(&result, XMMatrixInverse(&det, M));
}

inline MATRIX MATRIX::Transpose() const
{
	XMMATRIX M = XMLoadFloat4x4(this);
	MATRIX R;
	XMStoreFloat4x4(&R, XMMatrixTranspose(M));
	return R;
}

inline void MATRIX::Transpose(MATRIX& result) const
{
	XMMATRIX M = XMLoadFloat4x4(this);
	XMStoreFloat4x4(&result, XMMatrixTranspose(M));
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline MATRIX MATRIX::CreateBillboard(const VEC3& object, const VEC3& cameraPosition, const VEC3& cameraUp, const VEC3* cameraForward)
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

	MATRIX R;
	XMStoreFloat4x4(&R, M);
	return R;
}

inline MATRIX MATRIX::CreateConstrainedBillboard(const VEC3& object, const VEC3& cameraPosition, const VEC3& rotateAxis,
	const VEC3* cameraForward, const VEC3* objectForward)
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

	MATRIX R;
	XMStoreFloat4x4(&R, M);
	return R;
}

inline MATRIX MATRIX::CreateFromAxisAngle(const VEC3& axis, float angle)
{
	MATRIX R;
	XMVECTOR a = XMLoadFloat3(&axis);
	XMStoreFloat4x4(&R, XMMatrixRotationAxis(a, angle));
	return R;
}

inline MATRIX MATRIX::CreatePerspectiveFieldOfView(float fov, float aspectRatio, float nearPlane, float farPlane)
{
	MATRIX R;
	XMStoreFloat4x4(&R, XMMatrixPerspectiveFovRH(fov, aspectRatio, nearPlane, farPlane));
	return R;
}

inline MATRIX MATRIX::CreatePerspective(float width, float height, float nearPlane, float farPlane)
{
	MATRIX R;
	XMStoreFloat4x4(&R, XMMatrixPerspectiveRH(width, height, nearPlane, farPlane));
	return R;
}

inline MATRIX MATRIX::CreatePerspectiveOffCenter(float left, float right, float bottom, float top, float nearPlane, float farPlane)
{
	MATRIX R;
	XMStoreFloat4x4(&R, XMMatrixPerspectiveOffCenterRH(left, right, bottom, top, nearPlane, farPlane));
	return R;
}

inline MATRIX MATRIX::CreateOrthographic(float width, float height, float zNearPlane, float zFarPlane)
{
	MATRIX R;
	XMStoreFloat4x4(&R, XMMatrixOrthographicRH(width, height, zNearPlane, zFarPlane));
	return R;
}

inline MATRIX MATRIX::CreateOrthographicOffCenter(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane)
{
	MATRIX R;
	XMStoreFloat4x4(&R, XMMatrixOrthographicOffCenterRH(left, right, bottom, top, zNearPlane, zFarPlane));
	return R;
}

inline MATRIX MATRIX::CreateLookAt(const VEC3& eye, const VEC3& target, const VEC3& up)
{
	MATRIX R;
	XMVECTOR eyev = XMLoadFloat3(&eye);
	XMVECTOR targetv = XMLoadFloat3(&target);
	XMVECTOR upv = XMLoadFloat3(&up);
	XMStoreFloat4x4(&R, XMMatrixLookAtRH(eyev, targetv, upv));
	return R;
}

inline MATRIX MATRIX::CreateWorld(const VEC3& position, const VEC3& forward, const VEC3& up)
{
	XMVECTOR zaxis = XMVector3Normalize(XMVectorNegate(XMLoadFloat3(&forward)));
	XMVECTOR yaxis = XMLoadFloat3(&up);
	XMVECTOR xaxis = XMVector3Normalize(XMVector3Cross(yaxis, zaxis));
	yaxis = XMVector3Cross(zaxis, xaxis);

	MATRIX R;
	XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&R._11), xaxis);
	XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&R._21), yaxis);
	XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&R._31), zaxis);
	R._14 = R._24 = R._34 = 0.f;
	R._41 = position.x; R._42 = position.y; R._43 = position.z;
	R._44 = 1.f;
	return R;
}

inline MATRIX MATRIX::CreateShadow(const VEC3& lightDir, const PLANE& plane)
{
	MATRIX R;
	XMVECTOR light = XMLoadFloat3(&lightDir);
	XMVECTOR planev = XMLoadFloat4(&plane);
	XMStoreFloat4x4(&R, XMMatrixShadow(planev, light));
	return R;
}

inline MATRIX MATRIX::CreateReflection(const PLANE& plane)
{
	MATRIX R;
	XMVECTOR planev = XMLoadFloat4(&plane);
	XMStoreFloat4x4(&R, XMMatrixReflect(planev));
	return R;
}

inline void MATRIX::Lerp(const MATRIX& M1, const MATRIX& M2, float t, MATRIX& result)
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

inline MATRIX MATRIX::Lerp(const MATRIX& M1, const MATRIX& M2, float t)
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

	MATRIX result;
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._11), x1);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._21), x2);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._31), x3);
	XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&result._41), x4);
	return result;
}

inline MATRIX MATRIX::Rotation(float yaw, float pitch, float roll)
{
	MATRIX R;
	XMStoreFloat4x4(&R, XMMatrixRotationRollPitchYaw(pitch, yaw, roll));
	return R;
}

inline MATRIX MATRIX::Rotation(const VEC3& v)
{
	return Rotation(v.y, v.x, v.z);
}

inline MATRIX MATRIX::Rotation(const QUAT& rotation)
{
	MATRIX R;
	XMVECTOR quatv = XMLoadFloat4(&rotation);
	XMStoreFloat4x4(&R, XMMatrixRotationQuaternion(quatv));
	return R;
}

inline MATRIX MATRIX::RotationX(float radians)
{
	MATRIX R;
	XMStoreFloat4x4(&R, XMMatrixRotationX(radians));
	return R;
}

inline MATRIX MATRIX::RotationY(float radians)
{
	MATRIX R;
	XMStoreFloat4x4(&R, XMMatrixRotationY(radians));
	return R;
}

inline MATRIX MATRIX::RotationZ(float radians)
{
	MATRIX R;
	XMStoreFloat4x4(&R, XMMatrixRotationZ(radians));
	return R;
}

inline MATRIX MATRIX::Scale(const VEC3& scales)
{
	MATRIX R;
	XMStoreFloat4x4(&R, XMMatrixScaling(scales.x, scales.y, scales.z));
	return R;
}

inline MATRIX MATRIX::Scale(float xs, float ys, float zs)
{
	MATRIX R;
	XMStoreFloat4x4(&R, XMMatrixScaling(xs, ys, zs));
	return R;
}

inline MATRIX MATRIX::Scale(float scale)
{
	MATRIX R;
	XMStoreFloat4x4(&R, XMMatrixScaling(scale, scale, scale));
	return R;
}

inline void MATRIX::Transform(const MATRIX& M, const QUAT& rotation, MATRIX& result)
{
	XMVECTOR quatv = XMLoadFloat4(&rotation);

	XMMATRIX M0 = XMLoadFloat4x4(&M);
	XMMATRIX M1 = XMMatrixRotationQuaternion(quatv);

	XMStoreFloat4x4(&result, XMMatrixMultiply(M0, M1));
}

inline MATRIX MATRIX::Transform(const MATRIX& M, const QUAT& rotation)
{
	XMVECTOR quatv = XMLoadFloat4(&rotation);

	XMMATRIX M0 = XMLoadFloat4x4(&M);
	XMMATRIX M1 = XMMatrixRotationQuaternion(quatv);

	MATRIX result;
	XMStoreFloat4x4(&result, XMMatrixMultiply(M0, M1));
	return result;
}

inline MATRIX MATRIX::Transform2D(const VEC2* scaling_center, float scaling_rotation, const VEC2* scaling, const VEC2* rotation_center, float rotation, const VEC2* translation)
{
	XMVECTOR m_scaling_center = scaling_center ? *scaling_center : VEC2(0, 0),
		m_scaling = scaling ? *scaling : VEC2(1, 1),
		m_rotation_center = rotation_center ? *rotation_center : VEC2(0, 0),
		m_translation = translation ? *translation : VEC2(0, 0);
	return XMMatrixTransformation2D(m_scaling_center, scaling_rotation, m_scaling, m_rotation_center, rotation, m_translation);
}

inline MATRIX MATRIX::Translation(const VEC3& position)
{
	MATRIX R;
	XMStoreFloat4x4(&R, XMMatrixTranslation(position.x, position.y, position.z));
	return R;
}

inline MATRIX MATRIX::Translation(float x, float y, float z)
{
	MATRIX R;
	XMStoreFloat4x4(&R, XMMatrixTranslation(x, y, z));
	return R;
}

//*************************************************************************************************
//
// Quaternion
//
//*************************************************************************************************
inline QUAT::QUAT()
{
}

inline QUAT::QUAT(float x, float y, float z, float w) : XMFLOAT4(x, y, z, w)
{
}

inline QUAT::QUAT(const VEC3& v, float w) : XMFLOAT4(v.x, v.y, v.z, w)
{
}

inline QUAT::QUAT(const QUAT& q) : XMFLOAT4(q.x, q.y, q.z, q.w)
{
}

inline QUAT::QUAT(FXMVECTOR v)
{
	XMStoreFloat4(this, v);
}

inline QUAT::QUAT(const VEC4& v) : XMFLOAT4(v.x, v.y, v.z, v.w)
{
}

inline QUAT::QUAT(const XMVECTORF32& v) : XMFLOAT4(v.f[0], v.f[1], v.f[2], v.f[3])
{
}

inline QUAT::operator XMVECTOR() const
{
	return XMLoadFloat4(this);
}

inline bool QUAT::operator == (const QUAT& q) const
{
	XMVECTOR q1 = XMLoadFloat4(this);
	XMVECTOR q2 = XMLoadFloat4(&q);
	return XMQuaternionEqual(q1, q2);
}

inline bool QUAT::operator != (const QUAT& q) const
{
	XMVECTOR q1 = XMLoadFloat4(this);
	XMVECTOR q2 = XMLoadFloat4(&q);
	return XMQuaternionNotEqual(q1, q2);
}

inline QUAT& QUAT::operator = (const QUAT& q)
{
	x = q.x;
	y = q.y;
	z = q.z;
	w = q.w;
	return *this;
}

inline QUAT& QUAT::operator = (const XMVECTORF32& v)
{
	x = v.f[0];
	y = v.f[1];
	z = v.f[2];
	w = v.f[3];
	return *this;
}

inline QUAT& QUAT::operator += (const QUAT& q)
{
	XMVECTOR q1 = XMLoadFloat4(this);
	XMVECTOR q2 = XMLoadFloat4(&q);
	XMStoreFloat4(this, XMVectorAdd(q1, q2));
	return *this;
}

inline QUAT& QUAT::operator -= (const QUAT& q)
{
	XMVECTOR q1 = XMLoadFloat4(this);
	XMVECTOR q2 = XMLoadFloat4(&q);
	XMStoreFloat4(this, XMVectorSubtract(q1, q2));
	return *this;
}

inline QUAT& QUAT::operator *= (const QUAT& q)
{
	XMVECTOR q1 = XMLoadFloat4(this);
	XMVECTOR q2 = XMLoadFloat4(&q);
	XMStoreFloat4(this, XMQuaternionMultiply(q1, q2));
	return *this;
}

inline QUAT& QUAT::operator *= (float S)
{
	XMVECTOR q = XMLoadFloat4(this);
	XMStoreFloat4(this, XMVectorScale(q, S));
	return *this;
}

inline QUAT& QUAT::operator /= (const QUAT& q)
{
	XMVECTOR q1 = XMLoadFloat4(this);
	XMVECTOR q2 = XMLoadFloat4(&q);
	q2 = XMQuaternionInverse(q2);
	XMStoreFloat4(this, XMQuaternionMultiply(q1, q2));
	return *this;
}

inline QUAT QUAT::operator + () const
{
	return *this;
}

inline QUAT QUAT::operator - () const
{
	XMVECTOR q = XMLoadFloat4(this);

	QUAT R;
	XMStoreFloat4(&R, XMVectorNegate(q));
	return R;
}

inline QUAT QUAT::operator + (const QUAT& q) const
{
	XMVECTOR q1 = XMLoadFloat4(this);
	XMVECTOR q2 = XMLoadFloat4(&q);

	QUAT R;
	XMStoreFloat4(&R, XMVectorAdd(q1, q2));
	return R;
}

inline QUAT QUAT::operator - (const QUAT& q) const
{
	XMVECTOR q1 = XMLoadFloat4(this);
	XMVECTOR q2 = XMLoadFloat4(&q);

	QUAT R;
	XMStoreFloat4(&R, XMVectorSubtract(q1, q2));
	return R;
}

inline QUAT QUAT::operator * (const QUAT& q) const
{
	XMVECTOR q1 = XMLoadFloat4(this);
	XMVECTOR q2 = XMLoadFloat4(&q);

	QUAT R;
	XMStoreFloat4(&R, XMQuaternionMultiply(q1, q2));
	return R;
}

inline QUAT QUAT::operator * (float s) const
{
	XMVECTOR q = XMLoadFloat4(this);

	QUAT R;
	XMStoreFloat4(&R, XMVectorScale(q, s));
	return R;
}

inline QUAT QUAT::operator / (const QUAT& q) const
{
	XMVECTOR q1 = XMLoadFloat4(this);
	XMVECTOR q2 = XMLoadFloat4(&q);
	q2 = XMQuaternionInverse(q2);

	QUAT R;
	XMStoreFloat4(&R, XMQuaternionMultiply(q1, q2));
	return R;
}

inline QUAT operator* (float s, const QUAT& q)
{
	XMVECTOR q1 = XMLoadFloat4(&q);

	QUAT R;
	XMStoreFloat4(&R, XMVectorScale(q1, s));
	return R;
}

//------------------------------------------------------------------------------
// Methods
//------------------------------------------------------------------------------

inline void QUAT::Conjugate()
{
	XMVECTOR q = XMLoadFloat4(this);
	XMStoreFloat4(this, XMQuaternionConjugate(q));
}

inline void QUAT::Conjugate(QUAT& result) const
{
	XMVECTOR q = XMLoadFloat4(this);
	XMStoreFloat4(&result, XMQuaternionConjugate(q));
}

inline float QUAT::Dot(const QUAT& q) const
{
	XMVECTOR q1 = XMLoadFloat4(this);
	XMVECTOR q2 = XMLoadFloat4(&q);
	return XMVectorGetX(XMQuaternionDot(q1, q2));
}

inline void QUAT::Inverse(QUAT& result) const
{
	XMVECTOR q = XMLoadFloat4(this);
	XMStoreFloat4(&result, XMQuaternionInverse(q));
}

inline float QUAT::Length() const
{
	XMVECTOR q = XMLoadFloat4(this);
	return XMVectorGetX(XMQuaternionLength(q));
}

inline float QUAT::LengthSquared() const
{
	XMVECTOR q = XMLoadFloat4(this);
	return XMVectorGetX(XMQuaternionLengthSq(q));
}

inline void QUAT::Normalize()
{
	XMVECTOR q = XMLoadFloat4(this);
	XMStoreFloat4(this, XMQuaternionNormalize(q));
}

inline void QUAT::Normalize(QUAT& result) const
{
	XMVECTOR q = XMLoadFloat4(this);
	XMStoreFloat4(&result, XMQuaternionNormalize(q));
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline void QUAT::Concatenate(const QUAT& q1, const QUAT& q2, QUAT& result)
{
	XMVECTOR Q0 = XMLoadFloat4(&q1);
	XMVECTOR Q1 = XMLoadFloat4(&q2);
	XMStoreFloat4(&result, XMQuaternionMultiply(Q1, Q0));
}

inline QUAT QUAT::Concatenate(const QUAT& q1, const QUAT& q2)
{
	XMVECTOR Q0 = XMLoadFloat4(&q1);
	XMVECTOR Q1 = XMLoadFloat4(&q2);

	QUAT result;
	XMStoreFloat4(&result, XMQuaternionMultiply(Q1, Q0));
	return result;
}

inline QUAT QUAT::CreateFromAxisAngle(const VEC3& axis, float angle)
{
	XMVECTOR a = XMLoadFloat3(&axis);

	QUAT R;
	XMStoreFloat4(&R, XMQuaternionRotationAxis(a, angle));
	return R;
}

inline QUAT QUAT::CreateFromRotationMatrix(const MATRIX& M)
{
	XMMATRIX M0 = XMLoadFloat4x4(&M);

	QUAT R;
	XMStoreFloat4(&R, XMQuaternionRotationMatrix(M0));
	return R;
}

inline QUAT QUAT::CreateFromYawPitchRoll(float yaw, float pitch, float roll)
{
	QUAT R;
	XMStoreFloat4(&R, XMQuaternionRotationRollPitchYaw(pitch, yaw, roll));
	return R;
}

inline void QUAT::Lerp(const QUAT& q1, const QUAT& q2, float t, QUAT& result)
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

inline QUAT QUAT::Lerp(const QUAT& q1, const QUAT& q2, float t)
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

	QUAT result;
	XMStoreFloat4(&result, XMQuaternionNormalize(R));
	return result;
}

inline void QUAT::Slerp(const QUAT& q1, const QUAT& q2, float t, QUAT& result)
{
	XMVECTOR Q0 = XMLoadFloat4(&q1);
	XMVECTOR Q1 = XMLoadFloat4(&q2);
	XMStoreFloat4(&result, XMQuaternionSlerp(Q0, Q1, t));
}

inline QUAT QUAT::Slerp(const QUAT& q1, const QUAT& q2, float t)
{
	XMVECTOR Q0 = XMLoadFloat4(&q1);
	XMVECTOR Q1 = XMLoadFloat4(&q2);

	QUAT result;
	XMStoreFloat4(&result, XMQuaternionSlerp(Q0, Q1, t));
	return result;
}

//*************************************************************************************************
//
// Quaternion
//
//*************************************************************************************************
inline PLANE::PLANE()
{
}

inline PLANE::PLANE(float x, float y, float z, float w) : XMFLOAT4(x, y, z, w)
{
}

inline PLANE::PLANE(const VEC3& normal, float d) : XMFLOAT4(normal.x, normal.y, normal.z, d)
{
}

inline PLANE::PLANE(const VEC3& point1, const VEC3& point2, const VEC3& point3)
{
	XMVECTOR P0 = XMLoadFloat3(&point1);
	XMVECTOR P1 = XMLoadFloat3(&point2);
	XMVECTOR P2 = XMLoadFloat3(&point3);
	XMStoreFloat4(this, XMPlaneFromPoints(P0, P1, P2));
}

inline PLANE::PLANE(const VEC3& point, const VEC3& normal)
{
	XMVECTOR P = XMLoadFloat3(&point);
	XMVECTOR N = XMLoadFloat3(&normal);
	XMStoreFloat4(this, XMPlaneFromPointNormal(P, N));
}

inline PLANE::PLANE(FXMVECTOR v)
{
	XMStoreFloat4(this, v);
}

inline PLANE::PLANE(const VEC4& v) : XMFLOAT4(v.x, v.y, v.z, v.w)
{
}

inline PLANE::PLANE(const XMVECTORF32& v) : XMFLOAT4(v.f[0], v.f[1], v.f[2], v.f[3])
{
}

inline PLANE::operator XMVECTOR() const
{
	return XMLoadFloat4(this);
}

inline bool PLANE::operator == (const PLANE& p) const
{
	XMVECTOR p1 = XMLoadFloat4(this);
	XMVECTOR p2 = XMLoadFloat4(&p);
	return XMPlaneEqual(p1, p2);
}

inline bool PLANE::operator != (const PLANE& p) const
{
	XMVECTOR p1 = XMLoadFloat4(this);
	XMVECTOR p2 = XMLoadFloat4(&p);
	return XMPlaneNotEqual(p1, p2);
}

inline PLANE& PLANE::operator = (const PLANE& p)
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

inline void PLANE::Normalize()
{
	XMVECTOR p = XMLoadFloat4(this);
	XMStoreFloat4(this, XMPlaneNormalize(p));
}

inline void PLANE::Normalize(PLANE& result) const
{
	XMVECTOR p = XMLoadFloat4(this);
	XMStoreFloat4(&result, XMPlaneNormalize(p));
}

inline float PLANE::Dot(const VEC4& v) const
{
	XMVECTOR p = XMLoadFloat4(this);
	XMVECTOR v0 = XMLoadFloat4(&v);
	return XMVectorGetX(XMPlaneDot(p, v0));
}

inline float PLANE::DotCoordinate(const VEC3& position) const
{
	XMVECTOR p = XMLoadFloat4(this);
	XMVECTOR v0 = XMLoadFloat3(&position);
	return XMVectorGetX(XMPlaneDotCoord(p, v0));
}

inline float PLANE::DotNormal(const VEC3& normal) const
{
	XMVECTOR p = XMLoadFloat4(this);
	XMVECTOR n0 = XMLoadFloat3(&normal);
	return XMVectorGetX(XMPlaneDotNormal(p, n0));
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

inline void PLANE::Transform(const PLANE& plane, const MATRIX& M, PLANE& result)
{
	XMVECTOR p = XMLoadFloat4(&plane);
	XMMATRIX m0 = XMLoadFloat4x4(&M);
	XMStoreFloat4(&result, XMPlaneTransform(p, m0));
}

inline PLANE PLANE::Transform(const PLANE& plane, const MATRIX& M)
{
	XMVECTOR p = XMLoadFloat4(&plane);
	XMMATRIX m0 = XMLoadFloat4x4(&M);

	PLANE result;
	XMStoreFloat4(&result, XMPlaneTransform(p, m0));
	return result;
}

inline void PLANE::Transform(const PLANE& plane, const QUAT& rotation, PLANE& result)
{
	XMVECTOR p = XMLoadFloat4(&plane);
	XMVECTOR q = XMLoadFloat4(&rotation);
	XMVECTOR X = XMVector3Rotate(p, q);
	X = XMVectorSelect(p, X, g_XMSelect1110); // result.d = plane.d
	XMStoreFloat4(&result, X);
}

inline PLANE PLANE::Transform(const PLANE& plane, const QUAT& rotation)
{
	XMVECTOR p = XMLoadFloat4(&plane);
	XMVECTOR q = XMLoadFloat4(&rotation);
	XMVECTOR X = XMVector3Rotate(p, q);
	X = XMVectorSelect(p, X, g_XMSelect1110); // result.d = plane.d

	PLANE result;
	XMStoreFloat4(&result, X);
	return result;
}