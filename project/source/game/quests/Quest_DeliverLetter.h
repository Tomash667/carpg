#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
class Quest_DeliverLetter : public Quest
{
public:
	enum Progress
	{
		None,
		Started,
		Timeout,
		GotResponse,
		Finished
	};

	void Start();
	GameDialog* GetDialog(int dialog_type) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IsTimedout() const override;
	bool OnTimeout(TimeoutType ttype) override;
	bool IfHaveQuestItem() const override;
	const Item* GetQuestItem() override;
	void Save(HANDLE file) override;
	bool Load(HANDLE file) override;

private:
	int end_loc;
	OtherItem letter;
};
