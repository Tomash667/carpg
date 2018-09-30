#include "Pch.h"
#include "GameCore.h"
#include "Quest_Main.h"
#include "GameFile.h"

// removed in V_0_5

//=================================================================================================
void Quest_Main::Start()
{
}

//=================================================================================================
GameDialog* Quest_Main::GetDialog(int type2)
{
	return nullptr;
}

//=================================================================================================
void Quest_Main::SetProgress(int prog2)
{
}

//=================================================================================================
cstring Quest_Main::FormatString(const string& str)
{
	return nullptr;
}

//=================================================================================================
void Quest_Main::Save(GameWriter& f)
{
}

//=================================================================================================
bool Quest_Main::Load(GameReader& f)
{
	Quest::Load(f);

	if(prog == Progress::TalkedWithMayor)
	{
		f >> close_loc;
		f >> target_loc;
	}
	else
		f >> timer;

	return false;
}
