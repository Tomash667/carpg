#include "Pch.h"
#include "GameCore.h"
#include "Quest_Main.h"

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
void Quest_Main::Save(HANDLE file)
{
}

//=================================================================================================
bool Quest_Main::Load(HANDLE file)
{
	Quest::Load(file);

	FileReader f(file);

	if(prog == Progress::TalkedWithMayor)
	{
		f >> close_loc;
		f >> target_loc;
	}
	else
		f >> timer;

	return false;
}
