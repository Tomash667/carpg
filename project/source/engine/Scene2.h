#pragma once

class Scene2
{
public:
	Scene2() : clear_color(Color::None) {}
	virtual ~Scene2() {}
	bool Draw(RenderTarget* target);
	Camera2* GetCamera() { return camera; }
	Color GetClearColor() const { return clear_color; }
	void SetCamera(Camera2* camera);
	void SetClearColor(Color color) { clear_color = color; }

private:
	Camera2* camera;
	Color clear_color;
};
