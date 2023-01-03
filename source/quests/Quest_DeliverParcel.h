#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
// Deliver parcel from one city mayor to another. There may be bandits waiting on road.
class Quest_DeliverParcel final : public Quest_Encounter
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

	void Start() override;
	GameDialog* GetDialog(int dialogType) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IsTimedout() const override;
	bool OnTimeout(TimeoutType ttype) override;
	bool SpecialIf(DialogContext& ctx, cstring msg) override;
	bool IfHaveQuestItem() const override;
	const Item* GetQuestItem() override;
	void Save(GameWriter& f) override;
	LoadResult Load(GameReader& f) override;

private:
	int endLoc;
	OtherItem parcel;
};
