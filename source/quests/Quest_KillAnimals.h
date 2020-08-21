#pragma once

//-----------------------------------------------------------------------------
#include "Quest2.h"

//-----------------------------------------------------------------------------
// Go to forest/cave and kill all animals.
class Quest_KillAnimals final : public Quest2
{
public:
	enum Progress
	{
		None,
		Started,
		ClearedLocation,
		Finished,
		Timeout,
		OnTimeout
	};

	void Start() override;
	void SetProgress(int p) override;
	void FireEvent(ScriptEvent& event) override;
	cstring FormatString(const string& str) override;
	void SaveDetails(GameWriter& f) override;
	LoadResult Load(GameReader& f) override;
	void LoadDetails(GameReader& f) override;

private:
	int GetReward() const;

	Location* targetLoc;
	int st;
};
