#include "Pch.h"
#include "Quest.h"

#include "Game.h"
#include "GameGui.h"
#include "GameFile.h"
#include "GameMessages.h"
#include "Journal.h"
#include "LevelGui.h"
#include "QuestManager.h"
#include "SaveState.h"
#include "Var.h"
#include "World.h"

#include <scriptdictionary/scriptdictionary.h>

//=================================================================================================
Quest::Quest() : state(Hidden), prog(0), timeout(false)
{
}

//=================================================================================================
void Quest::Save(GameWriter& f)
{
	f << type;
	f << state;
	f << name;
	f << prog;
	f << id;
	f << start_time;
	f << start_loc;
	f << category;
	f.WriteStringArray<byte, word>(msgs);
	f << timeout;
}

//=================================================================================================
Quest::LoadResult Quest::Load(GameReader& f)
{
	// type is read in QuestManager to create this instance
	f >> state;
	f >> name;
	f >> prog;
	f >> id;
	f >> start_time;
	f >> start_loc;
	f >> category;
	f.ReadStringArray<byte, word>(msgs);
	f >> timeout;

	return LoadResult::Ok;
}

//=================================================================================================
void Quest::OnStart(cstring name)
{
	start_time = world->GetWorldtime();
	state = Quest::Started;
	this->name = name;
	quest_index = quest_mgr->quests.size();
	quest_mgr->quests.push_back(this);
	RemoveElement<Quest*>(quest_mgr->unaccepted_quests, this);
	game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
	game_gui->messages->AddGameMsg3(GMS_JOURNAL_UPDATED);
	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::ADD_QUEST;
		c.id = id;
	}
}

//=================================================================================================
void Quest::OnUpdate(const std::initializer_list<cstring>& new_msgs)
{
	assert(new_msgs.size() > 0u);
	for(cstring msg : new_msgs)
		msgs.push_back(msg);
	game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
	game_gui->messages->AddGameMsg3(GMS_JOURNAL_UPDATED);
	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.id = id;
		c.type = NetChange::UPDATE_QUEST;
		c.count = new_msgs.size();
	}
}

//=================================================================================================
Location& Quest::GetStartLocation()
{
	return *world->GetLocation(start_loc);
}

//=================================================================================================
const Location& Quest::GetStartLocation() const
{
	return *world->GetLocation(start_loc);
}

//=================================================================================================
cstring Quest::GetStartLocationName() const
{
	return GetStartLocation().name.c_str();
}

//=================================================================================================
void Quest_Dungeon::Save(GameWriter& f)
{
	Quest::Save(f);

	f << target_loc;
	f << done;
	f << at_level;
}

//=================================================================================================
Quest::LoadResult Quest_Dungeon::Load(GameReader& f)
{
	Quest::Load(f);

	f >> target_loc;
	f >> done;
	f >> at_level;

	return LoadResult::Ok;
}

//=================================================================================================
Location& Quest_Dungeon::GetTargetLocation()
{
	return *world->GetLocation(target_loc);
}

//=================================================================================================
const Location& Quest_Dungeon::GetTargetLocation() const
{
	return *world->GetLocation(target_loc);
}

//=================================================================================================
cstring Quest_Dungeon::GetTargetLocationName() const
{
	return GetTargetLocation().name.c_str();
}

//=================================================================================================
cstring Quest_Dungeon::GetTargetLocationDir() const
{
	return GetLocationDirName(GetStartLocation().pos, GetTargetLocation().pos);
}

//=================================================================================================
Quest_Event* Quest_Dungeon::GetEvent(int current_loc)
{
	Quest_Event* event = this;

	while(event)
	{
		if(event->target_loc == current_loc || event->target_loc == -1)
			return event;
		event = event->next_event;
	}

	return nullptr;
}

//=================================================================================================
void Quest_Encounter::RemoveEncounter()
{
	if(enc == -1)
		return;
	world->RemoveEncounter(enc);
	enc = -1;
}

//=================================================================================================
void Quest_Encounter::Save(GameWriter& f)
{
	Quest::Save(f);

	f << enc;
}

//=================================================================================================
Quest::LoadResult Quest_Encounter::Load(GameReader& f)
{
	Quest::Load(f);

	f >> enc;

	return LoadResult::Ok;
}

//=================================================================================================
void Quest::ConversionData::Add(cstring key, bool value)
{
	vars->Get(key)->SetBool(value);
}

//=================================================================================================
void Quest::ConversionData::Add(cstring key, int value)
{
	vars->Get(key)->SetInt(value);
}

//=================================================================================================
void Quest::ConversionData::Add(cstring key, Location* location)
{
	vars->Get(key)->SetPtr(location, Var::Type::Location);
}

//=================================================================================================
void Quest::ConversionData::Add(cstring key, const Item* item)
{
	vars->Get(key)->SetPtr(const_cast<Item*>(item), Var::Type::Item);
}
