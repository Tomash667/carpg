#pragma once

#include "Quest2.h"

class Quest_DireWolf : public Quest2
{
public:
	enum Progress
	{
		None,
		Started,
		Killed,
		Complete
	};

	void Start() override;
	void SetProgress(int p) override;
	void FireEvent(ScriptEvent& event) override;
	void SaveDetails(GameWriter& f) override;
	void LoadDetails(GameReader& f) override;

private:
	Location* camp;
	Location* forest;
};
