#pragma once

//-----------------------------------------------------------------------------
enum class QuestCategory
{
	NotSet = -1,
	Mayor = 0,
	Captain,
	Random,
	Unique
};

//-----------------------------------------------------------------------------
// Old quest identifiers
enum QUEST_TYPE
{
	Q_FORCE_DISABLED = -2,
	Q_FORCE_NONE = -1,
	Q_MINE = 0,
	Q_SAWMILL,
	Q_BANDITS,
	Q_MAGES,
	Q_MAGES2,
	Q_ORCS,
	Q_ORCS2,
	Q_GOBLINS,
	Q_EVIL,
	Q_DELIVER_LETTER, // removed in V_0_14
	Q_DELIVER_PARCEL,
	Q_SPREAD_NEWS,
	Q_RESCUE_CAPTIVE,
	Q_BANDITS_COLLECT_TOLL, // removed in V_0_13
	Q_CAMP_NEAR_CITY, // removed in V_0_16
	Q_RETRIEVE_PACKAGE,
	Q_KILL_ANIMALS,
	Q_LOST_ARTIFACT,
	Q_STOLEN_ARTIFACT,
	Q_FIND_ARTIFACT, // removed in V_0_15
	Q_CRAZIES,
	Q_WANTED,
	Q_MAIN, // removed
	Q_ARTIFACTS, // removed in V_0_14
	Q_SCRIPTED
};

//-----------------------------------------------------------------------------
// pre V_0_10 compatibility
namespace old
{
	enum QUEST_RUMOR
	{
		R_SAWMILL,
		R_MINE,
		R_CONTEST,
		R_BANDITS,
		R_MAGES,
		R_MAGES2,
		R_ORCS,
		R_GOBLINS,
		R_EVIL,
		R_MAX
	};
}

//-----------------------------------------------------------------------------
#define QUEST_ITEM_PLACEHOLDER ((const Item*)-1)
