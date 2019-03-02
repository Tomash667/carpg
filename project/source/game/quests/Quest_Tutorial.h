#pragma once

//-----------------------------------------------------------------------------
#include "Chest.h"
#include "UnitEventHandler.h"

//-----------------------------------------------------------------------------
class Quest_Tutorial final : public ChestEventHandler, public UnitEventHandler
{
	friend class TutorialLocationGenerator;

	struct Text
	{
		cstring text;
		Vec3 pos;
		int state; // 0 - nie aktywny, 1 - aktywny, 2 - uruchomiony
		int id;
	};

public:
	enum Event
	{
		OpenedChest1,
		OpenedChest2,
		KilledGoblin,
		Exit
	};

	Quest_Tutorial() : in_tutorial(false), finished_tutorial(false) {}
	void LoadLanguage();
	void Start();
	void Update();
	void Finish(int);
	void OnEvent(Event event);
	void HandleChestEvent(ChestEventHandler::Event event, Chest* chest) override;
	void HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit) override;
	int GetChestEventHandlerQuestRefid() override { return -1; } // can't save in tutorial
	int GetUnitEventHandlerQuestRefid() override { return -1; } // can't save in tutorial
	void HandleBulletCollision(void* ptr);

private:
	int state;
	vector<Text> texts;
	Vec3 dummy;
	Object* shield, *shield2;
	Chest* chests[2];
	cstring txTut[10], txTutNote, txTutLoc;

public:
	bool in_tutorial, finished_tutorial;
};
