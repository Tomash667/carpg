#include "Pch.h"
#include "Quest_Mine.h"

#include "AIController.h"
#include "Cave.h"
#include "CaveGenerator.h"
#include "Game.h"
#include "GameResources.h"
#include "Journal.h"
#include "Level.h"
#include "LevelPart.h"
#include "LocationHelper.h"
#include "Portal.h"
#include "QuestManager.h"
#include "World.h"
#include "Team.h"

const int PAYMENT = 750;
const int PAYMENT2 = 1500;

//=================================================================================================
void Quest_Mine::Start()
{
	type = Q_MINE;
	category = QuestCategory::Unique;
	dungeonLoc = -2;
	mineState = State::None;
	mineState2 = State2::None;
	mineState3 = State3::None;
	messenger = nullptr;
	days = 0;
	daysRequired = 0;
	persuaded = false;
	startLoc = world->GetRandomSettlement(questMgr->GetUsedCities());
	targetLoc = world->GetClosestLocation(L_CAVE, startLoc->pos);
	questMgr->AddQuestRumor(id, Format(questMgr->txRumorQ[1], GetStartLocationName()));

	if(game->devmode)
		Info("Quest 'Mine' - %s, %s.", GetStartLocationName(), GetTargetLocationName());
}

//=================================================================================================
GameDialog* Quest_Mine::GetDialog(int type2)
{
	if(type2 == QUEST_DIALOG_NEXT)
	{
		if(DialogContext::current->talker->data->id == "qMineInvestor")
			return GameDialog::TryGet("qMineInvestor");
		else if(DialogContext::current->talker->data->id == "qMineMessenger")
		{
			if(prog == Quest_Mine::Progress::SelectedShares)
				return GameDialog::TryGet("qMineMessenger");
			else if(prog == Quest_Mine::Progress::GotFirstGold || prog == Quest_Mine::Progress::SelectedGold)
			{
				if(days >= daysRequired)
					return GameDialog::TryGet("qMineMessenger2");
				else
					return GameDialog::TryGet("qMineMessengerTalked");
			}
			else if(prog == Quest_Mine::Progress::Invested)
			{
				if(days >= daysRequired)
					return GameDialog::TryGet("qMineMessenger3");
				else
					return GameDialog::TryGet("qMineMessengerTalked");
			}
			else if(prog == Quest_Mine::Progress::UpgradedMine)
			{
				if(days >= daysRequired)
					return GameDialog::TryGet("qMineMessenger4");
				else
					return GameDialog::TryGet("qMineMessengerTalked");
			}
			else
				return GameDialog::TryGet("qMineMessengerTalked");
		}
		else
			return GameDialog::TryGet("qMineBoss");
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
			OnStart(questMgr->txQuest[131]);

			locationEventHandler = this;

			atLevel = 0;
			targetLoc->activeQuest = this;
			targetLoc->SetKnown();
			if(targetLoc->state >= LS_VISITED)
				targetLoc->reset = true;
			targetLoc->st = 10;

			InitSub();

			msgs.push_back(Format(questMgr->txQuest[132], startLoc->name.c_str(), world->GetDate()));
			msgs.push_back(Format(questMgr->txQuest[133], targetLoc->name.c_str(), GetTargetLocationDir()));
		}
		break;
	case Progress::ClearedLocation:
		{
			OnUpdate(questMgr->txQuest[134]);
			team->AddExp(3000);
		}
		break;
	case Progress::SelectedShares:
		{
			OnUpdate(questMgr->txQuest[135]);
			mineState = State::Shares;
			mineState2 = State2::InBuild;
			days = 0;
			daysRequired = Random(30, 45);
			questMgr->RemoveQuestRumor(id);
		}
		break;
	case Progress::GotFirstGold:
		{
			state = Quest::Completed;
			OnUpdate(questMgr->txQuest[136]);
			team->AddReward(PAYMENT);
			team->AddInvestment(questMgr->txQuest[131], id, PAYMENT);
			mineState2 = State2::Built;
			days -= daysRequired;
			daysRequired = Random(60, 90);
			if(days >= daysRequired)
				days = daysRequired - 1;
			targetLoc->SetImage(LI_MINE);
			targetLoc->SetNamePrefix(questMgr->txQuest[131]);
		}
		break;
	case Progress::SelectedGold:
		{
			state = Quest::Completed;
			OnUpdate(questMgr->txQuest[137]);
			team->AddReward(3000);
			mineState2 = State2::InBuild;
			days = 0;
			daysRequired = Random(30, 45);
			questMgr->RemoveQuestRumor(id);
		}
		break;
	case Progress::NeedTalk:
		{
			state = Quest::Started;
			OnUpdate(questMgr->txQuest[138]);
			mineState2 = State2::CanExpand;
			world->AddNews(Format(questMgr->txQuest[139], GetTargetLocationName()));
		}
		break;
	case Progress::Talked:
		{
			OnUpdate(Format(questMgr->txQuest[140], mineState == State::Shares ? 10000 : 12000));
		}
		break;
	case Progress::NotInvested:
		{
			state = Quest::Completed;
			OnUpdate(questMgr->txQuest[141]);
			questMgr->EndUniqueQuest();
		}
		break;
	case Progress::Invested:
		{
			DialogContext::current->pc->unit->ModGold(mineState == State::Shares ? -10000 : -12000);
			OnUpdate(questMgr->txQuest[142]);
			mineState2 = State2::InExpand;
			days = 0;
			daysRequired = Random(30, 45);
		}
		break;
	case Progress::UpgradedMine:
		{
			state = Quest::Completed;
			OnUpdate(questMgr->txQuest[143]);
			team->AddReward(PAYMENT2);
			if(mineState == State::Shares)
				team->UpdateInvestment(id, PAYMENT2);
			else
				team->AddInvestment(questMgr->txQuest[131], id, PAYMENT2);
			mineState = State::BigShares;
			mineState2 = State2::Expanded;
			days -= daysRequired;
			daysRequired = Random(60, 90);
			if(days >= daysRequired)
				days = daysRequired - 1;
			world->AddNews(Format(questMgr->txQuest[144], GetTargetLocationName()));
		}
		break;
	case Progress::InfoAboutPortal:
		{
			state = Quest::Started;
			OnUpdate(questMgr->txQuest[145]);
			mineState2 = State2::FoundPortal;
			world->AddNews(Format(questMgr->txQuest[146], GetTargetLocationName()));
		}
		break;
	case Progress::TalkedWithMiner:
		{
			OnUpdate(questMgr->txQuest[147]);
			const Item* item = Item::Get("qMineKey");
			DialogContext::current->pc->unit->AddItem2(item, 1u, 1u);
		}
		break;
	case Progress::Finished:
		{
			state = Quest::Completed;
			OnUpdate(questMgr->txQuest[148]);
			questMgr->EndUniqueQuest();
			world->AddNews(questMgr->txQuest[149]);
			team->AddExp(10000);
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
	else if(str == "invest_price")
		return Format("%d", mineState == State::Shares ? 10000 : 12000);
	else if(str == "payment")
		return Format("%d", PAYMENT);
	else if(str == "payment2")
		return Format("%d", PAYMENT2);
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
bool Quest_Mine::Special(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "mine_persuaded") == 0)
		persuaded = true;
	else
		assert(0);
	return false;
}

//=================================================================================================
bool Quest_Mine::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "udzialy_w_kopalni") == 0 || persuaded)
		return mineState == State::Shares;
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
	f << dungeonLoc;
	f << mineState;
	f << mineState2;
	f << mineState3;
	f << days;
	f << daysRequired;
	f << messenger;
	f << persuaded;
}

//=================================================================================================
Quest::LoadResult Quest_Mine::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> sub.done;
	f >> dungeonLoc;
	f >> mineState;
	f >> mineState2;
	f >> mineState3;
	f >> days;
	f >> daysRequired;
	if(LOAD_VERSION < V_0_18)
	{
		int daysGold;
		f >> daysGold;
		if(mineState == State::Shares || mineState == State::BigShares)
			team->AddInvestment(questMgr->txQuest[131], id, mineState == State::Shares ? PAYMENT : PAYMENT2);
	}
	f >> messenger;
	if(LOAD_VERSION >= V_0_17)
		f >> persuaded;
	else
		persuaded = false;

	locationEventHandler = this;
	InitSub();

	return LoadResult::Ok;
}

//=================================================================================================
void Quest_Mine::InitSub()
{
	if(sub.done)
		return;

	ItemList& lis = ItemList::Get("ancientArmoryArmors");
	lis.Get(3, sub.itemToGive);
	sub.itemToGive[3] = Item::Get("angelskinArmor");
	sub.spawnItem = Quest_Event::Item_InChest;
	sub.targetLoc = dungeonLoc == -2 ? nullptr : world->GetLocation(dungeonLoc);
	sub.atLevel = 0;
	sub.chestEventHandler = this;
	nextEvent = &sub;
}

//=================================================================================================
int Quest_Mine::GenerateMine(CaveGenerator* caveGen, bool first)
{
	switch(mineState3)
	{
	case State3::None:
		break;
	case State3::GeneratedMine:
		if(mineState2 == State2::None)
			return 0;
		break;
	case State3::GeneratedInBuild:
		if(mineState2 <= State2::InBuild)
			return 0;
		break;
	case State3::GeneratedBuilt:
		if(mineState2 <= State2::Built)
			return 0;
		break;
	case State3::GeneratedExpanded:
		if(mineState2 <= State2::Expanded)
			return 0;
		break;
	case State3::GeneratedPortal:
		if(mineState2 <= State2::FoundPortal)
			return 0;
		break;
	default:
		assert(0);
		return 0;
	}

	Cave& cave = *(Cave*)gameLevel->location;
	cave.loadedResources = false;
	InsideLocationLevel& lvl = cave.GetLevelData();
	int updateFlags = 0;

	// remove old units & blood
	if(mineState3 <= State3::GeneratedMine && mineState2 >= State2::InBuild)
	{
		DeleteElements(cave.units);
		DeleteElements(game->ais);
		cave.units.clear();
		cave.bloods.clear();
		updateFlags |= LocationGenerator::PREVENT_RESPAWN_UNITS;
	}

	bool generateVeins = false;
	int goldChance, resize = 0;
	bool redraw = false;

	// at first entry just generate iron veins
	if(mineState3 == State3::None)
	{
		generateVeins = true;
		goldChance = 0;
	}

	// resize cave
	if(mineState2 >= State2::InBuild && mineState3 < State3::GeneratedBuilt)
	{
		generateVeins = true;
		goldChance = 0;
		++resize;
		redraw = true;
	}

	// more cave resize
	if(mineState2 >= State2::InExpand && mineState3 < State3::GeneratedExpanded)
	{
		generateVeins = true;
		goldChance = 4;
		++resize;
		redraw = true;
	}

	// resize if required
	vector<Int2> newTiles;
	for(int i = 0; i < resize; ++i)
	{
		for(int y = 1; y < lvl.h - 1; ++y)
		{
			for(int x = 1; x < lvl.w - 1; ++x)
			{
				if(lvl.map[x + y * lvl.w].type == WALL)
				{
#define A(xx,yy) lvl.map[x+(xx)+(y+(yy))*lvl.w].type
					if(Rand() % 2 == 0 && (!IsBlocking2(A(-1, 0)) || !IsBlocking2(A(1, 0)) || !IsBlocking2(A(0, -1)) || !IsBlocking2(A(0, 1)))
						&& (A(-1, -1) != ENTRY_PREV && A(-1, 1) != ENTRY_PREV && A(1, -1) != ENTRY_PREV && A(1, 1) != ENTRY_PREV))
					{
						newTiles.push_back(Int2(x, y));
					}
#undef A
				}
			}
		}

		for(vector<Int2>::iterator it = newTiles.begin(), end = newTiles.end(); it != end; ++it)
			lvl.map[it->x + it->y * lvl.w].type = EMPTY;
	}

	// generate portal
	if(mineState2 >= State2::FoundPortal && mineState3 < State3::GeneratedPortal)
	{
		generateVeins = true;
		goldChance = 7;
		redraw = true;

		// search for good position
		vector<Int2> goodPts;
		for(int y = 1; y < lvl.h - 5; ++y)
		{
			for(int x = 1; x < lvl.w - 5; ++x)
			{
				for(int h = 0; h < 5; ++h)
				{
					for(int w = 0; w < 5; ++w)
					{
						if(lvl.map[x + w + (y + h) * lvl.w].type != WALL)
							goto skip;
					}
				}

				goodPts.push_back(Int2(x, y));
			skip:
				;
			}
		}

		if(goodPts.empty())
		{
			if(lvl.prevEntryPt.x / 26 == 1)
			{
				if(lvl.prevEntryPt.y / 26 == 1)
					goodPts.push_back(Int2(1, 1));
				else
					goodPts.push_back(Int2(1, lvl.h - 6));
			}
			else
			{
				if(lvl.prevEntryPt.y / 26 == 1)
					goodPts.push_back(Int2(lvl.w - 6, 1));
				else
					goodPts.push_back(Int2(lvl.w - 6, lvl.h - 6));
			}
		}

		Int2 pt = goodPts[Rand() % goodPts.size()];

		// prepare room
		// BBZBB
		// B___B
		// Z___Z
		// B___B
		// BBZBB
		const Int2 blockades[] = {
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
		const Int2 usedTile[] = {
			Int2(2,0),
			Int2(0,2),
			Int2(4,2),
			Int2(2,4)
		};
		for(uint i = 0; i < countof(blockades); ++i)
		{
			Tile& p = lvl.map[(pt + blockades[i])(lvl.w)];
			p.type = BLOCKADE;
			p.flags = 0;
		}
		for(uint i = 0; i < countof(usedTile); ++i)
		{
			Tile& p = lvl.map[(pt + usedTile[i])(lvl.w)];
			p.type = USED;
			p.flags = 0;
		}

		// add entance - find nearest empty point
		const Int2 center(pt.x + 2, pt.y + 2);
		Int2 closest;
		int bestDist = 999;
		for(int y = 1; y < lvl.h - 1; ++y)
		{
			for(int x = 1; x < lvl.w - 1; ++x)
			{
				if(lvl.map[x + y * lvl.w].type == EMPTY)
				{
					int dist = Int2::Distance(Int2(x, y), center);
					if(dist < bestDist && dist > 2)
					{
						bestDist = dist;
						closest = Int2(x, y);
					}
				}
			}
		}

		// find path from point to room
		Int2 endPt;
		while(true)
		{
			if(abs(closest.x - center.x) > abs(closest.y - center.y))
			{
			byX:
				if(closest.x > center.x)
				{
					--closest.x;
					Tile& p = lvl.map[closest.x + closest.y * lvl.w];
					if(p.type == USED)
					{
						endPt = closest;
						break;
					}
					else if(p.type == BLOCKADE)
					{
						++closest.x;
						goto byY;
					}
					else
					{
						p.type = EMPTY;
						p.flags = 0;
						newTiles.push_back(closest);
					}
				}
				else
				{
					++closest.x;
					Tile& p = lvl.map[closest.x + closest.y * lvl.w];
					if(p.type == USED)
					{
						endPt = closest;
						break;
					}
					else if(p.type == BLOCKADE)
					{
						--closest.x;
						goto byY;
					}
					else
					{
						p.type = EMPTY;
						p.flags = 0;
						newTiles.push_back(closest);
					}
				}
			}
			else
			{
			byY:
				if(closest.y > center.y)
				{
					--closest.y;
					Tile& p = lvl.map[closest.x + closest.y * lvl.w];
					if(p.type == USED)
					{
						endPt = closest;
						break;
					}
					else if(p.type == BLOCKADE)
					{
						++closest.y;
						goto byX;
					}
					else
					{
						p.type = EMPTY;
						p.flags = 0;
						newTiles.push_back(closest);
					}
				}
				else
				{
					++closest.y;
					Tile& p = lvl.map[closest.x + closest.y * lvl.w];
					if(p.type == USED)
					{
						endPt = closest;
						break;
					}
					else if(p.type == BLOCKADE)
					{
						--closest.y;
						goto byX;
					}
					else
					{
						p.type = EMPTY;
						p.flags = 0;
						newTiles.push_back(closest);
					}
				}
			}
		}

		// set walls
		for(uint i = 0; i < countof(blockades); ++i)
		{
			Tile& p = lvl.map[(pt + blockades[i])(lvl.w)];
			p.type = WALL;
		}
		for(uint i = 0; i < countof(usedTile); ++i)
		{
			Tile& p = lvl.map[(pt + usedTile[i])(lvl.w)];
			p.type = WALL;
		}
		for(int y = 1; y < 4; ++y)
		{
			for(int x = 1; x < 4; ++x)
			{
				Tile& p = lvl.map[pt.x + x + (pt.y + y) * lvl.w];
				p.type = EMPTY;
				p.flags = Tile::F_SECOND_TEXTURE;
			}
		}
		Tile& p = lvl.map[endPt(lvl.w)];
		p.type = DOORS;
		p.flags = 0;

		// set room
		Room* room = Room::Get();
		room->target = RoomTarget::Portal;
		room->type = nullptr;
		room->pos = pt;
		room->size = Int2(5, 5);
		room->connected.clear();
		room->index = 0;
		room->group = -1;
		lvl.rooms.push_back(room);

		// doors
		{
			Object* o = new Object;
			o->mesh = gameRes->aDoorWall;
			o->pos = Vec3(float(endPt.x * 2) + 1, 0, float(endPt.y * 2) + 1);
			o->scale = 1;
			o->base = nullptr;
			cave.objects.push_back(o);

			// hack :3
			room = Room::Get();
			room->target = RoomTarget::Corridor;
			room->type = nullptr;
			lvl.rooms.push_back(room);

			if(IsBlocking(lvl.map[endPt.x - 1 + endPt.y * lvl.w].type))
			{
				o->rot = Vec3(0, 0, 0);
				if(endPt.y > center.y)
				{
					o->pos.z -= 0.8229f;
					lvl.At(endPt + Int2(0, 1)).room = 1;
				}
				else
				{
					o->pos.z += 0.8229f;
					lvl.At(endPt + Int2(0, -1)).room = 1;
				}
			}
			else
			{
				o->rot = Vec3(0, PI / 2, 0);
				if(endPt.x > center.x)
				{
					o->pos.x -= 0.8229f;
					lvl.At(endPt + Int2(1, 0)).room = 1;
				}
				else
				{
					o->pos.x += 0.8229f;
					lvl.At(endPt + Int2(-1, 0)).room = 1;
				}
			}

			Door* door = new Door;
			door->Register();
			door->pt = endPt;
			door->pos = o->pos;
			door->rot = o->rot.y;
			door->state = Door::Closed;
			door->locked = LOCK_MINE;
			cave.doors.push_back(door);
		}

		// torch
		{
			BaseObject* torch = BaseObject::Get("torch");
			Vec3 pos(2.f * (pt.x + 1), 0, 2.f * (pt.y + 1));

			switch(Rand() % 4)
			{
			case 0:
				pos.x += torch->r * 2;
				pos.z += torch->r * 2;
				break;
			case 1:
				pos.x += 6.f - torch->r * 2;
				pos.z += torch->r * 2;
				break;
			case 2:
				pos.x += torch->r * 2;
				pos.z += 6.f - torch->r * 2;
				break;
			case 3:
				pos.x += 6.f - torch->r * 2;
				pos.z += 6.f - torch->r * 2;
				break;
			}

			gameLevel->SpawnObjectEntity(cave, torch, pos, Random(MAX_ANGLE));
		}

		// portal
		{
			float rot;
			if(endPt.y == center.y)
			{
				if(endPt.x > center.x)
					rot = PI * 3 / 2;
				else
					rot = PI / 2;
			}
			else
			{
				if(endPt.y > center.y)
					rot = 0;
				else
					rot = PI;
			}

			rot = Clip(rot + PI);

			// object
			const Vec3 pos(2.f * pt.x + 5, 0, 2.f * pt.y + 5);
			gameLevel->SpawnObjectEntity(cave, BaseObject::Get("portal"), pos, rot);

			// destination location
			SingleInsideLocation* loc = new SingleInsideLocation;
			loc->activeQuest = this;
			loc->target = ANCIENT_ARMORY;
			loc->fromPortal = true;
			loc->name = game->txAncientArmory;
			loc->pos = Vec2(-999, -999);
			loc->group = UnitGroup::Get("golems");
			loc->st = 12;
			loc->type = L_DUNGEON;
			loc->image = LI_DUNGEON;
			loc->state = LS_HIDDEN;
			int locId = world->AddLocation(loc);
			dungeonLoc = locId;
			sub.targetLoc = loc;

			// portal info
			cave.portal = new Portal;
			cave.portal->atLevel = 0;
			cave.portal->index = 0;
			cave.portal->targetLoc = locId;
			cave.portal->rot = rot;
			cave.portal->nextPortal = nullptr;
			cave.portal->pos = pos;

			// destination portal info
			loc->portal = new Portal;
			loc->portal->atLevel = 0;
			loc->portal->index = 0;
			loc->portal->targetLoc = gameLevel->locationIndex;
			loc->portal->nextPortal = nullptr;
		}
	}

	if(!newTiles.empty())
		caveGen->RegenerateFlags();

	if(redraw && game->devmode)
		caveGen->DebugDraw();

	// generate veins
	if(generateVeins)
	{
		BaseUsable* ironVein = BaseUsable::Get("ironVein"),
			*goldVein = BaseUsable::Get("goldVein");

		// remove old veins
		if(mineState3 != State3::None)
			DeleteElements(cave.usables);
		if(!first)
		{
			gameLevel->RecreateObjects();
			updateFlags |= LocationGenerator::PREVENT_RECREATE_OBJECTS;
		}

		// add new
		for(int y = 1; y < lvl.h - 1; ++y)
		{
			for(int x = 1; x < lvl.w - 1; ++x)
			{
				if(Rand() % 3 == 0)
					continue;

#define P(xx,yy) !IsBlocking2(lvl.map[x-(xx)+(y+(yy))*lvl.w])
#undef S
#define S(xx,yy) lvl.map[x-(xx)+(y+(yy))*lvl.w].type == WALL

				if(lvl.map[x + y * lvl.w].type == EMPTY && Rand() % 3 != 0 && !IsSet(lvl.map[x + y * lvl.w].flags, Tile::F_SECOND_TEXTURE))
				{
					GameDirection dir = GDIR_INVALID;

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

					if(dir != GDIR_INVALID)
					{
						Vec3 pos(2.f * x + 1, 0, 2.f * y + 1);

						switch(dir)
						{
						case GDIR_DOWN:
							pos.z -= 0.9f;
							break;
						case GDIR_LEFT:
							pos.x -= 0.9f;
							break;
						case GDIR_UP:
							pos.z += 0.9f;
							break;
						case GDIR_RIGHT:
							pos.x += 0.9f;
							break;
						}

						float rot = Clip(DirToRot(dir) + PI);
						static float radius = max(ironVein->size.x, ironVein->size.y) * SQRT_2;

						Level::IgnoreObjects ignore = { 0 };
						ignore.ignoreBlocks = true;
						gameLevel->globalCol.clear();
						gameLevel->GatherCollisionObjects(cave, gameLevel->globalCol, pos, radius, &ignore);

						Box2d box(pos.x - ironVein->size.x, pos.z - ironVein->size.y, pos.x + ironVein->size.x, pos.z + ironVein->size.y);

						if(!gameLevel->Collide(gameLevel->globalCol, box, 0.f, rot))
						{
							Usable* u = new Usable;
							u->Register();
							u->pos = pos;
							u->rot = rot;
							u->base = (Rand() % 10 < goldChance ? goldVein : ironVein);
							cave.usables.push_back(u);

							CollisionObject& c = Add1(cave.lvlPart->colliders);
							btCollisionObject* cobj = new btCollisionObject;
							cobj->setCollisionShape(ironVein->shape);
							cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_USABLE);

							btTransform& tr = cobj->getWorldTransform();
							Vec3 pos2 = Vec3::TransformZero(*ironVein->matrix);
							pos2 += pos;
							tr.setOrigin(ToVector3(pos2));
							tr.setRotation(btQuaternion(rot, 0, 0));

							c.pos = pos2;
							c.w = ironVein->size.x;
							c.h = ironVein->size.y;
							if(NotZero(rot))
							{
								c.type = CollisionObject::RECTANGLE_ROT;
								c.rot = rot;
								c.radius = radius;
							}
							else
								c.type = CollisionObject::RECTANGLE;

							phyWorld->addCollisionObject(cobj, CG_USABLE);
						}
					}
#undef P
#undef S
				}
			}
		}
	}

	// generate new objects
	if(!newTiles.empty())
	{
		BaseObject* rock = BaseObject::Get("rock"),
			*plant = BaseObject::Get("plant2"),
			*mushrooms = BaseObject::Get("mushrooms");

		for(vector<Int2>::iterator it = newTiles.begin(), end = newTiles.end(); it != end; ++it)
		{
			if(Rand() % 10 == 0)
			{
				switch(Rand() % 3)
				{
				default:
				case 0:
					caveGen->GenerateDungeonObject(lvl, *it, rock);
					break;
				case 1:
					gameLevel->SpawnObjectEntity(cave, plant, Vec3(2.f * it->x + Random(0.1f, 1.9f), 0.f, 2.f * it->y + Random(0.1f, 1.9f)), Random(MAX_ANGLE));
					break;
				case 2:
					gameLevel->SpawnObjectEntity(cave, mushrooms, Vec3(2.f * it->x + Random(0.1f, 1.9f), 0.f, 2.f * it->y + Random(0.1f, 1.9f)), Random(MAX_ANGLE));
					break;
				}
			}
		}
	}

	// spawn units
	bool positionUnits = true;
	if(mineState3 < State3::GeneratedInBuild && mineState2 >= State2::InBuild)
	{
		positionUnits = false;

		// miner leader in front of entrance
		Int2 pt = lvl.GetPrevEntryFrontTile();
		int dist = 1;
		while(lvl.map[pt(lvl.w)].type == EMPTY && dist < 5)
		{
			pt += DirToPos(lvl.prevEntryDir);
			++dist;
		}
		pt -= DirToPos(lvl.prevEntryDir);
		const Vec3 lookAt(2.f * lvl.prevEntryPt.x + 1, 0, 2.f * lvl.prevEntryPt.y + 1);
		gameLevel->SpawnUnitNearLocation(cave, Vec3(2.f * pt.x + 1, 0, 2.f * pt.y + 1), *UnitData::Get("qMineLeader"), &lookAt, -2);

		// miners
		UnitData& miner = *UnitData::Get("miner");
		for(int i = 0; i < 10; ++i)
		{
			for(int j = 0; j < 15; ++j)
			{
				Int2 tile = cave.GetRandomTile();
				const Tile& p = lvl.At(tile);
				if(p.type == EMPTY && !IsSet(p.flags, Tile::F_SECOND_TEXTURE))
				{
					gameLevel->SpawnUnitNearLocation(cave, Vec3(2.f * tile.x + Random(0.4f, 1.6f), 0, 2.f * tile.y + Random(0.4f, 1.6f)), miner, nullptr, -2);
					break;
				}
			}
		}
	}

	// position units
	if(!positionUnits && mineState3 >= State3::GeneratedInBuild)
	{
		UnitData* miner = UnitData::Get("miner"),
			*minerLeader = UnitData::Get("qMineLeader");
		for(vector<Unit*>::iterator it = cave.units.begin(), end = cave.units.end(); it != end; ++it)
		{
			Unit* u = *it;
			if(u->IsAlive())
			{
				if(u->data == miner)
				{
					for(int i = 0; i < 10; ++i)
					{
						Int2 tile = cave.GetRandomTile();
						const Tile& p = lvl.At(tile);
						if(p.type == EMPTY && !IsSet(p.flags, Tile::F_SECOND_TEXTURE))
						{
							gameLevel->WarpUnit(*u, Vec3(2.f * tile.x + Random(0.4f, 1.6f), 0, 2.f * tile.y + Random(0.4f, 1.6f)));
							break;
						}
					}
				}
				else if(u->data == minerLeader)
				{
					Int2 pt = lvl.GetPrevEntryFrontTile();
					int dist = 1;
					while(lvl.map[pt(lvl.w)].type == EMPTY && dist < 5)
					{
						pt += DirToPos(lvl.prevEntryDir);
						++dist;
					}
					pt -= DirToPos(lvl.prevEntryDir);

					gameLevel->WarpUnit(*u, Vec3(2.f * pt.x + 1, 0, 2.f * pt.y + 1));
				}
			}
		}
	}

	// update state
	switch(mineState2)
	{
	case State2::None:
		mineState3 = State3::GeneratedMine;
		break;
	case State2::InBuild:
		mineState3 = State3::GeneratedInBuild;
		break;
	case State2::Built:
	case State2::CanExpand:
	case State2::InExpand:
		mineState3 = State3::GeneratedBuilt;
		break;
	case State2::Expanded:
		mineState3 = State3::GeneratedExpanded;
		break;
	case State2::FoundPortal:
		mineState3 = State3::GeneratedPortal;
		break;
	default:
		assert(0);
		break;
	}

	return updateFlags;
}

//=================================================================================================
void Quest_Mine::OnProgress(int d)
{
	if(mineState2 == State2::InBuild)
	{
		days += d;
		if(days >= daysRequired)
		{
			if(mineState == State::Shares)
			{
				// player invested in mine, inform him about finishing
				if(gameLevel->IsSafeSettlement() && game->gameState == GS_LEVEL)
				{
					Unit* u = gameLevel->SpawnUnitNearLocation(*team->leader->locPart, team->leader->pos, *UnitData::Get("qMineMessenger"), &team->leader->pos, -2, 2.f);
					if(u)
					{
						world->AddNews(Format(game->txMineBuilt, targetLoc->name.c_str()));
						messenger = u;
						u->OrderAutoTalk(true);
					}
				}
			}
			else
			{
				// player got gold, don't inform him
				world->AddNews(Format(game->txMineBuilt, targetLoc->name.c_str()));
				mineState2 = State2::Built;
				days -= daysRequired;
				daysRequired = Random(60, 90);
				if(days >= daysRequired)
					days = daysRequired - 1;
			}
		}
	}
	else if(mineState2 == State2::Built
		|| mineState2 == State2::InExpand
		|| mineState2 == State2::Expanded)
	{
		// mine is built/in expand/expanded
		// count time to news about expanding/finished expanding/found portal
		days += d;
		if(days >= daysRequired && gameLevel->IsSafeSettlement() && game->gameState == GS_LEVEL)
		{
			Unit* u = gameLevel->SpawnUnitNearLocation(*team->leader->locPart, team->leader->pos, *UnitData::Get("qMineMessenger"), &team->leader->pos, -2, 2.f);
			if(u)
			{
				messenger = u;
				u->OrderAutoTalk(true);
			}
		}
	}
}
