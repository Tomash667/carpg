#include "Pch.h"
#include "Base.h"
#include "PlayerInfo.h"
#include "SaveState.h"

//=================================================================================================
void PlayerInfo::Save(HANDLE file)
{
	WriteString1(file, name);
	WriteFile(file, &clas, sizeof(clas), &tmp, NULL);
	WriteFile(file, &id, sizeof(id), &tmp, NULL);
	WriteFile(file, &cheats, sizeof(cheats), &tmp, NULL);
	WriteFile(file, &left, sizeof(left), &tmp, NULL);
	hd.Save(file);
	int refid = (u ? u->refid : -1);
	WriteFile(file, &refid, sizeof(refid), &tmp, NULL);
	WriteStringArray<int, word>(file, notes);
	WriteFile(file, &left_reason, sizeof(left_reason), &tmp, NULL);
}

//=================================================================================================
void PlayerInfo::Load(HANDLE file)
{
	ReadString1(file, name);
	ReadFile(file, &clas, sizeof(clas), &tmp, NULL);
	if(LOAD_VERSION < V_DEVEL)
		clas = ClassInfo::OldToNew(clas);
	ReadFile(file, &id, sizeof(id), &tmp, NULL);
	ReadFile(file, &cheats, sizeof(cheats), &tmp, NULL);
	ReadFile(file, &left, sizeof(left), &tmp, NULL);
	hd.Load(file);
	int refid;
	ReadFile(file, &refid, sizeof(refid), &tmp, NULL);
	u = Unit::GetByRefid(refid);
	ReadStringArray<int, word>(file, notes);
	ReadFile(file, &left_reason, sizeof(left_reason), &tmp, NULL);
	loaded = false;
}
