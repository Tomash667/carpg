#include "Pch.h"
#include "GameCore.h"
#include "Quest.h"
#include "Game.h"
#include "SaveState.h"
#include "Journal.h"
#include "QuestManager.h"
#include "GameFile.h"
#include "World.h"
#include "GameGui.h"
#include "LevelGui.h"
#include "GameMessages.h"

//=================================================================================================
Quest::Quest() : state(Hidden), prog(0), timeout(false)
{
}

//=================================================================================================
void Quest::Save(GameWriter& f)
{
	f << quest_id;
	f << state;
	f << name;
	f << prog;
	f << refid;
	f << start_time;
	f << start_loc;
	f << type;
	f.WriteStringArray<byte, word>(msgs);
	f << timeout;
}

//=================================================================================================
bool Quest::Load(GameReader& f)
{
	// quest_id is read in QuestManager to create this instance
	f >> state;
	f >> name;
	f >> prog;
	f >> refid;
	f >> start_time;
	f >> start_loc;
	f >> type;
	f.ReadStringArray<byte, word>(msgs);
	f >> timeout;

	return true;
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
		c.id = refid;
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
		c.id = refid;
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
bool Quest_Dungeon::Load(GameReader& f)
{
	Quest::Load(f);

	f >> target_loc;
	f >> done;
	f >> at_level;

	return true;
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
bool Quest_Encounter::Load(GameReader& f)
{
	Quest::Load(f);

	f >> enc;

	return false;
}
