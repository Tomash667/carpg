#pragma once

class SceneNode2 : public ObjectPoolProxy<SceneNode2>
{
public:
	MeshInstance* GetMeshInstance() { return mesh_inst; }
	const Vec3& GetPos() const { return pos; }
	float GetSphereRadius() const { return sphere_radius; }
	void SetMesh(Mesh* mesh);
	void SetMeshInstance(MeshInstance* mesh_inst);
	void SetMeshInstance(Mesh* mesh);

private:
	Mesh* mesh;
	MeshInstance* mesh_inst;
	Vec3 pos;
	float sphere_radius;
	vector<SceneNode2*> childs;
};
