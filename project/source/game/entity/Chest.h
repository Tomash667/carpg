// skrzynia
#pragma once

//-----------------------------------------------------------------------------
#include "ItemContainer.h"
#include "MeshInstance.h"
#include "Item.h"

//-----------------------------------------------------------------------------
struct ChestEventHandler
{
	enum Event
	{
		Opened
	};

	virtual void HandleChestEvent(Event event) = 0;
	virtual int GetChestEventHandlerQuestRefid() = 0;
};

//-----------------------------------------------------------------------------
struct Chest : public ItemContainer
{
	Vec3 pos;
	float rot;
	MeshInstance* mesh_inst;
	ChestEventHandler* handler;
	int netid;
	bool looted; // czy skrzynia jest ograbiana - nie trzeba zapisywaæ

	static const int MIN_SIZE = 20;

	Chest() : mesh_inst(nullptr)
	{
	}
	~Chest()
	{
		delete mesh_inst;
	}

	void Save(HANDLE file, bool local);
	void Load(HANDLE file, bool local);
};
