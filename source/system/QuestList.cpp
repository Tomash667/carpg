#include "Pch.h"
#include "QuestList.h"

vector<QuestList*> QuestList::lists;

//=================================================================================================
QuestInfo* QuestList::GetRandom() const
{
	int x = Rand() % total, y = 0;
	for(const Entry& entry : entries)
	{
		y += entry.chance;
		if(x < y)
			return entry.info;
	}
	assert(0);
	return entries[0].info;
}

//=================================================================================================
QuestList* QuestList::TryGet(const string& id)
{
	for(QuestList* list : lists)
	{
		if(list->id == id)
			return list;
	}
	return nullptr;
}
