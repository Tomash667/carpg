#pragma once

class GameSystemType;

class GameSystemManager
{
public:
	GameSystemManager();
	~GameSystemManager();

	void AddType(GameSystemType* type);
	void LoadListOfFiles();
	void LoadSystemFromText();
	void LoadLanguageFromText();

private:
	vector<GameSystemType*> types;
};
