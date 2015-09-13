#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "DialogDefine.h"
#include "SaveState.h"
#include "Journal.h"

Game* Quest::game;
extern DWORD tmp;

//=================================================================================================
void Quest::Save(HANDLE file)
{
	WriteFile(file, &quest_id, sizeof(quest_id), &tmp, NULL);
	WriteFile(file, &state, sizeof(state), &tmp, NULL);
	byte len = (byte)name.length();
	WriteFile(file, &len, sizeof(len), &tmp, NULL);
	WriteFile(file, name.c_str(), len, &tmp, NULL);
	WriteFile(file, &prog, sizeof(prog), &tmp, NULL);
	WriteFile(file, &refid, sizeof(refid), &tmp, NULL);
	WriteFile(file, &start_time, sizeof(start_time), &tmp, NULL);
	WriteFile(file, &start_loc, sizeof(start_loc), &tmp, NULL);
	WriteFile(file, &type, sizeof(type), &tmp, NULL);
	len = (byte)msgs.size();
	WriteFile(file, &len, sizeof(len), &tmp, NULL);
	for(byte i=0; i<len; ++i)
	{
		word len2 = (word)msgs[i].length();
		WriteFile(file, &len2, sizeof(len2), &tmp, NULL);
		WriteFile(file, msgs[i].c_str(), len2, &tmp, NULL);
	}
	WriteFile(file, &timeout, sizeof(timeout), &tmp, NULL);
}

//=================================================================================================
void Quest::Load(HANDLE file)
{
	// quest_id jest ju¿ odczytane
	//ReadFile(file, &quest_id, sizeof(quest_id), &tmp, NULL);
	ReadFile(file, &state, sizeof(state), &tmp, NULL);
	byte len;
	ReadFile(file, &len, sizeof(len), &tmp, NULL);
	name.resize(len);
	ReadFile(file, (char*)name.c_str(), len, &tmp, NULL);
	ReadFile(file, &prog, sizeof(prog), &tmp, NULL);
	ReadFile(file, &refid, sizeof(refid), &tmp, NULL);
	ReadFile(file, &start_time, sizeof(start_time), &tmp, NULL);
	ReadFile(file, &start_loc, sizeof(start_loc), &tmp, NULL);
	ReadFile(file, &type, sizeof(type), &tmp, NULL);
	ReadFile(file, &len, sizeof(len), &tmp, NULL);
	msgs.resize(len);
	for(byte i=0; i<len; ++i)
	{
		word len2;
		ReadFile(file, &len2, sizeof(len2), &tmp, NULL);
		msgs[i].resize(len2);
		ReadFile(file, (char*)msgs[i].c_str(), len2, &tmp, NULL);
	}
	if(LOAD_VERSION == V_0_2)
	{
		bool ended;
		ReadFile(file, &ended, sizeof(ended), &tmp, NULL);
	}
	if(LOAD_VERSION >= V_DEVEL)
		ReadFile(file, &timeout, sizeof(timeout), &tmp, NULL);
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

	WriteFile(file, &target_loc, sizeof(target_loc), &tmp, NULL);
	WriteFile(file, &done, sizeof(done), &tmp, NULL);
	WriteFile(file, &at_level, sizeof(at_level), &tmp, NULL);
}

//=================================================================================================
void Quest_Dungeon::Load(HANDLE file)
{
	Quest::Load(file);

	ReadFile(file, &target_loc, sizeof(target_loc), &tmp, NULL);
	ReadFile(file, &done, sizeof(done), &tmp, NULL);
	if(LOAD_VERSION >= V_DEVEL || !done)
		ReadFile(file, &at_level, sizeof(at_level), &tmp, NULL);
	else
		at_level = 0;
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

	return NULL;
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

	WriteFile(file, &enc, sizeof(enc), &tmp, NULL);
}

//=================================================================================================
void Quest_Encounter::Load(HANDLE file)
{
	Quest::Load(file);

	ReadFile(file, &enc, sizeof(enc), &tmp, NULL);
}
