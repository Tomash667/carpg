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

	virtual void HandleChestEvent(Event event, Chest* chest) = 0;
	virtual int GetChestEventHandlerQuestRefid() = 0;
};

//-----------------------------------------------------------------------------
struct Chest : public EntityType<Chest>, public ItemContainer
{
	Vec3 pos;
	float rot;
	MeshInstance* mesh_inst;
	ChestEventHandler* handler;

private:
	// temporary - not saved
	Unit* user;

public:
	static const int MIN_SIZE = 20;
	static const float SOUND_DIST;

	Chest() : mesh_inst(nullptr), user(nullptr), handler(nullptr) {}
	~Chest() { delete mesh_inst; }

	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
	Vec3 GetCenter() const
	{
		Vec3 p = pos;
		p.y += 0.5f;
		return p;
	}
	bool AddItem(const Item* item, uint count, uint team_count, bool notify = true);
	bool AddItem(const Item* item, uint count = 1) { return AddItem(item, count, count); }
	void OpenClose(Unit* unit);
	Unit* GetUser() const { return user; }
};
