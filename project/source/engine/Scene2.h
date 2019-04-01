#pragma once

#include "SceneNode2.h"

struct DrawBatch2
{
	vector<SceneNode2*> nodes;
};

class Scene2
{
public:
	Scene2() : clear_color(Color::None) {}
	virtual ~Scene2() {}
	virtual void ListNodes(DrawBatch2& batch) = 0;
	bool Draw(RenderTarget* target);
	virtual void Update(float dt) = 0;
	virtual void AddNode(SceneNode2* node) = 0;
	virtual void RemoveNode(SceneNode2* node) = 0;
	Camera2* GetCamera() { return camera; }
	Color GetClearColor() const { return clear_color; }
	void SetCamera(Camera2* camera) { this->camera = camera; }
	void SetClearColor(Color color) { clear_color = color; }

protected:
	Camera2* camera;
	Color clear_color;
};
