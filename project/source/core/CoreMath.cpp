#include "Pch.h"
#include "Core.h"

#ifdef _DEBUG
HRESULT _d_hr;
#endif
RNG _RNG;

#define FLOAT_ALMOST_ZERO(F) ((absolute_cast<unsigned>(F) & 0x7f800000L) == 0)

const Int2 Int2::Zero = { 0,0 };

const Rect Rect::Zero = { 0,0,0,0 };

const Vec2 Vec2::Zero = { 0.f, 0.f };
const Vec2 Vec2::One = { 1.f, 1.f };
const Vec2 Vec2::UnitX = { 1.f, 0.f };
const Vec2 Vec2::UnitY = { 0.f, 1.f };

const Vec3 Vec3::Zero = { 0.f, 0.f, 0.f };
const Vec3 Vec3::One = { 1.f, 1.f, 1.f };
const Vec3 Vec3::UnitX = { 1.f, 0.f, 0.f };
const Vec3 Vec3::UnitY = { 0.f, 1.f, 0.f };
const Vec3 Vec3::UnitZ = { 0.f, 0.f, 1.f };
const Vec3 Vec3::Up = { 0.f, 1.f, 0.f };
const Vec3 Vec3::Down = { 0.f, -1.f, 0.f };
const Vec3 Vec3::Right = { 1.f, 0.f, 0.f };
const Vec3 Vec3::Left = { -1.f, 0.f, 0.f };
const Vec3 Vec3::Forward = { 0.f, 0.f, -1.f };
const Vec3 Vec3::Backward = { 0.f, 0.f, 1.f };

const Vec4 Vec4::Zero = { 0.f, 0.f, 0.f, 0.f };
const Vec4 Vec4::One = { 1.f, 1.f, 1.f, 1.f };
const Vec4 Vec4::UnitX = { 1.f, 0.f, 0.f, 0.f };
const Vec4 Vec4::UnitY = { 0.f, 1.f, 0.f, 0.f };
const Vec4 Vec4::UnitZ = { 0.f, 0.f, 1.f, 0.f };
const Vec4 Vec4::UnitW = { 0.f, 0.f, 0.f, 1.f };

const Matrix Matrix::IdentityMatrix = {
	1.f, 0.f, 0.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 1.f, 0.f,
	0.f, 0.f, 0.f, 1.f
};

const Quat Quat::Identity = { 0.f, 0.f, 0.f, 1.f };

//=================================================================================================
// Zwraca k¹t pomiêdzy dwoma punktami
//=================================================================================================
float Angle(float x1, float y1, float x2, float y2)
{
	float x = x2 - x1;
	float y = y2 - y1;

	if(x == 0)
	{
		if(y == 0)
			return 0;
		else if(y > 0)
			return PI / 2.f;
		else
			return PI * 3.f / 2.f;
	}
	else if(y == 0)
	{
		if(x > 0)
			return 0;
		else
			return PI;
	}
	else
	{
		if(x < 0)
			return atan(y / x) + PI;
		else if(y < 0)
			return atan(y / x) + (2 * PI);
		else
			return atan(y / x);
	}
}

//=================================================================================================
// Kolizja promienia z prostopad³oœcianem
// Jeœli promieñ nie przecina prostopad³oœcianu, zwraca false.
// Jeœli promieñ przecina prostopad³oœcian, zwraca true i przez out_t zwraca odleg³oœæ w wielokrotnoœciach d³ugoœci ray_dir.
// Jeœli promieñ przecina prostopad³oœcian od ty³u, funkcja te¿ zwraca true i zwraca out_t ujemne.
// Jeœli ray_pos jest wewn¹trz prostopad³oœcianu, funkcja zwraca true i out_t = 0.
// funkcja z TFQE
//=================================================================================================
bool RayToBox(const Vec3& ray_pos, const Vec3& ray_dir, const Box &box, float* out_t)
{
	// removed xn, yn, zn
	bool inside = true;
	float xt;//, xn;

	if(ray_pos.x < box.v1.x)
	{
		xt = box.v1.x - ray_pos.x;
		xt /= ray_dir.x;
		//xn = -1.0f;
		inside = false;
	}
	else if(ray_pos.x > box.v2.x)
	{
		xt = box.v2.x - ray_pos.x;
		xt /= ray_dir.x;
		//xn = 1.0f;
		inside = false;
	}
	else
		xt = -1.0f;

	float yt;//, yn;

	if(ray_pos.y < box.v1.y)
	{
		yt = box.v1.y - ray_pos.y;
		yt /= ray_dir.y;
		//yn = -1.0f;
		inside = false;
	}
	else if(ray_pos.y > box.v2.y)
	{
		yt = box.v2.y - ray_pos.y;
		yt /= ray_dir.y;
		//yn = 1.0f;
		inside = false;
	}
	else
		yt = -1.0f;

	float zt;//, zn;

	if(ray_pos.z < box.v1.z)
	{
		zt = box.v1.z - ray_pos.z;
		zt /= ray_dir.z;
		//zn = -1.0f;
		inside = false;
	}
	else if(ray_pos.z > box.v2.z)
	{
		zt = box.v2.z - ray_pos.z;
		zt /= ray_dir.z;
		//zn = 1.0f;
		inside = false;
	}
	else
		zt = -1.0f;

	if(inside)
	{
		*out_t = 0.0f;
		return true;
	}

	// Select the farthest plane - this is the plane of intersection
	int plane = 0;

	float t = xt;
	if(yt > t)
	{
		plane = 1;
		t = yt;
	}

	if(zt > t)
	{
		plane = 2;
		t = zt;
	}

	// Check if the point of intersection lays within the box face

	switch(plane)
	{
	case 0: // ray intersects with yz plane
		{
			float y = ray_pos.y + ray_dir.y * t;
			if(y < box.v1.y || y > box.v2.y) return false;
			float z = ray_pos.z + ray_dir.z * t;
			if(z < box.v1.z || z > box.v2.z) return false;
		}
		break;
	case 1: // ray intersects with xz plane
		{
			float x = ray_pos.x + ray_dir.x * t;
			if(x < box.v1.x || x > box.v2.x) return false;
			float z = ray_pos.z + ray_dir.z * t;
			if(z < box.v1.z || z > box.v2.z) return false;
		}
		break;
	default:
	case 2: // ray intersects with xy plane
		{
			float x = ray_pos.x + ray_dir.x * t;
			if(x < box.v1.x || x > box.v2.x) return false;
			float y = ray_pos.y + ray_dir.y * t;
			if(y < box.v1.y || y > box.v2.y) return false;
		}
		break;
	}

	*out_t = t;
	return true;
}

//=================================================================================================
// W któr¹ stronê trzeba siê obróciæ ¿eby by³o najszybciej
//=================================================================================================
float ShortestArc(float a, float b)
{
	if(fabs(b - a) < PI)
		return b - a;
	if(b > a)
		return b - a - PI*2.0f;
	return b - a + PI*2.0f;
}

//=================================================================================================
// Interpolacja k¹tów
//=================================================================================================
void LerpAngle(float& angle, float from, float to, float t)
{
	if(to > angle)
	{
		while(to - angle > PI)
			to -= PI * 2;
	}
	else
	{
		while(to - angle < -PI)
			to += PI * 2;
	}

	angle = from + t * (to - from);
}

bool CircleToRectangle(float circlex, float circley, float radius, float rectx, float recty, float w, float h)
{
	//
	//        /|\ -h
	//         |
	//         |
	//  -w <--(x,y)--> w
	//         |
	//         |
	//        \|/  h
	float dist_x = abs(circlex - rectx);
	float dist_y = abs(circley - recty);

	if((dist_x > (w + radius)) || (dist_y > (h + radius)))
		return false;

	if((dist_x <= w) || (dist_y <= h))
		return true;

	float dx = dist_x - w;
	float dy = dist_y - h;

	return (dx*dx + dy*dy) <= (radius*radius);
}


bool Box2d::IsFullyInside(const Vec2& v, float r) const
{
	return v.x - r >= v1.x
		&& v.x + r <= v2.x
		&& v.y - r >= v1.y
		&& v.y + r <= v2.y;
}

bool Plane::Intersect3Planes(const Plane& P1, const Plane& P2, const Plane& P3, Vec3& OutP)
{
	float fDet;
	float MN[9] = { P1.x, P1.y, P1.z, P2.x, P2.y, P2.z, P3.x, P3.y, P3.z };
	float IMN[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	float MD[3] = { -P1.w, -P2.w , -P3.w };

	IMN[0] = MN[4] * MN[8] - MN[5] * MN[7];
	IMN[3] = -(MN[3] * MN[8] - MN[5] * MN[6]);
	IMN[6] = MN[3] * MN[7] - MN[4] * MN[6];

	fDet = MN[0] * IMN[0] + MN[1] * IMN[3] + MN[2] * IMN[6];

	if(FLOAT_ALMOST_ZERO(fDet))
		return false;

	IMN[1] = -(MN[1] * MN[8] - MN[2] * MN[7]);
	IMN[4] = MN[0] * MN[8] - MN[2] * MN[6];
	IMN[7] = -(MN[0] * MN[7] - MN[1] * MN[6]);
	IMN[2] = MN[1] * MN[5] - MN[2] * MN[4];
	IMN[5] = -(MN[0] * MN[5] - MN[2] * MN[3]);
	IMN[8] = MN[0] * MN[4] - MN[1] * MN[3];

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

	OutP.x = IMN[0] * MD[0] + IMN[1] * MD[1] + IMN[2] * MD[2];
	OutP.y = IMN[3] * MD[0] + IMN[4] * MD[1] + IMN[5] * MD[2];
	OutP.z = IMN[6] * MD[0] + IMN[7] * MD[1] + IMN[8] * MD[2];

	return true;
}

void FrustumPlanes::Set(const Matrix& worldViewProj)
{
	// Left clipping plane
	planes[0].x = worldViewProj._14 + worldViewProj._11;
	planes[0].y = worldViewProj._24 + worldViewProj._21;
	planes[0].z = worldViewProj._34 + worldViewProj._31;
	planes[0].w = worldViewProj._44 + worldViewProj._41;
	planes[0].Normalize();

	// Right clipping plane
	planes[1].x = worldViewProj._14 - worldViewProj._11;
	planes[1].y = worldViewProj._24 - worldViewProj._21;
	planes[1].z = worldViewProj._34 - worldViewProj._31;
	planes[1].w = worldViewProj._44 - worldViewProj._41;
	planes[1].Normalize();

	// Top clipping plane
	planes[2].x = worldViewProj._14 - worldViewProj._12;
	planes[2].y = worldViewProj._24 - worldViewProj._22;
	planes[2].z = worldViewProj._34 - worldViewProj._32;
	planes[2].w = worldViewProj._44 - worldViewProj._42;
	planes[2].Normalize();

	// Bottom clipping plane
	planes[3].x = worldViewProj._14 + worldViewProj._12;
	planes[3].y = worldViewProj._24 + worldViewProj._22;
	planes[3].z = worldViewProj._34 + worldViewProj._32;
	planes[3].w = worldViewProj._44 + worldViewProj._42;
	planes[3].Normalize();

	// Near clipping plane
	planes[4].x = worldViewProj._13;
	planes[4].y = worldViewProj._23;
	planes[4].z = worldViewProj._33;
	planes[4].w = worldViewProj._43;
	planes[4].Normalize();

	// Far clipping plane
	planes[5].x = worldViewProj._14 - worldViewProj._13;
	planes[5].y = worldViewProj._24 - worldViewProj._23;
	planes[5].z = worldViewProj._34 - worldViewProj._33;
	planes[5].w = worldViewProj._44 - worldViewProj._43;
	planes[5].Normalize();
}

void FrustumPlanes::GetPoints(Vec3* points) const
{
	assert(points);

	Plane::Intersect3Planes(planes[4], planes[0], planes[3], points[0]);
	Plane::Intersect3Planes(planes[4], planes[1], planes[3], points[1]);
	Plane::Intersect3Planes(planes[4], planes[0], planes[2], points[2]);
	Plane::Intersect3Planes(planes[4], planes[1], planes[2], points[3]);
	Plane::Intersect3Planes(planes[5], planes[0], planes[3], points[4]);
	Plane::Intersect3Planes(planes[5], planes[1], planes[3], points[5]);
	Plane::Intersect3Planes(planes[5], planes[0], planes[2], points[6]);
	Plane::Intersect3Planes(planes[5], planes[1], planes[2], points[7]);
}

void FrustumPlanes::GetPoints(const Matrix& worldViewProj, Vec3* points)
{
	assert(points);

	Matrix worldViewProjInv;
	worldViewProj.Inverse(worldViewProjInv);

	Vec3 P[] = {
		Vec3(-1.f, -1.f, 0.f), Vec3(+1.f, -1.f, 0.f),
		Vec3(-1.f, +1.f, 0.f), Vec3(+1.f, +1.f, 0.f),
		Vec3(-1.f, -1.f, 1.f), Vec3(+1.f, -1.f, 1.f),
		Vec3(-1.f, +1.f, 1.f), Vec3(+1.f, +1.f, 1.f) };

	for(int i = 0; i < 8; ++i)
		points[i] = Vec3::Transform(P[i], worldViewProjInv);
}

bool FrustumPlanes::PointInFrustum(const Vec3 &p) const
{
	for(int i = 0; i < 6; ++i)
	{
		if(planes[i].DotCoordinate(p) <= 0.f)
			return false;
	}

	return true;
}

bool FrustumPlanes::BoxToFrustum(const Box& box) const
{
	Vec3 vmin;

	for(int i = 0; i < 6; i++)
	{
		if(planes[i].x <= 0.0f)
			vmin.x = box.v1.x;
		else
			vmin.x = box.v2.x;

		if(planes[i].y <= 0.0f)
			vmin.y = box.v1.y;
		else
			vmin.y = box.v2.y;

		if(planes[i].z <= 0.0f)
			vmin.z = box.v1.z;
		else
			vmin.z = box.v2.z;

		if(planes[i].DotCoordinate(vmin) < 0.0f)
			return false;
	}

	return true;
}

bool FrustumPlanes::BoxToFrustum(const Box2d& box) const
{
	Vec3 vmin;

	for(int i = 0; i < 6; i++)
	{
		if(planes[i].x <= 0.0f)
			vmin.x = box.v1.x;
		else
			vmin.x = box.v2.x;

		if(planes[i].y <= 0.0f)
			vmin.y = 0.f;
		else
			vmin.y = 25.f;

		if(planes[i].z <= 0.0f)
			vmin.z = box.v1.y;
		else
			vmin.z = box.v2.y;

		if(planes[i].DotCoordinate(vmin) < 0.0f)
			return false;
	}

	return true;
}

bool FrustumPlanes::BoxInFrustum(const Box& box) const
{
	if(!PointInFrustum(box.v1)) return false;
	if(!PointInFrustum(box.v2)) return false;
	if(!PointInFrustum(Vec3(box.v2.x, box.v1.y, box.v1.z))) return false;
	if(!PointInFrustum(Vec3(box.v1.x, box.v2.y, box.v1.z))) return false;
	if(!PointInFrustum(Vec3(box.v2.x, box.v2.y, box.v1.z))) return false;
	if(!PointInFrustum(Vec3(box.v1.x, box.v1.y, box.v2.z))) return false;
	if(!PointInFrustum(Vec3(box.v2.x, box.v1.y, box.v2.z))) return false;
	if(!PointInFrustum(Vec3(box.v1.x, box.v2.y, box.v2.z))) return false;

	return true;
}

bool FrustumPlanes::SphereToFrustum(const Vec3& sphere_center, float sphere_radius) const
{
	sphere_radius = -sphere_radius;

	for(int i = 0; i < 6; ++i)
	{
		if(planes[i].DotCoordinate(sphere_center) <= sphere_radius)
			return false;
	}

	return true;
}

bool FrustumPlanes::SphereInFrustum(const Vec3& sphere_center, float sphere_radius) const
{
	for(int i = 0; i < 6; ++i)
	{
		if(planes[i].DotCoordinate(sphere_center) < sphere_radius)
			return false;
	}

	return true;
}

// funkcja z TFQE
bool RayToPlane(const Vec3& ray_pos, const Vec3& ray_dir, const Plane& plane, float* out_t)
{
	// Napisane na podstawie:
	// http://www.siggraph.org/education/materials/HyperGraph/raytrace/rayplane_intersection.htm

	//       A xo + B yo + C zo + D
	// t = - ----------------------
	//         A xd + B yd + C zd

	// Ten sam wzór jest w ksi¹¿ce "3D Math Primer for Graphics and Game Development", str. 284.
	// Inna wersja dostêpna jest w ksi¹¿ce: "3D Game Engine Programming", Stefan Zerbst with Oliver Duvel, str. 136.

	float VD = plane.x * ray_dir.x + plane.y * ray_dir.y + plane.z * ray_dir.z;
	if(VD == 0.0f)
		return false;

	*out_t = -(plane.x * ray_pos.x + plane.y * ray_pos.y + plane.z * ray_pos.z + plane.w) / VD;

	return true;
}

// funkcja z TFQE
bool RayToSphere(const Vec3& _ray_pos, const Vec3& _ray_dir, const Vec3& _center, float _radius, float& _dist)
{
	// Równanie kwadratowe.
	// Napisane samodzielnie z ma³¹ pomoc¹:
	// http://www.siggraph.org/education/materials/HyperGraph/raytrace/rtinter1.htm
	// link znaleziony na:
	// http://www.realtimerendering.com/int/
	Vec3 RayOrig_minus_SphereCenter = _ray_pos - _center;
	float a = _ray_dir.Dot(_ray_dir); // ?
	float b = 2.f * _ray_dir.Dot(RayOrig_minus_SphereCenter);
	float c = RayOrig_minus_SphereCenter.Dot(RayOrig_minus_SphereCenter) - (_radius * _radius);
	float Delta = b * b - 4.f * a * c;

	if(Delta < 0.f)
		return false;

	float a_2 = 2.f * a;
	float minus_b = -b;
	float sqrt_Delta = sqrtf(Delta);

	// Pierwszy pierwiastek - ten mniejszy
	_dist = (minus_b - sqrt_Delta) / a_2;
	// Przypadek ¿e ca³a sfera jest przed ray_pos - pierwiastek mniejszy to wynik
	if(_dist >= 0.f)
		return true;
	// Drugi pierwiastek - ten wiêkszy
	_dist = (minus_b + sqrt_Delta) / a_2;
	// Przypadek ¿e poczatek promienia jest wewn¹trz sfery
	if(_dist >= 0.f)
	{
		_dist = 0.f;
		return true;
	}
	// Przypadek ¿e sfera jest z ty³u za promieniem
	return false;
}

// funkcja z TFQE
// kod do backface cullingu zakomentowany, lepiej daæ to jako osonn¹ funkcjê
// zwraca ujemn¹ wartoœæ jeœli przecina promieñ od ty³u
bool RayToTriangle(const Vec3& _ray_pos, const Vec3& _ray_dir, const Vec3& _v1, const Vec3& _v2, const Vec3& _v3, float& _dist)
{
	Vec3 tvec, pvec, qvec;

	// find vectors for two edges sharing vert0
	Vec3 edge1 = _v2 - _v1;
	Vec3 edge2 = _v3 - _v1;

	// begin calculating determinant - also used to calculate U parameter
	pvec = _ray_dir.Cross(edge2);

	// if determinant is near zero, ray lies in plane of triangle
	float det = edge1.Dot(pvec);
	//if (BackfaceCulling && det < 0.0f)
	//	return false;
	if(FLOAT_ALMOST_ZERO(det))
		return false;
	float inv_det = 1.0f / det;

	// calculate distance from vert0 to ray origin
	tvec = _ray_pos - _v1;

	// calculate U parameter and test bounds
	float u = tvec.Dot(pvec) * inv_det;
	if(u < 0.0f || u > 1.0f)
		return false;

	// prepare to test V parameter
	qvec = tvec.Cross(edge1);

	// calculate V parameter and test bounds
	float v = _ray_dir.Dot(qvec) * inv_det;
	if(v < 0.0f || u + v > 1.0f)
		return false;

	// calculate t, ray intersects triangle
	_dist = edge2.Dot(qvec) * inv_det;
	return true;
}

bool LineToLine(const Vec2& start1, const Vec2& end1, const Vec2& start2, const Vec2& end2, float* t)
{
	float ua_t = (end2.x - start2.x)*(start1.y - start2.y) - (end2.y - start2.y)*(start1.x - start2.x);
	float ub_t = (end1.x - start1.x)*(start1.y - start2.y) - (end1.y - start1.y)*(start1.x - start2.x);
	float u_b = (end2.y - start2.y)*(end1.x - start1.x) - (end2.x - start2.x)*(end1.y - start1.y);

	if(u_b != 0)
	{
		float ua = ua_t / u_b;
		float ub = ub_t / u_b;
		if(0 <= ua&&ua <= 1 && 0 <= ub&&ub <= 1)
		{
			// przeciêcie
			if(t)
				*t = ua;
			return true;
		}
		else
		{
			// brak przeciêcia
			return false;
		}
	}
	else
	{
		if(ua_t == 0 || ub_t == 0)
		{
			// zbierzne
			if(t)
				*t = 0;
			return true;
		}
		else
		{
			// równoleg³e
			return false;
		}
	}
}

bool LineToRectangle(const Vec2& start, const Vec2& end, const Vec2& rect_pos, const Vec2& rect_pos2, float* _t)
{
	assert(rect_pos.x <= rect_pos2.x && rect_pos.y <= rect_pos2.y);

	const Vec2 topRight(rect_pos2.x, rect_pos.y),
		bottomLeft(rect_pos.x, rect_pos2.y);

	if(_t)
	{
		float tt, t = 1.001f;

		if(LineToLine(start, end, rect_pos, topRight, &tt) && tt < t) t = tt;
		if(LineToLine(start, end, topRight, rect_pos2, &tt) && tt < t) t = tt;
		if(LineToLine(start, end, rect_pos2, bottomLeft, &tt) && tt < t) t = tt;
		if(LineToLine(start, end, bottomLeft, rect_pos, &tt) && tt < t) t = tt;

		*_t = t;

		return (t <= 1.f);
	}
	else
	{
		if(LineToLine(rect_pos, topRight, start, end))    return true;
		if(LineToLine(topRight, rect_pos2, start, end))   return true;
		if(LineToLine(rect_pos2, bottomLeft, start, end)) return true;
		if(LineToLine(bottomLeft, rect_pos, start, end))  return true;

		return false;
	}
}

void CreateAABBOX(Box& _out, const Matrix& _mat)
{
	Vec3 v1 = Vec3::Transform(Vec3(-2, -2, -2), _mat),
		v2 = Vec3::Transform(Vec3(2, 2, 2), _mat);
	_out.v1 = v1;
	_out.v2 = v2;
}

bool BoxToBox(const Box& box1, const Box& box2)
{
	return
		(box1.v1.x <= box2.v2.x) && (box1.v2.x >= box2.v1.x) &&
		(box1.v1.y <= box2.v2.y) && (box1.v2.y >= box2.v1.y) &&
		(box1.v1.z <= box2.v2.z) && (box1.v2.z >= box2.v1.z);
}

bool RectangleToRectangle(float x1, float y1, float x2, float y2, float a1, float b1, float a2, float b2)
{
	return (x1 <= a2) && (x2 >= a1) && (y1 <= b2) && (y2 >= b1);
}

// podpierdolone z CommonLib Regedita
void ClosestPointInBox(Vec3 *Out, const Box &Box, const Vec3 &p)
{
	Out->x = Clamp(p.x, Box.v1.x, Box.v2.x);
	Out->y = Clamp(p.y, Box.v1.y, Box.v2.y);
	Out->z = Clamp(p.z, Box.v1.z, Box.v2.z);
}

bool SphereToBox(const Vec3 &SphereCenter, float SphereRadius, const Box &Box)
{
	Vec3 PointInBox;
	ClosestPointInBox(&PointInBox, Box, SphereCenter);
	return Vec3::DistanceSquared(SphereCenter, PointInBox) < SphereRadius*SphereRadius;
}

// http://www.migapro.com/circle-and-rotated-rectangle-collision-detection/
bool CircleToRotatedRectangle(float cx, float cy, float radius, float rx, float ry, float w, float h, float rot)
{
	// Rotate circle's center point back
	/*float unrotatedCircleX = cos(rot) * (cx - rx) - sin(rot) * (cy - ry) + rx;
	float unrotatedCircleY = sin(rot) * (cx - rx) + cos(rot) * (cy - ry) + ry;

	// Closest point in the rectangle to the center of circle rotated backwards(unrotated)
	float closestX, closestY;

	// Find the unrotated closest x point from center of unrotated circle
	if (unrotatedCircleX  < rx)
		closestX = rx;
	else if (unrotatedCircleX  > rx + w)
		closestX = rx + w;
	else
		closestX = unrotatedCircleX;

	// Find the unrotated closest y point from center of unrotated circle
	if (unrotatedCircleY < ry)
		closestY = ry;
	else if (unrotatedCircleY > ry + h)
		closestY = ry + h;
	else
		closestY = unrotatedCircleY;

	// Determine collision
	return (distance_sqrt(unrotatedCircleX , unrotatedCircleY, closestX, closestY) < radius*radius);*/

	// doprowadzi³em ten algorytm do u¿ywalnoœci
	const float //rot = _rot,
		sina = sin(rot),
		cosa = cos(rot),
		difx = cx - rx,
		dify = cy - ry,
		x = cosa * difx - sina * dify + rx,
		y = sina * difx + cosa * dify + ry;

	// ??? wczeœniej dzia³a³o jak zanegowa³em rot, teraz bez tego ???
	// mo¿e coœ jest jeszcze Ÿle

	return CircleToRectangle(x, y, radius, rx, ry, w, h);
}

inline void RotateVector2DClockwise(Vec2& v, float ang)
{
	float t,
		cosa = cos(ang),
		sina = sin(ang);
	t = v.x;
	v.x = t*cosa + v.y*sina;
	v.y = -t*sina + v.y*cosa;
}

// Rotated Rectangles Collision Detection, Oren Becker, 2001
// http://ragestorm.net/samples/CDRR.C
bool RotatedRectanglesCollision(const RotRect& r1, const RotRect& r2)
{
	Vec2 A, B,   // vertices of the rotated rr2
		C,      // center of rr2
		BL, TR; // vertices of rr2 (bottom-left, top-right)

	float ang = r1.rot - r2.rot, // orientation of rotated rr1
		cosa = cos(ang),           // precalculated trigonometic -
		sina = sin(ang);           // - values for repeated use

	float t, x, a;      // temporary variables for various uses
	float dx;           // deltaX for linear equations
	float ext1, ext2;   // min/max vertical values

	// move r2 to make r1 cannonic
	C = r2.center - r1.center;

	// rotate r2 clockwise by r2.ang to make r2 axis-aligned
	RotateVector2DClockwise(C, r2.rot);

	// calculate vertices of (moved and axis-aligned := 'ma') r2
	BL = TR = C;
	BL -= r2.size;
	TR += r2.size;

	// calculate vertices of (rotated := 'r') r1
	A.x = -r1.size.y*sina; B.x = A.x; t = r1.size.x*cosa; A.x += t; B.x -= t;
	A.y = r1.size.y*cosa; B.y = A.y; t = r1.size.x*sina; A.y += t; B.y -= t;

	t = sina*cosa;

	// verify that A is vertical min/max, B is horizontal min/max
	if(t < 0)
	{
		t = A.x; A.x = B.x; B.x = t;
		t = A.y; A.y = B.y; B.y = t;
	}

	// verify that B is horizontal minimum (leftest-vertex)
	if(sina < 0) { B.x = -B.x; B.y = -B.y; }

	// if r2(ma) isn't in the horizontal range of
	// colliding with r1(r), collision is impossible
	if(B.x > TR.x || B.x > -BL.x) return false;

	// if r1(r) is axis-aligned, vertical min/max are easy to get
	if(t == 0) { ext1 = A.y; ext2 = -ext1; }
	// else, find vertical min/max in the range [BL.x, TR.x]
	else
	{
		x = BL.x - A.x; a = TR.x - A.x;
		ext1 = A.y;
		// if the first vertical min/max isn't in (BL.x, TR.x), then
		// find the vertical min/max on BL.x or on TR.x
		if(a*x > 0)
		{
			dx = A.x;
			if(x < 0) { dx -= B.x; ext1 -= B.y; x = a; }
			else { dx += B.x; ext1 += B.y; }
			ext1 *= x; ext1 /= dx; ext1 += A.y;
		}

		x = BL.x + A.x; a = TR.x + A.x;
		ext2 = -A.y;
		// if the second vertical min/max isn't in (BL.x, TR.x), then
		// find the local vertical min/max on BL.x or on TR.x
		if(a*x > 0)
		{
			dx = -A.x;
			if(x < 0) { dx -= B.x; ext2 -= B.y; x = a; }
			else { dx += B.x; ext2 += B.y; }
			ext2 *= x; ext2 /= dx; ext2 -= A.y;
		}
	}

	// check whether r2(ma) is in the vertical range of colliding with r1(r)
	// (for the horizontal range of r2)
	return !((ext1 < BL.y && ext2 < BL.y) ||
		(ext1 > TR.y && ext2 > TR.y));
}

// kolizja promienia (A->B) z cylindrem (P->Q, promieñ R)
// z Real Time Collision Detection str 197
//----------------------------------------------
// Intersect segment S(t)=sa+t(sb-sa), 0<=t<=1 against cylinder specifiedby p, q and r
int RayToCylinder(const Vec3& sa, const Vec3& sb, const Vec3& p, const Vec3& q, float r, float& t)
{
	Vec3 d = q - p, m = sa - p, n = sb - sa;
	float md = m.Dot(d);
	float nd = n.Dot(d);
	float dd = d.Dot(d);
	// Test if segment fully outside either endcap of cylinder
	if(md < 0.0f && md + nd < 0.0f)
		return 0; // Segment outside 'p' side of cylinder
	if(md > dd && md + nd > dd)
		return 0; // Segment outside 'q' side of cylinder
	float nn = n.Dot(n);
	float mn = m.Dot(n);
	float a = dd * nn - nd * nd;
	float k = m.Dot(m) - r * r;
	float c = dd * k - md * md;
	if(IsZero(a))
	{
		// Segment runs parallel to cylinder axis
		if(c > 0.0f) return 0; // 'a' and thus the segment lie outside cylinder
		// Now known that segment intersects cylinder; figure out how it intersects
		if(md < 0.0f) t = -mn / nn; // Intersect segment against 'p' endcap
		else if(md > dd) t = (nd - mn) / nn; // Intersect segment against 'q' endcap
		else t = 0.0f; // 'a' lies inside cylinder
		return 1;
	}
	float b = dd * mn - nd * md;
	float discr = b * b - a * c;
	if(discr < 0.0f) return 0; // No real roots; no intersection
	t = (-b - sqrt(discr)) / a;
	if(t < 0.0f || t > 1.0f) return 0; // Intersection lies outside segment
	if(md + t * nd < 0.0f)
	{
		// Intersection outside cylinder on 'p' side
		if(nd <= 0.0f) return 0; // Segment pointing away from endcap
		t = -md / nd;
		// Keep intersection if Dot(S(t) - p, S(t) - p) <= r /\ 2
		return k + 2 * t * (mn + t * nn) <= 0.0f;
	}
	else if(md + t * nd > dd)
	{
		// Intersection outside cylinder on 'q' side
		if(nd >= 0.0f) return 0; // Segment pointing away from endcap
		t = (dd - md) / nd;
		// Keep intersection if Dot(S(t) - q, S(t) - q) <= r /\ 2
		return k + dd - 2 * md + t * (2 * (mn - nd) + t * nn) <= 0.0f;
	}
	// Segment intersects cylinder between the endcaps; t is correct
	return 1;
}

struct MATRIX33
{
	Vec3 v[3];

	Vec3& operator [] (int n)
	{
		return v[n];
	}
};

// kolizja Oob z Oob
bool OOBToOOB(const Oob& a, const Oob& b)
{
	const float EPSILON = std::numeric_limits<float>::epsilon();

	float ra, rb;
	MATRIX33 R, AbsR;
	// Compute rotation matrix expressing b in a’s coordinate frame
	for(int i = 0; i < 3; i++)
		for(int j = 0; j < 3; j++)
			R[i][j] = a.u[i].Dot(b.u[j]);
	// Compute translation vector t
	Vec3 t = b.c - a.c;
	// Bring translation into a’s coordinate frame
	t = Vec3(t.Dot(a.u[0]), t.Dot(a.u[2]), t.Dot(a.u[2]));
	// Compute common subexpressions. Add in an epsilon term to
	// counteract arithmetic errors when two edges are parallel and
	// their cross product is (near) null (see text for details)
	for(int i = 0; i < 3; i++)
		for(int j = 0; j < 3; j++)
			AbsR[i][j] = abs(R[i][j]) + EPSILON;
	// Test axes L = A0, L = A1, L = A2
	for(int i = 0; i < 3; i++) {
		ra = a.e[i];
		rb = b.e[0] * AbsR[i][0] + b.e[1] * AbsR[i][1] + b.e[2] * AbsR[i][2];
		if(abs(t[i]) > ra + rb) return false;
	}
	// Test axes L = B0, L = B1, L = B2
	for(int i = 0; i < 3; i++) {
		ra = a.e[0] * AbsR[0][i] + a.e[1] * AbsR[1][i] + a.e[2] * AbsR[2][i];
		rb = b.e[i];
		if(abs(t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i]) > ra + rb) return false;
	}
	// Test axis L = A0 x B0
	ra = a.e[1] * AbsR[2][0] + a.e[2] * AbsR[1][0];
	rb = b.e[1] * AbsR[0][2] + b.e[2] * AbsR[0][1];
	if(abs(t[2] * R[1][0] - t[1] * R[2][0]) > ra + rb) return false;
	// Test axis L = A0 x B1
	ra = a.e[1] * AbsR[2][1] + a.e[2] * AbsR[1][1];
	rb = b.e[0] * AbsR[0][2] + b.e[2] * AbsR[0][0];
	if(abs(t[2] * R[1][1] - t[1] * R[2][1]) > ra + rb) return false;
	// Test axis L = A0 x B2
	ra = a.e[1] * AbsR[2][2] + a.e[2] * AbsR[1][2];
	rb = b.e[0] * AbsR[0][1] + b.e[1] * AbsR[0][0];
	if(abs(t[2] * R[1][2] - t[1] * R[2][2]) > ra + rb) return false;
	// Test axis L = A1 x B0
	ra = a.e[0] * AbsR[2][0] + a.e[2] * AbsR[0][0];
	rb = b.e[1] * AbsR[1][2] + b.e[2] * AbsR[1][1];
	if(abs(t[0] * R[2][0] - t[2] * R[0][0]) > ra + rb) return false;
	// Test axis L = A1 x B1
	ra = a.e[0] * AbsR[2][1] + a.e[2] * AbsR[0][1];
	rb = b.e[0] * AbsR[1][2] + b.e[2] * AbsR[1][0];
	if(abs(t[0] * R[2][1] - t[2] * R[0][1]) > ra + rb) return false;
	// Test axis L = A1 x B2
	ra = a.e[0] * AbsR[2][2] + a.e[2] * AbsR[0][2];
	rb = b.e[0] * AbsR[1][1] + b.e[1] * AbsR[1][0];
	if(abs(t[0] * R[2][2] - t[2] * R[0][2]) > ra + rb) return false;
	// Test axis L = A2 x B0
	ra = a.e[0] * AbsR[1][0] + a.e[1] * AbsR[0][0];
	rb = b.e[1] * AbsR[2][2] + b.e[2] * AbsR[2][1];
	if(abs(t[1] * R[0][0] - t[0] * R[1][0]) > ra + rb) return false;
	// Test axis L = A2 x B1
	ra = a.e[0] * AbsR[1][1] + a.e[1] * AbsR[0][1];
	rb = b.e[0] * AbsR[2][2] + b.e[2] * AbsR[2][0];
	if(abs(t[1] * R[0][1] - t[0] * R[1][1]) > ra + rb) return false;
	// Test axis L = A2 x B2
	ra = a.e[0] * AbsR[1][2] + a.e[1] * AbsR[0][2];
	rb = b.e[0] * AbsR[2][1] + b.e[1] * AbsR[2][0];
	if(abs(t[1] * R[0][2] - t[0] * R[1][2]) > ra + rb) return false;
	// Since no separating axis is found, the OBBs must be intersecting
	return true;
}

float DistanceRectangleToPoint(const Vec2& pos, const Vec2& size, const Vec2& pt)
{
	float dx = max(abs(pt.x - pos.x) - size.x / 2, 0.f);
	float dy = max(abs(pt.y - pos.y) - size.y / 2, 0.f);
	return sqrt(dx * dx + dy * dy);
}

float PointLineDistance(float x0, float y0, float x1, float y1, float x2, float y2)
{
	float x = x2 - x1;
	float y = y2 - y1;
	return abs(y*x0 - x*y0 + x2*y1 - y2*x1) / sqrt(y*y + x*x);
}

float GetClosestPointOnLineSegment(const Vec2& A, const Vec2& B, const Vec2& P, Vec2& result)
{
	Vec2 AP = P - A;       //Vector from A to P
	Vec2 AB = B - A;       //Vector from A to B

	float magnitudeAB = AB.LengthSquared(); //Magnitude of AB vector (it's length squared)
	float ABAPproduct = AP.Dot(AB); //The DOT product of a_to_p and a_to_b
	float distance = ABAPproduct / magnitudeAB; //The normalized "distance" from a to your closest point

	if(distance < 0)     //Check if P projection is over vectorAB
		result = A;
	else if(distance > 1)
		result = B;
	else
		result = A + AB * distance;

	return PointLineDistance(P.x, P.y, A.x, A.y, B.x, B.y);
}

const Vec2 POISSON_DISC_2D[] = {
	Vec2(-0.6271834f, -0.3647562f),
	Vec2(-0.6959124f, -0.1932297f),
	Vec2(-0.425675f, -0.4331925f),
	Vec2(-0.8259574f, -0.3775373f),
	Vec2(-0.4134415f, -0.2794108f),
	Vec2(-0.6711653f, -0.5842927f),
	Vec2(-0.505241f, -0.5710775f),
	Vec2(-0.5399489f, -0.1941965f),
	Vec2(-0.2056243f, -0.3328375f),
	Vec2(-0.2721521f, -0.4913186f),
	Vec2(0.009952361f, -0.4938473f),
	Vec2(-0.3341284f, -0.7402002f),
	Vec2(-0.009171869f, -0.1417411f),
	Vec2(-0.05370279f, -0.3561031f),
	Vec2(-0.2042215f, -0.1395438f),
	Vec2(0.1491909f, -0.7528881f),
	Vec2(-0.09437386f, -0.6736782f),
	Vec2(0.2218135f, -0.5837499f),
	Vec2(0.1357503f, -0.2823138f),
	Vec2(0.1759486f, -0.4372835f),
	Vec2(-0.8812768f, -0.1270963f),
	Vec2(-0.5861077f, -0.7143953f),
	Vec2(-0.4840448f, -0.8610057f),
	Vec2(-0.1953385f, -0.9313949f),
	Vec2(-0.3544169f, -0.1299241f),
	Vec2(0.4259588f, -0.3359875f),
	Vec2(0.1780135f, -0.006630601f),
	Vec2(0.3781602f, -0.174012f),
	Vec2(-0.6535406f, 0.07830032f),
	Vec2(-0.4176719f, 0.006290245f),
	Vec2(-0.2157413f, 0.1043319f),
	Vec2(-0.3825159f, 0.1611559f),
	Vec2(-0.04609891f, 0.1563928f),
	Vec2(-0.2525779f, 0.3147326f),
	Vec2(0.6283897f, -0.2800752f),
	Vec2(0.5242329f, -0.4569906f),
	Vec2(0.5337259f, -0.1482658f),
	Vec2(0.4243455f, -0.6266792f),
	Vec2(-0.8479414f, 0.08037262f),
	Vec2(-0.5815527f, 0.3148638f),
	Vec2(-0.790419f, 0.2343442f),
	Vec2(-0.4226354f, 0.3095743f),
	Vec2(-0.09465869f, 0.3677911f),
	Vec2(0.3935578f, 0.04151043f),
	Vec2(0.2390065f, 0.1743644f),
	Vec2(0.02775179f, 0.01711585f),
	Vec2(-0.3588479f, 0.4862351f),
	Vec2(-0.7332007f, 0.3809305f),
	Vec2(-0.5283061f, 0.5106883f),
	Vec2(0.7347565f, -0.04643056f),
	Vec2(0.5254471f, 0.1277963f),
	Vec2(-0.1984853f, 0.6903372f),
	Vec2(-0.1512452f, 0.5094652f),
	Vec2(-0.5878937f, 0.6584677f),
	Vec2(-0.4450369f, 0.7685395f),
	Vec2(0.691914f, -0.552465f),
	Vec2(0.293443f, -0.8303219f),
	Vec2(0.5147449f, -0.8018763f),
	Vec2(0.3373911f, -0.4752345f),
	Vec2(-0.7731022f, 0.6132235f),
	Vec2(-0.9054359f, 0.3877104f),
	Vec2(0.1200563f, -0.9095488f),
	Vec2(-0.05998399f, -0.8304204f),
	Vec2(0.1212275f, 0.4447584f),
	Vec2(-0.04844639f, 0.8149281f),
	Vec2(-0.1576151f, 0.9731216f),
	Vec2(-0.2921374f, 0.8280436f),
	Vec2(0.8305115f, -0.3373946f),
	Vec2(0.7025464f, -0.7087887f),
	Vec2(-0.9783711f, 0.1895637f),
	Vec2(-0.9950094f, 0.03602472f),
	Vec2(-0.02693105f, 0.6184058f),
	Vec2(-0.3686568f, 0.6363685f),
	Vec2(0.07644552f, 0.9160427f),
	Vec2(0.2174875f, 0.6892526f),
	Vec2(0.09518065f, 0.2284235f),
	Vec2(0.2566459f, 0.8855528f),
	Vec2(0.2196656f, -0.1571368f),
	Vec2(0.9549446f, -0.2014009f),
	Vec2(0.4562157f, 0.7741205f),
	Vec2(0.3333389f, 0.413012f),
	Vec2(0.5414181f, 0.2789065f),
	Vec2(0.7839744f, 0.2456573f),
	Vec2(0.6805856f, 0.1255756f),
	Vec2(0.3859844f, 0.2440029f),
	Vec2(0.4403853f, 0.600696f),
	Vec2(0.6249176f, 0.6072751f),
	Vec2(0.5145468f, 0.4502719f),
	Vec2(0.749785f, 0.4564187f),
	Vec2(0.9864355f, -0.0429658f),
	Vec2(0.8654963f, 0.04940263f),
	Vec2(0.9577024f, 0.1808657f)
};
const int poisson_disc_count = countof(POISSON_DISC_2D);

Vec2 Vec2::RandomPoissonDiscPoint()
{
	int index = Rand() % poisson_disc_count;
	const Vec2& pos = POISSON_DISC_2D[index];
	return pos;
}
