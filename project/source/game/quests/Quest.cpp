#include "Pch.h"
#include "GameCore.h"
#include "Quest.h"
#include "Game.h"
#include "SaveState.h"
#include "Journal.h"
#include "QuestManager.h"
#include "GameFile.h"
#include "World.h"

Game* Quest::game;

//=================================================================================================
Quest::Quest() : quest_manager(QM), state(Hidden), prog(0), timeout(false)
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
	if(LOAD_VERSION >= V_0_4)
		f >> timeout;
	else
		timeout = false;

	return true;
}

//=================================================================================================
Location& Quest::GetStartLocation()
{
	return *W.GetLocation(start_loc);
}

//=================================================================================================
const Location& Quest::GetStartLocation() const
{
	return *W.GetLocation(start_loc);
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
	if(LOAD_VERSION >= V_0_4 || !done)
		f >> at_level;
	else
		at_level = -1;
	if(LOAD_VERSION < V_0_4 && target_loc != -1 && GetTargetLocation().outside)
		at_level = -1;

	return true;
}

//=================================================================================================
Location& Quest_Dungeon::GetTargetLocation()
{
	return *W.GetLocation(target_loc);
}

//=================================================================================================
const Location& Quest_Dungeon::GetTargetLocation() const
{
	return *W.GetLocation(target_loc);
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
	W.RemoveEncounter(enc);
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
