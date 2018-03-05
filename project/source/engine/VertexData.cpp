#include "Pch.h"
#include "EngineCore.h"
#include "VertexData.h"

bool VertexData::RayToMesh(const Vec3& ray_pos, const Vec3& ray_dir, const Vec3& obj_pos, float obj_rot, float& out_dist) const
{
	// najpierw sprawdŸ kolizje promienia ze sfer¹ otaczaj¹c¹ model
	if(!RayToSphere(ray_pos, ray_dir, obj_pos, radius, out_dist))
		return false;

	// przekszta³æ promieñ o pozycjê i obrót modelu
	Matrix m = (Matrix::RotationY(obj_rot) * Matrix::Translation(obj_pos)).Inverse();
	Vec3 ray_pos_t = Vec3::Transform(ray_pos, m),
		ray_dir_t = Vec3::TransformNormal(ray_dir, m);

	// szukaj kolizji
	out_dist = 1.01f;
	float dist;
	bool hit = false;

	for(vector<Face>::const_iterator it = faces.begin(), end = faces.end(); it != end; ++it)
	{
		if(RayToTriangle(ray_pos_t, ray_dir_t, verts[it->idx[0]], verts[it->idx[1]], verts[it->idx[2]], dist) && dist < out_dist && dist >= 0.f)
		{
			hit = true;
			out_dist = dist;
		}
	}

	return hit;
}
