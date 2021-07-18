#pragma once

#include "App.h"

enum Action
{
	A_NONE,
	A_ADD_ROOM,
	A_MOVE,
	A_RESIZE
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
	void DrawBox(const Box& box, Color color);

	EditorUi* ui;
	MeshBuilder* builder;
	Level* level;
	Scene* scene;
	EditorCamera* camera;
	BasicShader* shader;
	CustomCollisionWorld* world;
	btCollisionObject* cobjGrid;
	btCollisionShape* shapeGrid;
	Vec3 marker;
	bool markerValid, drawLinks;
	Action action;
	Vec3 actionPos;
	Box roomBox;
	Room* roomHover, *roomSelect;
	Dir hoverDir;
	vector<Vec3> resizeLock;
};
