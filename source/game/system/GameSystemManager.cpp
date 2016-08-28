#include "Pch.h"
#include "Base.h"
#include "GameSystemType.h"
#include "GameSystemManager.h"

GameSystemManager::GameSystemManager()
{

}

GameSystemManager::~GameSystemManager()
{
	DeleteElements(types);
}

void GameSystemManager::AddType(GameSystemType* type)
{
	assert(type);
	types.push_back(type);
}

void GameSystemManager::LoadListOfFiles()
{

}

void GameSystemManager::LoadSystemFromText()
{

}

void GameSystemManager::LoadLanguageFromText()
{

}
