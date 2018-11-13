/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za¿ó³æ gêœl¹ jaŸñ
 * Math - Modu³ matematyczny
 * Dokumentacja: Patrz plik doc/Math.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#include "Base.hpp"
#include <algorithm>
#include "Math.hpp"


namespace common
{

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// WEKTOR 2D, 3D i 4D Z FLOATÓW

const Vec2 Vec2::ZERO = Vec2(0.0f, 0.0f);
const Vec2 Vec2::ONE  = Vec2(1.0f, 1.0f);
const Vec2 Vec2::POSITIVE_X = Vec2(+1.0f, 0.0f);
const Vec2 Vec2::POSITIVE_Y = Vec2(0.0f, +1.0f);
const Vec2 Vec2::NEGATIVE_X = Vec2(-1.0f, 0.0f);
const Vec2 Vec2::NEGATIVE_Y = Vec2(0.0f, -1.0f);

const Vec3 Vec3::ZERO = Vec3(0.0f, 0.0f, 0.0f);
const Vec3 Vec3::ONE  = Vec3(1.0f, 1.0f, 1.0f);
const Vec3 Vec3::POSITIVE_X = Vec3(+1.0f, 0.0f, 0.0f);
const Vec3 Vec3::POSITIVE_Y = Vec3(0.0f, +1.0f, 0.0f);
const Vec3 Vec3::POSITIVE_Z = Vec3(0.0f, 0.0f, +1.0f);
const Vec3 Vec3::NEGATIVE_X = Vec3(-1.0f, 0.0f, 0.0f);
const Vec3 Vec3::NEGATIVE_Y = Vec3(0.0f, -1.0f, 0.0f);
const Vec3 Vec3::NEGATIVE_Z = Vec3(0.0f, 0.0f, -1.0f);

const Vec4 Vec4::ZERO = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
const Vec4 Vec4::ONE  = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
const Vec4 Vec4::POSITIVE_X = Vec4(+1.0f, 0.0f, 0.0f, 0.0f);
const Vec4 Vec4::POSITIVE_Y = Vec4(0.0f, +1.0f, 0.0f, 0.0f);
const Vec4 Vec4::POSITIVE_Z = Vec4(0.0f, 0.0f, +1.0f, 0.0f);
const Vec4 Vec4::POSITIVE_W = Vec4(0.0f, 0.0f, 0.0f, +1.0f);
const Vec4 Vec4::NEGATIVE_X = Vec4(-1.0f, 0.0f, 0.0f, 0.0f);
const Vec4 Vec4::NEGATIVE_Y = Vec4(0.0f, -1.0f, 0.0f, 0.0f);
const Vec4 Vec4::NEGATIVE_Z = Vec4(0.0f, 0.0f, -1.0f, 0.0f);
const Vec4 Vec4::NEGATIVE_W = Vec4(0.0f, 0.0f, 0.0f, -1.0f);

/*
Algorytmy ortogonalizacji wektorów to wzór Gram-Schmidt na podstwie ksi¹¿ki "3D
Math Primer for Graphics and Game Development", str. 134.
*/

void Orthogonalize_Fast(Vec3 *OutR1, Vec3 *OutR2, Vec3 *OutR3, const Vec3 &R1, const Vec3 &R2, const Vec3 &R3)
{
	*OutR1 = R1;
	*OutR2 = R2 - ( Dot(R2, *OutR1) / Dot(*OutR1, *OutR1) ) * (*OutR1);
	*OutR3 = R3 - ( Dot(R3, *OutR1) / Dot(*OutR1, *OutR1) ) * (*OutR1) - ( Dot(R3, *OutR2) / Dot(*OutR2, *OutR2) ) * (*OutR2);
}

void Orthogonalize(Vec3 *OutR1, Vec3 *OutR2, Vec3 *OutR3, const Vec3 &R1, const Vec3 &R2, const Vec3 &R3)
{
	const float K = 0.25f;
	const uint C = 10;

	Vec3 src1 = R1, src2 = R2, src3 = R3;
	Vec3 dst1, dst2, dst3;
	Vec3 *psrc1 = &src1, *psrc2 = &src2, *psrc3 = &src3;
	Vec3 *pdst1 = &dst1, *pdst2 = &dst2, *pdst3 = &dst3;

	for (uint i = 0; i < C; i++)
	{
		*pdst1 = (*psrc1) - K * ( Dot(*psrc1, *psrc2) / Dot(*psrc2, *psrc2) ) * (*psrc2) - K * ( Dot(*psrc1, *psrc3) / Dot(*psrc3, *psrc3) ) * (*psrc3);
		*pdst2 = (*psrc2) - K * ( Dot(*psrc2, *psrc1) / Dot(*psrc1, *psrc1) ) * (*psrc1) - K * ( Dot(*psrc2, *psrc3) / Dot(*psrc3, *psrc3) ) * (*psrc3);
		*pdst3 = (*psrc3) - K * ( Dot(*psrc3, *psrc1) / Dot(*psrc1, *psrc1) ) * (*psrc1) - K * ( Dot(*psrc3, *psrc2) / Dot(*psrc2, *psrc2) ) * (*psrc2);

		std::swap(psrc1, pdst1);
		std::swap(psrc2, pdst2);
		std::swap(psrc3, pdst3);
	}

	// Na koniec ¿eby siê upewniæ ¿e s¹ doskonale prostopad³e
	Orthogonalize_Fast(OutR1, OutR2, OutR3, *psrc1, *psrc2, *psrc3);
}

void PerpedicularVectors(Vec3 *Out1, Vec3 *Out2, const Vec3 &v)
{
	if (FLOAT_ALMOST_ZERO(v.x) && FLOAT_ALMOST_ZERO(v.y))
		Cross(Out1, v, Vec3::POSITIVE_Y);
	else
		Cross(Out1, v, Vec3::POSITIVE_Z);
	Cross(Out2, *Out1, v);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// TRÓJK¥T

float TriangleArea(const Vec2 &p1, const Vec2 &p2, const Vec2 &p3)
{
	return 0.5f * ( (p1.y-p3.y)*(p2.x-p3.x) + (p2.y-p3.y)*(p3.x-p1.x) );
}

float TriangleArea(const Vec3 &p1, const Vec3 &p2, const Vec3 &p3)
{
	Vec3 cr; Cross(&cr, p3-p2, p1-p3);
	return 0.5f * Length(cr);
}

float TrianglePerimeter(const Vec2 &p1, const Vec2 &p2, const Vec2 &p3)
{
	return Distance(p1, p2) + Distance(p2, p3) + Distance(p3, p1);
}

float TrianglePerimeter(const Vec3 &p1, const Vec3 &p2, const Vec3 &p3)
{
	return Distance(p1, p2) + Distance(p2, p3) + Distance(p3, p1);
}

void CalculateBarycentric(const Vec3 &p, const Vec3 &p1, const Vec3 &p2, const Vec3 &p3, const Vec3 *Normal, float *e, float *f, float *g)
{
	// Na podstawie ksia¿ki "3D Math Primer for Graphics and Game Development", str. 263.

	// First, compute two clockwise edge vectors
	Vec3 d1 = p2-p1;
	Vec3 d2 = p3-p2;

	// Compute surface normal using cross product. In many cases
	// this step could be skipped, since we would have the surface
	// normal precomputed. We do not need to normalize it, although
	// if a precomputed normal was normalized, it would be OK.
	Vec3 n;
	if (Normal)
		n = *Normal;
	else
		Cross(&n, d1, d2);

	// Locate dominant axis of normal, and select plane of projection
	float u1, u2, u3, u4;
	float v1, v2, v3, v4;
	if ((fabsf(n.x) >= fabsf(n.y)) && (fabsf(n.x) >= fabsf(n.z)))
	{
		// Discard x, project onto yz plane
		u1 = p1.y-p3.y;
		u2 = p2.y-p3.y;
		u3 = p.y-p1.y;
		u4 = p.y-p3.y;

		v1 = p1.z-p3.z;
		v2 = p2.z-p3.z;
		v3 = p.z-p1.z;
		v4 = p.z-p3.z;
	}
	else if (fabsf(n.y) >= fabsf(n.z))
	{
		// Discard y, project onto xz plane
		u1 = p1.z-p3.z;
		u2 = p2.z-p3.z;
		u3 = p.z-p1.z;
		u4 = p.z-p3.z;

		v1 = p1.x-p3.x;
		v2 = p2.x-p3.x;
		v3 = p.x-p1.x;
		v4 = p.x-p3.x;
	}
	else
	{
		u1 = p1.x-p3.x;
		u2 = p2.x-p3.x;
		u3 = p.x-p1.x;
		u4 = p.x-p3.x;

		v1 = p1.y-p3.y;
		v2 = p2.y-p3.y;
		v3 = p.y-p1.y;
		v4 = p.y-p3.y;
	}

	// Compute denominator
	float denom = v1*u2 - v2*u1;
	// Jeœli denom ~= 0.0f to bêdzie dzielenie przez zero - trudno :P
	// Wtedy podany trójk¹t jest zdegenerowany.

	// Compute barycentric coordinates
	float OneOverDenom = 1.0f / denom;
	if (e) *e = (v4*u2 - v2*u4) * OneOverDenom;
	if (f) *f = (v1*u3 - v3*u1) * OneOverDenom;
	if (g) *g = 1.0f - *e - *f;
}

/*
Funkcje TriangleInscribedCircle i TriangleCircumscribedCircle s¹ napisane na
podstawie nVidia SDK, modu³ nv_algebra.
*/

void TriangleInscribedCircle(Vec2 *OutCenter, float *OutRadius, const Vec2 &v1, const Vec2 &v2, const Vec2 &v3)
{
	float area = fabsf(TriangleArea(v1, v2, v3));
	if (FLOAT_ALMOST_ZERO(area))
	{
		*OutCenter = v1;
		*OutRadius = 0.f;
		return;
	}

	float oo_perim = 1.f / TrianglePerimeter(v1, v2, v3);

	Vec2 diff;

	diff = v2 - v3;
	*OutCenter = v1 * Length(diff);

	diff = v3 - v1;
	*OutCenter += v2 * Length(diff);

	diff = v1 - v2;
	*OutCenter += v3 * Length(diff);

	*OutCenter *= oo_perim;
	*OutRadius = 2.f * area * oo_perim;
}

void TriangleInscribedCircle(Vec3 *OutCenter, float *OutRadius, const Vec3 &v1, const Vec3 &v2, const Vec3 &v3)
{
	float area = fabsf(TriangleArea(v1, v2, v3));
	if (FLOAT_ALMOST_ZERO(area))
	{
		*OutCenter = v1;
		*OutRadius = 0.f;
		return;
	}

	float oo_perim = 1.f / TrianglePerimeter(v1, v2, v3);

	Vec3 diff;

	diff = v2 - v3;
	*OutCenter = v1 * Length(diff);

	diff = v3 - v1;
	*OutCenter += v2 * Length(diff);

	diff = v1 - v2;
	*OutCenter += v3 * Length(diff);

	*OutCenter *= oo_perim;
	*OutRadius = 2.f * area * oo_perim;
}

void TriangleCircumscribedCircle(Vec2 *OutCenter, float *OutRadius, const Vec2 &v1, const Vec2 &v2, const Vec2 &v3)
{
	Vec2 e0, e1;
	float d1, d2, d3;
	float c1, c2, c3, oo_c;

	e0 = v3 - v1;
	e1 = v2 - v1;
	d1 = Dot(e0, e1);

	e0 = v3 - v2;
	e1 = v1 - v2;
	d2 = Dot(e0, e1);

	e0 = v1 - v3;
	e1 = v2 - v3;
	d3 = Dot(e0, e1);

	c1 = d2 * d3;
	c2 = d3 * d1;
	c3 = d1 * d2;
	oo_c = 1.f / (c1 + c2 + c3);

	*OutCenter  = v1 * (c2 + c3);
	*OutCenter += v2 * (c3 + c1);
	*OutCenter += v3 * (c1 + c2);
	*OutCenter *= oo_c * 0.5f;

	*OutRadius = 0.5f * sqrtf( (d1 + d2) * (d2 + d3) * (d3 + d1) * oo_c);
}

void TriangleCircumscribedCircle(Vec3 *OutCenter, float *OutRadius, const Vec3 &v1, const Vec3 &v2, const Vec3 &v3)
{
	Vec3 e0, e1;
	float d1, d2, d3;
	float c1, c2, c3, oo_c;

	e0 = v3 - v1;
	e1 = v2 - v1;
	d1 = Dot(e0, e1);

	e0 = v3 - v2;
	e1 = v1 - v2;
	d2 = Dot(e0, e1);

	e0 = v1 - v3;
	e1 = v2 - v3;
	d3 = Dot(e0, e1);

	c1 = d2 * d3;
	c2 = d3 * d1;
	c3 = d1 * d2;
	oo_c = 1.f / (c1 + c2 + c3);

	*OutCenter  = v1 * (c2 + c3);
	*OutCenter += v2 * (c3 + c1);
	*OutCenter += v3 * (c1 + c2);
	*OutCenter *= oo_c * 0.5f;

	*OutRadius = 0.5f * sqrtf( (d1 + d2) * (d2 + d3) * (d3 + d1) * oo_c);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// PROSTOK¥T

const RECTI RECTI::ZERO = RECTI(0, 0, 0, 0);
const RECTF RECTF::ZERO = RECTF(0.0f, 0.0f, 0.0f, 0.0f);


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// AXIS ALIGNED BOUNDING BOX

const BOX BOX::ZERO = BOX(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// P£ASZCZYZNA

const PLANE PLANE::ZERO = PLANE(0.0f, 0.0f, 0.0f, 0.0f);
const PLANE PLANE::POSITIVE_X = PLANE(1.0f, 0.0f, 0.0f, 0.0f);
const PLANE PLANE::POSITIVE_Y = PLANE(0.0f, 1.0f, 0.0f, 0.0f);
const PLANE PLANE::POSITIVE_Z = PLANE(0.0f, 0.0f, 1.0f, 0.0f);
const PLANE PLANE::NEGATIVE_X = PLANE(-1.0f, 0.0f, 0.0f, 0.0f);
const PLANE PLANE::NEGATIVE_Y = PLANE(0.0f, -1.0f, 0.0f, 0.0f);
const PLANE PLANE::NEGATIVE_Z = PLANE(0.0f, 0.0f, -1.0f, 0.0f);


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// PROSTA 2D

const LINE2D ZERO = LINE2D(0.f, 0.f, 0.f);
const LINE2D POSITIVE_X = LINE2D( 1.f, 0.f, 0.f);
const LINE2D NEGATIVE_X = LINE2D(-1.f, 0.f, 0.f);
const LINE2D POSITIVE_Y = LINE2D(0.f,  1.f, 0.f);
const LINE2D NEGATIVE_Y = LINE2D(0.f, -1.f, 0.f);

bool LinesIntersection(Vec2 *Out, const LINE2D &L1, const LINE2D &L2)
{
	float DET1 = L1.a*L2.b - L1.b*L2.a;
	if (FLOAT_ALMOST_ZERO(DET1)) return false;
	Out->x = (L1.b*L2.c - L1.c*L2.b) / DET1;
	Out->y = (L1.c*L2.a - L1.a*L2.c) / DET1;
	return true;
}

bool PointsCollinear(const Vec2 &p1, const Vec2 &p2, const Vec2 &p3)
{
	return FLOAT_ALMOST_ZERO(
		p1.x*p2.y + p1.y*p3.x + p2.x*p3.y -
		p3.x*p2.y - p2.x*p1.y - p3.y*p1.x);
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// MACIERZ 4 x 4

const MATRIX MATRIX::ZERO = MATRIX(
	0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f);
const MATRIX MATRIX::IDENTITY = MATRIX(
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f);

MATRIX MATRIX::operator - () const
{
	return MATRIX(
		-_11, -_12, -_13, -_14,
		-_21, -_22, -_23, -_24,
		-_31, -_32, -_33, -_34,
		-_41, -_42, -_43, -_44);
}

MATRIX MATRIX::operator + (const MATRIX &m) const
{
	return MATRIX(
		_11 + m._11, _12 + m._12, _13 + m._13, _14 + m._14,
		_21 + m._21, _22 + m._22, _23 + m._23, _24 + m._24,
		_31 + m._31, _32 + m._32, _33 + m._33, _34 + m._34,
		_41 + m._41, _42 + m._42, _43 + m._43, _44 + m._44);
}

MATRIX MATRIX::operator - (const MATRIX &m) const
{
	return MATRIX(
		_11 - m._11, _12 - m._12, _13 - m._13, _14 - m._14,
		_21 - m._21, _22 - m._22, _23 - m._23, _24 - m._24,
		_31 - m._31, _32 - m._32, _33 - m._33, _34 - m._34,
		_41 - m._41, _42 - m._42, _43 - m._43, _44 - m._44);
}

MATRIX & MATRIX::operator += (const MATRIX &m)
{
	_11 += m._11; _12 += m._12; _13 += m._13; _14 += m._14;
	_21 += m._21; _22 += m._22; _23 += m._23; _24 += m._24;
	_31 += m._31; _32 += m._32; _33 += m._33; _34 += m._34;
	_41 += m._41; _42 += m._42; _43 += m._43; _44 += m._44;
	return *this;
}

MATRIX & MATRIX::operator -= (const MATRIX &m)
{
	_11 -= m._11; _12 -= m._12; _13 -= m._13; _14 -= m._14;
	_21 -= m._21; _22 -= m._22; _23 -= m._23; _24 -= m._24;
	_31 -= m._31; _32 -= m._32; _33 -= m._33; _34 -= m._34;
	_41 -= m._41; _42 -= m._42; _43 -= m._43; _44 -= m._44;
	return *this;
}

MATRIX MATRIX::operator * (float s) const
{
	return MATRIX(
		_11 * s, _12 * s, _13 * s, _14 * s,
		_21 * s, _22 * s, _23 * s, _24 * s,
		_31 * s, _32 * s, _33 * s, _34 * s,
		_41 * s, _42 * s, _43 * s, _44 * s);
}

MATRIX MATRIX::operator / (float s) const
{
	s = 1.0f / s;
	return MATRIX(
		_11 * s, _12 * s, _13 * s, _14 * s,
		_21 * s, _22 * s, _23 * s, _24 * s,
		_31 * s, _32 * s, _33 * s, _34 * s,
		_41 * s, _42 * s, _43 * s, _44 * s);
}

MATRIX & MATRIX::operator *= (float s)
{
	_11 *= s; _12 *= s; _13 *= s; _14 *= s;
	_21 *= s; _22 *= s; _23 *= s; _24 *= s;
	_31 *= s; _32 *= s; _33 *= s; _34 *= s;
	_41 *= s; _42 *= s; _43 *= s; _44 *= s;
	return *this;
}

MATRIX & MATRIX::operator /= (float s)
{
	s = 1.0f / s;
	_11 *= s; _12 *= s; _13 *= s; _14 *= s;
	_21 *= s; _22 *= s; _23 *= s; _24 *= s;
	_31 *= s; _32 *= s; _33 *= s; _34 *= s;
	_41 *= s; _42 *= s; _43 *= s; _44 *= s;
	return *this;
}

MATRIX & MATRIX::operator *= (const MATRIX &m)
{
	MATRIX Tmp = MATRIX(
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
	*this = Tmp;

	return *this;
}

bool MatrixEqual(const MATRIX &m1, const MATRIX &m2)
{
	return
		float_equal(m1._11, m2._11) && float_equal(m1._12, m2._12) && float_equal(m1._13, m2._13) && float_equal(m1._14, m2._14) &&
		float_equal(m1._21, m2._21) && float_equal(m1._22, m2._22) && float_equal(m1._23, m2._23) && float_equal(m1._24, m2._24) &&
		float_equal(m1._31, m2._31) && float_equal(m1._32, m2._32) && float_equal(m1._33, m2._33) && float_equal(m1._34, m2._34) &&
		float_equal(m1._41, m2._41) && float_equal(m1._42, m2._42) && float_equal(m1._43, m2._43) && float_equal(m1._44, m2._44);
}

void Minus(MATRIX *m)
{
	m->_11 = -m->_11; m->_12 = -m->_12; m->_13 = -m->_13; m->_14 = -m->_14;
	m->_21 = -m->_21; m->_22 = -m->_22; m->_23 = -m->_23; m->_24 = -m->_24;
	m->_31 = -m->_31; m->_32 = -m->_32; m->_33 = -m->_33; m->_34 = -m->_34;
	m->_41 = -m->_41; m->_42 = -m->_42; m->_43 = -m->_43; m->_44 = -m->_44;
}

void Add(MATRIX *Out, const MATRIX &m1, const MATRIX &m2)
{
	Out->_11 = m1._11 + m2._11; Out->_12 = m1._12 + m2._12; Out->_13 = m1._13 + m2._13; Out->_14 = m1._14 + m2._14;
	Out->_21 = m1._21 + m2._21; Out->_22 = m1._22 + m2._22; Out->_23 = m1._23 + m2._23; Out->_24 = m1._24 + m2._24;
	Out->_31 = m1._31 + m2._31; Out->_32 = m1._32 + m2._32; Out->_33 = m1._33 + m2._33; Out->_34 = m1._34 + m2._34;
	Out->_41 = m1._41 + m2._41; Out->_42 = m1._42 + m2._42; Out->_43 = m1._43 + m2._43; Out->_44 = m1._44 + m2._44;
}

void Sub(MATRIX *Out, const MATRIX &m1, const MATRIX &m2)
{
	Out->_11 = m1._11 - m2._11; Out->_12 = m1._12 - m2._12; Out->_13 = m1._13 - m2._13; Out->_14 = m1._14 - m2._14;
	Out->_21 = m1._21 - m2._21; Out->_22 = m1._22 - m2._22; Out->_23 = m1._23 - m2._23; Out->_24 = m1._24 - m2._24;
	Out->_31 = m1._31 - m2._31; Out->_32 = m1._32 - m2._32; Out->_33 = m1._33 - m2._33; Out->_34 = m1._34 - m2._34;
	Out->_41 = m1._41 - m2._41; Out->_42 = m1._42 - m2._42; Out->_43 = m1._43 - m2._43; Out->_44 = m1._44 - m2._44;
}

void Mul(MATRIX *Out, const MATRIX &m1, const MATRIX &m2)
{
	Out->_11 = m1._11 * m2._11 + m1._12 * m2._21 + m1._13 * m2._31 + m1._14 * m2._41;
	Out->_12 = m1._11 * m2._12 + m1._12 * m2._22 + m1._13 * m2._32 + m1._14 * m2._42;
	Out->_13 = m1._11 * m2._13 + m1._12 * m2._23 + m1._13 * m2._33 + m1._14 * m2._43;
	Out->_14 = m1._11 * m2._14 + m1._12 * m2._24 + m1._13 * m2._34 + m1._14 * m2._44;

	Out->_21 = m1._21 * m2._11 + m1._22 * m2._21 + m1._23 * m2._31 + m1._24 * m2._41;
	Out->_22 = m1._21 * m2._12 + m1._22 * m2._22 + m1._23 * m2._32 + m1._24 * m2._42;
	Out->_23 = m1._21 * m2._13 + m1._22 * m2._23 + m1._23 * m2._33 + m1._24 * m2._43;
	Out->_24 = m1._21 * m2._14 + m1._22 * m2._24 + m1._23 * m2._34 + m1._24 * m2._44;

	Out->_31 = m1._31 * m2._11 + m1._32 * m2._21 + m1._33 * m2._31 + m1._34 * m2._41;
	Out->_32 = m1._31 * m2._12 + m1._32 * m2._22 + m1._33 * m2._32 + m1._34 * m2._42;
	Out->_33 = m1._31 * m2._13 + m1._32 * m2._23 + m1._33 * m2._33 + m1._34 * m2._43;
	Out->_34 = m1._31 * m2._14 + m1._32 * m2._24 + m1._33 * m2._34 + m1._34 * m2._44;

	Out->_41 = m1._41 * m2._11 + m1._42 * m2._21 + m1._43 * m2._31 + m1._44 * m2._41;
	Out->_42 = m1._41 * m2._12 + m1._42 * m2._22 + m1._43 * m2._32 + m1._44 * m2._42;
	Out->_43 = m1._41 * m2._13 + m1._42 * m2._23 + m1._43 * m2._33 + m1._44 * m2._43;
	Out->_44 = m1._41 * m2._14 + m1._42 * m2._24 + m1._43 * m2._34 + m1._44 * m2._44;
}

void Mul(MATRIX *m, float s)
{
	m->_11 *= s; m->_12 *= s; m->_13 *= s; m->_14 *= s;
	m->_21 *= s; m->_22 *= s; m->_23 *= s; m->_24 *= s;
	m->_31 *= s; m->_32 *= s; m->_33 *= s; m->_34 *= s;
	m->_41 *= s; m->_42 *= s; m->_43 *= s; m->_44 *= s;
}

void Div(MATRIX *m, float s)
{
	s = 1.0f / s;
	m->_11 *= s; m->_12 *= s; m->_13 *= s; m->_14 *= s;
	m->_21 *= s; m->_22 *= s; m->_23 *= s; m->_24 *= s;
	m->_31 *= s; m->_32 *= s; m->_33 *= s; m->_34 *= s;
	m->_41 *= s; m->_42 *= s; m->_43 *= s; m->_44 *= s;
}

void MatrixToStr(string *Out, const MATRIX &m)
{
	string s;

	FloatToStr(&s, m._11); *Out  = s; *Out += ',';
	FloatToStr(&s, m._12); *Out += s; *Out += ',';
	FloatToStr(&s, m._13); *Out += s; *Out += ',';
	FloatToStr(&s, m._14); *Out += s; *Out += ';';

	FloatToStr(&s, m._21); *Out += s; *Out += ',';
	FloatToStr(&s, m._22); *Out += s; *Out += ',';
	FloatToStr(&s, m._23); *Out += s; *Out += ',';
	FloatToStr(&s, m._24); *Out += s; *Out += ';';

	FloatToStr(&s, m._31); *Out += s; *Out += ',';
	FloatToStr(&s, m._32); *Out += s; *Out += ',';
	FloatToStr(&s, m._33); *Out += s; *Out += ',';
	FloatToStr(&s, m._34); *Out += s; *Out += ';';

	FloatToStr(&s, m._41); *Out += s; *Out += ',';
	FloatToStr(&s, m._42); *Out += s; *Out += ',';
	FloatToStr(&s, m._43); *Out += s; *Out += ',';
	FloatToStr(&s, m._44); *Out += s;
}

bool StrToMatrix(MATRIX *Out, const string &Str)
{
	string Parts[4], Numbers[4], Dupa;
	size_t Index = 0;
	string Delimiter1 = ";", Delimiter2 = ",";
	if (!Split(Str, Delimiter1, &Parts[0], &Index)) return false;
	if (!Split(Str, Delimiter1, &Parts[1], &Index)) return false;
	if (!Split(Str, Delimiter1, &Parts[2], &Index)) return false;
	if (!Split(Str, Delimiter1, &Parts[3], &Index)) return false;
	if (Split(Str, Delimiter1, &Dupa, &Index)) return false;

	for (size_t PartI = 0; PartI < 4; PartI++)
	{
		Index = 0;
		if (!Split(Parts[PartI], Delimiter2, &Numbers[0], &Index)) return false;
		if (!Split(Parts[PartI], Delimiter2, &Numbers[1], &Index)) return false;
		if (!Split(Parts[PartI], Delimiter2, &Numbers[2], &Index)) return false;
		if (!Split(Parts[PartI], Delimiter2, &Numbers[3], &Index)) return false;
		if (Split(Parts[PartI], Delimiter2, &Dupa, &Index)) return false;

		if (StrToFloat(&Out->operator ()(PartI, 0), Numbers[0]) != 0) return false;
		if (StrToFloat(&Out->operator ()(PartI, 1), Numbers[1]) != 0) return false;
		if (StrToFloat(&Out->operator ()(PartI, 2), Numbers[2]) != 0) return false;
		if (StrToFloat(&Out->operator ()(PartI, 3), Numbers[3]) != 0) return false;
	}

	return true;
}

/* Wzór mno¿enia wektora przez macierz:
	Out->x = v.x * m._11 + v.y * m._21 + v.z * m._31 + v.w * m._41;
	Out->y = v.x * m._12 + v.y * m._22 + v.z * m._32 + v.w * m._42;
	Out->z = v.x * m._13 + v.y * m._23 + v.z * m._33 + v.w * m._43;
	Out->w = v.x * m._14 + v.y * m._24 + v.z * m._34 + v.w * m._44;
*/

void Transform(Vec4 *Out, const Vec2 &v, const MATRIX &m)
{
	Out->x = v.x * m._11 + v.y * m._21 + m._41;
	Out->y = v.x * m._12 + v.y * m._22 + m._42;
	Out->z = v.x * m._13 + v.y * m._23 + m._43;
	Out->w = v.x * m._14 + v.y * m._24 + m._44;
}

void Transform(Vec2 *Out, const Vec2 &v, const MATRIX &m)
{
	Out->x = v.x * m._11 + v.y * m._21 + m._41;
	Out->y = v.x * m._12 + v.y * m._22 + m._42;
}

void TransformCoord(Vec2 *Out, const Vec2 &v, const MATRIX &m)
{
	Vec3 Out1; // To tak naprawdê [X,Y,0,W]
	Out1.x = v.x * m._11 + v.y * m._21 + m._41;
	Out1.y = v.x * m._12 + v.y * m._22 + m._42;
	Out1.z = v.x * m._14 + v.y * m._24 + m._44;
	float wrc = 1.0f / Out1.z;
	Out->x = Out1.x * wrc;
	Out->y = Out1.y * wrc;
}

void TransformNormal(Vec2 *Out, const Vec2 &v, const MATRIX &m)
{
	Out->x = v.x * m._11 + v.y * m._21;
	Out->y = v.x * m._12 + v.y * m._22;
}

void Transform(Vec4 *Out, const Vec3 &v, const MATRIX &m)
{
	Out->x = v.x * m._11 + v.y * m._21 + v.z * m._31 + m._41;
	Out->y = v.x * m._12 + v.y * m._22 + v.z * m._32 + m._42;
	Out->z = v.x * m._13 + v.y * m._23 + v.z * m._33 + m._43;
	Out->w = v.x * m._14 + v.y * m._24 + v.z * m._34 + m._44;
}

void Transform(Vec3 *Out, const Vec3 &v, const MATRIX &m)
{
	Out->x = v.x * m._11 + v.y * m._21 + v.z * m._31 + m._41;
	Out->y = v.x * m._12 + v.y * m._22 + v.z * m._32 + m._42;
	Out->z = v.x * m._13 + v.y * m._23 + v.z * m._33 + m._43;
}

void TransformCoord(Vec3 *Out, const Vec3 &v, const MATRIX &m)
{
	Vec4 Out1, vv = Vec4(v.x, v.y, v.z, 1.0f);
	Out1.x = vv.x * m._11 + vv.y * m._21 + vv.z * m._31 + vv.w * m._41;
	Out1.y = vv.x * m._12 + vv.y * m._22 + vv.z * m._32 + vv.w * m._42;
	Out1.z = vv.x * m._13 + vv.y * m._23 + vv.z * m._33 + vv.w * m._43;
	Out1.w = vv.x * m._14 + vv.y * m._24 + vv.z * m._34 + vv.w * m._44;
	float wrc = 1.0f / Out1.w;
	Out->x = Out1.x * wrc;
	Out->y = Out1.y * wrc;
	Out->z = Out1.z * wrc;
}

void TransformNormal(Vec3 *Out, const Vec3 &v, const MATRIX &m)
{
	Out->x = v.x * m._11 + v.y * m._21 + v.z * m._31;
	Out->y = v.x * m._12 + v.y * m._22 + v.z * m._32;
	Out->z = v.x * m._13 + v.y * m._23 + v.z * m._33;
}

void Transform(Vec4 *Out, const Vec4 &v, const MATRIX &m)
{
	Out->x = v.x * m._11 + v.y * m._21 + v.z * m._31 + v.w * m._41;
	Out->y = v.x * m._12 + v.y * m._22 + v.z * m._32 + v.w * m._42;
	Out->z = v.x * m._13 + v.y * m._23 + v.z * m._33 + v.w * m._43;
	Out->w = v.x * m._14 + v.y * m._24 + v.z * m._34 + v.w * m._44;
}

void Transform(PLANE *Out, const PLANE &p, const MATRIX &m)
{
	Out->a = p.a * m._11 + p.b * m._21 + p.c * m._31 + p.d * m._41;
	Out->b = p.a * m._12 + p.b * m._22 + p.c * m._32 + p.d * m._42;
	Out->c = p.a * m._13 + p.b * m._23 + p.c * m._33 + p.d * m._43;
	Out->d = p.a * m._14 + p.b * m._24 + p.c * m._34 + p.d * m._44;
}

void TransformArray(Vec3 OutPoints[], const Vec3 InPoints[], size_t PointCount, const MATRIX &M)
{
	for (size_t i = 0; i < PointCount; i++)
		Transform(&OutPoints[i], InPoints[i], M);
}

void TransformArray(Vec3 InOutPoints[], size_t PointCount, const MATRIX &M)
{
	Vec3 v;
	for (size_t i = 0; i < PointCount; i++)
	{
		Transform(&v, InOutPoints[i], M);
		InOutPoints[i] = v;
	}
}

void TransformNormalArray(Vec3 OutPoints[], const Vec3 InPoints[], size_t PointCount, const MATRIX &M)
{
	for (size_t i = 0; i < PointCount; i++)
		TransformNormal(&OutPoints[i], InPoints[i], M);
}

void TransformNormalArray(Vec3 InOutPoints[], size_t PointCount, const MATRIX &M)
{
	Vec3 v;
	for (size_t i = 0; i < PointCount; i++)
	{
		TransformNormal(&v, InOutPoints[i], M);
		InOutPoints[i] = v;
	}
}

void TransformCoordArray(Vec3 OutPoints[], const Vec3 InPoints[], size_t PointCount, const MATRIX &M)
{
	for (size_t i = 0; i < PointCount; i++)
		TransformCoord(&OutPoints[i], InPoints[i], M);
}

void TransformCoordArray(Vec3 InOutPoints[], size_t PointCount, const MATRIX &M)
{
	Vec3 v;
	for (size_t i = 0; i < PointCount; i++)
	{
		TransformCoord(&v, InOutPoints[i], M);
		InOutPoints[i] = v;
	}
}

void TransformRay(Vec3 *OutOrigin, Vec3 *OutDir, const Vec3 &RayOrigin, const Vec3 &RayDir, const MATRIX &m)
{
	Transform(OutOrigin, RayOrigin, m);
	TransformNormal(OutDir, RayDir, m);
}

void TransformBox(BOX *Out, const BOX &In, const MATRIX &M)
{
	Vec3 p1, p2;

	In.GetCorner(&p1, 0);
	Transform(&p2, p1, M);
	Out->p1 = Out->p2 = p2;

	for (size_t i = 1; i < 8; i++)
	{
		In.GetCorner(&p1, i);
		Transform(&p2, p1, M);
		Out->AddInternalPoint(p2);
	}
}

void TransformBoxCoord(BOX *Out, const BOX &In, const MATRIX &M)
{
	Vec3 p1, p2;

	In.GetCorner(&p1, 0);
	TransformCoord(&p2, p1, M);
	Out->p1 = Out->p2 = p2;

	for (size_t i = 1; i < 8; i++)
	{
		In.GetCorner(&p1, i);
		TransformCoord(&p2, p1, M);
		Out->AddInternalPoint(p2);
	}
}

/*
Wzór wype³niania macierzy (ta jest jednostkowa):
	Out->_11 = 1.0f; Out->_12 = 0.0f; Out->_13 = 0.0f; Out->_14 = 0.0f;
	Out->_21 = 0.0f; Out->_22 = 1.0f; Out->_23 = 0.0f; Out->_24 = 0.0f;
	Out->_31 = 0.0f; Out->_32 = 0.0f; Out->_33 = 1.0f; Out->_34 = 0.0f;
	Out->_41 = 0.0f; Out->_42 = 0.0f; Out->_43 = 0.0f; Out->_44 = 1.0f;
*/

void AxesToMatrix(MATRIX *Out, const Vec3 &AxisX, const Vec3 &AxisY, const Vec3 &AxisZ)
{
	// To by³o Ÿle! Nie wiem sk¹d siê to wziê³o, ale zgodnie z ksi¹¿k¹
	// "3D Math Primer for Graphics and Game Developers" oraz z moimi obliczeniami
	// kiedy wektory s¹ wierszowe i mno¿one prez macierz w kolejnoœci wektor*macierz
	// (czyli tak jak w DirectX) to do tej macierzy wpisujemy poszczególne wektory osi
	// w wierszach, a nie w kolumnach!
	//Out->_11 = AxisX.x; Out->_12 = AxisY.x; Out->_13 = AxisZ.x; Out->_14 = 0.0f;
	//Out->_21 = AxisX.y; Out->_22 = AxisY.y; Out->_23 = AxisZ.y; Out->_24 = 0.0f;
	//Out->_31 = AxisX.z; Out->_32 = AxisY.z; Out->_33 = AxisZ.z; Out->_34 = 0.0f;
	//Out->_41 = 0.0f;    Out->_42 = 0.0f;    Out->_43 = 0.0f;    Out->_44 = 1.0f;

	// Tak jest dobrze:
	Out->_11 = AxisX.x; Out->_12 = AxisX.y; Out->_13 = AxisX.z; Out->_14 = 0.0f;
	Out->_21 = AxisY.x; Out->_22 = AxisY.y; Out->_23 = AxisY.z; Out->_24 = 0.0f;
	Out->_31 = AxisZ.x; Out->_32 = AxisZ.y; Out->_33 = AxisZ.z; Out->_34 = 0.0f;
	Out->_41 = 0.0f;    Out->_42 = 0.0f;    Out->_43 = 0.0f;    Out->_44 = 1.0f;
}

void AxesToMatrixTranslation(MATRIX *Out, const Vec3 &Origin, const Vec3 &AxisX, const Vec3 &AxisY, const Vec3 &AxisZ)
{
	// Na podstawie powy¿szej (w wersji ju¿ poprawnionej):
	Out->_11 = AxisX.x;  Out->_12 = AxisX.y;  Out->_13 = AxisX.z;  Out->_14 = 0.0f;
	Out->_21 = AxisY.x;  Out->_22 = AxisY.y;  Out->_23 = AxisY.z;  Out->_24 = 0.0f;
	Out->_31 = AxisZ.x;  Out->_32 = AxisZ.y;  Out->_33 = AxisZ.z;  Out->_34 = 0.0f;
	Out->_41 = Origin.x; Out->_42 = Origin.y; Out->_43 = Origin.z; Out->_44 = 1.0f;
}

void LookAtLH(MATRIX *Out, const Vec3 &Eye, const Vec3 &Forward, const Vec3 &Up)
{
	Vec3 zaxis1 = Forward;
	Vec3 zaxis; Normalize(&zaxis, zaxis1);
	Vec3 xaxis1; Cross(&xaxis1, Up, zaxis);
	Vec3 xaxis; Normalize(&xaxis, xaxis1);
	Vec3 yaxis; Cross(&yaxis, zaxis, xaxis);

	Out->_11 = xaxis.x; Out->_12 = yaxis.x; Out->_13 = zaxis.x; Out->_14 = 0.0f;
	Out->_21 = xaxis.y; Out->_22 = yaxis.y; Out->_23 = zaxis.y; Out->_24 = 0.0f;
	Out->_31 = xaxis.z; Out->_32 = yaxis.z; Out->_33 = zaxis.z; Out->_34 = 0.0f;
	Out->_41 = -Dot(xaxis, Eye); Out->_42 = -Dot(yaxis, Eye); Out->_43 = -Dot(zaxis, Eye); Out->_44 = 1.0f;
}

// Niczym siê nie ró¿ni od powy¿szej - dziwne...
// Ale tak jest w DirectX SDK (zobacz dokumentacjê funkcji D3DXMatrixLookAtRH).
void LookAtRH(MATRIX *Out, const Vec3 &Eye, const Vec3 &Forward, const Vec3 &Up)
{
	Vec3 zaxis1 = Forward;
	Vec3 zaxis; Normalize(&zaxis, zaxis1);
	Vec3 xaxis1; Cross(&xaxis1, Up, zaxis);
	Vec3 xaxis; Normalize(&xaxis, xaxis1);
	Vec3 yaxis; Cross(&yaxis, zaxis, xaxis);

	Out->_11 = xaxis.x; Out->_12 = yaxis.x; Out->_13 = zaxis.x; Out->_14 = 0.0f;
	Out->_21 = xaxis.y; Out->_22 = yaxis.y; Out->_23 = zaxis.y; Out->_24 = 0.0f;
	Out->_31 = xaxis.z; Out->_32 = yaxis.z; Out->_33 = zaxis.z; Out->_34 = 0.0f;
	Out->_41 = -Dot(xaxis, Eye); Out->_42 = -Dot(yaxis, Eye); Out->_43 = -Dot(zaxis, Eye); Out->_44 = 1.0f;
}

void Identity(MATRIX *Out)
{
	Out->_11 = 1.0f; Out->_12 = 0.0f; Out->_13 = 0.0f; Out->_14 = 0.0f;
	Out->_21 = 0.0f; Out->_22 = 1.0f; Out->_23 = 0.0f; Out->_24 = 0.0f;
	Out->_31 = 0.0f; Out->_32 = 0.0f; Out->_33 = 1.0f; Out->_34 = 0.0f;
	Out->_41 = 0.0f; Out->_42 = 0.0f; Out->_43 = 0.0f; Out->_44 = 1.0f;
}

void Translation(MATRIX *Out, float x, float y, float z)
{
	Out->_11 = 1.0f; Out->_12 = 0.0f; Out->_13 = 0.0f; Out->_14 = 0.0f;
	Out->_21 = 0.0f; Out->_22 = 1.0f; Out->_23 = 0.0f; Out->_24 = 0.0f;
	Out->_31 = 0.0f; Out->_32 = 0.0f; Out->_33 = 1.0f; Out->_34 = 0.0f;
	Out->_41 = x;    Out->_42 = y;    Out->_43 = z;    Out->_44 = 1.0f;
}

void Translation(MATRIX *Out, const Vec3 &v)
{
	Out->_11 = 1.0f; Out->_12 = 0.0f; Out->_13 = 0.0f; Out->_14 = 0.0f;
	Out->_21 = 0.0f; Out->_22 = 1.0f; Out->_23 = 0.0f; Out->_24 = 0.0f;
	Out->_31 = 0.0f; Out->_32 = 0.0f; Out->_33 = 1.0f; Out->_34 = 0.0f;
	Out->_41 = v.x;  Out->_42 = v.y;  Out->_43 = v.z;  Out->_44 = 1.0f;
}

void Scaling(MATRIX *Out, float sx, float sy, float sz)
{
	Out->_11 = sx;   Out->_12 = 0.0f; Out->_13 = 0.0f; Out->_14 = 0.0f;
	Out->_21 = 0.0f; Out->_22 = sy;   Out->_23 = 0.0f; Out->_24 = 0.0f;
	Out->_31 = 0.0f; Out->_32 = 0.0f; Out->_33 = sz;   Out->_34 = 0.0f;
	Out->_41 = 0.0f; Out->_42 = 0.0f; Out->_43 = 0.0f; Out->_44 = 1.0f;
}

void Scaling(MATRIX *Out, const Vec3 &sv)
{
	Out->_11 = sv.x; Out->_12 = 0.0f; Out->_13 = 0.0f; Out->_14 = 0.0f;
	Out->_21 = 0.0f; Out->_22 = sv.y; Out->_23 = 0.0f; Out->_24 = 0.0f;
	Out->_31 = 0.0f; Out->_32 = 0.0f; Out->_33 = sv.z; Out->_34 = 0.0f;
	Out->_41 = 0.0f; Out->_42 = 0.0f; Out->_43 = 0.0f; Out->_44 = 1.0f;
}

void Scaling(MATRIX *Out, float s)
{
	Out->_11 = s;    Out->_12 = 0.0f; Out->_13 = 0.0f; Out->_14 = 0.0f;
	Out->_21 = 0.0f; Out->_22 = s;    Out->_23 = 0.0f; Out->_24 = 0.0f;
	Out->_31 = 0.0f; Out->_32 = 0.0f; Out->_33 = s;    Out->_34 = 0.0f;
	Out->_41 = 0.0f; Out->_42 = 0.0f; Out->_43 = 0.0f; Out->_44 = 1.0f;
}

void ScalingAxis(MATRIX *Out, const Vec3 &n, float k)
{
	float k1 = k - 1.0f;
	Out->_11 = 1.0f+k1*n.x*n.x; Out->_12 =      k1*n.x*n.y; Out->_13 =      k1*n.x*n.z; Out->_14 = 0.0f;
	Out->_21 =      k1*n.x*n.y; Out->_22 = 1.0f+k1*n.y*n.y; Out->_23 =      k1*n.y*n.z; Out->_24 = 0.0f;
	Out->_31 =      k1*n.x*n.z; Out->_32 =      k1*n.y*n.z; Out->_33 = 1.0f+k1*n.z*n.z; Out->_34 = 0.0f;
	Out->_41 = 0.0f;            Out->_42 = 0.0f;            Out->_43 = 0.0f;            Out->_44 = 1.0f;
}

void RotationX(MATRIX *Out, float Angle)
{
	float Sin = sinf(Angle);
	float Cos = cosf(Angle);
	Out->_11 = 1.0f; Out->_12 = 0.0f; Out->_13 = 0.0f; Out->_14 = 0.0f;
	Out->_21 = 0.0f; Out->_22 = Cos;  Out->_23 = Sin;  Out->_24 = 0.0f;
	Out->_31 = 0.0f; Out->_32 = -Sin; Out->_33 = Cos;  Out->_34 = 0.0f;
	Out->_41 = 0.0f; Out->_42 = 0.0f; Out->_43 = 0.0f; Out->_44 = 1.0f;
}

void RotationY(MATRIX *Out, float Angle)
{
	float Sin = sinf(Angle);
	float Cos = cosf(Angle);
	Out->_11 = Cos;  Out->_12 = 0.0f; Out->_13 = -Sin; Out->_14 = 0.0f;
	Out->_21 = 0.0f; Out->_22 = 1.0f; Out->_23 = 0.0f; Out->_24 = 0.0f;
	Out->_31 = Sin;  Out->_32 = 0.0f; Out->_33 = Cos;  Out->_34 = 0.0f;
	Out->_41 = 0.0f; Out->_42 = 0.0f; Out->_43 = 0.0f; Out->_44 = 1.0f;
}

void RotationZ(MATRIX *Out, float Angle)
{
	float Sin = sinf(Angle);
	float Cos = cosf(Angle);
	Out->_11 = Cos;  Out->_12 = Sin;  Out->_13 = 0.0f; Out->_14 = 0.0f;
	Out->_21 = -Sin; Out->_22 = Cos;  Out->_23 = 0.0f; Out->_24 = 0.0f;
	Out->_31 = 0.0f; Out->_32 = 0.0f; Out->_33 = 1.0f; Out->_34 = 0.0f;
	Out->_41 = 0.0f; Out->_42 = 0.0f; Out->_43 = 0.0f; Out->_44 = 1.0f;
}

void RotationAxisLH(MATRIX *Out, const Vec3 &Axis, float Angle)
{
	float c = cosf(Angle), s = sinf(Angle), t = 1.0f - c;
	Out->_11 = t*Axis.x*Axis.x+c;        Out->_12 = t*Axis.x*Axis.y+s*Axis.z; Out->_13 = t*Axis.x*Axis.z-s*Axis.y; Out->_14 = 0.0f;
	Out->_21 = t*Axis.x*Axis.y-s*Axis.z; Out->_22 = t*Axis.y*Axis.y+c;        Out->_23 = t*Axis.y*Axis.z+s*Axis.x; Out->_24 = 0.0f;
	Out->_31 = t*Axis.x*Axis.z+s*Axis.y; Out->_32 = t*Axis.y*Axis.z-s*Axis.x; Out->_33 = t*Axis.z*Axis.z+c;        Out->_34 = 0.0f;
	Out->_41 = 0.0f;                     Out->_42 = 0.0f;                     Out->_43 = 0.0f;                     Out->_44 = 1.0f;
}

/* Tu by³y funkcje takie jak ta, wszystkie 6 kombinacji, ale one s¹ 1. powolne 2. niepotrzebne
void RotationYZX(MATRIX *Out, float Yaw, float Pitch, float Roll)
{
	MATRIX my, mx, mz, Out1;
	RotationY(&my, Yaw);
	RotationX(&mx, Pitch);
	RotationZ(&mz, Roll);
	Mul(&Out1, my, mz);
	Mul(Out, Out1, mx);
}*/

void RotationYawPitchRoll(MATRIX *Out, float Yaw, float Pitch, float Roll)
{
	// Wersja oryginalna:
	//MATRIX my, mx, mz, Out1;
	//RotationY(&my, Yaw);
	//RotationX(&mx, Pitch);
	//RotationZ(&mz, Roll);
	//Mul(&Out1, mz, mx);
	//Mul(Out, Out1, my);

	// Wersja rozpisana, du¿o szybsza:
	float sy = sinf(Yaw), cy = cosf(Yaw);
	float sp = sinf(Pitch), cp = cosf(Pitch);
	float sr = sinf(Roll), cr = cosf(Roll);
	Out->_11 = cy*cr+sy*sp*sr;  Out->_12 = sr*cp; Out->_13 = -sy*cr+cy*sp*sr; Out->_14 = 0.0f;
	Out->_21 = -cy*sr+sy*sp*cr; Out->_22 = cr*cp; Out->_23 = sr*sy+cy*sp*cr;  Out->_24 = 0.0f;
	Out->_31 = sy*cp;           Out->_32 = -sp;   Out->_33 = cy*cp;           Out->_34 = 0.0f;
	Out->_41 = 0.0f;            Out->_42 = 0.0f;  Out->_43 = 0.0f;            Out->_44 = 1.0f;
}

void RotationYawPitchRoll(MATRIX *Out, const Vec3 &EulerAngles)
{
	float sy = sinf(EulerAngles.y), cy = cosf(EulerAngles.y);
	float sp = sinf(EulerAngles.x), cp = cosf(EulerAngles.x);
	float sr = sinf(EulerAngles.z), cr = cosf(EulerAngles.z);
	Out->_11 = cy*cr+sy*sp*sr;  Out->_12 = sr*cp; Out->_13 = -sy*cr+cy*sp*sr; Out->_14 = 0.0f;
	Out->_21 = -cy*sr+sy*sp*cr; Out->_22 = cr*cp; Out->_23 = sr*sy+cy*sp*cr;  Out->_24 = 0.0f;
	Out->_31 = sy*cp;           Out->_32 = -sp;   Out->_33 = cy*cp;           Out->_34 = 0.0f;
	Out->_41 = 0.0f;            Out->_42 = 0.0f;  Out->_43 = 0.0f;            Out->_44 = 1.0f;
}

void RotationYawPitchRollInv(MATRIX *Out, float Yaw, float Pitch, float Roll)
{
	// Wersja rozpisana, du¿o szybsza:
	float sy = sinf(Yaw), cy = cosf(Yaw);
	float sp = sinf(Pitch), cp = cosf(Pitch);
	float sr = sinf(Roll), cr = cosf(Roll);
	Out->_11 = cy*cr+sy*sp*sr;  Out->_12 = -cy*sr+sy*sp*cr; Out->_13 = sy*cp; Out->_14 = 0.0f;
	Out->_21 = sr*cp;           Out->_22 = cr*cp;           Out->_23 = -sp;   Out->_24 = 0.0f;
	Out->_31 = -sy*cr+cy*sp*sr; Out->_32 = sr*sy+cy*sp*cr;  Out->_33 = cy*cp; Out->_34 = 0.0f;
	Out->_41 = 0.0f;            Out->_42 = 0.0f;            Out->_43 = 0.0f;  Out->_44 = 1.0f;
}

void RotationYawPitchRollInv(MATRIX *Out, const Vec3 &EulerAngles)
{
	float sy = sinf(EulerAngles.y), cy = cosf(EulerAngles.y);
	float sp = sinf(EulerAngles.x), cp = cosf(EulerAngles.x);
	float sr = sinf(EulerAngles.z), cr = cosf(EulerAngles.z);
	Out->_11 = cy*cr+sy*sp*sr;  Out->_12 = -cy*sr+sy*sp*cr; Out->_13 = sy*cp; Out->_14 = 0.0f;
	Out->_21 = sr*cp;           Out->_22 = cr*cp;           Out->_23 = -sp;   Out->_24 = 0.0f;
	Out->_31 = -sy*cr+cy*sp*sr; Out->_32 = sr*sy+cy*sp*cr;  Out->_33 = cy*cp; Out->_34 = 0.0f;
	Out->_41 = 0.0f;            Out->_42 = 0.0f;            Out->_43 = 0.0f;  Out->_44 = 1.0f;
}

// Napisane na podstawie ksi¹¿ki "3D Math Primer for Graphics and Game Development".
// Niestety nie dzia³a.
// Po wygenerowaniu macierzy za pomoc¹ funkcji RotationYawPitchRoll (próbowa³em te¿ RotationYawPitchRollInv)
// i wyci¹gniêciu z powrotem k¹tów Eulera wychodz¹ zupe³nie inne liczby - nie przeciwne, nie podobne,
// nie zerowe czy INF, po prostu zupe³nie inne i nie wiem dlaczego :(
/*void RotationMatrixToEulerAngles(const MATRIX &m, float *Yaw, float *Pitch, float *Roll)
{
	// Extract pitch from m23, being careful for domain errors with asin().
	// We could have values slightly out of range due to floating point arithmetic.
	float sp = -m._23;
	if (sp <= -1.0f)
		*Pitch = -PI_2;
	else if (sp >= 1.0f)
		*Pitch = PI_2;
	else
		*Pitch = asinf(sp);

	// Check for the Gimbal lock case, giving a slight tolerance for numerical imprecision
	if (sp > 0.9999f)
	{
		// We are looking straight up or down.
		// Slam roll to zero and host set yaw.
		*Roll = 0.0f;
		*Yaw = atan2f(-m._31, m._11);
	}
	else
	{
		// Compute yaw from m13 and m33
		*Yaw = atan2f(m._13, m._33);
		// Compute roll from m21 and m22
		*Roll = atan2f(m._21, m._22);
	}
}*/

void OrthoLH(MATRIX *Out, float Width, float Height, float ZNear, float ZFar)
{
	Out->_11 = 2.0f/Width;  Out->_12 = 0.0f;        Out->_13 = 0.0f;               Out->_14 = 0.0f;
	Out->_21 = 0.0f;        Out->_22 = 2.0f/Height; Out->_23 = 0.0f;               Out->_24 = 0.0f;
	Out->_31 = 0.0f;        Out->_32 = 0.0f;        Out->_33 = 1.0f/(ZFar-ZNear);  Out->_34 = 0.0f;
	Out->_41 = 0.0f;        Out->_42 = 0.0f;        Out->_43 = ZNear/(ZNear-ZFar); Out->_44 = 1.0f;
}

void OrthoRH(MATRIX *Out, float Width, float Height, float ZNear, float ZFar)
{
	Out->_11 = 2.0f/Width;  Out->_12 = 0.0f;        Out->_13 = 0.0f;               Out->_14 = 0.0f;
	Out->_21 = 0.0f;        Out->_22 = 2.0f/Height; Out->_23 = 0.0f;               Out->_24 = 0.0f;
	Out->_31 = 0.0f;        Out->_32 = 0.0f;        Out->_33 = 1.0f/(ZNear-ZFar);  Out->_34 = 0.0f;
	Out->_41 = 0.0f;        Out->_42 = 0.0f;        Out->_43 = ZNear/(ZNear-ZFar); Out->_44 = 1.0f;
}

void OrthoOffCenterLH(MATRIX *Out, float Left, float Right, float Bottom, float Top, float ZNear, float ZFar)
{
	Out->_11 = 2.0f/(Right-Left);         Out->_12 = 0.0f;                      Out->_13 = 0.0f;               Out->_14 = 0.0f;
	Out->_21 = 0.0f;                      Out->_22 = 2.0f/(Top-Bottom);         Out->_23 = 0.0f;               Out->_24 = 0.0f;
	Out->_31 = 0.0f;                      Out->_32 = 0.0f;                      Out->_33 = 1.0f/(ZFar-ZNear);  Out->_34 = 0.0f;
	Out->_41 = (Left+Right)/(Left-Right); Out->_42 = (Top+Bottom)/(Bottom-Top); Out->_43 = ZNear/(ZNear-ZFar); Out->_44 = 1.0f;
}

void OrthoOffCenterRH(MATRIX *Out, float Left, float Right, float Bottom, float Top, float ZNear, float ZFar)
{
	Out->_11 = 2.0f/(Right-Left);         Out->_12 = 0.0f;                      Out->_13 = 0.0f;               Out->_14 = 0.0f;
	Out->_21 = 0.0f;                      Out->_22 = 2.0f/(Top-Bottom);         Out->_23 = 0.0f;               Out->_24 = 0.0f;
	Out->_31 = 0.0f;                      Out->_32 = 0.0f;                      Out->_33 = 1.0f/(ZNear-ZFar);  Out->_34 = 0.0f;
	Out->_41 = (Left+Right)/(Left-Right); Out->_42 = (Top+Bottom)/(Bottom-Top); Out->_43 = ZNear/(ZNear-ZFar); Out->_44 = 1.0f;
}

void PerspectiveLH(MATRIX *Out, float Width, float Height, float ZNear, float ZFar)
{
	Out->_11 = 2.0f*ZNear/Width; Out->_12 = 0.0f;              Out->_13 = 0.0f;                    Out->_14 = 0.0f;
	Out->_21 = 0.0f;             Out->_22 = 2.0f*ZNear/Height; Out->_23 = 0.0f;                    Out->_24 = 0.0f;
	Out->_31 = 0.0f;             Out->_32 = 0.0f;              Out->_33 = ZFar/(ZFar-ZNear);       Out->_34 = 1.0f;
	Out->_41 = 0.0f;             Out->_42 = 0.0f;              Out->_43 = ZNear*ZFar/(ZNear-ZFar); Out->_44 = 0.0f;
}

void PerspectiveRH(MATRIX *Out, float Width, float Height, float ZNear, float ZFar)
{
	Out->_11 = 2.0f*ZNear/Width; Out->_12 = 0.0f;              Out->_13 = 0.0f;                    Out->_14 = 0.0f;
	Out->_21 = 0.0f;             Out->_22 = 2.0f*ZNear/Height; Out->_23 = 0.0f;                    Out->_24 = 0.0f;
	Out->_31 = 0.0f;             Out->_32 = 0.0f;              Out->_33 = ZFar/(ZNear-ZFar);       Out->_34 = 1.0f;
	Out->_41 = 0.0f;             Out->_42 = 0.0f;              Out->_43 = ZNear*ZFar/(ZNear-ZFar); Out->_44 = 0.0f;
}

void PerspectiveFovLH(MATRIX *Out, float FovY, float Aspect, float ZNear, float ZFar)
{
	float YScale = 1.0f/tanf(FovY*0.5f);
	float XScale = YScale / Aspect;
	Out->_11 = XScale; Out->_12 = 0.0f;   Out->_13 = 0.0f;                     Out->_14 = 0.0f;
	Out->_21 = 0.0f;   Out->_22 = YScale; Out->_23 = 0.0f;                     Out->_24 = 0.0f;
	Out->_31 = 0.0f;   Out->_32 = 0.0f;   Out->_33 = ZFar/(ZFar-ZNear);        Out->_34 = 1.0f;
	Out->_41 = 0.0f;   Out->_42 = 0.0f;   Out->_43 = -ZNear*ZFar/(ZFar-ZNear); Out->_44 = 0.0f;
}

void PerspectiveFovRH(MATRIX *Out, float FovY, float Aspect, float ZNear, float ZFar)
{
	float YScale = 1.0f/tanf(FovY*0.5f); // cot
	float XScale = YScale / Aspect;
	Out->_11 = XScale; Out->_12 = 0.0f;   Out->_13 = 0.0f;                     Out->_14 = 0.0f;
	Out->_21 = 0.0f;   Out->_22 = YScale; Out->_23 = 0.0f;                     Out->_24 = 0.0f;
	Out->_31 = 0.0f;   Out->_32 = 0.0f;   Out->_33 = ZFar/(ZNear-ZFar);        Out->_34 = 1.0f;
	Out->_41 = 0.0f;   Out->_42 = 0.0f;   Out->_43 = ZNear*ZFar/(ZNear-ZFar);  Out->_44 = 0.0f;
}

void PerspectiveOffCenterLH(MATRIX *Out, float Left, float Right, float Bottom, float Top, float ZNear, float ZFar)
{
	Out->_11 = 2.0f*ZNear/(Right-Left);   Out->_12 = 0.0f;                      Out->_13 = 0.0f;                    Out->_14 = 0.0f;
	Out->_21 = 0.0f;                      Out->_22 = 2.0f*ZNear/(Top-Bottom);   Out->_23 = 0.0f;                    Out->_24 = 0.0f;
	Out->_31 = (Left+Right)/(Left-Right); Out->_32 = (Top+Bottom)/(Bottom-Top); Out->_33 = ZFar/(ZFar-ZNear);       Out->_34 = 1.0f;
	Out->_41 = 0.0f;                      Out->_42 = 0.0f;                      Out->_43 = ZNear*ZFar/(ZNear-ZFar); Out->_44 = 0.0f;
}

void PerspectiveOffCenterRH(MATRIX *Out, float Left, float Right, float Bottom, float Top, float ZNear, float ZFar)
{
	Out->_11 = 2.0f*ZNear/(Right-Left);   Out->_12 = 0.0f;                      Out->_13 = 0.0f;                    Out->_14 = 0.0f;
	Out->_21 = 0.0f;                      Out->_22 = 2.0f*ZNear/(Top-Bottom);   Out->_23 = 0.0f;                    Out->_24 = 0.0f;
	Out->_31 = (Left+Right)/(Right-Left); Out->_32 = (Top+Bottom)/(Top-Bottom); Out->_33 = ZFar/(ZNear-ZFar);       Out->_34 = 1.0f;
	Out->_41 = 0.0f;                      Out->_42 = 0.0f;                      Out->_43 = ZNear*ZFar/(ZNear-ZFar); Out->_44 = 0.0f;
}

void PerspectiveFovLH_Inf(MATRIX *Out, float FovY, float Aspect, float ZNear)
{
	float YScale = 1.0f/tanf(FovY*0.5f);
	float XScale = YScale / Aspect;
	Out->_11 = XScale; Out->_12 = 0.0f;   Out->_13 = 0.0f;   Out->_14 = 0.0f;
	Out->_21 = 0.0f;   Out->_22 = YScale; Out->_23 = 0.0f;   Out->_24 = 0.0f;
	Out->_31 = 0.0f;   Out->_32 = 0.0f;   Out->_33 = 1.0f;   Out->_34 = 1.0f;
	Out->_41 = 0.0f;   Out->_42 = 0.0f;   Out->_43 = -ZNear; Out->_44 = 0.0f;
}

void MatrixShadow(MATRIX *Out, const Vec4 &Light, const PLANE &Plane)
{
	float D = Dot(Plane, Light);
	Out->_11 = 1.0f; Out->_12 = 0.0f; Out->_13 = 0.0f; Out->_14 = 0.0f;
	Out->_21 = 0.0f; Out->_22 = 1.0f; Out->_23 = 0.0f; Out->_24 = 0.0f;
	Out->_31 = 0.0f; Out->_32 = 0.0f; Out->_33 = 1.0f; Out->_34 = 0.0f;
	Out->_41 = 0.0f; Out->_42 = 0.0f; Out->_43 = 0.0f; Out->_44 = 1.0f;

	Out->_11 = Plane.a * Light.x + D; Out->_12 = Plane.a * Light.y;     Out->_13 = Plane.a * Light.z;     Out->_14 = Plane.a * Light.w;
	Out->_21 = Plane.b * Light.x;     Out->_22 = Plane.b * Light.y + D; Out->_23 = Plane.b * Light.z;     Out->_24 = Plane.b * Light.w;
	Out->_31 = Plane.c * Light.x;     Out->_32 = Plane.c * Light.y;     Out->_33 = Plane.c * Light.z + D; Out->_34 = Plane.c * Light.w;
	Out->_41 = Plane.d * Light.x;     Out->_42 = Plane.d * Light.y;     Out->_43 = Plane.d * Light.z;     Out->_44 = Plane.d * Light.w + D;
}

void MatrixReflect(MATRIX *Out, const PLANE &p)
{
	Out->_11 = -2.0f * p.a * p.a + 1.0f; Out->_12 = -2.0f * p.b * p.a;        Out->_13 = -2.0f * p.c * p.a;        Out->_14 = 0.0f;
	Out->_21 = -2.0f * p.a * p.b;        Out->_22 = -2.0f * p.b * p.b + 1.0f; Out->_23 = -2.0f * p.c * p.b;        Out->_24 = 0.0f;
	Out->_31 = -2.0f * p.a * p.c;        Out->_32 = -2.0f * p.b * p.c;        Out->_33 = -2.0f * p.c * p.c + 1.0f; Out->_34 = 0.0f;
	Out->_41 = -2.0f * p.a * p.d;        Out->_42 = -2.0f * p.b * p.d;        Out->_43 = -2.0f * p.c * p.d;        Out->_44 = 1.0f;
}

void Transpose(MATRIX *m)
{
	std::swap(m->_12, m->_21);
	std::swap(m->_13, m->_31);
	std::swap(m->_14, m->_41);
	std::swap(m->_23, m->_32);
	std::swap(m->_24, m->_42);
	std::swap(m->_34, m->_43);
}

void Lerp(MATRIX *Out, const MATRIX &m1, const MATRIX &m2, float t)
{
	Out->_11 = Lerp(m1._11, m2._11, t);
	Out->_12 = Lerp(m1._12, m2._12, t);
	Out->_13 = Lerp(m1._13, m2._13, t);
	Out->_14 = Lerp(m1._14, m2._14, t);

	Out->_21 = Lerp(m1._21, m2._21, t);
	Out->_22 = Lerp(m1._22, m2._22, t);
	Out->_23 = Lerp(m1._23, m2._23, t);
	Out->_24 = Lerp(m1._24, m2._24, t);

	Out->_31 = Lerp(m1._31, m2._31, t);
	Out->_32 = Lerp(m1._32, m2._32, t);
	Out->_33 = Lerp(m1._33, m2._33, t);
	Out->_34 = Lerp(m1._34, m2._34, t);

	Out->_41 = Lerp(m1._41, m2._41, t);
	Out->_42 = Lerp(m1._42, m2._42, t);
	Out->_43 = Lerp(m1._43, m2._43, t);
	Out->_44 = Lerp(m1._44, m2._44, t);
}

float Det(const MATRIX &m)
{
	return
		(m(0, 0) * m(1, 1) - m(1, 0) * m(0, 1)) * (m(2, 2) * m(3, 3) - m(3, 2) * m(2, 3)) - (m(0, 0) * m(2, 1) - m(2, 0) * m(0, 1)) * (m(1, 2) * m(3, 3) - m(3, 2) * m(1, 3)) +
		(m(0, 0) * m(3, 1) - m(3, 0) * m(0, 1)) * (m(1, 2) * m(2, 3) - m(2, 2) * m(1, 3)) + (m(1, 0) * m(2, 1) - m(2, 0) * m(1, 1)) * (m(0, 2) * m(3, 3) - m(3, 2) * m(0, 3)) -
		(m(1, 0) * m(3, 1) - m(3, 0) * m(1, 1)) * (m(0, 2) * m(2, 3) - m(2, 2) * m(0, 3)) + (m(2, 0) * m(3, 1) - m(3, 0) * m(2, 1)) * (m(0, 2) * m(1, 3) - m(1, 2) * m(0, 3));
}

bool Inverse(MATRIX *Out, const MATRIX &m)
{
	float d = Det(m);
	if (d == 0.0f) return false;
	d = 1.0f / d;
	(*Out)(0, 0) = d * (m(1, 1) * (m(2, 2) * m(3, 3) - m(3, 2) * m(2, 3)) + m(2, 1) * (m(3, 2) * m(1, 3) - m(1, 2) * m(3, 3)) + m(3, 1) * (m(1, 2) * m(2, 3) - m(2, 2) * m(1, 3)));
	(*Out)(1, 0) = d * (m(1, 2) * (m(2, 0) * m(3, 3) - m(3, 0) * m(2, 3)) + m(2, 2) * (m(3, 0) * m(1, 3) - m(1, 0) * m(3, 3)) + m(3, 2) * (m(1, 0) * m(2, 3) - m(2, 0) * m(1, 3)));
	(*Out)(2, 0) = d * (m(1, 3) * (m(2, 0) * m(3, 1) - m(3, 0) * m(2, 1)) + m(2, 3) * (m(3, 0) * m(1, 1) - m(1, 0) * m(3, 1)) + m(3, 3) * (m(1, 0) * m(2, 1) - m(2, 0) * m(1, 1)));
	(*Out)(3, 0) = d * (m(1, 0) * (m(3, 1) * m(2, 2) - m(2, 1) * m(3, 2)) + m(2, 0) * (m(1, 1) * m(3, 2) - m(3, 1) * m(1, 2)) + m(3, 0) * (m(2, 1) * m(1, 2) - m(1, 1) * m(2, 2)));
	(*Out)(0, 1) = d * (m(2, 1) * (m(0, 2) * m(3, 3) - m(3, 2) * m(0, 3)) + m(3, 1) * (m(2, 2) * m(0, 3) - m(0, 2) * m(2, 3)) + m(0, 1) * (m(3, 2) * m(2, 3) - m(2, 2) * m(3, 3)));
	(*Out)(1, 1) = d * (m(2, 2) * (m(0, 0) * m(3, 3) - m(3, 0) * m(0, 3)) + m(3, 2) * (m(2, 0) * m(0, 3) - m(0, 0) * m(2, 3)) + m(0, 2) * (m(3, 0) * m(2, 3) - m(2, 0) * m(3, 3)));
	(*Out)(2, 1) = d * (m(2, 3) * (m(0, 0) * m(3, 1) - m(3, 0) * m(0, 1)) + m(3, 3) * (m(2, 0) * m(0, 1) - m(0, 0) * m(2, 1)) + m(0, 3) * (m(3, 0) * m(2, 1) - m(2, 0) * m(3, 1)));
	(*Out)(3, 1) = d * (m(2, 0) * (m(3, 1) * m(0, 2) - m(0, 1) * m(3, 2)) + m(3, 0) * (m(0, 1) * m(2, 2) - m(2, 1) * m(0, 2)) + m(0, 0) * (m(2, 1) * m(3, 2) - m(3, 1) * m(2, 2)));
	(*Out)(0, 2) = d * (m(3, 1) * (m(0, 2) * m(1, 3) - m(1, 2) * m(0, 3)) + m(0, 1) * (m(1, 2) * m(3, 3) - m(3, 2) * m(1, 3)) + m(1, 1) * (m(3, 2) * m(0, 3) - m(0, 2) * m(3, 3)));
	(*Out)(1, 2) = d * (m(3, 2) * (m(0, 0) * m(1, 3) - m(1, 0) * m(0, 3)) + m(0, 2) * (m(1, 0) * m(3, 3) - m(3, 0) * m(1, 3)) + m(1, 2) * (m(3, 0) * m(0, 3) - m(0, 0) * m(3, 3)));
	(*Out)(2, 2) = d * (m(3, 3) * (m(0, 0) * m(1, 1) - m(1, 0) * m(0, 1)) + m(0, 3) * (m(1, 0) * m(3, 1) - m(3, 0) * m(1, 1)) + m(1, 3) * (m(3, 0) * m(0, 1) - m(0, 0) * m(3, 1)));
	(*Out)(3, 2) = d * (m(3, 0) * (m(1, 1) * m(0, 2) - m(0, 1) * m(1, 2)) + m(0, 0) * (m(3, 1) * m(1, 2) - m(1, 1) * m(3, 2)) + m(1, 0) * (m(0, 1) * m(3, 2) - m(3, 1) * m(0, 2)));
	(*Out)(0, 3) = d * (m(0, 1) * (m(2, 2) * m(1, 3) - m(1, 2) * m(2, 3)) + m(1, 1) * (m(0, 2) * m(2, 3) - m(2, 2) * m(0, 3)) + m(2, 1) * (m(1, 2) * m(0, 3) - m(0, 2) * m(1, 3)));
	(*Out)(1, 3) = d * (m(0, 2) * (m(2, 0) * m(1, 3) - m(1, 0) * m(2, 3)) + m(1, 2) * (m(0, 0) * m(2, 3) - m(2, 0) * m(0, 3)) + m(2, 2) * (m(1, 0) * m(0, 3) - m(0, 0) * m(1, 3)));
	(*Out)(2, 3) = d * (m(0, 3) * (m(2, 0) * m(1, 1) - m(1, 0) * m(2, 1)) + m(1, 3) * (m(0, 0) * m(2, 1) - m(2, 0) * m(0, 1)) + m(2, 3) * (m(1, 0) * m(0, 1) - m(0, 0) * m(1, 1)));
	(*Out)(3, 3) = d * (m(0, 0) * (m(1, 1) * m(2, 2) - m(2, 1) * m(1, 2)) + m(1, 0) * (m(2, 1) * m(0, 2) - m(0, 1) * m(2, 2)) + m(2, 0) * (m(0, 1) * m(1, 2) - m(1, 1) * m(0, 2)));
	return true;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// KWATERNION

const QUATERNION QUATERNION::ZERO = QUATERNION(0.0f, 0.0f, 0.0f, 0.0f);
const QUATERNION QUATERNION::IDENTITY = QUATERNION(0.0f, 0.0f, 0.0f, 1.0f);

void QUATERNION::CalcAxis(Vec3 *Out) const
{
	// Compute sin^2(theta/2). Remember that w = cos(theta/2),
	// and sin^2(x) + cos^2(x) = 1
	float SinThetaOver25q = 1.0f - w*w;
	// Protect against numerical imprecision
	if (SinThetaOver25q <= 0.0f)
	{
		// Identity quaternion, or numerical imprecision. Just
		// return any valid vector, since it doesn't matter
		Out->x = 0.0f;
		Out->y = 0.0f;
		Out->z = 0.0f;
		return;
	}
	// Compite 1 / sin(theta/2)
	float OneOverSinThetaOver2 = 1.0f / sqrtf(SinThetaOver25q);
	// Return axis of rotation
	Out->x = x * OneOverSinThetaOver2;
	Out->y = y * OneOverSinThetaOver2;
	Out->z = z * OneOverSinThetaOver2;
}

// == Do klasy QUATERNION ==
// Wersja stara sprzed odwrócenia kolejnoœci:
// QUATERNION operator * (const QUATERNION &q) const { return QUATERNION(q.x * w + q.y * z - q.z * y + q.w * x, q.y * w - q.x * z + q.w * y + q.z * x, q.z * w + q.w * z + q.x * y - q.y * x, q.w * w - q.z * z - q.y * y - q.x * x); }
// QUATERNION & operator *= (const QUATERNION &q) { QUATERNION R; R.x = q.x * w + q.y * z - q.z * y + q.w * x; R.y = q.y * w - q.x * z + q.w * y + q.z * x; R.z = q.z * w + q.w * z + q.x * y - q.y * x; R.w = q.w * w - q.z * z - q.y * y - q.x * x; *this = R; return *this; }


void Minus(QUATERNION *q)
{
	q->x = -q->x;
	q->y = -q->y;
	q->z = -q->z;
	q->w = -q->w;
}

void Add(QUATERNION *Out, const QUATERNION &q1, const QUATERNION &q2)
{
	Out->x = q1.x + q2.x;
	Out->y = q1.y + q2.y;
	Out->z = q1.z + q2.z;
	Out->w = q1.w + q2.w;
}

void Sub(QUATERNION *Out, const QUATERNION &q1, const QUATERNION &q2)
{
	Out->x = q1.x - q2.x;
	Out->y = q1.y - q2.y;
	Out->z = q1.z - q2.z;
	Out->w = q1.w - q2.w;
}

void Mul(QUATERNION *Out, const QUATERNION &q1, const QUATERNION &q2)
{
/* Stara wersja - chyba identyczna, ale to ni¿ej napisane ³adnie
	Out->x = q1.w * q2.x + q1.z * q2.y - q1.y * q2.z + q1.x * q2.w;
	Out->y = q1.w * q2.y - q1.z * q2.x + q1.y * q2.w + q1.x * q2.z;
	Out->z = q1.w * q2.z + q1.z * q2.w + q1.y * q2.x - q1.x * q2.y;
	Out->w = q1.w * q2.w - q1.z * q2.z - q1.y * q2.y - q1.x * q2.x;

	Out->x = q1.w*q2.x + q1.x*q2.w + q1.z*q2.y - q1.y*q2.z;
	Out->y = q1.w*q2.y + q1.y*q2.w + q1.x*q2.z - q1.z*q2.x;
	Out->z = q1.w*q2.z + q1.z*q2.w + q1.y*q2.x - q1.x*q2.y;
	Out->w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z;

	Nowa wersja odwrócona, ¿eby mno¿enie by³o nie od koñca jak w teorii tylko tak jak w macierzach.
*/

	Out->x = q1.w*q2.x + q1.x*q2.w + q1.y*q2.z - q1.z*q2.y;
	Out->y = q1.w*q2.y + q1.y*q2.w + q1.z*q2.x - q1.x*q2.z;
	Out->z = q1.w*q2.z + q1.z*q2.w + q1.x*q2.y - q1.y*q2.x;
	Out->w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z;
}

void Mul(QUATERNION *q, float v)
{
	q->x *= v;
	q->y *= v;
	q->z *= v;
	q->w *= v;
}

void Div(QUATERNION *q, float v)
{
	v = 1.0f / v;
	q->x *= v;
	q->y *= v;
	q->z *= v;
	q->w *= v;
}

void AxisToQuaternion(QUATERNION *Out, const Vec3 &Axis, float Angle)
{
	Angle *= 0.5f;
	float Sin = sinf(Angle);
	Out->x = Sin*Axis.x;
	Out->y = Sin*Axis.y;
	Out->z = Sin*Axis.z;
	Out->w = cosf(Angle);
}

void RotationMatrixToQuaternion(QUATERNION *Out, const MATRIX &RotationMatrix)
{
	// Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
	// article "Quaternion Calculus and Fast Animation".

	float Trace = RotationMatrix(0, 0) + RotationMatrix(1, 1) + RotationMatrix(2, 2);
	float Root;

	if (Trace > 0.0f)
	{
		Root = sqrtf(Trace + 1.0f);
		Out->w = 0.5f * Root;
		Root = 0.5f/Root;
		Out->x = -(RotationMatrix(2, 1)-RotationMatrix(1, 2)) * Root;
		Out->y = -(RotationMatrix(0, 2)-RotationMatrix(2, 0)) * Root;
		Out->z = -(RotationMatrix(1, 0)-RotationMatrix(0, 1)) * Root;
	}
	else
	{
		static size_t Next[3] = { 1, 2, 0 };
		size_t i = 0;
		if (RotationMatrix(1, 1) > RotationMatrix(0, 0))
			i = 1;
		if (RotationMatrix(2, 2) > RotationMatrix(i, i))
			i = 2;
		size_t j = Next[i];
		size_t k = Next[j];

		Root = sqrtf(RotationMatrix(i, i) - RotationMatrix(j, j) - RotationMatrix(k, k) + 1.0f);
		float *apkQuat[3] = { &Out->x, &Out->y, &Out->z };
		*apkQuat[i] = -0.5f * Root;
		Root = 0.5f / Root;
		Out->w = (RotationMatrix(k, j)-RotationMatrix(j, k)) * Root;
		*apkQuat[j] = -(RotationMatrix(j, i)+RotationMatrix(i, j))*Root;
		*apkQuat[k] = -(RotationMatrix(k, i)+RotationMatrix(i, k))*Root;
	}
}

/*
======== Wersja 1 ========

Poprawna, ale wolniejsza.

	Out->_11 = 1.0f-2.0f*q.y*q.y-2.0f*q.z*q.z; Out->_12 = 2.0f*q.x*q.y+2.0f*q.w*q.z;      Out->_13 = 2.0f*q.x*q.z-2.0f*q.w*q.y;      Out->_14 = 0.0f;
	Out->_21 = 2.0f*q.x*q.y-2.0f*q.w*q.z;      Out->_22 = 1.0f-2.0f*q.x*q.x-2.0f*q.z*q.z; Out->_23 = 2.0f*q.y*q.z+2.0f*q.w*q.x;      Out->_24 = 0.0f;
	Out->_31 = 2.0f*q.x*q.z+2.0f*q.w*q.y;      Out->_32 = 2.0f*q.y*q.z-2.0f*q.w*q.x;      Out->_33 = 1.0f-2.0f*q.x*q.x-2.0f*q.y*q.y; Out->_34 = 0.0f;
	Out->_41 = 0.0f;                           Out->_42 = 0.0f;                           Out->_43 = 0.0f;                           Out->_44 = 1.0f;

======== Wersja 2 ========

Ta co jest aktualnie niezakomentowana.

Dzia³a tak samo jak wersja 1 (wynik testu: 99.9% przypadków), ale jest
optymalniejsza.

Na podstawie kodu z ksi¹¿ki "Game Programming Gems", tom 5, rozdz. 1.6
"Improved Frustum Culling", Frank Puig Placeres.
*/

void QuaternionToRotationMatrix(MATRIX *Out, const QUATERNION &q)
{
	float
		xx = q.x * q.x,
		yy = q.y * q.y,
		zz = q.z * q.z,
		xy = q.x * q.y,
		xz = q.x * q.z,
		yz = q.y * q.z,
		wx = q.w * q.x,
		wy = q.w * q.y,
		wz = q.w * q.z;

	Out->_11 = 1.0f - 2.0f * ( yy + zz ); Out->_12 = 2.0f * ( xy + wz );        Out->_13 = 2.0f * ( xz - wy );        Out->_14 = 0.0f;
	Out->_21 = 2.0f * ( xy - wz );        Out->_22 = 1.0f - 2.0f * ( xx + zz ); Out->_23 = 2.0f * ( yz + wx );        Out->_24 = 0.0f;
	Out->_31 = 2.0f * ( xz + wy );        Out->_32 = 2.0f * ( yz - wx );        Out->_33 = 1.0f - 2.0f * ( xx + yy ); Out->_34 = 0.0f;
	Out->_41 = 0.0f;                      Out->_42 = 0.0f;                      Out->_43 = 0.0f;                      Out->_44 = 1.0f;
}

/*
======== Wersja 1 ========

Poprawna, ale wolniejsza.

	QUATERNION q_inv; Inverse(&q_inv, q);
	QUATERNION q_p = QUATERNION(p.x, p.y, p.z, 0.0f);
	QUATERNION tmp1; Mul(&tmp1, q_inv, q_p);
	QUATERNION tmp2; Mul(&tmp2, tmp1, q);
	Out->x = tmp2.x;
	Out->y = tmp2.y;
	Out->z = tmp2.z;

======== Wersja 2 ========

Ta co jest aktualnie niezakomentowana.

Sprawdzone, ¿e dzia³a tak samo jak wersja 1 (wynik testu: 100% przypadków), ale
jest optymalniejsza.

U¿ywa tak naprawdê rozpisanego przekszta³cenia na macierz obrotu i pomno¿enia
wektora przez t¹ macierz.

Na podstawie kodu z ksi¹¿ki "Game Programming Gems", tom 5, rozdz. 1.6
"Improved Frustum Culling", Frank Puig Placeres.
*/

void QuaternionTransform(Vec3 *Out, const Vec3 &p, const QUATERNION &q)
{
	float
		xx = q.x * q.x,
		yy = q.y * q.y,
		zz = q.z * q.z,
		xy = q.x * q.y,
		xz = q.x * q.z,
		yz = q.y * q.z,
		wx = q.w * q.x,
		wy = q.w * q.y,
		wz = q.w * q.z;

	Out->x = (1.0f - 2.0f * ( yy + zz ))*p.x + (2.0f * ( xy - wz ))       *p.y  + (2.0f * ( xz + wy ))       *p.z;
	Out->y = (2.0f * ( xy + wz ))       *p.x + (1.0f - 2.0f * ( xx + zz ))*p.y  + (2.0f * ( yz - wx ))       *p.z;
	Out->z = (2.0f * ( xz - wy ))       *p.x + (2.0f * ( yz + wx ))       *p.y  + (1.0f - 2.0f * ( xx + yy ))*p.z;
}

void EulerAnglesToQuaternionO2I(QUATERNION *Out, float Yaw, float Pitch, float Roll)
{
	Yaw *= 0.5f; Pitch *= 0.5f; Roll *= 0.5f;

	float sy = sinf(Yaw), cy = cosf(Yaw);
	float sp = sinf(Pitch), cp = cosf(Pitch);
	float sr = sinf(Roll), cr = cosf(Roll);

	Out->x = cy*sp*cr + sy*cp*sr;
	Out->y = sy*cp*cr - cy*sp*sr;
	Out->z = cy*cp*sr - sy*sp*cr;
	Out->w = cy*cp*cr + sy*sp*sr;
}

void EulerAnglesToQuaternionI2O(QUATERNION *Out, float Yaw, float Pitch, float Roll)
{
	Yaw *= 0.5f; Pitch *= 0.5f; Roll *= 0.5f;

	float sy = sinf(Yaw), cy = cosf(Yaw);
	float sp = sinf(Pitch), cp = cosf(Pitch);
	float sr = sinf(Roll), cr = cosf(Roll);

	Out->x = -cy*sp*cr - sy*cp*sr;
	Out->y = cy*sp*sr - sy*cp*cr;
	Out->z = sy*sp*cr - cy*cp*sr;
	Out->w = cy*cp*cr + sy*sp*sr;
}

void QuaternionO2IToEulerAngles(float *Yaw, float *Pitch, float *Roll, const QUATERNION &q)
{
	// Extract sin(Pitch)
	float sp = -2.0f * (q.y*q.z - q.w*q.x);

	// Check for Gimbal lock, giving slight tolerance for numerical imprecision
	if (float_equal(sp, 1.0f))
	{
		// Looking straight up or down
		*Pitch = PI_2 * sp;
		// Compute yaw, slam roll to zero
		*Yaw = atan2f(-q.x*q.z + q.w*q.y, 0.5f - q.y*q.y - q.z*q.z);
		*Roll = 0.0f;
	}
	else
	{
		// Compute angles
		*Pitch = asinf(sp);
		*Yaw = atan2f(q.x*q.z + q.w*q.y, 0.5f - q.x*q.x - q.y*q.y);
		*Roll = atan2f(q.x*q.y + q.w*q.z, 0.5f - q.x*q.x - q.z*q.z);
	}
}

void QuaternionI2OToEulerAngles(float *Yaw, float *Pitch, float *Roll, const QUATERNION &q)
{
	// Extract sin(Pitch)
	float sp = -2.0f * (q.y*q.z + q.w*q.x);

	// Check for Gimbal lock, giving slight tolerance for numerical imprecision
	if (float_equal(sp, 1.0f))
	{
		// Looking straight up or down
		*Pitch = PI_2 * sp;
		// Compute yaw, slam roll to zero
		*Yaw = atan2f(-q.x*q.z - q.w*q.y, 0.5f - q.y*q.y - q.z*q.z);
		*Roll = 0.0f;
	}
	else
	{
		// Compute angles
		*Pitch = asinf(sp);
		*Yaw = atan2f(q.x*q.z - q.w*q.y, 0.5f - q.x*q.x - q.y*q.y);
		*Roll = atan2f(q.x*q.y - q.w*q.z, 0.5f - q.x*q.x - q.z*q.z);
	}
}

void QuaternionRotationX(QUATERNION *Out, float a)
{
	a *= 0.5f;
	Out->x = sinf(a);
	Out->y = 0.0f;
	Out->z = 0.0f;
	Out->w = cosf(a);
}

void QuaternionRotationY(QUATERNION *Out, float a)
{
	a *= 0.5f;
	Out->x = 0.0f;
	Out->y = sinf(a);
	Out->z = 0.0f;
	Out->w = cosf(a);
}

void QuaternionRotationZ(QUATERNION *Out, float a)
{
	a *= 0.5f;
	Out->x = 0.0f;
	Out->y = 0.0f;
	Out->z = sinf(a);
	Out->w = cosf(a);
}

void QuaternionDiff(QUATERNION *Out, const QUATERNION &a, const QUATERNION &b)
{
	// Ta funkcja dzia³a tak a nie inaczej dlatego ¿e mno¿enie mamy odwrotnie, inaczej by³by inny wzór.
	QUATERNION a_inv; Inverse(&a_inv, a);
	Mul(Out, a_inv, b);
}

void Conjugate(QUATERNION *q)
{
	q->x = -q->x;
	q->y = -q->y;
	q->z = -q->z;
}

void Inverse(QUATERNION *Out, const QUATERNION &q)
{
	float NormRc = 1.0f / sqrtf(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
	Out->w = q.w * NormRc; 
	Out->x = - q.x * NormRc; 
	Out->y = - q.y * NormRc; 
	Out->z = - q.z * NormRc; 
}

void Normalize(QUATERNION *Out, const QUATERNION &q)
{
	float n = 1.0f / sqrtf(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
	Out->x = q.x * n;
	Out->y = q.y * n;
	Out->z = q.z * n;
	Out->w = q.w * n;
}

void Normalize(QUATERNION *InOut)
{
	float n = 1.0f / Length(*InOut);
	InOut->x *= n;
	InOut->y *= n;
	InOut->z *= n;
	InOut->w *= n;
}

float Dot(const QUATERNION &q1, const QUATERNION &q2)
{
	return q1.x*q2.x + q1.y*q2.y + q1.z*q2.z + q1.w*q2.w;
}

float LengthSq(const QUATERNION &q)
{
	return q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
}

float Length(const QUATERNION &q)
{
	return sqrtf(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
}

void Log(QUATERNION *Out, const QUATERNION &q)
{
	Out->w = 0.0f;
	float Theta = acosf(q.w);
	float SinTheta = sinf(Theta);
	if (FLOAT_ALMOST_ZERO(SinTheta))
	{
		Out->x = q.x;
		Out->y = q.y;
		Out->z = q.z;
	}
	else
	{
		float Factor = Theta / SinTheta;
		Out->x = q.x * Factor;
		Out->y = q.y * Factor;
		Out->z = q.z * Factor;
	}
}

void Exp(QUATERNION *Out, const QUATERNION &q)
{
	float Angle = sqrtf(q.x*q.x + q.y*q.y + q.z*q.z);
	float Sin = sinf(Angle);

	Out->w = cosf(Angle);

	if (FLOAT_ALMOST_ZERO(Sin))
	{
		Out->x = q.x;
		Out->y = q.y;
		Out->z = q.z;
	}
	else
	{
		float Coeff = Sin / Angle;
		Out->x = Coeff * q.x;
		Out->y = Coeff * q.y;
		Out->z = Coeff * q.z;
	}
}

void Pow(QUATERNION *InOut, float t)
{
	// Kwaternion identycznioœciowy ma daæ w wyniku ten sam kwaternion,
	// dlatego trzeba to sprawdzaæ szczególnie, bo inaczej by³oby dzielenie przez 0.
	if (!float_equal(InOut->w, 1.0f))
	{
		// Extract the half angle alpha (alpha = theta/2)
		float alpha = acosf(InOut->w);
		// Compute new alpha value
		float newAlpha = alpha * t;
		// Compute new w value
		InOut->w = cosf(newAlpha);
		// Compute new xyz values
		float mult = sinf(newAlpha) / sinf(alpha);
		InOut->x *= mult;
		InOut->y *= mult;
		InOut->z *= mult;
	}
}

/*
INTERPOLACJA SFERYCZNA KWATERNIONÓW

======== Wersja 1 ========

Liczy³a Ÿle, dawa³a wyniki inne ni¿ D3DX.

	float costheta = q1.w * q2.w + q1.x * q2.x + q1.y * q2.y + q1.z * q2.z;
	float theta = acosf(costheta);
	if (abs(theta) < 0.01f)
	{
		Out->w = q1.w;
		Out->x = q1.x;
		Out->y = q1.y;
		Out->z = q1.z;
	}
	float sinTheta = sqrtf(1.0f - costheta*costheta);
	if (fabsf(sinTheta) < 0.01f)
	{
		Out->w = (q1.w * 0.5f + q2.w * 0.5f);
		Out->x = (q1.x * 0.5f + q2.x * 0.5f);
		Out->y = (q1.y * 0.5f + q2.y * 0.5f);
		Out->z = (q1.z * 0.5f + q2.z * 0.5f);
	}
	float ratioA = sin((1.0f - t) * theta) / sinTheta;
	float ratioB = sin(t * theta) / sinTheta;

	Out->w = (q1.w * ratioA + q2.w * ratioB);
	Out->x = (q1.x * ratioA + q2.x * ratioB);
	Out->y = (q1.y * ratioA + q2.y * ratioB);
	Out->z = (q1.z * ratioA + q2.z * ratioB);

======== Wersja 2 ========

Ta która jest poni¿ej niezakomentowana.

Mia³a w nag³ówku taki komentarz (niezgodny z prawd¹):

// (Ta wersja, pochodz¹ca z ksi¹¿ki "3D Math Primer for Graphics and Game Development"
// daje wyniki zgodne z D3DX w dok³adnie po³owie przypadów - nie wiem dlaczego.
// Próbowa³em zakomentowaæ sekcje "if (cosOmega < 0.0f)", "if (cosOmega > 0.9999f)",
// ich kombinacje i wci¹¿ to samo.)

======== Wersja 3 ========

Na podstawie kodu z ksi¹¿ki "Game Programming Gems", tom 5, rozdz. 1.6
"Improved Frustum Culling", Frank Puig Placeres.

Wygl¹da ³adnie, ale daje wyniki kompletnie niezgodne z D3DX.

	float cosfine = (q0.x * q1.x) + (q0.y * q1.y) * (q0.z * q1.z) * (q0.w * q1.w);
	float angleR = acosf(cosfine);

	float lower_weight;
	float upper_weight;

	// set up our weights
	if (FLOAT_ALMOST_ZERO(angleR))
	{
		// numerically unstable when angle is close to 0, lerp instead
		lower_weight = t;
		upper_weight = 1.f - t;
	}
	else
	{
		// this will also fall apart if w approaches k*pi/2 for k = [1, 2, ...]
		float i_sinf_angle = 1.f / sinf(angleR);
		lower_weight = sinf(angleR - angleR * t) * i_sinf_angle;
		upper_weight = sinf(angleR * t)          * i_sinf_angle;
	}

	Out->w = (q0.w * lower_weight) + (q1.w * upper_weight);
	Out->x = (q0.x * lower_weight) + (q1.x * upper_weight);
	Out->y = (q0.y * lower_weight) + (q1.y * upper_weight);
	Out->z = (q0.z * lower_weight) + (q1.z * upper_weight);

========

Porównanie zgodnoœci wyników moich funkcji z D3DXQuaternionSlerp dla losowych
kwaternionów:

                       Wersja 1   Wersja 2   Wersja 3
Nie normalizowane       50 %      100 %        0 %
Normalizowane wejœcie   80 %      100 %        0 %
Normalizowane wyjœcie   50 %      100 %        0 %
Normalizowane we i wy   80 %      100 %        0 %
*/

void Slerp(QUATERNION *Out, const QUATERNION &q0, const QUATERNION &q1, float t)
{

	// Compute the "cosine of the angle" between the quaternions, using the dot product
	float cosOmega = Dot(q0, q1);
	// If negative dot, negate one of the input quaternions to take the shorter 4D "arc"
	QUATERNION new_q1 = q1;
	if (cosOmega < 0.0f)
	{
		new_q1.x = -new_q1.x;
		new_q1.y = -new_q1.y;
		new_q1.z = -new_q1.z;
		new_q1.w = -new_q1.w;
	}

	// Check if they are very close together to protect against divide-by-zero
	float k0, k1;
	if (cosOmega > 0.9999f)
	{
		// Very close - jost use linear interpolation
		k0 = 1.0f - t;
		k1 = t;
	}
	else
	{
		// Compute the sin of the angle using the trig identity sin^x(omega)+cos^2(omega) = 1
		float sinOmega = sqrtf(1.0f - cosOmega*cosOmega);
		// Compute the angle from its sin and cosine
		float omega = atan2f(sinOmega, cosOmega);
		// Compute the inverse of denominator, so we only have to divide once
		float oneOverSinOmega = 1.0f / sinOmega;
		// Compute interpolation parameters
		k0 = sinf((1.0f - t) * omega) * oneOverSinOmega;
		k1 = sinf(t*omega) * oneOverSinOmega;
	}
	// Interpolate
	Out->x = q0.x*k0 + new_q1.x*k1;
	Out->y = q0.y*k0 + new_q1.y*k1;
	Out->z = q0.z*k0 + new_q1.z*k1;
	Out->w = q0.w*k0 + new_q1.w*k1;
}

void Lerp(QUATERNION *Out, const QUATERNION &q1, const QUATERNION &q2, float t)
{
	float w1 = t;
	float w2 = 1.f - t;

	Out->x = (q1.x * w1) + (q2.x * w2);
	Out->y = (q1.y * w1) + (q2.y * w2);
	Out->z = (q1.z * w1) + (q2.z * w2);
	Out->w = (q1.w * w1) + (q2.w * w2);
}

void Squad(QUATERNION *Out, const QUATERNION &Q1, const QUATERNION &A, const QUATERNION &B, const QUATERNION &C, float t)
{
	QUATERNION Tmp1; Slerp(&Tmp1, Q1, C, t);
	QUATERNION Tmp2; Slerp(&Tmp2, A, B, t);
	Slerp(Out, Tmp1, Tmp2, 2.0f*t*(1.0f-t));
}

void SquadSetup(QUATERNION *OutA, QUATERNION *OutB, QUATERNION *OutC, const QUATERNION &Q0, const QUATERNION &Q1, const QUATERNION &Q2, const QUATERNION &Q3)
{
	QUATERNION q0 = LengthSq(Q0+Q1) < LengthSq(Q0-Q1) ? -Q0 : Q0;
	QUATERNION q2 = LengthSq(Q1+Q2) < LengthSq(Q1-Q2) ? -Q2 : Q2;
	QUATERNION q3 = LengthSq(Q2+Q3) < LengthSq(Q2-Q3) ? -Q3 : Q3;

	QUATERNION tmp1, tmp2, tmp3, tmp4, tmp33, tmp44, q1_exp, q2_exp;
	Exp(&q1_exp, Q1);
	Exp(&q2_exp, q2);

	Mul(&tmp33, q1_exp, q2);
	Mul(&tmp44, q1_exp, q0);
	Log(&tmp3, tmp33);
	Log(&tmp4, tmp44);
	Add(&tmp2, tmp3, tmp4);
	Mul(&tmp2, -0.25f);
	Exp(&tmp1, tmp2);
	Mul(OutA, Q1, tmp1);

	Mul(&tmp33, q2_exp, q3);
	Mul(&tmp44, q2_exp, Q1);
	Log(&tmp3, tmp33);
	Log(&tmp4, tmp44);
	Add(&tmp2, tmp3, tmp4);
	Mul(&tmp2, -0.25f);
	Exp(&tmp1, tmp2);
	Mul(OutB, q2, tmp1);

	*OutC = q2;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// FRUSTUM

void FRUSTUM_PLANES::Set(const MATRIX &WorldViewProj)
{
	// Left clipping plane
	Planes[0].a = WorldViewProj._14 + WorldViewProj._11;
	Planes[0].b = WorldViewProj._24 + WorldViewProj._21;
	Planes[0].c = WorldViewProj._34 + WorldViewProj._31;
	Planes[0].d = WorldViewProj._44 + WorldViewProj._41;
	// Right clipping plane
	Planes[1].a = WorldViewProj._14 - WorldViewProj._11;
	Planes[1].b = WorldViewProj._24 - WorldViewProj._21;
	Planes[1].c = WorldViewProj._34 - WorldViewProj._31;
	Planes[1].d = WorldViewProj._44 - WorldViewProj._41;
	// Top clipping plane
	Planes[2].a = WorldViewProj._14 - WorldViewProj._12;
	Planes[2].b = WorldViewProj._24 - WorldViewProj._22;
	Planes[2].c = WorldViewProj._34 - WorldViewProj._32;
	Planes[2].d = WorldViewProj._44 - WorldViewProj._42;
	// Bottom clipping plane
	Planes[3].a = WorldViewProj._14 + WorldViewProj._12;
	Planes[3].b = WorldViewProj._24 + WorldViewProj._22;
	Planes[3].c = WorldViewProj._34 + WorldViewProj._32;
	Planes[3].d = WorldViewProj._44 + WorldViewProj._42;
	// Near clipping plane
	Planes[4].a = WorldViewProj._13;
	Planes[4].b = WorldViewProj._23;
	Planes[4].c = WorldViewProj._33;
	Planes[4].d = WorldViewProj._43;
	// Far clipping plane
	Planes[5].a = WorldViewProj._14 - WorldViewProj._13;
	Planes[5].b = WorldViewProj._24 - WorldViewProj._23;
	Planes[5].c = WorldViewProj._34 - WorldViewProj._33;
	Planes[5].d = WorldViewProj._44 - WorldViewProj._43;
}

void FRUSTUM_PLANES::Set(const FRUSTUM_POINTS &FrustumPoints)
{
	PointsToPlane(&Planes[PLANE_NEAR],
		FrustumPoints[FRUSTUM_POINTS::NEAR_RIGHT_BOTTOM],
		FrustumPoints[FRUSTUM_POINTS::NEAR_RIGHT_TOP],
		FrustumPoints[FRUSTUM_POINTS::NEAR_LEFT_TOP]);
	PointsToPlane(&Planes[PLANE_FAR],
		FrustumPoints[FRUSTUM_POINTS::FAR_LEFT_BOTTOM],
		FrustumPoints[FRUSTUM_POINTS::FAR_LEFT_TOP],
		FrustumPoints[FRUSTUM_POINTS::FAR_RIGHT_TOP]);
	PointsToPlane(&Planes[PLANE_LEFT],
		FrustumPoints[FRUSTUM_POINTS::NEAR_LEFT_BOTTOM],
		FrustumPoints[FRUSTUM_POINTS::NEAR_LEFT_TOP],
		FrustumPoints[FRUSTUM_POINTS::FAR_LEFT_TOP]);
	PointsToPlane(&Planes[PLANE_RIGHT],
		FrustumPoints[FRUSTUM_POINTS::FAR_RIGHT_BOTTOM],
		FrustumPoints[FRUSTUM_POINTS::FAR_RIGHT_TOP],
		FrustumPoints[FRUSTUM_POINTS::NEAR_RIGHT_TOP]);
	PointsToPlane(&Planes[PLANE_BOTTOM],
		FrustumPoints[FRUSTUM_POINTS::NEAR_LEFT_BOTTOM],
		FrustumPoints[FRUSTUM_POINTS::FAR_LEFT_BOTTOM],
		FrustumPoints[FRUSTUM_POINTS::FAR_RIGHT_BOTTOM]);
	PointsToPlane(&Planes[PLANE_TOP],
		FrustumPoints[FRUSTUM_POINTS::FAR_LEFT_TOP],
		FrustumPoints[FRUSTUM_POINTS::NEAR_LEFT_TOP],
		FrustumPoints[FRUSTUM_POINTS::NEAR_RIGHT_TOP]);
}

void FRUSTUM_PLANES::Normalize()
{
	common::Normalize(&Planes[0]);
	common::Normalize(&Planes[1]);
	common::Normalize(&Planes[2]);
	common::Normalize(&Planes[3]);
	common::Normalize(&Planes[4]);
	common::Normalize(&Planes[5]);
}

void FRUSTUM_POINTS::Set(const FRUSTUM_PLANES &FrustumPlanes)
{
	Intersect3Planes(FrustumPlanes.Planes[4], FrustumPlanes.Planes[0], FrustumPlanes.Planes[3], &Points[0]);
	Intersect3Planes(FrustumPlanes.Planes[4], FrustumPlanes.Planes[1], FrustumPlanes.Planes[3], &Points[1]);
	Intersect3Planes(FrustumPlanes.Planes[4], FrustumPlanes.Planes[0], FrustumPlanes.Planes[2], &Points[2]);
	Intersect3Planes(FrustumPlanes.Planes[4], FrustumPlanes.Planes[1], FrustumPlanes.Planes[2], &Points[3]);
	Intersect3Planes(FrustumPlanes.Planes[5], FrustumPlanes.Planes[0], FrustumPlanes.Planes[3], &Points[4]);
	Intersect3Planes(FrustumPlanes.Planes[5], FrustumPlanes.Planes[1], FrustumPlanes.Planes[3], &Points[5]);
	Intersect3Planes(FrustumPlanes.Planes[5], FrustumPlanes.Planes[0], FrustumPlanes.Planes[2], &Points[6]);
	Intersect3Planes(FrustumPlanes.Planes[5], FrustumPlanes.Planes[1], FrustumPlanes.Planes[2], &Points[7]);
}

void FRUSTUM_POINTS::Set(const MATRIX &WorldViewProjInv)
{
	Vec3 P[] = {
		Vec3(-1.f, -1.f, 0.f), Vec3(+1.f, -1.f, 0.f),
		Vec3(-1.f, +1.f, 0.f), Vec3(+1.f, +1.f, 0.f),
		Vec3(-1.f, -1.f, 1.f), Vec3(+1.f, -1.f, 1.f),
		Vec3(-1.f, +1.f, 1.f), Vec3(+1.f, +1.f, 1.f) };

	TransformCoord(&Points[0], P[0], WorldViewProjInv);
	TransformCoord(&Points[1], P[1], WorldViewProjInv);
	TransformCoord(&Points[2], P[2], WorldViewProjInv);
	TransformCoord(&Points[3], P[3], WorldViewProjInv);
	TransformCoord(&Points[4], P[4], WorldViewProjInv);
	TransformCoord(&Points[5], P[5], WorldViewProjInv);
	TransformCoord(&Points[6], P[6], WorldViewProjInv);
	TransformCoord(&Points[7], P[7], WorldViewProjInv);
}

void FRUSTUM_POINTS::CalcCenter(Vec3 *Out) const
{
	// Œrodek near clipping plane
	Vec3 NearCenter = (Points[NEAR_LEFT_TOP] + Points[NEAR_RIGHT_BOTTOM]) * 0.5f;
	// Œrodek far clipping plane
	Vec3 FarCenter = (Points[FAR_LEFT_BOTTOM] + Points[FAR_RIGHT_TOP]) * 0.5f;
	// Punkt w po³owie odleg³oœci miêdzy nimi to œrodek sfery
	*Out = (NearCenter + FarCenter) * 0.5f;
}

void FRUSTUM_POINTS::CalcBoundingBox(BOX *Box) const
{
	Box->p1 = Box->p2 = Points[0];

	for (int i = 1; i < 8; i++)
	{
		Min(&Box->p1, Box->p1, Points[i]);
		Max(&Box->p2, Box->p2, Points[i]);
	}
}

void FRUSTUM_POINTS::CalcBoundingSphere(Vec3 *SphereCenter, float *SphereRadius) const
{
	// Œrodek frustuma to œrodek sfery
	CalcCenter(SphereCenter);
	// Promieñ sfery to odleg³oœæ tego punktu od jakiegoœ punktu z rogu far clipping plane
	*SphereRadius = Distance(*SphereCenter, Points[FAR_LEFT_TOP]);
}

void FRUSTUM_RADAR::SetFovAndAspect(float FovY, float Aspect)
{
	float FovY_half = FovY * 0.5f;
	float FovY_half_tan = tanf(FovY_half);
	float FovY_half_tan_aspect = FovY_half_tan * Aspect;

	m_UFactor = FovY_half_tan;
	m_RFactor = FovY_half_tan_aspect;

	m_USphereFactor = 1.f / cosf(FovY_half);
	m_RSphereFactor = 1.f / cosf(atanf(FovY_half_tan_aspect));
}

void FRUSTUM_RADAR::SetProjection(float FovY, float Aspect, float ZNear, float ZFar)
{
	m_ZNear = ZNear;
	m_ZFar  = ZFar;
	
	SetFovAndAspect(FovY, Aspect);
}

void FRUSTUM_RADAR::SetView(const Vec3 &Eye, const Vec3 &Forward, const Vec3 &Up, const Vec3 &Right)
{
	m_Eye = Eye;
	m_Forward = Forward;
	m_Up = Up;
	m_Right = Right;
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// KAPSU£A

float CapsuleVolume(const Vec3 &p1, const Vec3 &p2, float R)
{
	return PI * R * R * (
		4.0f/3.0f * R + // kula
		Distance(p1, p2)); // walec
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// INNE

void SphericalToCartesian(Vec3 *Out, float Yaw, float Pitch, float R)
{
	float sy = sinf(Yaw), cy = cosf(Yaw);
	float sp = sinf(Pitch), cp = cosf(Pitch);

	Out->x = R * cp * cy;
	Out->y = R * sp;
	Out->z = - R * cp * sy;
}

void CartesianToSpherical(float *OutYaw, float *OutPitch, float *OutR, const Vec3 &Pos)
{
	float R = Length(Pos);

	if (OutR)
		*OutR = R;
	if (OutYaw)
		*OutYaw = atan2f(-Pos.z, Pos.x);
	if (OutPitch)
		*OutPitch = asinf(Pos.y / R);
}

struct ConvexHull2D_IsLeftSort
{
private:
	Vec2 m_PivotPt;

public:
	static float IsLeft(const Vec2 p0, const Vec2 &p1, const Vec2 &p2)
	{
		return (p1.x-p0.x)*(p2.y-p0.y) - (p2.x-p0.x)*(p1.y-p0.y);
	}

	ConvexHull2D_IsLeftSort(const Vec2 PivotPt) : m_PivotPt(PivotPt) { }

	bool operator() (const Vec2 &p1, const Vec2 &p2)
	{
		return ( IsLeft(m_PivotPt, p1, p2) >= 0 );
	}
};

void ConvexHull2D(std::vector<Vec2> *OutPoints, const Vec2 InPoints[], size_t InPointCount)
{
	assert(InPointCount > 0);

	OutPoints->clear();

	if (InPointCount == 1)
	{
		OutPoints->push_back(InPoints[0]);
		return;
	}

	OutPoints->reserve(InPointCount); // wiêcej ni¿ potrzeba

	Vec2 Pivot = InPoints[0];
	std::vector<Vec2> PointSet; // Stores all points except the pivot
	PointSet.reserve(InPointCount-1);

	for (size_t i = 1; i < InPointCount; i++)
	{
		Vec2 Tmp = InPoints[i];
		if ( (Tmp.y < Pivot.y) || (Tmp.y == Pivot.y && Tmp.x > Pivot.x) )
		{
			PointSet.push_back(Pivot);
			Pivot = Tmp;
		}
		else
			PointSet.push_back(Tmp);
	}

	std::vector<Vec2>::iterator PtEnd = std::unique(PointSet.begin(), PointSet.end()); // Tego nie rozumiem po co to ani jak mo¿e wystêpowaæ przed sort zamiast po, ale w koñcu ja tu nie jestem od rozumienia tylko od kopiowania dobrego kodu od innych ;)
	PointSet.erase(PtEnd, PointSet.end());

	std::sort(PointSet.begin(), PointSet.end(), ConvexHull2D_IsLeftSort(Pivot));

	OutPoints->push_back(Pivot);
	OutPoints->push_back(PointSet[0]);

	size_t cnt = 1;
	while (cnt < PointSet.size())
	{
		const Vec2 & PT1 = (*OutPoints)[OutPoints->size()-1];
		const Vec2 & PT2 = (*OutPoints)[OutPoints->size()-2];
		const Vec2 & PK  = PointSet[cnt];
		float LeftTest = ConvexHull2D_IsLeftSort::IsLeft(PT2, PT1, PK);
		if (LeftTest > 0.f)
		{
			OutPoints->push_back(PK);
			cnt++;
		}
		else if (LeftTest == 0.f)
		{
			cnt++;
			Vec2 DiffVec0 = PK - PT2;
			Vec2 DiffVec1 = PT1 - PT2;
			if ( Dot(DiffVec0, DiffVec0) > Dot(DiffVec1, DiffVec1) )
				(*OutPoints)[OutPoints->size()-1] = PK;
		}
		else
			OutPoints->pop_back();
	}
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// KOLIZJE

float ClosestPointOnLine(const Vec3 &p, const Vec3 &RayOrig, const Vec3 &RayDir)
{
	// Napisane na podstawie ksi¹¿ki "3D Math Primer for Graphics and Game Development", str. 278.
	return Dot(RayDir, p-RayOrig);
}

float PointToLineDistance(const Vec3 &P, const Vec3 &LineOrig, const Vec3 &LineDir)
{
	// Wzór z netu (Worlfram MathWorld): http://mathworld.wolfram.com/Point-LineDistance3-Dimensional.html

	Vec3 vsub; Sub(&vsub, LineOrig, P);
	Vec3 vcr; Cross(&vcr, LineDir, vsub);
	return Length(vcr);
}

bool PointOnLine(const Vec3 &P, const Vec3 &LineOrig, const Vec3 &LineDir)
{
	// Wzór z netu (Worlfram MathWorld): http://mathworld.wolfram.com/Point-LineDistance3-Dimensional.html

	Vec3 vsub; Sub(&vsub, LineOrig, P);
	Vec3 vcr; Cross(&vcr, LineDir, vsub);
	return FLOAT_ALMOST_ZERO(LengthSq(vcr));
}

float PointToLine(const Vec3 &P, const Vec3 &LineOrig, const Vec3 &LineDir)
{
	// Algorytm wymyœli³em sam.

	float abs_dir_x = fabsf(LineDir.x);
	float abs_dir_y = fabsf(LineDir.y);
	float abs_dir_z = fabsf(LineDir.z);

	if (abs_dir_x > abs_dir_y && abs_dir_x > abs_dir_z)
		return (P.x - LineOrig.x) / LineDir.x;
	else if (abs_dir_y > abs_dir_z)
		return (P.y - LineOrig.y) / LineDir.y;
	else
		return (P.z - LineOrig.z) / LineDir.z;
}

bool PointInLineSegment(const Vec3 &P, const Vec3 &SegmentBeg, const Vec3 &SegmentEnd)
{
	// Algorytm wymyœli³em sam.

	float minx = std::min(SegmentBeg.x, SegmentEnd.x);
	float maxx = std::max(SegmentBeg.x, SegmentEnd.x);
	float miny = std::min(SegmentBeg.y, SegmentEnd.y);
	float maxy = std::max(SegmentBeg.y, SegmentEnd.y);
	float minz = std::min(SegmentBeg.z, SegmentEnd.z);
	float maxz = std::max(SegmentBeg.z, SegmentEnd.z);

	float abs_diff_x = maxx - minx;
	float abs_diff_y = maxy - miny;
	float abs_diff_z = maxz - minz;

	if (abs_diff_x > abs_diff_y && abs_diff_x > abs_diff_z)
		return (P.x >= minx && P.x <= maxx);
	else if (abs_diff_y > abs_diff_z)
		return (P.y >= miny && P.y <= maxy);
	else
		return (P.z >= minz && P.z <= maxz);
}

void ClosestPointOnLineSegment(Vec3 *Out, const Vec3 &P, const Vec3 &P1, const Vec3 &P2)
{
	Vec3 Dir = P2-P1;
	float t = ClosestPointOnLine(P, P1, Dir) / Distance(P1, P2);
	t = minmax(0.f, t, 1.f);
	*Out = P1 + Dir * t;
}

void ClosestPointInBox(Vec3 *Out, const BOX &Box, const Vec3 &p)
{
	// Algorytm wymyœli³em sam.

	Out->x = minmax(Box.p1.x, p.x, Box.p2.x);
	Out->y = minmax(Box.p1.y, p.y, Box.p2.y);
	Out->z = minmax(Box.p1.z, p.z, Box.p2.z);
}

float PointToBoxDistance(const Vec3 &p, const BOX &box)
{
	// Algorytm wymyœli³em sam.

	// p jest po lewej
	if (p.x < box.p1.x)
	{
		// p jest na dole
		if (p.y < box.p1.y)
		{
			// p jest z przodu
			if (p.z < box.p1.z)
				return Distance(p, Vec3(box.p1.x, box.p1.y, box.p1.z));
			// p jest w g³êbi
			else if (p.z > box.p2.z)
				return Distance(p, Vec3(box.p1.x, box.p1.y, box.p2.z));
			// p jest poœrodku na z
			else
				return Distance(p, Vec3(box.p1.x, box.p1.y, p.z));
		}
		// p jest u góry
		else if (p.y > box.p2.y)
		{
			// p jest z przodu
			if (p.z < box.p1.z)
				return Distance(p, Vec3(box.p1.x, box.p2.y, box.p1.z));
			// p jest w g³êbi
			else if (p.z > box.p2.z)
				return Distance(p, Vec3(box.p1.x, box.p2.y, box.p2.z));
			// p jest poœrodku na z
			else
				return Distance(p, Vec3(box.p1.x, box.p2.y, p.z));
		}
		// p jest poœrodku na y
		else
		{
			// p jest z przodu
			if (p.z < box.p1.z)
				return Distance(p, Vec3(box.p1.x, p.y, box.p1.z));
			// p jest w g³êbi
			else if (p.z > box.p2.z)
				return Distance(p, Vec3(box.p1.x, p.y, box.p2.z));
			// p jest poœrodku na z
			else
				return box.p1.x - p.x;
				//return Distance(p, Vec3(box.p1.x, p.y, p.z));
		}
	}
	// p jest po prawej
	else if (p.x > box.p2.x)
	{
		// p jest na dole
		if (p.y < box.p1.y)
		{
			// p jest z przodu
			if (p.z < box.p1.z)
				return Distance(p, Vec3(box.p2.x, box.p1.y, box.p1.z));
			// p jest w g³êbi
			else if (p.z > box.p2.z)
				return Distance(p, Vec3(box.p2.x, box.p1.y, box.p2.z));
			// p jest poœrodku na z
			else
				return Distance(p, Vec3(box.p2.x, box.p1.y, p.z));
		}
		// p jest u góry
		else if (p.y > box.p2.y)
		{
			// p jest z przodu
			if (p.z < box.p1.z)
				return Distance(p, Vec3(box.p2.x, box.p2.y, box.p1.z));
			// p jest w g³êbi
			else if (p.z > box.p2.z)
				return Distance(p, Vec3(box.p2.x, box.p2.y, box.p2.z));
			// p jest poœrodku na z
			else
				return Distance(p, Vec3(box.p2.x, box.p2.y, p.z));
		}
		// p jest poœrodku na y
		else
		{
			// p jest z przodu
			if (p.z < box.p1.z)
				return Distance(p, Vec3(box.p2.x, p.y, box.p1.z));
			// p jest w g³êbi
			else if (p.z > box.p2.z)
				return Distance(p, Vec3(box.p2.x, p.y, box.p2.z));
			// p jest poœrodku na z
			else
				return p.x - box.p2.x;
				//return Distance(p, Vec3(box.p2.x, p.y, p.z));
		}
	}
	// p jest poœrodku na x
	else
	{
		// p jest na dole
		if (p.y < box.p1.y)
		{
			// p jest z przodu
			if (p.z < box.p1.z)
				return Distance(p, Vec3(p.x, box.p1.y, box.p1.z));
			// p jest w g³êbi
			else if (p.z > box.p2.z)
				return Distance(p, Vec3(p.x, box.p1.y, box.p2.z));
			// p jest poœrodku na z
			else
				return box.p1.y - p.y;
				//return Distance(p, Vec3(p.x, box.p1.y, p.z));
		}
		// p jest u góry
		else if (p.y > box.p2.y)
		{
			// p jest z przodu
			if (p.z < box.p1.z)
				return Distance(p, Vec3(p.x, box.p2.y, box.p1.z));
			// p jest w g³êbi
			else if (p.z > box.p2.z)
				return Distance(p, Vec3(p.x, box.p2.y, box.p2.z));
			// p jest poœrodku na z
			else
				return p.y - box.p2.y;
				//return Distance(p, Vec3(p.x, box.p2.y, p.z));
		}
		// p jest poœrodku na y
		else
		{
			// p jest z przodu
			if (p.z < box.p1.z)
				return box.p1.z - p.z;
				//return Distance(p, Vec3(p.x, p.y, box.p1.z));
			// p jest w g³êbi
			else if (p.z > box.p2.z)
				return p.z - box.p2.z;
				//return Distance(p, Vec3(p.x, p.y, box.p2.z));
			// p jest poœrodku na z
			// Czyli p le¿y wewn¹trz prostopad³oœcianu
			else
				return 0.0f;
		}
	}
}

bool PointInSphere(const Vec3 &p, const Vec3 &SphereCenter, float SphereRadius)
{
	//return Distance(p, SphereCenter) <= SphereRadius;
	// Nowe, szybsze, mam nadziejê ¿e poprawne
	return DistanceSq(p, SphereCenter) <= SphereRadius * SphereRadius;
}

float PointToSphereDistance(const Vec3 &p, const Vec3 &SphereCenter, float SphereRadius)
{
	return Distance(p, SphereCenter) - SphereRadius;
}

void ClosestPointOnSphere(Vec3 *Out, const Vec3 &SphereCenter, float SphereRadius, const Vec3 &p)
{
	// Napisane na podstawie ksi¹¿ki "3D Math Primer for Graphics and Game Development", str. 280.
	Vec3 d = SphereCenter - p;
	float length_d = Length(d);
	*Out = p + d * ((length_d - SphereRadius) / length_d);
}

bool PointOnPlane(const Vec3 &p, const PLANE &plane)
{
	return FLOAT_ALMOST_ZERO(DotCoord(plane, p));
}

void ClosestPointOnPlane(Vec3 *Out, const PLANE &Plane, const Vec3 &p)
{
	// Napisane na podstawie ksi¹¿ki "3D Math Primer for Graphics and Game Development", str. 279.
	*Out = p - Plane.GetNormal() * DotCoord(Plane, p);
}

bool PointOnTriangle(const Vec3 &point, const Vec3 &pa, const Vec3 &pb, const Vec3 &pc)
{
	// ród³o: Improved Collision detection and Response, Kasper Fauerby

	Vec3 e10 = pb-pa;
	Vec3 e20 = pc-pa;
	float a = Dot(e10, e10);
	float b = Dot(e10, e20);
	float c = Dot(e20, e20);
	float ac_bb = (a*c) - (b*b);
	Vec3 vp(point.x-pa.x, point.y-pa.y, point.z-pa.z);
	float d = Dot(vp, e10);
	float e = Dot(vp, e20);
	float x = (d*c) - (e*b);
	float y = (e*a) - (d*b);
	float z = x+y - ac_bb;
	return (( absolute_cast<unsigned>(z) & ~(absolute_cast<unsigned>(x)|absolute_cast<unsigned>(y)) ) & 0x80000000) != 0;
}

bool PointInFrustum(const Vec3 &p, const FRUSTUM_PLANES &Frustum)
{
	if (DotCoord(Frustum.Planes[0], p) <= 0.0f) return false;
	if (DotCoord(Frustum.Planes[1], p) <= 0.0f) return false;
	if (DotCoord(Frustum.Planes[2], p) <= 0.0f) return false;
	if (DotCoord(Frustum.Planes[3], p) <= 0.0f) return false;
	if (DotCoord(Frustum.Planes[4], p) <= 0.0f) return false;
	if (DotCoord(Frustum.Planes[5], p) <= 0.0f) return false;

	return true;
}

bool PointInFrustum(const Vec3 &p, const FRUSTUM_RADAR &Frustum)
{
	Vec3 OP = p - Frustum.GetEye();

	float f = Dot(OP, Frustum.GetForward());
	if (f < Frustum.GetZNear() || f > Frustum.GetZFar())
		return false;

	float r = Dot(OP, Frustum.GetRight());
	float RLimit = Frustum.GetRFactor() * f;
	if (r < -RLimit || r > RLimit)
		return false;

	float u = Dot(OP, Frustum.GetUp());
	float ULimit = Frustum.GetUFactor() * f;
	if (u < -ULimit || u > ULimit)
		return false;

	return true;
}

bool ClosestPointsOnLines(float *OutT1, float *OutT2, const Vec3 &RayOrig1, const Vec3 &RayDir1, const Vec3 &RayOrig2, const Vec3 &RayDir2)
{
	// Napisane na podstawie ksi¹¿ki "3D Math Primer for Graphics and Game Development", str. 283.

	Vec3 orig_diff = RayOrig2-RayOrig1;
	Vec3 dcr; Cross(&dcr, RayDir1, RayDir2);
	float denom = LengthSq(dcr);
	if (FLOAT_ALMOST_ZERO(denom))
		return false;
	float denom_inv = 1.0f / denom;

	Vec3 cr; Cross(&cr, orig_diff, RayDir2);
	*OutT1 = Dot(cr, dcr) * denom_inv;
	Cross(&cr, orig_diff, RayDir1);
	*OutT2 = Dot(cr, dcr) * denom_inv;

	return true;
}

bool RayToBox(float *OutT, const Vec3 &RayOrig, const Vec3 &RayDir, const BOX &Box)
{
	bool inside = true;
	float xt, xn;

	if (RayOrig.x < Box.p1.x)
	{
		xt = Box.p1.x - RayOrig.x;
		xt /= RayDir.x;
		xn = -1.0f;
		inside = false;
	}
	else if (RayOrig.x > Box.p2.x)
	{
		xt = Box.p2.x - RayOrig.x;
		xt /= RayDir.x;
		xn = 1.0f;
		inside = false;
	}
	else
		xt = -1.0f;

	float yt, yn;

	if (RayOrig.y < Box.p1.y)
	{
		yt = Box.p1.y - RayOrig.y;
		yt /= RayDir.y;
		yn = -1.0f;
		inside = false;
	}
	else if (RayOrig.y > Box.p2.y)
	{
		yt = Box.p2.y - RayOrig.y;
		yt /= RayDir.y;
		yn = 1.0f;
		inside = false;
	}
	else
		yt = -1.0f;

	float zt, zn;

	if (RayOrig.z < Box.p1.z)
	{
		zt = Box.p1.z - RayOrig.z;
		zt /= RayDir.z;
		zn = -1.0f;
		inside = false;
	}
	else if (RayOrig.z > Box.p2.z)
	{
		zt = Box.p2.z - RayOrig.z;
		zt /= RayDir.z;
		zn = 1.0f;
		inside = false;
	}
	else
		zt = -1.0f;

	if (inside)
	{
		*OutT = 0.0f;
		return true;
	}

	// Select the farthest plane - this is the plane of intersection
	int plane = 0;

	float t = xt;
	if (yt > t)
	{
		plane = 1;
		t = yt;
	}

	if (zt > t)
	{
		plane = 2;
		t = zt;
	}

	// Check if the point of intersection lays within the box face

	switch(plane)
	{
	case 0: // ray intersects with yz plane
		{
			float y = RayOrig.y + RayDir.y * t;
			if (y < Box.p1.y || y > Box.p2.y) return false;
			float z = RayOrig.z + RayDir.z * t;
			if (z < Box.p1.z || z > Box.p2.z) return false;
		}
		break;
	case 1: // ray intersects with xz plane
		{
			float x = RayOrig.x + RayDir.x * t;
			if (x < Box.p1.x || x > Box.p2.x) return false;
			float z = RayOrig.z + RayDir.z * t;
			if (z < Box.p1.z || z > Box.p2.z) return false;
		}
		break;
	default:
		case 2: // ray intersects with xy plane
		{
			float x = RayOrig.x + RayDir.x * t;
			if (x < Box.p1.x || x > Box.p2.x) return false;
			float y = RayOrig.y + RayDir.y * t;
			if (y < Box.p1.y || y > Box.p2.y) return false;
		}
		break;
	}

	*OutT = t;
	return true;
}

bool RayToSphere(const Vec3 &RayOrig, const Vec3 &RayDir, const Vec3 &SphereCenter, float SphereRadius, float *OutT)
{
	// Równanie kwadratowe.
	// Napisane samodzielnie z ma³¹ pomoc¹:
	// http://www.siggraph.org/education/materials/HyperGraph/raytrace/rtinter1.htm
	// link znaleziony na:
	// http://www.realtimerendering.com/int/

	Vec3 RayOrig_minus_SphereCenter = RayOrig - SphereCenter;
	float a = Dot(RayDir, RayDir);
	float b = 2.f * Dot(RayDir, RayOrig_minus_SphereCenter);
	float c = Dot(RayOrig_minus_SphereCenter, RayOrig_minus_SphereCenter) - (SphereRadius * SphereRadius);
	float Delta = b * b - 4.f * a * c;

	if (Delta < 0.f)
		return false;

	float a_2 = 2.f * a;
	float minus_b = - b;
	float sqrt_Delta = sqrtf(Delta);

	// Pierwszy pierwiastek - ten mniejszy
	*OutT = (minus_b - sqrt_Delta) / a_2;
	// Przypadek ¿e ca³a sfera jest przed RayOrig - pierwiastek mniejszy to wynik
	if (*OutT >= 0.f)
		return true;
	// Drugi pierwiastek - ten wiêkszy
	*OutT = (minus_b + sqrt_Delta) / a_2;
	// Przypadek ¿e poczatek promienia jest wewn¹trz sfery
	if (*OutT >= 0.f)
	{
		*OutT = 0.f;
		return true;
	}
	// Przypadek ¿e sfera jest z ty³u za promieniem
	return true;
}

/*
Wersja stara, która liczy Ÿle! Nie wiem sk¹d pochodzi.

bool RayToSphere(const Vec3 &RayOrigin, const Vec3 &RayDir, const Vec3 &SphereCenter, float SphereRadius, float *t)
{
	Vec3 Q = SphereCenter - RayOrigin;
	float c = Length(Q);
	float v = Dot(Q, RayDir);
	float d = SphereRadius*SphereRadius - (c*c - v*v);
	// There was no intersection
	if (d < 0.0f)
		return false;
	// Return the distance to the [first] intersecting point
	*t = v - sqrtf(d);
	return true;
}

Jakiœ algorytm na to jest te¿ w ksi¹¿ce "3D Math Primer for Graphics and Game Development", str. 288.
*/

bool RayToPlane(const Vec3 &RayOrig, const Vec3 &RayDir, const PLANE &Plane, float *OutT, float *OutVD)
{
	// Napisane na podstawie:
	// http://www.siggraph.org/education/materials/HyperGraph/raytrace/rayplane_intersection.htm

	//       A xo + B yo + C zo + D
	// t = - ----------------------
	//         A xd + B yd + C zd

	// Ten sam wzór jest w ksi¹¿ce "3D Math Primer for Graphics and Game Development", str. 284.
	// Inna wersja dostêpna jest w ksi¹¿ce: "3D Game Engine Programming", Stefan Zerbst with Oliver Duvel, str. 136.

	*OutVD = Plane.a * RayDir.x + Plane.b * RayDir.y + Plane.c * RayDir.z;
	if (*OutVD == 0.0f)
		return false;
	
	*OutT = - (Plane.a * RayOrig.x + Plane.b * RayOrig.y + Plane.c * RayOrig.z + Plane.d) / *OutVD;
	
	return true;
}

bool RayToTriangle(
	const Vec3 &RayOrig, const Vec3 &RayDir,
	const Vec3 &p0, const Vec3 &p1, const Vec3 &p2,
	bool BackfaceCulling,
	float *OutT)
{
	////// Nowy, piêkny algorytm :)
	// Znaleziony w Google Code Search:
	// http://www.google.com/codesearch?hl=en&q=+RayTriangleIntersect+show:M63-4o6bYUI:fUr9QIwtaKY:Dw059DARM5E&sa=N&cd=1&ct=rc&cs_p=http://www.angstrom-distribution.org/unstable/sources/ode-snapshot-20060210.tar.bz2&cs_f=ode-snapshot-20060210/ode/src/collision_trimesh_trimesh.cpp#first
	// Dopisa³em do niego Backface Culling, dzia³a bez problemu :D

	Vec3 tvec, pvec, qvec;

	// find vectors for two edges sharing vert0
	Vec3 edge1 = p1 - p0;
	Vec3 edge2 = p2 - p0;

	// begin calculating determinant - also used to calculate U parameter
	Cross(&pvec, RayDir, edge2);

	// if determinant is near zero, ray lies in plane of triangle
	float det = Dot(edge1, pvec);
	if (BackfaceCulling && det < 0.0f)
		return false;
	if (FLOAT_ALMOST_ZERO(det))
		return false;
	float inv_det = 1.0f / det;

	// calculate distance from vert0 to ray origin
	tvec = RayOrig - p0;

	// calculate U parameter and test bounds
	float u = Dot(tvec, pvec) * inv_det;
	if (u < 0.0f || u > 1.0f)
		return false;

	// prepare to test V parameter
	Cross(&qvec, tvec, edge1);

	// calculate V parameter and test bounds
	float v = Dot(RayDir, qvec) * inv_det;
	if (v < 0.0f || u + v > 1.0f)
		return false;

	// calculate t, ray intersects triangle
	if (OutT != NULL)
		*OutT = Dot(edge2, qvec) * inv_det;
	return true;

	////// Stary algorytm: Moller i Trumbore
	// Na podstawie ksi¹¿ki: "3D Game Engine Programming", Stefan Zerbst with Oliver Duvel, str. 134.
	// Dobry, ale niestety nie obs³uguje on Backface Culling i nijak nie uda³o mi siê tego do niego wprowadziæ.

/*	Vec3 pvec, tvec, qvec;

	Vec3 edge1 = p1 - p0;
	Vec3 edge2 = p2 - p0;

	Cross(&pvec, RayDir, edge2);

	// If close to 0 ray os parallel
	float det = Dot(edge1, pvec);

	//if (Cull && det < 0.0f)
	if (det < 0.0f)
		return false;
	if (FLOAT_ALMOST_ZERO(det))
		return false;

	// Distance to plane, < 0 means aray behind the plane
	tvec = RayOrig - p0;
	float u = Dot(tvec, pvec);
	if (u < 0.0f || u > det)
		return false;

	Cross(&qvec, tvec, edge1);
	float v = Dot(RayDir, qvec);
	if (v < 0.0f || u+v > det)
		return false;

	if (OutT)
	{
		*OutT = Dot(edge2, qvec);
		float InvDet = 1.0f / det;
		*OutT *= InvDet;
	}
	return true;*/

	// Jakiœ algorytm, du¿o d³u¿szy, jest te¿ w ksi¹¿ce "3D Math Primer for Graphics and Game Development", str. 294.
}

bool RayToFrustum(const Vec3 &RayOrig, const Vec3 &RayDir, const FRUSTUM_PLANES &Frustum, float *t_near, float *t_far)
{
	// To jest uniwersalny algorytm na kolizjê promienia/prostej/odcinka z wieloœcianem wypuk³ym.
	// Na podstawie Graphics Gems II, rozdzia³ V.1.

	*t_near = MINFLOAT;
	*t_far = MAXFLOAT;
	float vn, vd, t;

	for (int i = 0; i < 6; i++)
	{
		vn = DotCoord(Frustum.Planes[i], RayOrig);
		vd = DotNormal(Frustum.Planes[i], RayDir);

		// Nie ma przeciêcia z p³aszczyzn¹
		if (FLOAT_ALMOST_ZERO(vd))
		{
			// Promieñ na zewn¹trz - na pewno nie ma przeciêcia
			if (vn < 0.0f)
				return false;
		}
		else
		{
			t = -vn / vd;

			if (vd > 0.0f)
				*t_near = std::max(*t_near, t);
			else
				*t_far = std::min(*t_far, t);

			if (*t_near > *t_far)
				return false;
		}
	}
	return (*t_near != MINFLOAT && *t_far != MAXFLOAT);
}

bool SphereToBox(const Vec3 &SphereCenter, float SphereRadius, const BOX &Box)
{
	Vec3 PointInBox;
	ClosestPointInBox(&PointInBox, Box, SphereCenter);
	return DistanceSq(SphereCenter, PointInBox) < SphereRadius*SphereRadius;
}

bool SphereInBox(const Vec3 &SphereCenter, float SphereRadius, const BOX &Box)
{
	return
		(SphereCenter.x - SphereRadius >= Box.p1.x) &&
		(SphereCenter.y - SphereRadius >= Box.p1.y) &&
		(SphereCenter.z - SphereRadius >= Box.p1.z) &&
		(SphereCenter.x + SphereRadius <= Box.p2.x) &&
		(SphereCenter.y + SphereRadius <= Box.p2.y) &&
		(SphereCenter.z + SphereRadius <= Box.p2.z);
}

bool BoxInSphere(const BOX &Box, const Vec3 &SphereCenter, float SphereRadius)
{
	Vec3 Corner;
	for (uint i = 0; i < 8; i++)
	{
		Box.GetCorner(&Corner, i);
		if (!PointInSphere(Corner, SphereCenter, SphereRadius))
			return false;
	}
	return true;
}

bool PlaneToBox(const PLANE &plane, const BOX &box)
{
	// Ten algorytm jest te¿ (ale tylko ogólnie) opisany w ksi¹¿ce "3D Math Primer for Graphics and Game Development", str. 285.

	const Vec3 &pn = plane.GetNormal();
	Vec3 vmin, vmax;
	// X
	if (pn.x > 0.0f)
	{
		vmin.x = box.p1.x;
		vmax.x = box.p2.x;
	}
	else
	{
		vmin.x = box.p2.x;
		vmax.x = box.p1.x;
	}
	// Y
	if (pn.y > 0.0f)
	{
		vmin.y = box.p1.y;
		vmax.y = box.p2.y;
	}
	else
	{
		vmin.y = box.p2.y;
		vmax.y = box.p1.y;
	}
	// Z
	if (pn.z > 0.0f)
	{
		vmin.z = box.p1.z;
		vmax.z = box.p2.z;
	}
	else
	{
		vmin.z = box.p2.z;
		vmax.z = box.p1.z;
	}

	if (DotCoord(plane, vmin) > 0.0f)
		return false;
	if (DotCoord(plane, vmax) >= 0.0f)
		return true;
	return false;
}

int ClassifyBoxToPlane(const PLANE &plane, const BOX &box)
{
	if (PlaneToBox(plane, box))
		return 0;
	return (DotCoord(plane, box.p1) > 0.0f) ? +1 : -1;
}

bool TriangleInBox(const Vec3 &p1, const Vec3 &p2, const Vec3 &p3, const BOX &Box)
{
	return PointInBox(p1, Box) && PointInBox(p2, Box) && PointInBox(p3, Box);
}

bool TriangleToBox(const Vec3 &p1, const Vec3 &p2, const Vec3 &p3, const BOX &Box, const PLANE *OptTrianglePlane)
{
	// Algorytm wymyœli³em sam, na podstawie Separaring Axis Theorem.

	// Testowanie z p³aszczyznami œcian prostopad³oœcianu
	if (p1.x < Box.p1.x && p2.x < Box.p1.x && p3.x < Box.p1.x)
		return false;
	if (p1.x > Box.p2.x && p2.x > Box.p2.x && p3.x > Box.p2.x)
		return false;
	if (p1.y < Box.p1.y && p2.y < Box.p1.y && p3.y < Box.p1.y)
		return false;
	if (p1.y > Box.p2.y && p2.y > Box.p2.y && p3.y > Box.p2.y)
		return false;
	if (p1.z < Box.p1.z && p2.z < Box.p1.z && p3.z < Box.p1.z)
		return false;
	if (p1.z > Box.p2.z && p2.z > Box.p2.z && p3.z > Box.p2.z)
		return false;

	// Testowanie wierzcho³ków prostopad³oœcianu z p³aszczyzn¹ trójk¹ta
	PLANE TrianglePlane;
	if (OptTrianglePlane == NULL)
	{
		PointsToPlane(&TrianglePlane, p1, p2, p3);
		OptTrianglePlane = &TrianglePlane;
	}
	Vec3 Corner;
	Box.GetCorner(&Corner, 0);
	float d = DotCoord(*OptTrianglePlane, Corner);
	if (d == 0.0f)
		return true;
	for (int i = 1; i < 8; i++)
	{
		Box.GetCorner(&Corner, i);
		// Jeœli wysz³o = 0 (wierzcho³ek dotyka p³aszczyzny) lub < 0
		// (wierzcho³ek jest po przeciwnej stronie ni¿ pierwszy), to jest kolizja
		if (d * DotCoord(*OptTrianglePlane, Corner) <= 0.f)
			return true;
	}
	return false;
}

/* Stara wersja.
Jest dobrze sprawdzone, ¿e dzia³a dok³adnie tak samo jak ta nowa ale jest trochê wolniejsza.

bool PlaneToBox(const PLANE &plane, const BOX &box)
{
	float side = DotCoord(plane, box.p1);

	if (DotCoord(plane, Vec3(box.p2.x, box.p1.y, box.p1.z)) * side < 0.0f)
		return true;
	if (DotCoord(plane, Vec3(box.p1.x, box.p1.y, box.p2.z)) * side < 0.0f)
		return true;
	if (DotCoord(plane, Vec3(box.p2.x, box.p1.y, box.p2.z)) * side < 0.0f)
		return true;
	if (DotCoord(plane, Vec3(box.p1.x, box.p2.y, box.p1.z)) * side < 0.0f)
		return true;
	if (DotCoord(plane, Vec3(box.p2.x, box.p2.y, box.p1.z)) * side < 0.0f)
		return true;
	if (DotCoord(plane, Vec3(box.p1.x, box.p2.y, box.p2.z)) * side < 0.0f)
		return true;
	if (DotCoord(plane, box.p2) * side < 0.0f)
		return true;

	return false;
}*/

bool BoxToFrustum_Fast(const BOX &Box, const FRUSTUM_PLANES &Frustum)
{
	// Na podstawie ksi¹¿ki "3D Game Engine Programming", Stefan Zerbst with Oliver Duvel

	Vec3 vmin;
	for (int i = 0; i < 6; i++)
	{
		if (Frustum.Planes[i].a <= 0.0f)
			vmin.x = Box.p1.x;
		else
			vmin.x = Box.p2.x;

		if (Frustum.Planes[i].b <= 0.0f)
			vmin.y = Box.p1.y;
		else
			vmin.y = Box.p2.y;

		if (Frustum.Planes[i].c <= 0.0f)
			vmin.z = Box.p1.z;
		else
			vmin.z = Box.p2.z;

		if (DotCoord(Frustum.Planes[i], vmin) < 0.0f)
			return false;
	}
	return true;
}

bool BoxToFrustum(const BOX &Box, const FRUSTUM_PLANES &FrustumPlanes, const FRUSTUM_POINTS *OptFrustumPoints)
{
	// Najpierw test podstawowy
	if (!BoxToFrustum_Fast(Box, FrustumPlanes))
		return false;

	// Wylicz Frustum Points jeœli trzeba
	FRUSTUM_POINTS MyFrustumPoints;
	const FRUSTUM_POINTS *FrustumPointsPtr = OptFrustumPoints;
	if (FrustumPointsPtr == NULL)
	{
		MyFrustumPoints.Set(FrustumPlanes);
		FrustumPointsPtr = &MyFrustumPoints;
	}

	// Dodatkowy test wg Separating Axis Theorem

	Vec3 BoxCenter; Box.CalcCenter(&BoxCenter);
	Vec3 FrustumCenter; FrustumPointsPtr->CalcCenter(&FrustumCenter);
	Vec3 BoxCorner;
	BoxCorner.x = (FrustumCenter.x > BoxCenter.x ? Box.p2.x : Box.p1.x);
	BoxCorner.y = (FrustumCenter.y > BoxCenter.y ? Box.p2.y : Box.p1.y);
	BoxCorner.z = (FrustumCenter.z > BoxCenter.z ? Box.p2.z : Box.p1.z);

	Vec3 BoxToFrustumVec = FrustumCenter - BoxCenter;
	Vec3 BoxToFrustumDir; Normalize(&BoxToFrustumDir, BoxToFrustumVec);

	PLANE SeparatingPlane;
	PointNormalToPlane(&SeparatingPlane, BoxCorner, BoxToFrustumDir);

	if (ClassifyFrustumToPlane(*FrustumPointsPtr, SeparatingPlane) > 0)
		return false;

	return true;
}

bool BoxToFrustum(const BOX &Box, const FRUSTUM_RADAR &Frustum)
{
	// Algorytm na podstawie kodu z ksi¹¿ki "Game Programming Gems", tom 5, rozdz. 1.6
	// "Improved Frustum Culling", Frank Puig Placeres

	Vec3 P;
	int OutOfLeft = 0, OutOfRight = 0, OutOfFar = 0, OutOfNear = 0, OutOfTop = 0, OutOfBottom = 0;
	bool IsInRightTest, IsInUpTest, IsInFrontTest;

	Vec3 Corners[2];
	Corners[0] = Box.p1 - Frustum.GetEye();
	Corners[1] = Box.p2 - Frustum.GetEye();

	for (int i = 0; i < 8; i++)
	{
		IsInRightTest = IsInUpTest = IsInFrontTest = false;
		P.x = Corners[ i        & 1 ].x;
		P.y = Corners[ (i >> 2) & 1 ].y;
		P.z = Corners[ (i >> 1) & 1 ].z;

		float r = Dot(P, Frustum.GetRight());
		float u = Dot(P, Frustum.GetUp());
		float f = Dot(P, Frustum.GetForward());

		if (r < -Frustum.GetRFactor() * f)
			OutOfLeft++;
		else if (r > Frustum.GetRFactor() * f)
			OutOfRight++;
		else
			IsInRightTest = true;

		if (u < -Frustum.GetUFactor() * f)
			OutOfBottom++;
		else if (u > Frustum.GetUFactor() * f)
			OutOfTop++;
		else
			IsInUpTest = true;

		if (f < -Frustum.GetZNear())
			OutOfNear++;
		else if (f > Frustum.GetZFar())
			OutOfFar++;
		else
			IsInUpTest = true;

		if (IsInRightTest && IsInFrontTest && IsInUpTest)
			return true;
	}

	if (OutOfLeft == 8 || OutOfRight == 8 || OutOfFar == 8 || OutOfNear == 8 || OutOfTop == 8 || OutOfBottom == 8)
		return false;
	return true;
}

bool BoxInFrustum(const BOX &Box, const FRUSTUM_PLANES &Frustum)
{
	// Ka¿dy punkt AABB musi le¿eæ w jego wnêtrzu
	if (!PointInFrustum(Box.p1, Frustum)) return false;
	if (!PointInFrustum(Vec3(Box.p2.x, Box.p1.y, Box.p1.z), Frustum)) return false;
	if (!PointInFrustum(Vec3(Box.p1.x, Box.p2.y, Box.p1.z), Frustum)) return false;
	if (!PointInFrustum(Vec3(Box.p2.x, Box.p2.y, Box.p1.z), Frustum)) return false;
	if (!PointInFrustum(Vec3(Box.p1.x, Box.p1.y, Box.p2.z), Frustum)) return false;
	if (!PointInFrustum(Vec3(Box.p2.x, Box.p1.y, Box.p2.z), Frustum)) return false;
	if (!PointInFrustum(Vec3(Box.p1.x, Box.p2.y, Box.p2.z), Frustum)) return false;
	if (!PointInFrustum(Box.p2, Frustum)) return false;
	return true;
}

bool SphereToSphere(const Vec3 &Center1, float Radius1, const Vec3 &Center2, float Radius2)
{
	float x = Radius1 + Radius2;
	return DistanceSq(Center1, Center2) <= x*x;
}

bool SphereToPlane(const Vec3 &SphereCenter, float SphereRadius, const PLANE &Plane)
{
	return fabsf(DotCoord(Plane, SphereCenter)) <= SphereRadius;
}

int ClassifySphereToPlane(const Vec3 &SphereCenter, float SphereRadius, const PLANE &Plane)
{
	float d = DotCoord(Plane, SphereCenter);
	if (d > SphereRadius)
		return +1;
	else if (d < -SphereRadius)
		return -1;
	else
		return 0;
}

bool SphereToFrustum_Fast(const Vec3 &SphereCenter, float SphereRadius, const FRUSTUM_PLANES &Frustum)
{
	SphereRadius = -SphereRadius;
	if (DotCoord(Frustum.Planes[0], SphereCenter) <= SphereRadius) return false;
	if (DotCoord(Frustum.Planes[1], SphereCenter) <= SphereRadius) return false;
	if (DotCoord(Frustum.Planes[2], SphereCenter) <= SphereRadius) return false;
	if (DotCoord(Frustum.Planes[3], SphereCenter) <= SphereRadius) return false;
	if (DotCoord(Frustum.Planes[4], SphereCenter) <= SphereRadius) return false;
	if (DotCoord(Frustum.Planes[5], SphereCenter) <= SphereRadius) return false;

	return true;
}

bool SphereToFrustum(
	const Vec3 &SphereCenter, float SphereRadius,
	const FRUSTUM_PLANES &FrustumPlanes, const FRUSTUM_POINTS *OptFrustumPoints)
{
	if (!SphereToFrustum_Fast(SphereCenter, SphereRadius, FrustumPlanes))
		return false;

	// Wylicz Frustum Points jeœli trzeba
	FRUSTUM_POINTS MyFrustumPoints;
	const FRUSTUM_POINTS *FrustumPointsPtr = OptFrustumPoints;
	if (FrustumPointsPtr == NULL)
	{
		MyFrustumPoints.Set(FrustumPlanes);
		FrustumPointsPtr = &MyFrustumPoints;
	}

	// Dodatkowy test wg Separating Axis Theorem

	Vec3 FrustumCenter; FrustumPointsPtr->CalcCenter(&FrustumCenter);
	Vec3 SphereToFrustumVec = FrustumCenter - SphereCenter;
	Vec3 SphereToFrustumDir; Normalize(&SphereToFrustumDir, SphereToFrustumVec);

	PLANE SeparatingPlane;
	PointNormalToPlane(&SeparatingPlane, SphereCenter + SphereToFrustumDir * SphereRadius, SphereToFrustumDir);

	if (ClassifyFrustumToPlane(*FrustumPointsPtr, SeparatingPlane) > 0)
		return false;

	return true;
}

bool SphereToFrustum(const Vec3 &SphereCenter, float SphereRadius, const FRUSTUM_RADAR &Frustum)
{
	Vec3 OP = SphereCenter - Frustum.GetEye();

	float f = Dot(OP, Frustum.GetForward());
	if (f < Frustum.GetZNear() - SphereRadius || f > Frustum.GetZFar() + SphereRadius)
		return false;

	// dr i du to jest poprawka, której nie uwzglêdnia oryginalny artyku³ o reprezentacji radarowej
	// w ksi¹¿ce "Game Programming Gems 5", tom 5, rozdz. 1.6 - "Improved Frustum Culling",
	// Frank Puig Placeres, ale uwzglêdnia j¹ autor "View Frustum Culling Tutorial",
	// António Ramires Fernandes, http://www.lighthouse3d.com/opengl/viewfrustum/index.php

	float dr = SphereRadius * Frustum.GetRSphereFactor();
	float r = Dot(OP, Frustum.GetRight());
	float RLimit = Frustum.GetRFactor() * f;
	if (r < -RLimit - dr || r > RLimit + dr)
		return false;

	float du = SphereRadius * Frustum.GetUSphereFactor();
	float u = Dot(OP, Frustum.GetUp());
	float ULimit = Frustum.GetUFactor() * f;
	if (u < -ULimit - du || u > ULimit + du)
		return false;

	return true;
}

bool SphereInFrustum(const Vec3 &SphereCenter, float SphereRadius, const FRUSTUM_PLANES &Frustum)
{
	if (DotCoord(Frustum.Planes[0], SphereCenter) < SphereRadius) return false;
	if (DotCoord(Frustum.Planes[1], SphereCenter) < SphereRadius) return false;
	if (DotCoord(Frustum.Planes[2], SphereCenter) < SphereRadius) return false;
	if (DotCoord(Frustum.Planes[3], SphereCenter) < SphereRadius) return false;
	if (DotCoord(Frustum.Planes[4], SphereCenter) < SphereRadius) return false;
	if (DotCoord(Frustum.Planes[5], SphereCenter) < SphereRadius) return false;

	return true;
}

bool Intersect2Planes(const PLANE &Plane1, const PLANE &Plane2, Vec3 *LineOrig, Vec3 *LineDir)
{
	float dot = Dot(Plane1.GetNormal(), Plane2.GetNormal());
	if (FLOAT_ALMOST_ZERO(dot))
		return false;
	Cross(LineDir, Plane1.GetNormal(), Plane2.GetNormal());
	float abs;
	float maxabs = fabsf(LineDir->x);
	int index = 0;
	if ((abs = fabsf(LineDir->y)) > maxabs) { maxabs = abs; index = 1; }
	if ((abs = fabsf(LineDir->z)) > maxabs) { maxabs = abs; index = 2; }

	switch (index)
	{
	case 0:
		LineOrig->x = 0.0f;
		LineOrig->y = (Plane1.c*Plane2.d - Plane2.c*Plane1.d) / LineDir->x;
		LineOrig->z = (Plane2.b*Plane1.d - Plane1.b*Plane2.d) / LineDir->x;
		break;
	case 1:
		LineOrig->x = (Plane2.c*Plane1.d - Plane1.c*Plane2.d) / LineDir->y;
		LineOrig->y = 0.0f;
		LineOrig->z = (Plane1.a*Plane2.d - Plane2.a*Plane1.d) / LineDir->y;
		break;
	case 2:
		LineOrig->x = (Plane1.b*Plane2.d - Plane2.b*Plane1.d) / LineDir->z;
		LineOrig->y = (Plane2.a*Plane1.d - Plane1.a*Plane2.d) / LineDir->z;
		LineOrig->z = 0.0f;
		break;
	}

	return true;
}

/* Stara wersja, sprawdzone ¿e dzia³a tak samo jak powy¿sza tylko wolniej i zwraca
inny punkt na prostej (ale ten mo¿e byæ dowolny).

bool PlaneToPlane(const PLANE &Plane1, const PLANE &Plane2, Vec3 *OutRayOrig, Vec3 *OutRayDir)
{
	Vec3 cr;
	float lsq;
	// If cross product equals 0 planes are parallel
	Cross(&cr, Plane1.GetNormal(), Plane2.GetNormal());
	lsq = LengthSq(cr);
	if (FLOAT_ALMOST_ZERO(lsq))
		return false;

	// Intersection line if needed
	if (OutRayOrig && OutRayDir)
	{
		float N00 = LengthSq(Plane1.GetNormal());
		float N01 = Dot(Plane1.GetNormal(), Plane2.GetNormal());
		float N11 = LengthSq(Plane2.GetNormal());
		float Det = N00*N11 - N01*N01;
		if (FLOAT_ALMOST_ZERO(Det))
			return false;
		float InvDet = 1.0f / Det;
		// Tu musia³em odwróciæ sk³adniki odejmowania celem skorygowania D,
		// bo by³o uwzglêdniane odwrotnie ni¿ ja stosujê w swojej reprezentacji p³aszczyzny.
		float C0 = (N01*Plane2.d - N11*Plane1.d) * InvDet;
		float C1 = (N01*Plane1.d - N00*Plane2.d) * InvDet;
		*OutRayDir = cr;
		*OutRayOrig = Plane1.GetNormal() * C0 + Plane2.GetNormal() * C1;
	}

	return true;
}*/

// Get the intersection point of three planes.
//                    -1
// |x|     | n1.x n1.y n1.z |     |d1|
// |y|  =  | n2.x n2.y n2.z |  X  |d2|
// |z|     | n3.x n3.y n3.z |     |d3|
bool Intersect3Planes(const PLANE &P1, const PLANE &P2, const PLANE &P3, Vec3 *OutP)
{
	// Inny algorytm (albo inaczej zapisany) jest te¿ w ksi¹¿ce "3D Math Primer for Graphics and Game Development", str. 286.

	float fDet;
    float MN[9]  = { P1.a, P1.b, P1.c, P2.a, P2.b, P2.c, P3.a, P3.b, P3.c };
    float IMN[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    float MD[3]  = { -P1.d, -P2.d , -P3.d };

    IMN[0] =   MN[4] * MN[8] - MN[5] * MN[7];
    IMN[3] = -(MN[3] * MN[8] - MN[5] * MN[6]);
    IMN[6] =   MN[3] * MN[7] - MN[4] * MN[6];

    fDet = MN[0] * IMN[0] + MN[1] * IMN[3] + MN[2] * IMN[6];

    if(FLOAT_ALMOST_ZERO(fDet))
        return false;

    IMN[1] = -(MN[1] * MN[8] - MN[2] * MN[7]);
    IMN[4] =   MN[0] * MN[8] - MN[2] * MN[6];
    IMN[7] = -(MN[0] * MN[7] - MN[1] * MN[6]);
    IMN[2] =   MN[1] * MN[5] - MN[2] * MN[4];
    IMN[5] = -(MN[0] * MN[5] - MN[2] * MN[3]);
    IMN[8] =   MN[0] * MN[4] - MN[1] * MN[3];

    fDet = 1.0f / fDet;

    IMN[0] *= fDet;
    IMN[1] *= fDet;
    IMN[2] *= fDet;
    IMN[3] *= fDet;
    IMN[4] *= fDet;
    IMN[5] *= fDet;
    IMN[6] *= fDet;
    IMN[7] *= fDet;
    IMN[8] *= fDet;

    OutP->x = IMN[0] * MD[0] + IMN[1] * MD[1] + IMN[2] * MD[2];
    OutP->y = IMN[3] * MD[0] + IMN[4] * MD[1] + IMN[5] * MD[2];
    OutP->z = IMN[6] * MD[0] + IMN[7] * MD[1] + IMN[8] * MD[2];

    return true;
}

bool PlaneToTriangle(const PLANE &Plane, const Vec3 &p0, const Vec3 &p1, const Vec3 &p2)
{
	float F = DotCoord(Plane, p0);
	if (F == 0.0f)
		return true;
	if (DotCoord(Plane, p1)*F <= 0.0f || DotCoord(Plane, p2)*F <= 0.0f)
		return true;
	return false;
}

int ClassifyPlaneToTriangle(const PLANE &Plane, const Vec3 &p0, const Vec3 &p1, const Vec3 &p2)
{
	float F = DotCoord(Plane, p0);
	if (F == 0.0f)
		return 0;
	if (DotCoord(Plane, p1)*F <= 0.0f || DotCoord(Plane, p2)*F <= 0.0f)
		return 0;
	return (F > 0.0f) ? +1 : -1;
}

int ClassifyFrustumToPlane(const FRUSTUM_POINTS &Frustum, const PLANE &Plane)
{
	float a = DotCoord(Plane, Frustum.Points[0]); if (a == 0.0f) return 0;
	float b = DotCoord(Plane, Frustum.Points[1]); if (b*a <= 0.0f) return 0;
	b = DotCoord(Plane, Frustum.Points[2]); if (b*a <= 0.0f) return 0;
	b = DotCoord(Plane, Frustum.Points[3]); if (b*a <= 0.0f) return 0;
	b = DotCoord(Plane, Frustum.Points[4]); if (b*a <= 0.0f) return 0;
	b = DotCoord(Plane, Frustum.Points[5]); if (b*a <= 0.0f) return 0;
	b = DotCoord(Plane, Frustum.Points[6]); if (b*a <= 0.0f) return 0;
	b = DotCoord(Plane, Frustum.Points[7]); if (b*a <= 0.0f) return 0;
	return (a > 0.0f) ? +1 : -1;
}

bool IntersectLines(const Vec2 &p1, const Vec2 &p2, const Vec2 &p3, const Vec2 &p4)
{
	// NIE TESTOWANA

	// test 1 - bouding boksy
	RECTF r1, r2;

	r1.left   = std::min(p1.x, p2.x);
	r1.right  = std::max(p1.x, p2.x);
	r1.top    = std::min(p1.y, p2.y);
	r1.bottom = std::max(p1.y, p2.y);

	r2.left   = std::min(p3.x, p4.x);
	r2.right  = std::max(p3.x, p4.x);
	r2.top    = std::min(p3.y, p4.y);
	r2.bottom = std::max(p3.y, p4.y);

	if (!OverlapRect(r1, r2))
		return false;

	// test 2 - odcinek 1 z 2
	Vec2 v, v1, v2;
	float d1, d2;
	v = p2 - p1;

	v1 = p3 - p1;
	v2 = p4 - p1;
	d1 = v.x*v1.y - v.y*v1.x;
	d2 = v.x*v2.y - v.y*v2.x;

	if (d1*d2 > 0.0f)
		return false;

	// test 3 - odcinek 2 z 1
	v = p4 - p3;

	v1 = p1 - p3;
	v2 = p2 - p3;
	d1 = v.x*v1.y - v.y*v1.x;
	d2 = v.x*v2.y - v.y*v2.x;

	if (d1*d2 > 0.0f)
		return false;

	// wszystko OK
	return true;
}

// sort so that a <= b
inline void TT_Sort(float &a, float &b)
{
	if (a > b) { float c; c = a; a = b; b = c; }
}

// this edge to edge test is based on Franlin Antonio's gem:
// "Faster Line Segment Intersection", in Graphics Gems III,
// pp. 199-202.
#define TT_EDGE_EDGE_TEST(V0,U0,U1)                    \
	Bx=U0[i0]-U1[i0];                                  \
	By=U0[i1]-U1[i1];                                  \
	Cx=V0[i0]-U0[i0];                                  \
	Cy=V0[i1]-U0[i1];                                  \
	f=Ay*Bx-Ax*By;                                     \
	d=By*Cx-Bx*Cy;                                     \
	if((f>0 && d>=0 && d<=f) || (f<0 && d<=0 && d>=f)) \
	{                                                  \
		e=Ax*Cy-Ay*Cx;                                 \
		if(f>0)                                        \
		{                                              \
			if(e>=0 && e<=f) return true;              \
		}                                              \
		else                                           \
		{                                              \
			if(e<=0 && e>=f) return true;              \
		}                                              \
	}                                

#define TT_EDGE_AGAINST_TRI_EDGES(V0,V1,U0,U1,U2) \
{                                                 \
	float Ax,Ay,Bx,By,Cx,Cy,e,d,f;                \
	Ax=V1[i0]-V0[i0];                             \
	Ay=V1[i1]-V0[i1];                             \
	/* test edge U0,U1 against V0,V1 */           \
	TT_EDGE_EDGE_TEST(V0,U0,U1);                  \
	/* test edge U1,U2 against V0,V1 */           \
	TT_EDGE_EDGE_TEST(V0,U1,U2);                  \
	/* test edge U2,U1 against V0,V1 */           \
	TT_EDGE_EDGE_TEST(V0,U2,U0);                  \
}

#define TT_POINT_IN_TRI(V0,U0,U1,U2)          \
{                                             \
	float a,b,c,d0,d1,d2;                     \
	/* is T1 completly inside T2? */          \
	/* check if V0 is inside tri(U0,U1,U2) */ \
	a=U1[i1]-U0[i1];                          \
	b=-(U1[i0]-U0[i0]);                       \
	c=-a*U0[i0]-b*U0[i1];                     \
	d0=a*V0[i0]+b*V0[i1]+c;                   \
	                                          \
	a=U2[i1]-U1[i1];                          \
	b=-(U2[i0]-U1[i0]);                       \
	c=-a*U1[i0]-b*U1[i1];                     \
	d1=a*V0[i0]+b*V0[i1]+c;                   \
	                                          \
	a=U0[i1]-U2[i1];                          \
	b=-(U0[i0]-U2[i0]);                       \
	c=-a*U2[i0]-b*U2[i1];                     \
	d2=a*V0[i0]+b*V0[i1]+c;                   \
	if(d0*d1>0.0)                             \
	{                                         \
		if(d0*d2>0.0) return true;            \
	}                                         \
}

bool TT_ComplanarTriTri(const Vec3 &N,const Vec3 &V0,const Vec3 &V1,const Vec3 &V2,
	const Vec3 &U0,const Vec3 &U1,const Vec3 &U2)
{
	float A[3];
	short i0,i1;
	// first project onto an axis-aligned plane, that maximizes the area
	// of the triangles, compute indices: i0,i1.
	A[0]=fabs(N[0]);
	A[1]=fabs(N[1]);
	A[2]=fabs(N[2]);
	if(A[0]>A[1])
	{
		if(A[0]>A[2])  
		{
			i0=1; // A[0] is greatest
			i1=2;
		}
		else
		{
			i0=0; // A[2] is greatest
			i1=1;
		}
	}
	else //A[0]<=A[1]
	{
		if(A[2]>A[1])
		{
			i0=0; // A[2] is greatest
			i1=1;
		}
		else
		{
			i0=0; // A[1] is greatest
			i1=2;
		}
	}

	// test all edges of triangle 1 against the edges of triangle 2
	TT_EDGE_AGAINST_TRI_EDGES(V0,V1,U0,U1,U2);
	TT_EDGE_AGAINST_TRI_EDGES(V1,V2,U0,U1,U2);
	TT_EDGE_AGAINST_TRI_EDGES(V2,V0,U0,U1,U2);

	// finally, test if tri1 is totally contained in tri2 or vice versa
	TT_POINT_IN_TRI(V0,U0,U1,U2);
	TT_POINT_IN_TRI(U0,V0,V1,V2);

	return false;
}

#define NEWCOMPUTE_INTERVALS(VV0,VV1,VV2,D0,D1,D2,D0D1,D0D2,A,B,C,X0,X1) \
{ \
        if(D0D1>0.0f) \
        { \
                /* here we know that D0D2<=0.0 */ \
            /* that is D0, D1 are on the same side, D2 on the other or on the plane */ \
                A=VV2; B=(VV0-VV2)*D2; C=(VV1-VV2)*D2; X0=D2-D0; X1=D2-D1; \
        } \
        else if(D0D2>0.0f)\
        { \
                /* here we know that d0d1<=0.0 */ \
            A=VV1; B=(VV0-VV1)*D1; C=(VV2-VV1)*D1; X0=D1-D0; X1=D1-D2; \
        } \
        else if(D1*D2>0.0f || D0!=0.0f) \
        { \
                /* here we know that d0d1<=0.0 or that D0!=0.0 */ \
                A=VV0; B=(VV1-VV0)*D0; C=(VV2-VV0)*D0; X0=D0-D1; X1=D0-D2; \
        } \
        else if(D1!=0.0f) \
        { \
                A=VV1; B=(VV0-VV1)*D1; C=(VV2-VV1)*D1; X0=D1-D0; X1=D1-D2; \
        } \
        else if(D2!=0.0f) \
        { \
                A=VV2; B=(VV0-VV2)*D2; C=(VV1-VV2)*D2; X0=D2-D0; X1=D2-D1; \
        } \
        else \
        { \
                /* triangles are coplanar */ \
                return TT_ComplanarTriTri(N1,V0,V1,V2,U0,U1,U2); \
        } \
}

/*
Ta funkcja i powy¿sze TT_ to algorytm testu trójk¹t-trójk¹t wg:
"A Fast Triangle-Triangle Intersection Test", Thomas Moller
Kod na podstawie: http://jgt.akpeters.com/papers/Moller97/tritri.html
*/
bool TriangleToTriangle(
	const Vec3 &V0,const Vec3 &V1,const Vec3 &V2,
	const Vec3 &U0,const Vec3 &U1,const Vec3 &U2)
{
  Vec3 E1,E2;
  Vec3 N1,N2;
  float d1,d2;
  float du0,du1,du2,dv0,dv1,dv2;
  Vec3 D;
  Vec2 isect1, isect2;
  float du0du1,du0du2,dv0dv1,dv0dv2;
  short index;
  float vp0,vp1,vp2;
  float up0,up1,up2;
  float bb,cc,max;
  float a,b,c,x0,x1;
  float d,e,f,y0,y1;
  float xx,yy,xxyy,tmp;

  /* compute plane equation of triangle(V0,V1,V2) */
  E1 = V1 - V0;
  E2 = V2 - V0;
  Cross(&N1, E1, E2);
  d1 = -Dot(N1, V0);
  /* plane equation 1: N1.X+d1=0 */

  /* put U0,U1,U2 into plane equation 1 to compute signed distances to the plane*/
  du0=Dot(N1,U0)+d1;
  du1=Dot(N1,U1)+d1;
  du2=Dot(N1,U2)+d1;

  /* coplanarity robustness check */
  if (FLOAT_ALMOST_ZERO(du0)) du0 = 0.f;
  if (FLOAT_ALMOST_ZERO(du1)) du1 = 0.f;
  if (FLOAT_ALMOST_ZERO(du2)) du2 = 0.f;

  du0du1=du0*du1;
  du0du2=du0*du2;

  if(du0du1>0.0f && du0du2>0.0f) /* same sign on all of them + not equal 0 ? */
    return false;                    /* no intersection occurs */

  /* compute plane of triangle (U0,U1,U2) */
  E1 = U1 - U0;
  E2 = U2 - U0;
  Cross(&N2, E1, E2);
  d2=-Dot(N2,U0);
  /* plane equation 2: N2.X+d2=0 */

  /* put V0,V1,V2 into plane equation 2 */
  dv0=Dot(N2,V0)+d2;
  dv1=Dot(N2,V1)+d2;
  dv2=Dot(N2,V2)+d2;

  if (FLOAT_ALMOST_ZERO(dv0)) dv0 = 0.f;
  if (FLOAT_ALMOST_ZERO(dv1)) dv1 = 0.f;
  if (FLOAT_ALMOST_ZERO(dv2)) dv2 = 0.f;

  dv0dv1=dv0*dv1;
  dv0dv2=dv0*dv2;

  if(dv0dv1>0.0f && dv0dv2>0.0f) /* same sign on all of them + not equal 0 ? */
    return false;                    /* no intersection occurs */

  /* compute direction of intersection line */
  Cross(&D,N1,N2);

  /* compute and index to the largest component of D */
  max=(float)fabsf(D[0]);
  index=0;
  bb=(float)fabsf(D[1]);
  cc=(float)fabsf(D[2]);
  if(bb>max) max=bb,index=1;
  if(cc>max) max=cc,index=2;

  /* this is the simplified projection onto L*/
  vp0=V0[index];
  vp1=V1[index];
  vp2=V2[index];

  up0=U0[index];
  up1=U1[index];
  up2=U2[index];

  /* compute interval for triangle 1 */
  NEWCOMPUTE_INTERVALS(vp0,vp1,vp2,dv0,dv1,dv2,dv0dv1,dv0dv2,a,b,c,x0,x1);

  /* compute interval for triangle 2 */
  NEWCOMPUTE_INTERVALS(up0,up1,up2,du0,du1,du2,du0du1,du0du2,d,e,f,y0,y1);

  xx=x0*x1;
  yy=y0*y1;
  xxyy=xx*yy;

  tmp=a*xxyy;
  isect1[0]=tmp+b*x1*yy;
  isect1[1]=tmp+c*x0*yy;

  tmp=d*xxyy;
  isect2[0]=tmp+e*xx*y1;
  isect2[1]=tmp+f*xx*y0;

  TT_Sort(isect1[0],isect1[1]);
  TT_Sort(isect2[0],isect2[1]);

  if(isect1[1]<isect2[0] || isect2[1]<isect1[0]) return false;
  return true;
}

bool TriangleInFrustum(const Vec3 &t1, const Vec3 &t2, const Vec3 &t3, const FRUSTUM_PLANES &Frustum)
{
	return
		PointInFrustum(t1, Frustum) &&
		PointInFrustum(t2, Frustum) &&
		PointInFrustum(t3, Frustum);
}

bool TriangleToFrustum(
	const Vec3 &t1, const Vec3 &t2, const Vec3 &t3, const PLANE *OptTrianglePlane,
	const FRUSTUM_PLANES &FrustumPlanes, const FRUSTUM_POINTS *OptFrustumPoints)
{
	// Algorytm wymyœli³em sam na podstawie Separating Axis Theorem.

	// Etap 1

	// Ca³y trójk¹t le¿y po ujemnej stronie jednej z p³aszczyzn frustuma - odrzuæ
	for (uint i = 0; i < 6; i++)
	{
		if (
			DotCoord(FrustumPlanes[i], t1) < 0.f &&
			DotCoord(FrustumPlanes[i], t2) < 0.f &&
			DotCoord(FrustumPlanes[i], t3) < 0.f)
		{
			return false;
		}
	}

	// Etap 2

	// Wylicz TrianglePlane
	const PLANE *TrianglePlanePtr = OptTrianglePlane;
	PLANE TrianglePlane;
	if (TrianglePlanePtr == NULL)
	{
		PointsToPlane(&TrianglePlane, t1, t2, t3);
		TrianglePlanePtr = &TrianglePlane;
	}

	// Wylicz FrustumPoints
	const FRUSTUM_POINTS *FrustumPointsPtr = OptFrustumPoints;
	FRUSTUM_POINTS FrustumPoints;
	if (FrustumPointsPtr == NULL)
	{
		FrustumPoints.Set(FrustumPlanes);
		FrustumPointsPtr = &FrustumPoints;
	}

	// Frustum nie przecina p³aszczyzny trójk¹ta - odrzuæ
	if (ClassifyFrustumToPlane(*FrustumPointsPtr, *TrianglePlanePtr) != 0)
		return false;

	return true;
}

bool FrustumToFrustum(
	const FRUSTUM_PLANES &Frustum1_Planes, const FRUSTUM_POINTS &Frustum1_Points,
	const FRUSTUM_PLANES &Frustum2_Planes, const FRUSTUM_POINTS &Frustum2_Points)
{
	// Na podstawie Separating Axis Theorem:
	// Dwa frustumy nie zachodz¹ na siebie wtedy i tylko wtedy, gdy wszystkie punkty
	// jednego z nich le¿¹ po ujemnej stronie jednej z p³aszczyzn drugiego.

	for (uint i = 0; i < 6; i++)
		if (ClassifyFrustumToPlane(Frustum2_Points, Frustum1_Planes[i]) < 0)
			return false;
	for (uint i = 0; i < 6; i++)
		if (ClassifyFrustumToPlane(Frustum1_Points, Frustum2_Planes[i]) < 0)
			return false;

	return true;

	// Inny, ale niekoniecznie szybszy algorytm znajduje siê w ksi¹¿ce ShaderX, tom 3, rozdz. 5.4, str. 437.
}

bool PointInPolygon(const Vec2 &Point, const Vec2 PolygonPoints[], uint PolygonPointCount)
{
	// Na podstawie:
	// Point-In-Polygon Algorithm — Determining Whether A Point Is Inside A Complex Polygon. Darel Rex Finley.
	// http://www.alienryderflex.com/polygon/
	uint i, j = 0;
	bool OddNodes = false;
	for (i = 0; i < PolygonPointCount; i++)
	{
		j++;
		if (j == PolygonPointCount)
		j = 0;
		if (PolygonPoints[i].y < Point.y && PolygonPoints[j].y >= Point.y ||
			PolygonPoints[j].y < Point.y && PolygonPoints[i].y >= Point.y)
		{
			if (PolygonPoints[i].x + (Point.y - PolygonPoints[i].y) /
				(PolygonPoints[j].y - PolygonPoints[i].y) *
				(PolygonPoints[j].x - PolygonPoints[i].x) < Point.x)
			{
				OddNodes = !OddNodes;
			}
		}
	}
	return OddNodes;
}

/*
Funkcja by³a testowana tylko dla przypadku gdy abs(Plane.Normal.y) najwiêksze czyli p³aszczyzna le¿y w XZ,
ale jeœli siê nigdzie nie pomyli³em przy zamianie wspó³rzêdnych przy kopiowaniu to powinna dzia³aæ wszêdzie.
*/
bool RayToPolygon(const Vec3 &RayOrig, const Vec3 &RayDir, const Vec3 PolygonPoints[], uint PolygonPointCount, bool BackfaceCulling, float *OutT)
{
	if (PolygonPointCount < 3)
		return false;

	PLANE Plane;
	PointsToPlane(&Plane, PolygonPoints[0], PolygonPoints[1], PolygonPoints[2]);
	
	float VD;
	if (!RayToPlane(RayOrig, RayDir, Plane, OutT, &VD))
		return false;
	if (BackfaceCulling && VD > 0.0f)
		return false;

	// Algorytm skopiowany z PointInPolygon
	
	Vec3 Point = RayOrig + RayDir * (*OutT);
	float fnx = fabsf(Plane.a);
	float fny = fabsf(Plane.b);
	float fnz = fabsf(Plane.c);
	uint i, j = 0;
	bool OddNodes = false;

	// Najwiêkszy jest X - rzut na p³aszczyznê YZ
	if (fnx > fny && fnx > fnz)
	{
		for (i = 0; i < PolygonPointCount; i++)
		{
			j++;
			if (j == PolygonPointCount)
			j = 0;
			if (PolygonPoints[i].y < Point.y && PolygonPoints[j].y >= Point.y ||
				PolygonPoints[j].y < Point.y && PolygonPoints[i].y >= Point.y)
			{
				if (PolygonPoints[i].z + (Point.y - PolygonPoints[i].y) /
					(PolygonPoints[j].y - PolygonPoints[i].y) *
					(PolygonPoints[j].z - PolygonPoints[i].z) < Point.z)
				{
					OddNodes = !OddNodes;
				}
			}
		}
		return OddNodes;
	}
	// Najwiêkszy jest Z - rzut na p³aszczyznê XY
	else if (fnz > fny)
	{
		for (i = 0; i < PolygonPointCount; i++)
		{
			j++;
			if (j == PolygonPointCount)
			j = 0;
			if (PolygonPoints[i].y < Point.y && PolygonPoints[j].y >= Point.y ||
				PolygonPoints[j].y < Point.y && PolygonPoints[i].y >= Point.y)
			{
				if (PolygonPoints[i].x + (Point.y - PolygonPoints[i].y) /
					(PolygonPoints[j].y - PolygonPoints[i].y) *
					(PolygonPoints[j].x - PolygonPoints[i].x) < Point.x)
				{
					OddNodes = !OddNodes;
				}
			}
		}
		return OddNodes;
	}
	// Najwiêkszy jest Y - rzut na p³aszczyznê XZ
	else
	{
		for (i = 0; i < PolygonPointCount; i++)
		{
			j++;
			if (j == PolygonPointCount)
			j = 0;
			if (PolygonPoints[i].z < Point.z && PolygonPoints[j].z >= Point.z ||
				PolygonPoints[j].z < Point.z && PolygonPoints[i].z >= Point.z)
			{
				if (PolygonPoints[i].x + (Point.z - PolygonPoints[i].z) /
					(PolygonPoints[j].z - PolygonPoints[i].z) *
					(PolygonPoints[j].x - PolygonPoints[i].x) < Point.x)
				{
					OddNodes = !OddNodes;
				}
			}
		}
		return OddNodes;
	}
}

void SphereBoundingPoints(Vec3 *OutSphereCenter, float *OutSphereRadius, const Vec3 Points[], size_t PointCount)
{
	assert(PointCount > 0);

	const Vec3 *PtIt = &Points[0];
	const Vec3 *EndIt = &Points[0] + PointCount;
	float Radius = 0.f;
	Vec3 Center = *PtIt++;

	while (PtIt != EndIt)
	{
		const Vec3 &Tmp = *PtIt++;
		Vec3 CVec = Tmp - Center;
		float d = Dot(CVec, CVec);
		if (d > Radius*Radius)
		{
			d = sqrtf(d);
			float r = 0.5f * (d + Radius);
			float scale = (r-Radius) / d;
			Center += scale * CVec;
			Radius = r;
		}
	}

	*OutSphereCenter = Center;
	*OutSphereRadius = Radius;
}

bool SweptSphereToPlane(const Vec3 &SphereCenter, float SphereRadius, const Vec3 &SphereSweepDir, const PLANE &Plane, float *OutT0, float *OutT1)
{
	float b_dot_n = DotCoord(Plane, SphereCenter);
	float d_dot_n = DotNormal(Plane, SphereSweepDir);

	if (d_dot_n == 0.f)
	{
		if (fabsf(b_dot_n) <= SphereRadius)
		{
			if (OutT0) *OutT0 = 0.f;
			if (OutT1) *OutT1 = MAXFLOAT;
			return true;
		}
		else
			return false;
	}
	else
	{
		if (OutT0 || OutT1)
		{
			d_dot_n = 1.f / d_dot_n;
			float Tmp0 = ( SphereRadius - b_dot_n) * d_dot_n;
			float Tmp1 = (-SphereRadius - b_dot_n) * d_dot_n;
			if (OutT0) *OutT0 = std::min(Tmp0, Tmp1);
			if (OutT1) *OutT1 = std::max(Tmp0, Tmp1);
		}
		return true;
	}
}

bool SweptSphereToFrustum(const Vec3 &SphereCenter, float SphereRadius, const Vec3 &SphereSweepDir, const FRUSTUM_PLANES &Frustum)
{
	// Funkcja na podstawie: PracticalPSM - przyk³adowy program autorstwa nVidia Coroporation.
	// Ale znacznie ulepszona, bo tam by³a napisana beznadziejnie!

    // algorithm -- get all 12 intersection points of the swept sphere with the view frustum
    // for all points >0, displace sphere along the sweep driection.  if the displaced sphere
    // is inside the frustum, return TRUE.  else, return FALSE

	float a, b;
	Vec3 DisplacedSphereCenter;
	float DisplacedSphereRadius = SphereRadius; // * 1.1f; // <- tak by³o w oryginale, ale WTF???

	for (size_t i = 0; i < 6; i++)
	{
		if (SweptSphereToPlane(SphereCenter, SphereRadius, SphereSweepDir, Frustum[i], &a, &b))
		{
			if (a >= 0.f)
			{
				DisplacedSphereCenter = SphereCenter + SphereSweepDir * a;
				if (SphereToFrustum_Fast(DisplacedSphereCenter, DisplacedSphereRadius, Frustum))
					return true;
			}
			if (b >= 0.f)
			{
				DisplacedSphereCenter = SphereCenter + SphereSweepDir * b;
				if (SphereToFrustum_Fast(DisplacedSphereCenter, DisplacedSphereRadius, Frustum))
					return true;
			}
		}
	}

	return false;
}

bool SweptBoxToBox(const BOX &Box1, const BOX &Box2, const Vec3 &SweepDirBox1, float *OutT)
{
	// Zmniejsz Box1 do pojedynczego poruszaj¹cego sie punktu (promienia),
	// a Box2 rozszerz o rozmiar Box1 (suma minkowskiego)
	// W ten sposób problem sprowadza siê do kolizji Ray-Box.
	// Algorytm wymyœli³em sam.

	Vec3 RayOrig; Box1.CalcCenter(&RayOrig);
	Vec3 BoxExtend; Box1.GetSize(&BoxExtend);
	BoxExtend *= 0.5f;
	BOX Box;
	Box.p1.x = Box2.p1.x - BoxExtend.x;
	Box.p1.y = Box2.p1.y - BoxExtend.y;
	Box.p1.z = Box2.p1.z - BoxExtend.z;
	Box.p2.x = Box2.p2.x + BoxExtend.x;
	Box.p2.y = Box2.p2.y + BoxExtend.y;
	Box.p2.z = Box2.p2.z + BoxExtend.z;

	return RayToBox(OutT, RayOrig, SweepDirBox1, Box);
}

void BoxBoundingPoints(BOX *box, const Vec3 points[], size_t PointCount)
{
	assert(PointCount > 0);

	box->p1 = box->p2 = points[0];

	for (size_t i = 1; i < PointCount; i++)
	{
		Min(&box->p1, box->p1, points[i]);
		Max(&box->p2, box->p2, points[i]);
	}
}

void BoxBoundingSphere(Vec3 *SphereCenter, float *SphereRadius, const BOX &Box)
{
	Box.CalcCenter(SphereCenter);
	float Diagonal = Length(Box.p2 - Box.p1); // Przek¹tna
	*SphereRadius = Diagonal / 2.f;
}

void SphereBoundingBox(BOX *Out, const Vec3 &SphereCenter, float SphereRadius)
{
	Out->p1 = SphereCenter;
	Out->p1.x -= SphereRadius;
	Out->p1.y -= SphereRadius;
	Out->p1.z -= SphereRadius;
	Out->p2 = SphereCenter;
	Out->p2.x += SphereRadius;
	Out->p2.y += SphereRadius;
	Out->p2.z += SphereRadius;
}

void BoxBoundingSpheres(BOX *OutBox, const Vec3 SpheresCenter[], const float SpheresRadius[], size_t SphereCount)
{
	assert(SphereCount > 0);

	OutBox->p1 = OutBox->p2 = SpheresCenter[0];
	float r = SpheresRadius[0];
	OutBox->p1.x -= r; OutBox->p1.y -= r; OutBox->p1.z -= r;
	OutBox->p2.x += r; OutBox->p2.y += r; OutBox->p2.z += r;

	for (size_t i = 1; i < SphereCount; i++)
	{
		const Vec3 &c = SpheresCenter[i];
		r = SpheresRadius[i];

		OutBox->p1.x = std::min(OutBox->p1.x, c.x - r);
		OutBox->p1.y = std::min(OutBox->p1.y, c.y - r);
		OutBox->p1.z = std::min(OutBox->p1.z, c.z - r);
		OutBox->p2.x = std::max(OutBox->p2.x, c.x + r);
		OutBox->p2.y = std::max(OutBox->p2.y, c.y + r);
		OutBox->p2.z = std::max(OutBox->p2.z, c.z + r);
	}
}

} // namespace common
