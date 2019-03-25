#pragma once

class SceneNode2 : public ObjectPoolProxy<SceneNode2>
{
public:
	void SetMesh(Mesh* mesh);

private:
	Mesh* mesh;
};
