#include "Pch.h"
#include "Base.h"
#include "QuestManager.h"

#include "Quest_Bandits.h"
#include "Quest_BanditsCollectToll.h"
#include "Quest_CampNearCity.h"
#include "Quest_Crazies.h"
#include "Quest_DeliverLetter.h"
#include "Quest_DeliverParcel.h"
#include "Quest_Evil.h"
#include "Quest_FindArtifact.h"
#include "Quest_Goblins.h"
#include "Quest_KillAnimals.h"
#include "Quest_LostArtifact.h"
#include "Quest_Mages.h"
#include "Quest_Main.h"
#include "Quest_Mine.h"
#include "Quest_Orcs.h"
#include "Quest_RescueCaptive.h"
#include "Quest_RetrivePackage.h"
#include "Quest_Sawmill.h"
#include "Quest_SpreadNews.h"
#include "Quest_StolenArtifact.h"
#include "Quest_Wanted.h"

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
	case Q_FIND_ARTIFACT:
		return new Quest_FindArtifact;
	case Q_LOST_ARTIFACT:
		return new Quest_LostArtifact;
	case Q_STOLEN_ARTIFACT:
		return new Quest_StolenArtifact;
	case Q_SAWMILL:
		return new Quest_Sawmill;
	case Q_MINE:
		return new Quest_Mine;
	case Q_BANDITS:
		return new Quest_Bandits;
	case Q_MAGES:
		return new Quest_Mages;
	case Q_MAGES2:
		return new Quest_Mages2;
	case Q_ORCS:
		return new Quest_Orcs;
	case Q_ORCS2:
		return new Quest_Orcs2;
	case Q_GOBLINS:
		return new Quest_Goblins;
	case Q_EVIL:
		return new Quest_Evil;
	case Q_CRAZIES:
		return new Quest_Crazies;
	case Q_WANTED:
		return new Quest_Wanted;
	case Q_MAIN:
		return new Quest_Main;
	default:
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
Quest* QuestManager::GetMayorQuest(int force)
{
	if(force == -1)
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
			return nullptr;
		}
	}
	else
	{
		switch(force)
		{
		case 0:
		default:
			return nullptr;
		case 1:
			return new Quest_DeliverLetter;
		case 2:
			return new Quest_DeliverParcel;
		case 3:
			return new Quest_SpreadNews;
		case 4:
			return new Quest_RetrivePackage;
		}
	}
}

//=================================================================================================
Quest* QuestManager::GetCaptainQuest(int force)
{
	if(force == -1)
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
			return new Quest_Wanted;
		case 10:
		default:
			return nullptr;
		}
	}
	else
	{
		switch(force)
		{
		case 0:
		default:
			return nullptr;
		case 1:
			return new Quest_RescueCaptive;
		case 2:
			return new Quest_BanditsCollectToll;
		case 3:
			return new Quest_CampNearCity;
		case 4:
			return new Quest_KillAnimals;
		case 5:
			return new Quest_Wanted;
		}
	}
}

//=================================================================================================
Quest* QuestManager::GetAdventurerQuest(int force)
{
	if(force == -1)
	{
		switch(rand2() % 3)
		{
		case 0:
		default:
			return new Quest_FindArtifact;
		case 1:
			return new Quest_LostArtifact;
		case 2:
			return new Quest_StolenArtifact;
		}
	}
	else
	{
		switch(force)
		{
		case 1:
		default:
			return new Quest_FindArtifact;
		case 2:
			return new Quest_LostArtifact;
		case 3:
			return new Quest_StolenArtifact;
		}
	}
}
