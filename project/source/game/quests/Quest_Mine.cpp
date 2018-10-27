#include "Pch.h"
#include "GameCore.h"
#include "Quest_Mine.h"
#include "Game.h"
#include "Journal.h"
#include "GameFile.h"
#include "SaveState.h"
#include "LocationHelper.h"
#include "QuestManager.h"
#include "World.h"
#include "Level.h"
#include "Cave.h"
#include "CaveGenerator.h"
#include "Portal.h"
#include "AIController.h"

//=================================================================================================
void Quest_Mine::Start()
{
	quest_id = Q_MINE;
	type = QuestType::Unique;
	dungeon_loc = -2;
	mine_state = State::None;
	mine_state2 = State2::None;
	mine_state3 = State3::None;
	messenger = nullptr;
	days = 0;
	days_required = 0;
	days_gold = 0;
}

//=================================================================================================
GameDialog* Quest_Mine::GetDialog(int type2)
{
	if(type2 == QUEST_DIALOG_NEXT)
	{
		if(DialogContext::current->talker->data->id == "inwestor")
			return GameDialog::TryGet("q_mine_investor");
		else if(DialogContext::current->talker->data->id == "poslaniec_kopalnia")
		{
			if(prog == Quest_Mine::Progress::SelectedShares)
				return GameDialog::TryGet("q_mine_messenger");
			else if(prog == Quest_Mine::Progress::GotFirstGold || prog == Quest_Mine::Progress::SelectedGold)
			{
				if(days >= days_required)
					return GameDialog::TryGet("q_mine_messenger2");
				else
					return GameDialog::TryGet("messenger_talked");
			}
			else if(prog == Quest_Mine::Progress::Invested)
			{
				if(days >= days_required)
					return GameDialog::TryGet("q_mine_messenger3");
				else
					return GameDialog::TryGet("messenger_talked");
			}
			else if(prog == Quest_Mine::Progress::UpgradedMine)
			{
				if(days >= days_required)
					return GameDialog::TryGet("q_mine_messenger4");
				else
					return GameDialog::TryGet("messenger_talked");
			}
			else
				return GameDialog::TryGet("messenger_talked");
		}
		else
			return GameDialog::TryGet("q_mine_boss");
	}
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
void Quest_Mine::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started:
		{
			OnStart(game->txQuest[131]);

			location_event_handler = this;

			Location& sl = GetStartLocation();
			Location& tl = GetTargetLocation();
			at_level = 0;
			tl.active_quest = this;
			tl.SetKnown();
			if(tl.state >= LS_ENTERED)
				tl.reset = true;
			tl.st = 10;

			InitSub();

			msgs.push_back(Format(game->txQuest[132], sl.name.c_str(), W.GetDate()));
			msgs.push_back(Format(game->txQuest[133], tl.name.c_str(), GetTargetLocationDir()));
		}
		break;
	case Progress::ClearedLocation:
		{
			OnUpdate(game->txQuest[134]);
		}
		break;
	case Progress::SelectedShares:
		{
			OnUpdate(game->txQuest[135]);
			mine_state = State::Shares;
			mine_state2 = State2::InBuild;
			days = 0;
			days_required = Random(30, 45);
			quest_manager.RemoveQuestRumor(R_MINE);
		}
		break;
	case Progress::GotFirstGold:
		{
			state = Quest::Completed;
			OnUpdate(game->txQuest[136]);
			game->AddReward(500);
			mine_state2 = State2::Built;
			days -= days_required;
			days_required = Random(60, 90);
			if(days >= days_required)
				days = days_required - 1;
			days_gold = 0;
		}
		break;
	case Progress::SelectedGold:
		{
			state = Quest::Completed;
			OnUpdate(game->txQuest[137]);
			game->AddReward(3000);
			mine_state2 = State2::InBuild;
			days = 0;
			days_required = Random(30, 45);
			quest_manager.RemoveQuestRumor(R_MINE);
		}
		break;
	case Progress::NeedTalk:
		{
			state = Quest::Started;
			OnUpdate(game->txQuest[138]);
			mine_state2 = State2::CanExpand;
			W.AddNews(Format(game->txQuest[139], GetTargetLocationName()));
		}
		break;
	case Progress::Talked:
		{
			OnUpdate(Format(game->txQuest[140], mine_state == State::Shares ? 10000 : 12000));
		}
		break;
	case Progress::NotInvested:
		{
			state = Quest::Completed;
			OnUpdate(game->txQuest[141]);
			quest_manager.EndUniqueQuest();
		}
		break;
	case Progress::Invested:
		{
			DialogContext::current->pc->unit->ModGold(mine_state == State::Shares ? -10000 : -12000);
			OnUpdate(game->txQuest[142]);
			mine_state2 = State2::InExpand;
			days = 0;
			days_required = Random(30, 45);
		}
		break;
	case Progress::UpgradedMine:
		{
			state = Quest::Completed;
			OnUpdate(game->txQuest[143]);
			game->AddReward(1000);
			mine_state = State::BigShares;
			mine_state2 = State2::Expanded;
			days -= days_required;
			days_required = Random(60, 90);
			if(days >= days_required)
				days = days_required - 1;
			days_gold = 0;
			W.AddNews(Format(game->txQuest[144], GetTargetLocationName()));
		}
		break;
	case Progress::InfoAboutPortal:
		{
			state = Quest::Started;
			OnUpdate(game->txQuest[145]);
			mine_state2 = State2::FoundPortal;
			W.AddNews(Format(game->txQuest[146], GetTargetLocationName()));
		}
		break;
	case Progress::TalkedWithMiner:
		{
			OnUpdate(game->txQuest[147]);
			const Item* item = Item::Get("key_kopalnia");
			DialogContext::current->pc->unit->AddItem2(item, 1u, 1u);
		}
		break;
	case Progress::Finished:
		{
			state = Quest::Completed;
			OnUpdate(game->txQuest[148]);
			quest_manager.EndUniqueQuest();
			W.AddNews(game->txQuest[149]);
		}
		break;
	}
}

//=================================================================================================
cstring Quest_Mine::FormatString(const string& str)
{
	if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetTargetLocationDir();
	else if(str == "zloto")
		return Format("%d", mine_state == State::Shares ? 10000 : 12000);
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_Mine::IfNeedTalk(cstring topic) const
{
	return strcmp(topic, "kopalnia") == 0;
}

//=================================================================================================
bool Quest_Mine::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "udzialy_w_kopalni") == 0)
		return mine_state == State::Shares;
	assert(0);
	return false;
}

//=================================================================================================
bool Quest_Mine::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(prog == Progress::Started && event == LocationEventHandler::CLEARED)
		SetProgress(Progress::ClearedLocation);
	return false;
}

//=================================================================================================
void Quest_Mine::HandleChestEvent(ChestEventHandler::Event event, Chest* chest)
{
	if(prog == Progress::TalkedWithMiner && event == ChestEventHandler::Opened)
		SetProgress(Progress::Finished);
}

//=================================================================================================
void Quest_Mine::Save(GameWriter& f)
{
	Quest_Dungeon::Save(f);

	f << sub.done;
	f << dungeon_loc;
	f << mine_state;
	f << mine_state2;
	f << mine_state3;
	f << days;
	f << days_required;
	f << days_gold;
	f << messenger;
}

//=================================================================================================
bool Quest_Mine::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> sub.done;
	f >> dungeon_loc;

	if(LOAD_VERSION >= V_0_4)
	{
		f >> mine_state;
		f >> mine_state2;
		f >> mine_state3;
		f >> days;
		f >> days_required;
		f >> days_gold;
		f >> messenger;
	}

	location_event_handler = this;
	InitSub();

	return true;
}

//=================================================================================================
void Quest_Mine::LoadOld(GameReader& f)
{
	int city, cave;

	f >> mine_state;
	f >> mine_state2;
	f >> mine_state3;
	f >> city;
	f >> cave;
	f >> refid;
	f >> days;
	f >> days_required;
	f >> days_gold;
	f >> messenger;
}

//=================================================================================================
void Quest_Mine::InitSub()
{
	if(sub.done)
		return;

	ItemListResult result = ItemList::Get("ancient_armory_armors");
	result.lis->Get(3, sub.item_to_give);
	sub.item_to_give[3] = Item::Get("al_angelskin");
	sub.spawn_item = Quest_Event::Item_InChest;
	sub.target_loc = dungeon_loc;
	sub.at_level = 0;
	sub.chest_event_handler = this;
	next_event = &sub;
}

//=================================================================================================
int Quest_Mine::GetIncome(int days_passed)
{
	if(mine_state == State::Shares && mine_state2 >= State2::Built)
	{
		days_gold += days_passed;
		int count = days_gold / 30;
		if(count)
		{
			days_gold -= count * 30;
			return count * 500;
		}
	}
	else if(mine_state == State::BigShares && mine_state2 >= State2::Expanded)
	{
		days_gold += days_passed;
		int count = days_gold / 30;
		if(count)
		{
			days_gold -= count * 30;
			return count * 1000;
		}
	}
	return 0;
}

//=================================================================================================
bool Quest_Mine::GenerateMine(CaveGenerator* cave_gen)
{
	switch(mine_state3)
	{
	case State3::None:
		break;
	case State3::GeneratedMine:
		if(mine_state2 == State2::None)
			return true;
		break;
	case State3::GeneratedInBuild:
		if(mine_state2 <= State2::InBuild)
			return true;
		break;
	case State3::GeneratedBuilt:
		if(mine_state2 <= State2::Built)
			return true;
		break;
	case State3::GeneratedExpanded:
		if(mine_state2 <= State2::Expanded)
			return true;
		break;
	case State3::GeneratedPortal:
		if(mine_state2 <= State2::FoundPortal)
			return true;
		break;
	default:
		assert(0);
		return true;
	}

	Cave* cave = (Cave*)L.location;
	cave->loaded_resources = false;
	InsideLocationLevel& lvl = cave->GetLevelData();

	bool respawn_units = true;

	// usu� stare jednostki i krew
	if(mine_state3 <= State3::GeneratedMine && mine_state2 >= State2::InBuild)
	{
		DeleteElements(L.local_ctx.units);
		DeleteElements(game->ais);
		L.local_ctx.units->clear();
		L.local_ctx.bloods->clear();
		respawn_units = false;
	}

	bool generuj_rude = false;
	int zloto_szansa, powieksz = 0;
	bool rysuj_m = false;

	if(mine_state3 == State3::None)
	{
		generuj_rude = true;
		zloto_szansa = 0;
	}

	// pog��b jaskinie
	if(mine_state2 >= State2::InBuild && mine_state3 < State3::GeneratedBuilt)
	{
		generuj_rude = true;
		zloto_szansa = 0;
		++powieksz;
		rysuj_m = true;
	}

	// bardziej pog��b jaskinie
	if(mine_state2 >= State2::InExpand && mine_state3 < State3::GeneratedExpanded)
	{
		generuj_rude = true;
		zloto_szansa = 4;
		++powieksz;
		rysuj_m = true;
	}

	vector<Int2> nowe;

	for(int i = 0; i < powieksz; ++i)
	{
		for(int y = 1; y < lvl.h - 1; ++y)
		{
			for(int x = 1; x < lvl.w - 1; ++x)
			{
				if(lvl.map[x + y * lvl.w].type == WALL)
				{
#define A(xx,yy) lvl.map[x+(xx)+(y+(yy))*lvl.w].type
					if(Rand() % 2 == 0 && (!IsBlocking2(A(-1, 0)) || !IsBlocking2(A(1, 0)) || !IsBlocking2(A(0, -1)) || !IsBlocking2(A(0, 1))) &&
						(A(-1, -1) != STAIRS_UP && A(-1, 1) != STAIRS_UP && A(1, -1) != STAIRS_UP && A(1, 1) != STAIRS_UP))
					{
						nowe.push_back(Int2(x, y));
					}
#undef A
				}
			}
		}

		// nie potrzebnie dwa razy to robi je�li powi�ksz = 2
		for(vector<Int2>::iterator it = nowe.begin(), end = nowe.end(); it != end; ++it)
			lvl.map[it->x + it->y*lvl.w].type = EMPTY;
	}

	// generuj portal
	if(mine_state2 >= State2::FoundPortal && mine_state3 < State3::GeneratedPortal)
	{
		generuj_rude = true;
		zloto_szansa = 7;
		rysuj_m = true;

		// szukaj dobrego miejsca
		vector<Int2> good_pts;
		for(int y = 1; y < lvl.h - 5; ++y)
		{
			for(int x = 1; x < lvl.w - 5; ++x)
			{
				for(int h = 0; h < 5; ++h)
				{
					for(int w = 0; w < 5; ++w)
					{
						if(lvl.map[x + w + (y + h)*lvl.w].type != WALL)
							goto dalej;
					}
				}

				// jest dobre miejsce
				good_pts.push_back(Int2(x, y));
			dalej:
				;
			}
		}

		if(good_pts.empty())
		{
			if(lvl.staircase_up.x / 26 == 1)
			{
				if(lvl.staircase_up.y / 26 == 1)
					good_pts.push_back(Int2(1, 1));
				else
					good_pts.push_back(Int2(1, lvl.h - 6));
			}
			else
			{
				if(lvl.staircase_up.y / 26 == 1)
					good_pts.push_back(Int2(lvl.w - 6, 1));
				else
					good_pts.push_back(Int2(lvl.w - 6, lvl.h - 6));
			}
		}

		Int2 pt = good_pts[Rand() % good_pts.size()];

		// przygotuj pok�j
		// BBZBB
		// B___B
		// Z___Z
		// B___B
		// BBZBB
		const Int2 p_blokady[] = {
			Int2(0,0),
			Int2(1,0),
			Int2(3,0),
			Int2(4,0),
			Int2(0,1),
			Int2(4,1),
			Int2(0,3),
			Int2(4,3),
			Int2(0,4),
			Int2(1,4),
			Int2(3,4),
			Int2(4,4)
		};
		const Int2 p_zajete[] = {
			Int2(2,0),
			Int2(0,2),
			Int2(4,2),
			Int2(2,4)
		};
		for(uint i = 0; i < countof(p_blokady); ++i)
		{
			Tile& p = lvl.map[(pt + p_blokady[i])(lvl.w)];
			p.type = BLOCKADE;
			p.flags = 0;
		}
		for(uint i = 0; i < countof(p_zajete); ++i)
		{
			Tile& p = lvl.map[(pt + p_zajete[i])(lvl.w)];
			p.type = USED;
			p.flags = 0;
		}

		// dor�b wej�cie
		// znajd� najbli�szy pokoju wolny punkt
		const Int2 center(pt.x + 2, pt.y + 2);
		Int2 closest;
		int best_dist = 999;
		for(int y = 1; y < lvl.h - 1; ++y)
		{
			for(int x = 1; x < lvl.w - 1; ++x)
			{
				if(lvl.map[x + y * lvl.w].type == EMPTY)
				{
					int dist = Int2::Distance(Int2(x, y), center);
					if(dist < best_dist && dist > 2)
					{
						best_dist = dist;
						closest = Int2(x, y);
					}
				}
			}
		}

		// prowad� drog� do �rodka
		// tu mo�e by� niesko�czona p�tla ale nie powinno jej by� chyba �e b�d� jakie� blokady na mapie :3
		Int2 end_pt;
		while(true)
		{
			if(abs(closest.x - center.x) > abs(closest.y - center.y))
			{
			po_x:
				if(closest.x > center.x)
				{
					--closest.x;
					Tile& p = lvl.map[closest.x + closest.y*lvl.w];
					if(p.type == USED)
					{
						end_pt = closest;
						break;
					}
					else if(p.type == BLOCKADE)
					{
						++closest.x;
						goto po_y;
					}
					else
					{
						p.type = EMPTY;
						p.flags = 0;
						nowe.push_back(closest);
					}
				}
				else
				{
					++closest.x;
					Tile& p = lvl.map[closest.x + closest.y*lvl.w];
					if(p.type == USED)
					{
						end_pt = closest;
						break;
					}
					else if(p.type == BLOCKADE)
					{
						--closest.x;
						goto po_y;
					}
					else
					{
						p.type = EMPTY;
						p.flags = 0;
						nowe.push_back(closest);
					}
				}
			}
			else
			{
			po_y:
				if(closest.y > center.y)
				{
					--closest.y;
					Tile& p = lvl.map[closest.x + closest.y*lvl.w];
					if(p.type == USED)
					{
						end_pt = closest;
						break;
					}
					else if(p.type == BLOCKADE)
					{
						++closest.y;
						goto po_x;
					}
					else
					{
						p.type = EMPTY;
						p.flags = 0;
						nowe.push_back(closest);
					}
				}
				else
				{
					++closest.y;
					Tile& p = lvl.map[closest.x + closest.y*lvl.w];
					if(p.type == USED)
					{
						end_pt = closest;
						break;
					}
					else if(p.type == BLOCKADE)
					{
						--closest.y;
						goto po_x;
					}
					else
					{
						p.type = EMPTY;
						p.flags = 0;
						nowe.push_back(closest);
					}
				}
			}
		}

		// ustaw �ciany
		for(uint i = 0; i < countof(p_blokady); ++i)
		{
			Tile& p = lvl.map[(pt + p_blokady[i])(lvl.w)];
			p.type = WALL;
		}
		for(uint i = 0; i < countof(p_zajete); ++i)
		{
			Tile& p = lvl.map[(pt + p_zajete[i])(lvl.w)];
			p.type = WALL;
		}
		for(int y = 1; y < 4; ++y)
		{
			for(int x = 1; x < 4; ++x)
			{
				Tile& p = lvl.map[pt.x + x + (pt.y + y)*lvl.w];
				p.type = EMPTY;
				p.flags = Tile::F_SECOND_TEXTURE;
			}
		}
		Tile& p = lvl.map[end_pt(lvl.w)];
		p.type = DOORS;
		p.flags = 0;

		// ustaw pok�j
		Room& room = Add1(lvl.rooms);
		room.target = RoomTarget::Portal;
		room.pos = pt;
		room.size = Int2(5, 5);

		// dodaj drzwi, portal, pochodnie
		BaseObject* portal = BaseObject::Get("portal"),
			*pochodnia = BaseObject::Get("torch");

		// drzwi
		{
			Object* o = new Object;
			o->mesh = game->aDoorWall;
			o->pos = Vec3(float(end_pt.x * 2) + 1, 0, float(end_pt.y * 2) + 1);
			o->scale = 1;
			o->base = nullptr;
			L.local_ctx.objects->push_back(o);

			// hack :3
			Room& r2 = Add1(lvl.rooms);
			r2.target = RoomTarget::Corridor;

			if(IsBlocking(lvl.map[end_pt.x - 1 + end_pt.y*lvl.w].type))
			{
				o->rot = Vec3(0, 0, 0);
				if(end_pt.y > center.y)
				{
					o->pos.z -= 0.8229f;
					lvl.At(end_pt + Int2(0, 1)).room = 1;
				}
				else
				{
					o->pos.z += 0.8229f;
					lvl.At(end_pt + Int2(0, -1)).room = 1;
				}
			}
			else
			{
				o->rot = Vec3(0, PI / 2, 0);
				if(end_pt.x > center.x)
				{
					o->pos.x -= 0.8229f;
					lvl.At(end_pt + Int2(1, 0)).room = 1;
				}
				else
				{
					o->pos.x += 0.8229f;
					lvl.At(end_pt + Int2(-1, 0)).room = 1;
				}
			}

			Door* door = new Door;
			L.local_ctx.doors->push_back(door);
			door->pt = end_pt;
			door->pos = o->pos;
			door->rot = o->rot.y;
			door->state = Door::Closed;
			door->locked = LOCK_MINE;
			door->netid = Door::netid_counter++;
		}

		// pochodnia
		{
			Vec3 pos(2.f*(pt.x + 1), 0, 2.f*(pt.y + 1));

			switch(Rand() % 4)
			{
			case 0:
				pos.x += pochodnia->r * 2;
				pos.z += pochodnia->r * 2;
				break;
			case 1:
				pos.x += 6.f - pochodnia->r * 2;
				pos.z += pochodnia->r * 2;
				break;
			case 2:
				pos.x += pochodnia->r * 2;
				pos.z += 6.f - pochodnia->r * 2;
				break;
			case 3:
				pos.x += 6.f - pochodnia->r * 2;
				pos.z += 6.f - pochodnia->r * 2;
				break;
			}

			L.SpawnObjectEntity(L.local_ctx, pochodnia, pos, Random(MAX_ANGLE));
		}

		// portal
		{
			float rot;
			if(end_pt.y == center.y)
			{
				if(end_pt.x > center.x)
					rot = PI * 3 / 2;
				else
					rot = PI / 2;
			}
			else
			{
				if(end_pt.y > center.y)
					rot = 0;
				else
					rot = PI;
			}

			rot = Clip(rot + PI);

			// obiekt
			const Vec3 pos(2.f*pt.x + 5, 0, 2.f*pt.y + 5);
			L.SpawnObjectEntity(L.local_ctx, portal, pos, rot);

			// lokacja
			SingleInsideLocation* loc = new SingleInsideLocation;
			loc->active_quest = this;
			loc->target = KOPALNIA_POZIOM;
			loc->from_portal = true;
			loc->name = game->txAncientArmory;
			loc->pos = Vec2(-999, -999);
			loc->spawn = SG_GOLEMS;
			loc->st = 14;
			loc->type = L_DUNGEON;
			loc->image = LI_DUNGEON;
			int loc_id = W.AddLocation(loc);
			sub.target_loc = dungeon_loc = loc_id;

			// funkcjonalno�� portalu
			cave->portal = new Portal;
			cave->portal->at_level = 0;
			cave->portal->target = 0;
			cave->portal->target_loc = loc_id;
			cave->portal->rot = rot;
			cave->portal->next_portal = nullptr;
			cave->portal->pos = pos;

			// info dla portalu po drugiej stronie
			loc->portal = new Portal;
			loc->portal->at_level = 0;
			loc->portal->target = 0;
			loc->portal->target_loc = L.location_index;
			loc->portal->next_portal = nullptr;
		}
	}

	if(!nowe.empty())
		cave_gen->RegenerateFlags();

	if(rysuj_m && game->devmode)
		cave_gen->DebugDraw();

	// generuj rud�
	if(generuj_rude)
	{
		auto iron_vein = BaseUsable::Get("iron_vein"),
			gold_vein = BaseUsable::Get("gold_vein");

		// usu� star� rud�
		if(mine_state3 != State3::None)
			DeleteElements(L.local_ctx.usables);

		// dodaj now�
		for(int y = 1; y < lvl.h - 1; ++y)
		{
			for(int x = 1; x < lvl.w - 1; ++x)
			{
				if(Rand() % 3 == 0)
					continue;

#define P(xx,yy) !IsBlocking2(lvl.map[x-(xx)+(y+(yy))*lvl.w])
#undef S
#define S(xx,yy) lvl.map[x-(xx)+(y+(yy))*lvl.w].type == WALL

				// ruda jest generowana dla takich przypadk�w, w tym obr�conych
				//  ### ### ###
				//  _?_ #?_ #?#
				//  ___ #__ #_#
				if(lvl.map[x + y * lvl.w].type == EMPTY && Rand() % 3 != 0 && !IS_SET(lvl.map[x + y * lvl.w].flags, Tile::F_SECOND_TEXTURE))
				{
					GameDirection dir = GDIR_INVALID;

					// ma by� �ciana i wolne z ty�u oraz wolne na lewo lub prawo lub zaj�te z obu stron
					// __#
					// _?#
					// __#
					if(P(-1, 0) && S(1, 0) && S(1, -1) && S(1, 1) && ((P(-1, -1) && P(0, -1)) || (P(-1, 1) && P(0, 1)) || (S(-1, -1) && S(0, -1) && S(-1, 1) && S(0, 1))))
					{
						dir = GDIR_LEFT;
					}
					// #__
					// #?_
					// #__
					else if(P(1, 0) && S(-1, 0) && S(-1, 1) && S(-1, -1) && ((P(0, -1) && P(1, -1)) || (P(0, 1) && P(1, 1)) || (S(0, -1) && S(1, -1) && S(0, 1) && S(1, 1))))
					{
						dir = GDIR_RIGHT;
					}
					// ###
					// _?_
					// ___
					else if(P(0, 1) && S(0, -1) && S(-1, -1) && S(1, -1) && ((P(-1, 0) && P(-1, 1)) || (P(1, 0) && P(1, 1)) || (S(-1, 0) && S(-1, 1) && S(1, 0) && S(1, 1))))
					{
						dir = GDIR_DOWN;
					}
					// ___
					// _?_
					// ###
					else if(P(0, -1) && S(0, 1) && S(-1, 1) && S(1, 1) && ((P(-1, 0) && P(-1, -1)) || (P(1, 0) && P(1, -1)) || (S(-1, 0) && S(-1, -1) && S(1, 0) && S(1, -1))))
					{
						dir = GDIR_UP;
					}

					if(dir != -1)
					{
						Vec3 pos(2.f*x + 1, 0, 2.f*y + 1);

						switch(dir)
						{
						case GDIR_DOWN:
							pos.z -= 1.f;
							break;
						case GDIR_LEFT:
							pos.x -= 1.f;
							break;
						case GDIR_UP:
							pos.z += 1.f;
							break;
						case GDIR_RIGHT:
							pos.x += 1.f;
							break;
						}

						float rot = Clip(DirToRot(dir) + PI);
						static float radius = max(iron_vein->size.x, iron_vein->size.y) * SQRT_2;

						Level::IgnoreObjects ignore = { 0 };
						ignore.ignore_blocks = true;
						L.global_col.clear();
						L.GatherCollisionObjects(L.local_ctx, L.global_col, pos, radius, &ignore);

						Box2d box(pos.x - iron_vein->size.x, pos.z - iron_vein->size.y, pos.x + iron_vein->size.x, pos.z + iron_vein->size.y);

						if(!L.Collide(L.global_col, box, 0.f, rot))
						{
							Usable* u = new Usable;
							u->pos = pos;
							u->rot = rot;
							u->base = (Rand() % 10 < zloto_szansa ? gold_vein : iron_vein);
							u->user = nullptr;
							u->netid = Usable::netid_counter++;
							L.local_ctx.usables->push_back(u);

							CollisionObject& c = Add1(L.local_ctx.colliders);
							btCollisionObject* cobj = new btCollisionObject;
							cobj->setCollisionShape(iron_vein->shape);
							cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_OBJECT);

							btTransform& tr = cobj->getWorldTransform();
							Vec3 pos2 = Vec3::TransformZero(*iron_vein->matrix);
							pos2 += pos;
							tr.setOrigin(ToVector3(pos2));
							tr.setRotation(btQuaternion(rot, 0, 0));

							c.pt = Vec2(pos2.x, pos2.z);
							c.w = iron_vein->size.x;
							c.h = iron_vein->size.y;
							if(NotZero(rot))
							{
								c.type = CollisionObject::RECTANGLE_ROT;
								c.rot = rot;
								c.radius = radius;
							}
							else
								c.type = CollisionObject::RECTANGLE;

							game->phy_world->addCollisionObject(cobj, CG_OBJECT);
						}
					}
#undef P
#undef S
				}
			}
		}
	}

	// generuj nowe obiekty
	if(!nowe.empty())
	{
		BaseObject* kamien = BaseObject::Get("rock"),
			*krzak = BaseObject::Get("plant2"),
			*grzyb = BaseObject::Get("mushrooms");

		for(vector<Int2>::iterator it = nowe.begin(), end = nowe.end(); it != end; ++it)
		{
			if(Rand() % 10 == 0)
			{
				BaseObject* obj;
				switch(Rand() % 3)
				{
				default:
				case 0:
					obj = kamien;
					break;
				case 1:
					obj = krzak;
					break;
				case 2:
					obj = grzyb;
					break;
				}

				L.SpawnObjectEntity(L.local_ctx, obj, Vec3(2.f*it->x + Random(0.1f, 1.9f), 0.f, 2.f*it->y + Random(0.1f, 1.9f)), Random(MAX_ANGLE));
			}
		}
	}

	// generuj jednostki
	bool ustaw = true;

	if(mine_state3 < State3::GeneratedInBuild && mine_state2 >= State2::InBuild)
	{
		ustaw = false;

		// szef g�rnik�w na wprost wej�cia
		Int2 pt = lvl.GetUpStairsFrontTile();
		int odl = 1;
		while(lvl.map[pt(lvl.w)].type == EMPTY && odl < 5)
		{
			pt += DirToPos(lvl.staircase_up_dir);
			++odl;
		}
		pt -= DirToPos(lvl.staircase_up_dir);

		L.SpawnUnitNearLocation(L.local_ctx, Vec3(2.f*pt.x + 1, 0, 2.f*pt.y + 1), *UnitData::Get("gornik_szef"), &Vec3(2.f*lvl.staircase_up.x + 1, 0, 2.f*lvl.staircase_up.y + 1), -2);

		// g�rnicy
		UnitData& gornik = *UnitData::Get("gornik");
		for(int i = 0; i < 10; ++i)
		{
			for(int j = 0; j < 15; ++j)
			{
				Int2 tile = cave->GetRandomTile();
				const Tile& p = lvl.At(tile);
				if(p.type == EMPTY && !IS_SET(p.flags, Tile::F_SECOND_TEXTURE))
				{
					L.SpawnUnitNearLocation(L.local_ctx, Vec3(2.f*tile.x + Random(0.4f, 1.6f), 0, 2.f*tile.y + Random(0.4f, 1.6f)), gornik, nullptr, -2);
					break;
				}
			}
		}
	}

	// ustaw jednostki
	if(!ustaw && mine_state3 >= State3::GeneratedInBuild)
	{
		UnitData* gornik = UnitData::Get("gornik"),
			*szef_gornikow = UnitData::Get("gornik_szef");
		for(vector<Unit*>::iterator it = L.local_ctx.units->begin(), end = L.local_ctx.units->end(); it != end; ++it)
		{
			Unit* u = *it;
			if(u->IsAlive())
			{
				if(u->data == gornik)
				{
					for(int i = 0; i < 10; ++i)
					{
						Int2 tile = cave->GetRandomTile();
						const Tile& p = lvl.At(tile);
						if(p.type == EMPTY && !IS_SET(p.flags, Tile::F_SECOND_TEXTURE))
						{
							L.WarpUnit(*u, Vec3(2.f*tile.x + Random(0.4f, 1.6f), 0, 2.f*tile.y + Random(0.4f, 1.6f)));
							break;
						}
					}
				}
				else if(u->data == szef_gornikow)
				{
					Int2 pt = lvl.GetUpStairsFrontTile();
					int odl = 1;
					while(lvl.map[pt(lvl.w)].type == EMPTY && odl < 5)
					{
						pt += DirToPos(lvl.staircase_up_dir);
						++odl;
					}
					pt -= DirToPos(lvl.staircase_up_dir);

					L.WarpUnit(*u, Vec3(2.f*pt.x + 1, 0, 2.f*pt.y + 1));
				}
			}
		}
	}

	switch(mine_state2)
	{
	case State2::None:
		mine_state3 = State3::GeneratedMine;
		break;
	case State2::InBuild:
		mine_state3 = State3::GeneratedInBuild;
		break;
	case State2::Built:
	case State2::CanExpand:
	case State2::InExpand:
		mine_state3 = State3::GeneratedBuilt;
		break;
	case State2::Expanded:
		mine_state3 = State3::GeneratedExpanded;
		break;
	case State2::FoundPortal:
		mine_state3 = State3::GeneratedPortal;
		break;
	default:
		assert(0);
		break;
	}

	return respawn_units;
}
