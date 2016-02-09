// przedmiot na ziemi
#include "Pch.h"
#include "Base.h"
#include "Game.h"

//=================================================================================================
void GroundItem::Save(HANDLE file)
{
	WriteFile(file, &pos, sizeof(pos), &tmp, nullptr);
	WriteFile(file, &rot, sizeof(rot), &tmp, nullptr);
	WriteFile(file, &count, sizeof(count), &tmp, nullptr);
	WriteFile(file, &team_count, sizeof(team_count), &tmp, nullptr);
	WriteString1(file, item->id);
	if(item->id[0] == '$')
		WriteFile(file, &item->refid, sizeof(item->refid), &tmp, nullptr);
	WriteFile(file, &netid, sizeof(netid), &tmp, nullptr);
}

//=================================================================================================
void GroundItem::Load(HANDLE file)
{
	ReadFile(file, &pos, sizeof(pos), &tmp, nullptr);
	ReadFile(file, &rot, sizeof(rot), &tmp, nullptr);
	ReadFile(file, &count, sizeof(count), &tmp, nullptr);
	ReadFile(file, &team_count, sizeof(team_count), &tmp, nullptr);
	byte len;
	ReadFile(file, &len, sizeof(len), &tmp, nullptr);
	BUF[len] = 0;
	ReadFile(file, BUF, len, &tmp, nullptr);
	if(BUF[0] != '$')
		item = FindItem(BUF);
	else
	{
		int quest_refid;
		ReadFile(file, &quest_refid, sizeof(quest_refid), &tmp, nullptr);
		Game::Get().AddQuestItemRequest(&item, BUF, quest_refid, nullptr);
		item = QUEST_ITEM_PLACEHOLDER;
	}
	ReadFile(file, &netid, sizeof(netid), &tmp, nullptr);
}
