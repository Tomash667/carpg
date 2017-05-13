#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
class Quest_SpreadNews : public Quest
{
public:
	enum Progress
	{
		None,
		Started,
		Deliver,
		Timeout,
		Finished
	};

	struct Entry
	{
		int location;
		float dist;
		bool given;
	};

	void Start();
	GameDialog* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IsTimedout() const;
	bool OnTimeout(TimeoutType ttype);
	bool IfNeedTalk(cstring topic) const;
	void Save(HANDLE file);
	void Load(HANDLE file);

private:
	vector<Entry> entries;
};
