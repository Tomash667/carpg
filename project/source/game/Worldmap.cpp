#include "Pch.h"
#include "GameCore.h"
#include "Game.h"
#include "Terrain.h"
#include "Inventory.h"
#include "Quest_Sawmill.h"
#include "Quest_Bandits.h"
#include "Quest_Orcs.h"
#include "Quest_Evil.h"
#include "Quest_Crazies.h"
#include "Perlin.h"
#include "LocationHelper.h"
#include "QuestManager.h"
#include "Encounter.h"
#include "Cave.h"
#include "Camp.h"
#include "MultiInsideLocation.h"
#include "WorldMapGui.h"
#include "GameGui.h"
#include "InfoBox.h"
#include "LoadScreen.h"
#include "MainMenu.h"
#include "MultiplayerPanel.h"
#include "AIController.h"
#include "Team.h"
#include "ItemContainer.h"
#include "BuildingScript.h"
#include "BuildingGroup.h"
#include "Stock.h"
#include "UnitGroup.h"
#include "Portal.h"
#include "World.h"
#include "Level.h"
#include "DirectX.h"
#include "ScriptManager.h"
#include "Var.h"
#include "Quest_Contest.h"
#include "Quest_Secret.h"
#include "Quest_Tournament.h"
#include "LocationGeneratorFactory.h"
#include "Texture.h"
#include "BitStreamFunc.h"

extern Matrix m1, m2, m3, m4;

void Game::GenerateWorld()
{
	if(next_seed != 0)
	{
		Srand(next_seed);
		next_seed = 0;
	}
	else if(force_seed != 0 && force_seed_all)
		Srand(force_seed);

	Info("Generating world, seed %u.", RandVal());

	W.GenerateWorld();

	Info("Randomness integrity: %d", RandVal());
}

void Game::EnterLocation(int level, int from_portal, bool close_portal)
{
	Location& l = *L.location;

	world_map->visible = false;
	game_gui->Reset();
	game_gui->visible = true;

	const bool reenter = L.is_open;
	L.is_open = true;
	if(W.GetState() != World::State::INSIDE_ENCOUNTER)
		W.SetState(World::State::INSIDE_LOCATION);
	if(from_portal != -1)
		L.enter_from = ENTER_FROM_PORTAL + from_portal;
	else
		L.enter_from = ENTER_FROM_OUTSIDE;
	if(!reenter)
		L.light_angle = Random(PI * 2);

	L.dungeon_level = level;
	L.event_handler = nullptr;
	pc_data.before_player = BP_NONE;
	arena_free = true; //zabezpieczenie :3
	Inventory::lock = nullptr;

	bool first = false;

	if(l.state != LS_ENTERED && l.state != LS_CLEARED)
	{
		first = true;
		level_generated = true;
	}
	else
		level_generated = false;

	if(!reenter)
		InitQuadTree();

	if(Net::IsOnline() && N.active_players > 1)
	{
		BitStreamWriter f;
		f << ID_CHANGE_LEVEL;
		f << (byte)L.location_index;
		f << (byte)0;
		f << (W.GetState() == World::State::INSIDE_ENCOUNTER);
		int ack = N.SendAll(f, HIGH_PRIORITY, RELIABLE_WITH_ACK_RECEIPT, Stream_TransferServer);
		for(auto info : N.players)
		{
			if(info->id == my_id)
				info->state = PlayerInfo::IN_GAME;
			else
			{
				info->state = PlayerInfo::WAITING_FOR_RESPONSE;
				info->ack = ack;
				info->timer = 5.f;
			}
		}
		Net_FilterServerChanges();
	}

	// calculate number of loading steps for drawing progress bar
	LocationGenerator* loc_gen = loc_gen_factory->Get(&l, first, reenter);
	int steps = loc_gen->GetNumberOfSteps();
	LoadingStart(steps);
	LoadingStep(txEnteringLocation);

	// generate map on first visit
	if(l.state != LS_ENTERED && l.state != LS_CLEARED)
	{
		if(next_seed != 0)
		{
			Srand(next_seed);
			next_seed = 0;
		}
		else if(force_seed != 0 && force_seed_all)
			Srand(force_seed);

		// log what is required to generate location for testing
		if(l.type == L_CITY)
		{
			City* city = (City*)&l;
			Info("Generating location '%s', seed %u [%d].", l.name.c_str(), RandVal(), city->citizens);
		}
		else
			Info("Generating location '%s', seed %u.", l.name.c_str(), RandVal());
		l.seed = RandVal();
		if(!l.outside)
		{
			InsideLocation* inside = (InsideLocation*)L.location;
			if(inside->IsMultilevel())
			{
				MultiInsideLocation* multi = (MultiInsideLocation*)inside;
				multi->infos[L.dungeon_level].seed = l.seed;
			}
		}

		// nie odwiedzono, trzeba wygenerowaæ
		l.state = LS_ENTERED;

		LoadingStep(txGeneratingMap);

		loc_gen->Generate();
	}
	else if(l.type != L_DUNGEON && l.type != L_CRYPT)
		Info("Entering location '%s'.", l.name.c_str());

	L.city_ctx = nullptr;

	if(L.location->outside)
	{
		loc_gen->OnEnter();
		SetTerrainTextures();
		CalculateQuadtree();
	}
	else
		EnterLevel(loc_gen);

	bool loaded_resources = RequireLoadingResources(L.location, nullptr);
	LoadResources(txLoadingComplete, false);

	l.last_visit = W.GetWorldtime();
	CheckIfLocationCleared();
	cam.Reset();
	pc_data.rot_buf = 0.f;
	SetMusic();

	if(close_portal)
	{
		delete L.location->portal;
		L.location->portal = nullptr;
	}

	if(Net::IsOnline())
	{
		net_mode = NM_SERVER_SEND;
		net_state = NetState::Server_Send;
		if(N.active_players > 1)
		{
			prepared_stream.Reset();
			PrepareLevelData(prepared_stream, loaded_resources);
			Info("Generated location packet: %d.", prepared_stream.GetNumberOfBytesUsed());
		}
		else
			N.GetMe().state = PlayerInfo::IN_GAME;

		info_box->Show(txWaitingForPlayers);
	}
	else
	{
		clear_color = clear_color2;
		game_state = GS_LEVEL;
		load_screen->visible = false;
		main_menu->visible = false;
		game_gui->visible = true;
	}

	if(L.location->outside)
	{
		OnEnterLevelOrLocation();
		OnEnterLocation();
	}

	Info("Randomness integrity: %d", RandVal());
	Info("Entered location.");
}

template<typename T>
void InsertRandomItem(vector<ItemSlot>& container, vector<T*>& items, int price_limit, int exclude_flags, uint count = 1)
{
	for(int i = 0; i < 100; ++i)
	{
		T* item = items[Rand() % items.size()];
		if(item->value > price_limit || IS_SET(item->flags, exclude_flags))
			continue;
		InsertItemBare(container, item, count);
		return;
	}
}

void Game::GenerateStockItems()
{
	Location& loc = *L.location;

	assert(loc.type == L_CITY);

	City& city = (City&)loc;
	int price_limit, price_limit2, count_mod;
	bool is_city;

	if(!city.IsVillage())
	{
		price_limit = Random(2000, 2500);
		price_limit2 = 99999;
		count_mod = 0;
		is_city = true;
	}
	else
	{
		price_limit = Random(500, 1000);
		price_limit2 = Random(1250, 2500);
		count_mod = -Random(1, 3);
		is_city = false;
	}

	// merchant
	if(IS_SET(city.flags, City::HaveMerchant))
		GenerateMerchantItems(chest_merchant, price_limit);

	// blacksmith
	if(IS_SET(city.flags, City::HaveBlacksmith))
	{
		chest_blacksmith.clear();
		for(int i = 0, count = Random(12, 18) + count_mod; i < count; ++i)
		{
			switch(Rand() % 4)
			{
			case IT_WEAPON:
				InsertRandomItem(chest_blacksmith, Weapon::weapons, price_limit2, ITEM_NOT_SHOP | ITEM_NOT_BLACKSMITH);
				break;
			case IT_BOW:
				InsertRandomItem(chest_blacksmith, Bow::bows, price_limit2, ITEM_NOT_SHOP | ITEM_NOT_BLACKSMITH);
				break;
			case IT_SHIELD:
				InsertRandomItem(chest_blacksmith, Shield::shields, price_limit2, ITEM_NOT_SHOP | ITEM_NOT_BLACKSMITH);
				break;
			case IT_ARMOR:
				InsertRandomItem(chest_blacksmith, Armor::armors, price_limit2, ITEM_NOT_SHOP | ITEM_NOT_BLACKSMITH);
				break;
			}
		}
		// basic equipment
		Stock::Get("blacksmith")->Parse(5, is_city, chest_blacksmith);
		SortItems(chest_blacksmith);
	}

	// alchemist
	if(IS_SET(city.flags, City::HaveAlchemist))
	{
		chest_alchemist.clear();
		for(int i = 0, count = Random(8, 12) + count_mod; i < count; ++i)
			InsertRandomItem(chest_alchemist, Consumable::consumables, 99999, ITEM_NOT_SHOP | ITEM_NOT_ALCHEMIST, Random(3, 6));
		SortItems(chest_alchemist);
	}

	// innkeeper
	if(IS_SET(city.flags, City::HaveInn))
	{
		chest_innkeeper.clear();
		Stock::Get("innkeeper")->Parse(5, is_city, chest_innkeeper);
		const ItemList* lis2 = ItemList::Get("normal_food").lis;
		for(uint i = 0, count = Random(10, 20) + count_mod; i < count; ++i)
			InsertItemBare(chest_innkeeper, lis2->Get());
		SortItems(chest_innkeeper);
	}

	// food seller
	if(IS_SET(city.flags, City::HaveFoodSeller))
	{
		chest_food_seller.clear();
		const ItemList* lis = ItemList::Get("food_and_drink").lis;
		for(const Item* item : lis->items)
		{
			uint value = Random(50, 100);
			if(!is_city)
				value /= 2;
			InsertItemBare(chest_food_seller, item, value / item->value);
		}
		if(Rand() % 4 == 0)
			InsertItemBare(chest_food_seller, Item::Get("frying_pan"));
		if(Rand() % 4 == 0)
			InsertItemBare(chest_food_seller, Item::Get("ladle"));
		SortItems(chest_food_seller);
	}
}

void Game::GenerateMerchantItems(vector<ItemSlot>& items, int price_limit)
{
	items.clear();
	InsertItemBare(items, Item::Get("p_nreg"), Random(5, 10));
	InsertItemBare(items, Item::Get("p_hp"), Random(5, 10));
	for(int i = 0, count = Random(15, 20); i < count; ++i)
	{
		switch(Rand() % 6)
		{
		case IT_WEAPON:
			InsertRandomItem(items, Weapon::weapons, price_limit, ITEM_NOT_SHOP | ITEM_NOT_MERCHANT);
			break;
		case IT_BOW:
			InsertRandomItem(items, Bow::bows, price_limit, ITEM_NOT_SHOP | ITEM_NOT_MERCHANT);
			break;
		case IT_SHIELD:
			InsertRandomItem(items, Shield::shields, price_limit, ITEM_NOT_SHOP | ITEM_NOT_MERCHANT);
			break;
		case IT_ARMOR:
			InsertRandomItem(items, Armor::armors, price_limit, ITEM_NOT_SHOP | ITEM_NOT_MERCHANT);
			break;
		case IT_CONSUMABLE:
			InsertRandomItem(items, Consumable::consumables, price_limit / 5, ITEM_NOT_SHOP | ITEM_NOT_MERCHANT, Random(2, 5));
			break;
		case IT_OTHER:
			InsertRandomItem(items, OtherItem::others, price_limit, ITEM_NOT_SHOP | ITEM_NOT_MERCHANT);
			break;
		}
	}
	SortItems(items);
}

// dru¿yna opuœci³a lokacje
void Game::LeaveLocation(bool clear, bool end_buffs)
{
	if(Net::IsLocal())
	{
		// zawody
		if(QM.quest_tournament->GetState() != Quest_Tournament::TOURNAMENT_NOT_DONE)
			QM.quest_tournament->Clean();
		// arena
		if(!arena_free)
			CleanArena();
	}

	if(clear)
	{
		if(L.is_open)
			LeaveLevel(true);
		return;
	}

	if(!L.is_open)
		return;

	Info("Leaving location.");

	pvp_response.ok = false;

	if(Net::IsLocal() && (QM.quest_crazies->check_stone
		|| (QM.quest_crazies->crazies_state >= Quest_Crazies::State::PickedStone && QM.quest_crazies->crazies_state < Quest_Crazies::State::End)))
		QM.quest_crazies->CheckStone();

	// drinking contest
	Quest_Contest* contest = QM.quest_contest;
	if(contest->state >= Quest_Contest::CONTEST_STARTING)
		contest->Cleanup();

	// clear blood & bodies from orc base
	if(Net::IsLocal() && QM.quest_orcs2->orcs_state == Quest_Orcs2::State::ClearDungeon && L.location_index == QM.quest_orcs2->target_loc)
	{
		QM.quest_orcs2->orcs_state = Quest_Orcs2::State::End;
		L.UpdateLocation(31, 100, false);
	}

	if(L.city_ctx && game_state != GS_EXIT_TO_MENU && Net::IsLocal())
	{
		// opuszczanie miasta
		Team.BuyTeamItems();
	}

	LeaveLevel();

	if(L.is_open)
	{
		if(Net::IsLocal())
		{
			// usuñ questowe postacie
			RemoveQuestUnits(true);
		}

		L.ProcessRemoveUnits(true);

		if(L.location->type == L_ENCOUNTER)
		{
			OutsideLocation* outside = (OutsideLocation*)L.location;
			outside->bloods.clear();
			DeleteElements(outside->objects);
			DeleteElements(outside->chests);
			DeleteElements(outside->items);
			outside->objects.clear();
			DeleteElements(outside->usables);
			for(vector<Unit*>::iterator it = outside->units.begin(), end = outside->units.end(); it != end; ++it)
				delete *it;
			outside->units.clear();
		}
	}

	if(Team.crazies_attack)
	{
		Team.crazies_attack = false;
		if(Net::IsOnline())
			Net::PushChange(NetChange::CHANGE_FLAGS);
	}

	if(!Net::IsLocal())
		pc = nullptr;
	else if(end_buffs)
	{
		// usuñ tymczasowe bufy
		for(Unit* unit : Team.members)
			unit->EndEffects();
	}

	L.is_open = false;
	L.city_ctx = nullptr;
}

void Game::Event_RandomEncounter(int)
{
	dialog_enc = nullptr;
	if(Net::IsOnline())
		Net::PushChange(NetChange::CLOSE_ENCOUNTER);
	W.StartEncounter();
	EnterLocation();
}
