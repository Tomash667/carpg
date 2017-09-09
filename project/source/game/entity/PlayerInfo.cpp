#include "Pch.h"
#include "Core.h"
#include "PlayerInfo.h"
#include "SaveState.h"

//=================================================================================================
void PlayerInfo::Save(HANDLE file)
{
	WriteString1(file, name);
	WriteFile(file, &clas, sizeof(clas), &tmp, nullptr);
	WriteFile(file, &id, sizeof(id), &tmp, nullptr);
	WriteFile(file, &devmode, sizeof(devmode), &tmp, nullptr);
	hd.Save(file);
	int refid = (u ? u->refid : -1);
	WriteFile(file, &refid, sizeof(refid), &tmp, nullptr);
	WriteStringArray<int, word>(file, notes);
	WriteFile(file, &left, sizeof(left), &tmp, nullptr);
}

//=================================================================================================
void PlayerInfo::Load(HANDLE file)
{
	ReadString1(file, name);
	ReadFile(file, &clas, sizeof(clas), &tmp, nullptr);
	if(LOAD_VERSION < V_0_4)
		clas = ClassInfo::OldToNew(clas);
	ReadFile(file, &id, sizeof(id), &tmp, nullptr);
	ReadFile(file, &devmode, sizeof(devmode), &tmp, nullptr);
	int old_left;
	if(LOAD_VERSION < V_CURRENT)
	{
		bool left;
		ReadFile(file, &left, sizeof(left), &tmp, nullptr);
		old_left = (left ? 1 : 0);
	}
	else
		old_left = -1;
	hd.Load(file);
	int refid;
	ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
	u = Unit::GetByRefid(refid);
	ReadStringArray<int, word>(file, notes);
	ReadFile(file, &left, sizeof(left), &tmp, nullptr);
	if(old_left == 0)
		left = LEFT_NO;
	loaded = false;
}
