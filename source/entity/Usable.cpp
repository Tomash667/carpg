#include "Pch.h"
#include "Usable.h"

#include "BitStreamFunc.h"
#include "ItemContainer.h"
#include "Net.h"
#include "Object.h"
#include "Quest2.h"
#include "QuestManager.h"
#include "Unit.h"

const float Usable::SOUND_DIST = 1.5f;
EntityType<Usable>::Impl EntityType<Usable>::impl;

//=================================================================================================
void Usable::Save(GameWriter& f)
{
	f << id;
	f << base->hash;
	f << pos;
	f << rot;
	if(base->variants)
		f << variant;
	if(IsSet(base->useFlags, BaseUsable::CONTAINER))
		container->Save(f);
	// events
	f << events.size();
	for(Event& e : events)
	{
		f << e.type;
		f << e.quest->id;
	}
	// user
	if(f.isLocal && !IsSet(base->useFlags, BaseUsable::CONTAINER))
		f << user;
}

//=================================================================================================
void Usable::Load(GameReader& f)
{
	if(LOAD_VERSION >= V_0_12)
		f >> id;
	Register();
	if(LOAD_VERSION >= V_0_16)
		base = BaseUsable::Get(f.Read<int>());
	else
		base = BaseUsable::Get(f.ReadString1());
	f >> pos;
	f >> rot;
	if(LOAD_VERSION < V_0_12)
		f.Skip<int>(); // old netid
	if(base->variants)
		f >> variant;
	if(IsSet(base->useFlags, BaseUsable::CONTAINER))
	{
		container = new ItemContainer;
		container->Load(f);
	}
	// events
	if(LOAD_VERSION >= V_DEV)
	{
		events.resize(f.Read<uint>());
		for(Event& e : events)
		{
			int questId;
			f >> e.type;
			f >> questId;
			questMgr->AddQuestRequest(questId, (Quest**)&e.quest, [&]()
			{
				EventPtr event;
				event.source = EventPtr::LOCATION;
				event.type = e.type;
				event.usable = this;
				e.quest->AddEventPtr(event);
			});
		}
	}
	else
		events.clear();

	if(f.isLocal && !IsSet(base->useFlags, BaseUsable::CONTAINER))
		f >> user;
	else
		user = nullptr;
}

//=================================================================================================
void Usable::Write(BitStreamWriter& f) const
{
	f << id;
	f << base->hash;
	f << pos;
	f << rot;
	f.WriteCasted<byte>(variant);
	f << user;
}

//=================================================================================================
bool Usable::Read(BitStreamReader& f)
{
	int hash;
	f >> id;
	f >> hash;
	f >> pos;
	f >> rot;
	f.ReadCasted<byte>(variant);
	f >> user;
	if(!f)
		return false;
	base = BaseUsable::Get(hash);
	Register();
	if(IsSet(base->useFlags, BaseUsable::CONTAINER))
		container = new ItemContainer;
	return true;
}

//=================================================================================================
Mesh* Usable::GetMesh() const
{
	if(base->variants)
	{
		assert(InRange(variant, 0, (int)base->variants->meshes.size()));
		return base->variants->meshes[variant];
	}
	else
		return base->mesh;
}

//=================================================================================================
Vec3 Usable::GetCenter() const
{
	return pos + base->mesh->head.bbox.Midpoint();
}

//=================================================================================================
void Usable::AddEventHandler(Quest2* quest, EventType type)
{
	assert(type == EVENT_DESTROY);

	Event e;
	e.quest = quest;
	e.type = type;
	events.push_back(e);

	EventPtr event;
	event.source = EventPtr::USABLE;
	event.type = type;
	event.usable = this;
	quest->AddEventPtr(event);
}

//=================================================================================================
void Usable::RemoveEventHandler(Quest2* quest, EventType type, bool cleanup)
{
	LoopAndRemove(events, [=](Event& e)
	{
		return e.quest == quest && (type == EVENT_ANY || e.type == type);
	});

	if(!cleanup)
	{
		EventPtr event;
		event.source = EventPtr::USABLE;
		event.type = type;
		event.usable = this;
		quest->RemoveEventPtr(event);
	}
}

//=================================================================================================
void Usable::FireEvent(ScriptEvent& e)
{
	if(events.empty())
		return;

	for(Event& event : events)
	{
		if(event.type == e.type)
			event.quest->FireEvent(e);
	}
}
