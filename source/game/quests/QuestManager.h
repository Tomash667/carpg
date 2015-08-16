#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
class QuestManager
{
public:
	Quest* CreateQuest(QUEST quest_id);
	Quest* GetMayorQuest(int force = -1);
	Quest* GetCaptainQuest(int force = -1);
	Quest* GetAdventurerQuest(int force = -1);
};
