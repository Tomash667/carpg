#include "Pch.h"
#include "GameCore.h"
#include "GroundItem.h"
#include "Item.h"
#include "QuestConsts.h"
#include "QuestManager.h"

int GroundItem::netid_counter;

//=================================================================================================
void GroundItem::Save(FileWriter& f)
{
	f << pos;
	f << rot;
	f << count;
	f << team_count;
	f << item->id;
	if(item->id[0] == '$')
		f << item->refid;
	f << netid;
}

//=================================================================================================
void GroundItem::Load(FileReader& f)
{
	f >> pos;
	f >> rot;
	f >> count;
	f >> team_count;
	const string& item_id = f.ReadString1();
	if(item_id[0] != '$')
		item = Item::Get(item_id);
	else
	{
		int quest_refid = f.Read<int>();
		QM.AddQuestItemRequest(&item, item_id.c_str(), quest_refid, nullptr);
		item = QUEST_ITEM_PLACEHOLDER;
	}
	f >> netid;
}
