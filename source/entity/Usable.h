#pragma once

//-----------------------------------------------------------------------------
#include "BaseUsable.h"
#include "Event.h"
#include "ItemContainer.h"

//-----------------------------------------------------------------------------
struct Usable : EntityType<Usable>
{
	BaseUsable* base;
	Vec3 pos;
	float rot;
	Entity<Unit> user;
	ItemContainer* container;
	int variant;
	vector<Event> events;

	static const float SOUND_DIST;
	static const int MIN_SIZE = 29;

	Usable() : variant(-1), container(nullptr) {}
	~Usable() { delete container; }

	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Write(BitStreamWriter& f) const;
	bool Read(BitStreamReader& f);
	Mesh* GetMesh() const;
	Vec3 GetCenter() const;
	void AddEventHandler(Quest2* quest, EventType type);
	void RemoveEventHandler(Quest2* quest, EventType type, bool cleanup = false);
	void RemoveAllEventHandlers();
	void FireEvent(ScriptEvent& e);
	void Destroy();
};

//-----------------------------------------------------------------------------
inline void operator << (GameWriter& f, Usable* usable)
{
	f << usable->id;
}
inline void operator >> (GameReader& f, Usable*& usable)
{
	int id = f.Read<int>();
	usable = Usable::GetById(id);
}
