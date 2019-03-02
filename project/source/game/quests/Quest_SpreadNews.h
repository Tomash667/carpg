#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
class Quest_SpreadNews final : public Quest
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

	void Start() override;
	GameDialog* GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IsTimedout() const override;
	bool OnTimeout(TimeoutType ttype) override;
	bool IfNeedTalk(cstring topic) const override;
	void Save(GameWriter& f) override;
	bool Load(GameReader& f) override;

private:
	vector<Entry> entries;
};
