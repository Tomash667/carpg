#pragma once

//-----------------------------------------------------------------------------
enum class QuestType
{
	Mayor,
	Captain,
	Random,
	Unique
};

//-----------------------------------------------------------------------------
// identyfikatory questów
// kolejnoœæ nie jest nigdzie u¿ywana, mo¿na dawaæ jak siê chce, ale na koñcu ¿eby zapisy by³y kompatybilne
enum QUEST
{
	Q_MINE,
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
};

//-----------------------------------------------------------------------------
static const int UNIQUE_QUESTS = 8;

//-----------------------------------------------------------------------------
enum PLOTKA_QUESTOWA
{
	P_TARTAK,
	P_KOPALNIA,
	P_ZAWODY_W_PICIU,
	P_BANDYCI,
	P_MAGOWIE,
	P_MAGOWIE2,
	P_ORKOWIE,
	P_GOBLINY,
	P_ZLO,
	P_MAX
};

//-----------------------------------------------------------------------------
#define QUEST_ITEM_PLACEHOLDER ((const Item*)-1)
