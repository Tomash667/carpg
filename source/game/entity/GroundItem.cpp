// przedmiot na ziemi
#include "Pch.h"
#include "Base.h"
#include "Game.h"

//=================================================================================================
void GroundItem::Save(HANDLE file)
{
	WriteFile(file, &pos, sizeof(pos), &tmp, NULL);
	WriteFile(file, &rot, sizeof(rot), &tmp, NULL);
	WriteFile(file, &count, sizeof(count), &tmp, NULL);
	WriteFile(file, &team_count, sizeof(team_count), &tmp, NULL);
	WriteString1(file, item->id);
	if(item->id[0] == '$')
		WriteFile(file, &item->refid, sizeof(item->refid), &tmp, NULL);
	WriteFile(file, &netid, sizeof(netid), &tmp, NULL);
}

//=================================================================================================
void GroundItem::Load(HANDLE file)
{
	ReadFile(file, &pos, sizeof(pos), &tmp, NULL);
	ReadFile(file, &rot, sizeof(rot), &tmp, NULL);
	ReadFile(file, &count, sizeof(count), &tmp, NULL);
	ReadFile(file, &team_count, sizeof(team_count), &tmp, NULL);
	byte len;
	ReadFile(file, &len, sizeof(len), &tmp, NULL);
	BUF[len] = 0;
	ReadFile(file, BUF, len, &tmp, NULL);
	if(BUF[0] != '$')
	{
		if(strcmp(BUF, "gold") == 0)
			item = &Game::Get().gold_item;
		else
			item = FindItem(BUF);
	}
	else
	{
		int quest_refid;
		ReadFile(file, &quest_refid, sizeof(quest_refid), &tmp, NULL);
		Game::Get().AddQuestItemRequest(&item, BUF, quest_refid, NULL);
		item = QUEST_ITEM_PLACEHOLDER;
	}
	ReadFile(file, &netid, sizeof(netid), &tmp, NULL);
}
