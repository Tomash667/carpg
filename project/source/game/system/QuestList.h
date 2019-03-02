#pragma once

//-----------------------------------------------------------------------------
struct QuestList
{
	struct Entry
	{
		QuestInfo* info;
		int chance;
	};

	string id;
	vector<Entry> entries;
	int total;

	QuestInfo* GetRandom() const;

	static vector<QuestList*> lists;
	static QuestList* TryGet(const string& id);
};
