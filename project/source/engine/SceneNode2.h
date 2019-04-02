#pragma once

class SceneNode2 : public ObjectPoolProxy<SceneNode2>
{
public:
	static const int BONE_NONE = -1;
	static const int BONE_USE_PARENT_SKELETON = -2;

	void AddChild(SceneNode2* node, int parent_bone = BONE_NONE);
	MeshInstance* GetMeshInstance() { return mesh_inst; }
	const Vec3& GetPos() const { return pos; }
	float GetSphereRadius() const { return sphere_radius; }
	void SetMesh(Mesh* mesh);
	void SetMeshInstance(MeshInstance* mesh_inst);
	void SetMeshInstance(Mesh* mesh);

private:
	int id;
	SceneNode2* parent;
	vector<SceneNode2*> childs;
	Mesh* mesh;
	MeshInstance* mesh_inst;
	Vec3 pos;
	float sphere_radius;
	const TexId* tex_override;
	Color tint;
};
