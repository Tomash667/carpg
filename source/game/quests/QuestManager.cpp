#include "Pch.h"
#include "Base.h"
#include "QuestManager.h"

#include "Quest_BanditsCollectToll.h"
#include "Quest_CampNearCity.h"
#include "Quest_DeliverLetter.h"
#include "Quest_DeliverParcel.h"
#include "Quest_KillAnimals.h"
#include "Quest_Main.h"
#include "Quest_RescueCaptive.h"
#include "Quest_RetrivePackage.h"
#include "Quest_SpreadNews.h"

//=================================================================================================
Quest* QuestManager::CreateQuest(QUEST quest_id)
{
	switch(quest_id)
	{
	case Q_DELIVER_LETTER:
		return new Quest_DeliverLetter;
	case Q_DELIVER_PARCEL:
		return new Quest_DeliverParcel;
	case Q_SPREAD_NEWS:
		return new Quest_SpreadNews;
	case Q_RETRIVE_PACKAGE:
		return new Quest_RetrivePackage;
	case Q_RESCUE_CAPTIVE:
		return new Quest_RescueCaptive;
	case Q_BANDITS_COLLECT_TOLL:
		return new Quest_BanditsCollectToll;
	case Q_CAMP_NEAR_CITY:
		return new Quest_CampNearCity;
	case Q_KILL_ANIMALS:
		return new Quest_KillAnimals;
	case Q_PRZYNIES_ARTEFAKT:
		return new Quest_ZnajdzArtefakt;
	case Q_ZGUBIONY_PRZEDMIOT:
		return new Quest_ZgubionyPrzedmiot;
	case Q_UKRADZIONY_PRZEDMIOT:
		return new Quest_UkradzionyPrzedmiot;
	case Q_TARTAK:
		return new Quest_Tartak;
	case Q_KOPALNIA:
		return new Quest_Kopalnia;
	case Q_BANDYCI:
		return new Quest_Bandyci;
	case Q_MAGOWIE:
		return new Quest_Magowie;
	case Q_MAGOWIE2:
		return new Quest_Magowie2;
	case Q_ORKOWIE:
		return new Quest_Orkowie;
	case Q_ORKOWIE2:
		return new Quest_Orkowie2;
	case Q_GOBLINY:
		return new Quest_Gobliny;
	case Q_ZLO:
		return new Quest_Zlo;
	case Q_SZALENI:
		return new Quest_Szaleni;
	case Q_LIST_GONCZY:
		return new Quest_ListGonczy;
	case Q_MAIN:
		return new Quest_Main;
	default:
		assert(0);
		return NULL;
	}
}

//=================================================================================================
Quest* QuestManager::GetMayorQuest()
{
	switch(rand2() % 12)
	{
	case 0:
	case 1:
	case 2:
		return new Quest_DeliverLetter;
	case 3:
	case 4:
	case 5:
		return new Quest_DeliverParcel;
	case 6:
	case 7:
		return new Quest_SpreadNews;
		break;
	case 8:
	case 9:
		return new Quest_RetrivePackage;
	case 10:
	case 11:
	default:
		return NULL;
	}
}

//=================================================================================================
Quest* QuestManager::GetCaptainQuest()
{
	switch(rand2() % 11)
	{
	case 0:
	case 1:
		return new Quest_RescueCaptive;
	case 2:
	case 3:
		return new Quest_BanditsCollectToll;
	case 4:
	case 5:
		return new Quest_CampNearCity;
	case 6:
	case 7:
		return new Quest_KillAnimals;
	case 8:
	case 9:
		return new Quest_ListGonczy;
	case 10:
	default:
		return NULL;
	}
}

//=================================================================================================
Quest* QuestManager::GetAdventurerQuest()
{
	switch(rand2() % 3)
	{
	case 0:
	default:
		return new Quest_ZnajdzArtefakt;
	case 1:
		return new Quest_ZgubionyPrzedmiot;
	case 2:
		return new Quest_UkradzionyPrzedmiot;
	}
}
