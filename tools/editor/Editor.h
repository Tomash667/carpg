#pragma once

#include "App.h"

class Editor : public App
{
	friend class EditorUi;
public:
	Editor();
	~Editor();
	bool OnInit() override;
	void OnDraw() override;
	void OnUpdate(float dt) override;

private:
	void NewLevel();
	void OpenLevel();
	bool DoLoadLevel();
	void SaveLevel();
	void SaveLevelAs();
	void DoSaveLevel();

	EditorUi* ui;
	Level* level;
	Scene* scene;
	EditorCamera* camera;
	BasicShader* shader;
};
