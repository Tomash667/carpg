#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "SaveState.h"
#include "Journal.h"

Game* Quest::game;
extern DWORD tmp;

//=================================================================================================
void Quest::Save(HANDLE file)
{
	WriteFile(file, &quest_id, sizeof(quest_id), &tmp, nullptr);
	WriteFile(file, &state, sizeof(state), &tmp, nullptr);
	byte len = (byte)name.length();
	WriteFile(file, &len, sizeof(len), &tmp, nullptr);
	WriteFile(file, name.c_str(), len, &tmp, nullptr);
	WriteFile(file, &prog, sizeof(prog), &tmp, nullptr);
	WriteFile(file, &refid, sizeof(refid), &tmp, nullptr);
	WriteFile(file, &start_time, sizeof(start_time), &tmp, nullptr);
	WriteFile(file, &start_loc, sizeof(start_loc), &tmp, nullptr);
	WriteFile(file, &type, sizeof(type), &tmp, nullptr);
	len = (byte)msgs.size();
	WriteFile(file, &len, sizeof(len), &tmp, nullptr);
	for(byte i=0; i<len; ++i)
	{
		word len2 = (word)msgs[i].length();
		WriteFile(file, &len2, sizeof(len2), &tmp, nullptr);
		WriteFile(file, msgs[i].c_str(), len2, &tmp, nullptr);
	}
	WriteFile(file, &timeout, sizeof(timeout), &tmp, nullptr);
}

//=================================================================================================
void Quest::Load(HANDLE file)
{
	// quest_id jest ju¿ odczytane
	//ReadFile(file, &quest_id, sizeof(quest_id), &tmp, nullptr);
	ReadFile(file, &state, sizeof(state), &tmp, nullptr);
	byte len;
	ReadFile(file, &len, sizeof(len), &tmp, nullptr);
	name.resize(len);
	ReadFile(file, (char*)name.c_str(), len, &tmp, nullptr);
	ReadFile(file, &prog, sizeof(prog), &tmp, nullptr);
	ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
	ReadFile(file, &start_time, sizeof(start_time), &tmp, nullptr);
	ReadFile(file, &start_loc, sizeof(start_loc), &tmp, nullptr);
	ReadFile(file, &type, sizeof(type), &tmp, nullptr);
	ReadFile(file, &len, sizeof(len), &tmp, nullptr);
	msgs.resize(len);
	for(byte i=0; i<len; ++i)
	{
		word len2;
		ReadFile(file, &len2, sizeof(len2), &tmp, nullptr);
		msgs[i].resize(len2);
		ReadFile(file, (char*)msgs[i].c_str(), len2, &tmp, nullptr);
	}
	if(LOAD_VERSION >= V_0_4)
		ReadFile(file, &timeout, sizeof(timeout), &tmp, nullptr);
	else
		timeout = false;
}

//=================================================================================================
Location& Quest::GetStartLocation()
{
	return *game->locations[start_loc];
}

//=================================================================================================
const Location& Quest::GetStartLocation() const
{
	return *game->locations[start_loc];
}

//=================================================================================================
cstring Quest::GetStartLocationName() const
{
	return GetStartLocation().name.c_str();
}

//=================================================================================================
void Quest_Dungeon::Save(HANDLE file)
{
	Quest::Save(file);

	WriteFile(file, &target_loc, sizeof(target_loc), &tmp, nullptr);
	WriteFile(file, &done, sizeof(done), &tmp, nullptr);
	WriteFile(file, &at_level, sizeof(at_level), &tmp, nullptr);
}

//=================================================================================================
void Quest_Dungeon::Load(HANDLE file)
{
	Quest::Load(file);

	ReadFile(file, &target_loc, sizeof(target_loc), &tmp, nullptr);
	ReadFile(file, &done, sizeof(done), &tmp, nullptr);
	if(LOAD_VERSION >= V_0_4 || !done)
		ReadFile(file, &at_level, sizeof(at_level), &tmp, nullptr);
	else
		at_level = -1;
	if(LOAD_VERSION < V_0_4 && target_loc != -1)
	{
		Location* loc = game->locations[target_loc];
		if(loc->outside)
			at_level = -1;
	}
}

//=================================================================================================
Location& Quest_Dungeon::GetTargetLocation()
{
	return *game->locations[target_loc];
}

//=================================================================================================
const Location& Quest_Dungeon::GetTargetLocation() const
{
	return *game->locations[target_loc];
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
	game->RemoveEncounter(enc);
	enc = -1;
}

//=================================================================================================
void Quest_Encounter::Save(HANDLE file)
{
	Quest::Save(file);

	WriteFile(file, &enc, sizeof(enc), &tmp, nullptr);
}

//=================================================================================================
void Quest_Encounter::Load(HANDLE file)
{
	Quest::Load(file);

	ReadFile(file, &enc, sizeof(enc), &tmp, nullptr);
}
