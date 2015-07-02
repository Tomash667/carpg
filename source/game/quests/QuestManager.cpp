#include "Pch.h"
#include "Base.h"
#include "QuestManager.h"
#include "MainQuest.h"

//=================================================================================================
Quest* QuestManager::CreateQuest(QUEST quest_id)
{
	switch(quest_id)
	{
	case Q_DOSTARCZ_LIST:
		return new Quest_DostarczList;
	case Q_DOSTARCZ_PACZKE:
		return new Quest_DostarczPaczke;
	case Q_ROZNIES_WIESCI:
		return new Quest_RozniesWiesci;
	case Q_ODZYSKAJ_PACZKE:
		return new Quest_OdzyskajPaczke;
	case Q_URATUJ_PORWANA_OSOBE:
		return new Quest_UratujPorwanaOsobe;
	case Q_BANDYCI_POBIERAJA_OPLATE:
		return new Quest_BandyciPobierajaOplate;
	case Q_OBOZ_KOLO_MIASTA:
		return new Quest_ObozKoloMiasta;
	case Q_ZABIJ_ZWIERZETA:
		return new Quest_ZabijZwierzeta;
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
		return new MainQuest;
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
		return new Quest_DostarczList;
	case 3:
	case 4:
	case 5:
		return new Quest_DostarczPaczke;
	case 6:
	case 7:
		return new Quest_RozniesWiesci;
		break;
	case 8:
	case 9:
		return new Quest_OdzyskajPaczke;
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
		return new Quest_UratujPorwanaOsobe;
	case 2:
	case 3:
		return new Quest_BandyciPobierajaOplate;
	case 4:
	case 5:
		return new Quest_ObozKoloMiasta;
	case 6:
	case 7:
		return new Quest_ZabijZwierzeta;
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
