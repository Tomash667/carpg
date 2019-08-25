#include "Pch.h"
#include "GameCore.h"
#include "GroundItem.h"
#include "Item.h"
#include "QuestConsts.h"
#include "QuestManager.h"
#include "BitStreamFunc.h"
#include "Game.h"
#include "SaveState.h"

EntityType<GroundItem>::Impl EntityType<GroundItem>::impl;

//=================================================================================================
void GroundItem::Save(FileWriter& f)
{
	f << id;
	f << pos;
	f << rot;
	f << count;
	f << team_count;
	f << item->id;
	if(item->id[0] == '$')
		f << item->quest_id;
}

//=================================================================================================
void GroundItem::Load(FileReader& f)
{
	if(LOAD_VERSION >= V_DEV)
		f >> id;
	Register();
	f >> pos;
	f >> rot;
	f >> count;
	f >> team_count;
	const string& item_id = f.ReadString1();
	if(item_id[0] != '$')
		item = Item::Get(item_id);
	else
	{
		int quest_id = f.Read<int>();
		quest_mgr->AddQuestItemRequest(&item, item_id.c_str(), quest_id, nullptr);
		item = QUEST_ITEM_PLACEHOLDER;
	}
	if(LOAD_VERSION < V_DEV)
		f.Skip<int>(); // old netid
}

//=================================================================================================
void GroundItem::Write(BitStreamWriter& f)
{
	f << id;
	f << pos;
	f << rot;
	f << count;
	f << team_count;
	f << item->id;
	if(item->IsQuest())
		f << item->quest_id;
}

//=================================================================================================
bool GroundItem::Read(BitStreamReader& f)
{
	f >> id;
	f >> pos;
	f >> rot;
	f >> count;
	f >> team_count;
	Register();
	return f.IsOk() && game->ReadItemAndFind(f, item) > 0;
}
