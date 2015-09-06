#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
class Quest_DeliverParcel : public Quest_Encounter
{
public:
	enum Progress
	{
		None,
		Started,
		DeliverAfterTime,
		Timeout,
		Finished,
		AttackedBandits,
		ParcelGivenToBandits,
		NoParcelAttackedBandits
	};

	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IsTimedout() const;
	bool IfHaveQuestItem() const;
	const Item* GetQuestItem();
	void Save(HANDLE file);
	void Load(HANDLE file);

private:
	int end_loc;
	OtherItem parcel;
};
