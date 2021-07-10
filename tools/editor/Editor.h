#pragma once

#include "App.h"

enum Action
{
	A_NONE,
	A_ADD_ROOM
};

class Editor : public App
{
	friend class EditorUi;
public:
	Editor();
	~Editor();
	bool OnInit() override;
	void OnCleanup() override;
	void OnDraw() override;
	void OnUpdate(float dt) override;

private:
	void NewLevel();
	void OpenLevel();
	bool DoLoadLevel();
	void SaveLevel();
	void SaveLevelAs();
	void DoSaveLevel();
	void DrawRect(const Box& box, Color color);

	EditorUi* ui;
	Level* level;
	Scene* scene;
	EditorCamera* camera;
	BasicShader* shader;
	CustomCollisionWorld* world;
	btCollisionObject* cobjGrid;
	btCollisionShape* shapeGrid;
	Vec3 marker;
	bool markerValid;
	Action action;
	Vec3 actionPos;
	Box roomBox;
};
