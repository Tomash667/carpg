#pragma once

class SceneNode2 : public ObjectPoolProxy<SceneNode2>
{
public:
	static const int NONE = -1;
	static const int USE_PARENT_SKELETON = -2;

	void AddChild(SceneNode2* node, int parent_point = NONE);
	void AddChild(SceneNode2* node, cstring name);
	void OnGet();
	void OnFree();
	bool IsVisible() const { return visible; }
	bool HaveChilds() const { return !childs.empty(); }
	MeshInstance* GetMeshInstance() { return mesh_inst; }
	const Vec3& GetPos() const { return pos; }
	float GetSphereRadius() const { return sphere_radius; }
	int GetId() const { return id; }
	const Vec4& GetTint() const { return tint; }
	SceneNode2* GetNode(int id);
	const TexId* GetTextureOverride() const { return tex_override; }
	int GetParentPoint() const { return parent_point; }
	vector<SceneNode2*>& GetChilds() { return childs; }
	void SetMesh(Mesh* mesh);
	void SetMeshInstance(MeshInstance* mesh_inst);
	void SetMeshInstance(Mesh* mesh);
	void SetVisible(bool visible) { this->visible = visible; }
	void SetId(int id) { this->id = id; }
	void SetTint(const Vec4& tint) { this->tint = tint; }
	void SetTextureOverride(const TexId* tex_override) { this->tex_override = tex_override; }
	void SetParentPoint(int parent_point);
	void SetParentPoint(cstring name);

private:
	bool VerifyParentPoint(int parent_point) const;

	int id;
	SceneNode2* parent;
	int parent_point;
	vector<SceneNode2*> childs;
	Mesh* mesh;
	MeshInstance* mesh_inst;
	Vec3 pos;
	float sphere_radius;
	const TexId* tex_override;
	Vec4 tint;
	bool visible;
};
