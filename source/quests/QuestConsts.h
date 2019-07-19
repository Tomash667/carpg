#pragma once

//-----------------------------------------------------------------------------
enum class QuestType
{
	NotSet = -1,
	Mayor = 0,
	Captain,
	Random,
	Unique
};

//-----------------------------------------------------------------------------
// identyfikatory questów
// kolejnoœæ nie jest nigdzie u¿ywana, mo¿na dawaæ jak siê chce, ale na koñcu ¿eby zapisy by³y kompatybilne
enum QUEST
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
	Q_DELIVER_LETTER,
	Q_DELIVER_PARCEL,
	Q_SPREAD_NEWS,
	Q_RESCUE_CAPTIVE,
	Q_BANDITS_COLLECT_TOLL,
	Q_CAMP_NEAR_CITY,
	Q_RETRIEVE_PACKAGE,
	Q_KILL_ANIMALS,
	Q_LOST_ARTIFACT,
	Q_STOLEN_ARTIFACT,
	Q_FIND_ARTIFACT,
	Q_CRAZIES,
	Q_WANTED,
	Q_MAIN,
	Q_ARTIFACTS,
	Q_SCRIPTED
};

//-----------------------------------------------------------------------------
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
