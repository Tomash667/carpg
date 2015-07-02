#include "Pch.h"
#include "Base.h"
#include "MainQuest.h"
#include "Dialog.h"
#include "DialogDefine.h"
#include "Game.h"

//-----------------------------------------------------------------------------
DialogEntry dialog_main[] = {
	TALK2(1307),
	TALK(1308),
	TALK2(1309),
	TALK(1310),
	SET_QUEST_PROGRESS(1),
	TALK(1311),
	END
};

//=================================================================================================
void MainQuest::Start()
{
	start_loc = game->current_location;
	quest_id = Q_MAIN;
	type = Type::Unique;
}

//=================================================================================================
DialogEntry* MainQuest::GetDialog(int type2)
{
	return dialog_main;
}

//=================================================================================================
void MainQuest::SetProgress(int prog2)
{
	if(prog == prog2)
		return;

	prog = prog2;

	if(prog == TalkedWithMayor)
	{
	}
}

//=================================================================================================
cstring MainQuest::FormatString(const string& str)
{
	return NULL;
}

//=================================================================================================
bool MainQuest::IfNeedTalk(cstring topic)
{
	return strcmp(topic, "main") == 0;
}
