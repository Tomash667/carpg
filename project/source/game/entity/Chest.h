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
	int netid;
	Vec3 pos;
	float rot;
	MeshInstance* mesh_inst;
	ChestEventHandler* handler;
	// temporary - not saved
	bool looted;

	static const int MIN_SIZE = 20;
	static int netid_counter;

	Chest() : mesh_inst(nullptr) {}
	~Chest() { delete mesh_inst; }

	void Save(FileWriter& f, bool local);
	void Load(FileReader& f, bool local);
	Vec3 GetCenter() const
	{
		Vec3 p = pos;
		p.y += 0.5f;
		return p;
	}
};
