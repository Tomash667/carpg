#pragma once

//-----------------------------------------------------------------------------
#include "Chest.h"
#include "UnitEventHandler.h"

//-----------------------------------------------------------------------------
// Move thru dungeon and learn basics
class Quest_Tutorial final : public ChestEventHandler, public UnitEventHandler
{
	friend class TutorialLocationGenerator;

	struct Text
	{
		cstring text;
		Vec3 pos;
		int state; // 0 - not active, 1 - active, 2 - done
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
	void HandleEvent(int activate, int unlock);
	void HandleChestEvent(ChestEventHandler::Event event, Chest* chest) override;
	void HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit) override;
	int GetChestEventHandlerQuestId() override { return -1; } // can't save in tutorial
	int GetUnitEventHandlerQuestId() override { return -1; } // can't save in tutorial
	void HandleMeleeAttackCollision();
	void HandleBulletCollision();

private:
	int state;
	vector<Text> texts;
	Vec3 dummy;
	Chest* chests[2];
	cstring txTut[10], txTutNote, txTutLoc;

public:
	bool in_tutorial, finished_tutorial;
};
