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
	for(Event& event : events)
	{
		f << event.type;
		f << event.quest->id;
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
		for(Event& event : events)
		{
			int questId;
			f >> event.type;
			f >> questId;
			questMgr->AddQuestRequest(questId, (Quest**)&event.quest, [&]()
			{
				EventPtr e;
				e.source = EventPtr::USABLE;
				e.type = event.type;
				e.usable = this;
				event.quest->AddEventPtr(e);
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

	Event event;
	event.quest = quest;
	event.type = type;
	events.push_back(event);

	EventPtr e;
	e.source = EventPtr::USABLE;
	e.type = type;
	e.usable = this;
	quest->AddEventPtr(e);
}

//=================================================================================================
void Usable::RemoveEventHandler(Quest2* quest, EventType type, bool cleanup)
{
	LoopAndRemove(events, [=](Event& event)
	{
		return event.quest == quest && (type == EVENT_ANY || event.type == type);
	});

	if(!cleanup)
	{
		EventPtr e;
		e.source = EventPtr::USABLE;
		e.type = type;
		e.usable = this;
		quest->RemoveEventPtr(e);
	}
}

//=================================================================================================
void Usable::RemoveAllEventHandlers()
{
	for(Event& event : events)
	{
		EventPtr e;
		e.source = EventPtr::USABLE;
		e.type = event.type;
		e.usable = this;
		event.quest->RemoveEventPtr(e);
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
