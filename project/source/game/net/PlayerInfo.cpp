#include "Pch.h"
#include "GameCore.h"
#include "PlayerInfo.h"
#include "SaveState.h"
#include "NetChangePlayer.h"

//=================================================================================================
PlayerInfo::PlayerInfo() : pc(nullptr), u(nullptr), clas(Class::INVALID), left(LEFT_NO), update_flags(0), ready(false), loaded(false), warping(false)
{
}

//=================================================================================================
void PlayerInfo::Save(FileWriter& f)
{
	f << name;
	f << clas;
	f << id;
	f << devmode;
	hd.Save(f.GetHandle());
	f << (u ? u->refid : -1);
	f.WriteStringArray<int, word>(notes);
}

//=================================================================================================
void PlayerInfo::Load(FileReader& f)
{
	f >> name;
	f >> clas;
	if(LOAD_VERSION < V_0_4)
		clas = ClassInfo::OldToNew(clas);
	f >> id;
	f >> devmode;
	int old_left;
	if(LOAD_VERSION < V_0_5_1)
	{
		bool left;
		f >> left;
		old_left = (left ? 1 : 0);
	}
	else
		old_left = -1;
	hd.Load(f.GetHandle());
	int refid;
	f >> refid;
	u = Unit::GetByRefid(refid);
	f.ReadStringArray<int, word>(notes);
	if(LOAD_VERSION < V_0_5_1)
		f >> left;
	if(old_left == 0 || old_left == -1)
		left = LEFT_NO;
	loaded = false;
}
