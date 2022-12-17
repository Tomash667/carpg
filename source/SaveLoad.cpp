#include "Pch.h"
#include "Game.h"

#include "Ability.h"
#include "AbilityPanel.h"
#include "AIController.h"
#include "AIManager.h"
#include "Arena.h"
#include "BitStreamFunc.h"
#include "Cave.h"
#include "City.h"
#include "CommandParser.h"
#include "Console.h"
#include "CraftPanel.h"
#include "CreateServerPanel.h"
#include "Electro.h"
#include "GameGui.h"
#include "GameMenu.h"
#include "GameMessages.h"
#include "GameResources.h"
#include "GameStats.h"
#include "Gui.h"
#include "InfoBox.h"
#include "Inventory.h"
#include "ItemHelper.h"
#include "Journal.h"
#include "Level.h"
#include "LevelGui.h"
#include "LevelPart.h"
#include "LoadingHandler.h"
#include "LoadScreen.h"
#include "LocationGeneratorFactory.h"
#include "MainMenu.h"
#include "MpBox.h"
#include "MultiplayerPanel.h"
#include "PlayerInfo.h"
#include "Quest.h"
#include "QuestManager.h"
#include "Quest_Contest.h"
#include "Quest_Secret.h"
#include "Quest_Tournament.h"
#include "Quest_Tutorial.h"
#include "SaveLoadPanel.h"
#include "ScriptManager.h"
#include "Team.h"
#include "Version.h"
#include "World.h"
#include "WorldMapGui.h"

#include <ParticleSystem.h>
#include <Render.h>
#include <RenderTarget.h>
#include <SceneManager.h>
#include <SoundManager.h>

enum SaveFlags
{
	SF_ONLINE = 1 << 0,
	SF_DEBUG = 1 << 2,
	SF_HARDCORE = 1 << 4,
	SF_ON_WORLDMAP = 1 << 5
};

int LOAD_VERSION;

//=================================================================================================
ActionResult Game::CanSaveGame() const
{
	if(!Any(gameState, GS_LEVEL, GS_WORLDMAP))
		return ActionResult::Ignore;

	delegate<bool(DialogBox*)> pred = [](DialogBox* dialog)
	{
		return dialog->name != "console" && dialog->name != "gameMenu" && dialog->name != "saveload" && dialog->name != "GetTextDialog";
	};
	if(gui->HaveDialog(pred))
		return ActionResult::Ignore;

	if(GS_MAIN_MENU || questMgr->questSecret->state == Quest_Secret::SECRET_FIGHT)
		return ActionResult::No;

	if(gameState == GS_WORLDMAP)
	{
		if(Any(world->GetState(), World::State::TRAVEL, World::State::ENCOUNTER))
			return ActionResult::No;
	}
	else
	{
		if(questMgr->questTutorial->inTutorial || arena->mode != Arena::NONE || questMgr->questContest->state >= Quest_Contest::CONTEST_STARTING
			|| questMgr->questTournament->GetState() != Quest_Tournament::TOURNAMENT_NOT_DONE)
			return ActionResult::No;
	}

	if(Net::IsOnline())
	{
		if(team->IsAnyoneAlive() && Net::IsServer())
			return ActionResult::Yes;
	}
	else if(pc->unit->IsAlive() || pc->unit->inArena != -1)
		return ActionResult::Yes;

	return ActionResult::No;
}

//=================================================================================================
ActionResult Game::CanLoadGame() const
{
	if(!Any(gameState, GS_MAIN_MENU, GS_LEVEL, GS_WORLDMAP))
		return ActionResult::Ignore;

	delegate<bool(DialogBox*)> pred = [](DialogBox* dialog)
	{
		return dialog->name != "console" && dialog->name != "gameMenu" && dialog->name != "saveload" && dialog->name != "GetTextDialog";
	};
	if(gui->HaveDialog(pred))
		return ActionResult::Ignore;

	if(Net::IsOnline())
	{
		if(Net::IsClient() || !Any(gameState, GS_LEVEL, GS_WORLDMAP))
			return ActionResult::No;
	}
	else if(hardcoreMode)
		return ActionResult::No;

	return ActionResult::Yes;
}

//=================================================================================================
bool Game::SaveGameSlot(int slot, cstring text)
{
	assert(InRange(slot, 1, SaveSlot::MAX_SLOTS));

	ActionResult actionResult = CanSaveGame();
	if(actionResult == ActionResult::Yes)
	{
		LocalString filename = Format(Net::IsOnline() ? "saves/multi/%d.sav" : "saves/single/%d.sav", slot);
		return SaveGameCommon(filename, slot, text);
	}
	else if(actionResult == ActionResult::No)
	{
		// can't save right now
		gui->SimpleDialog(Net::IsClient() ? txOnlyServerCanSave : txCantSaveGame, gameGui->saveload->visible ? gameGui->saveload : nullptr);
		return false;
	}
	else
		return false;
}

//=================================================================================================
void Game::SaveGameFilename(const string& name)
{
	string filename = "saves/";
	if(Net::IsOnline())
		filename += "multi/";
	else
		filename += "single/";
	filename += name;
	if(!EndsWith(filename, ".sav"))
		filename += ".sav";

	SaveGameCommon(filename.c_str(), -1, nullptr);
}

//=================================================================================================
bool Game::SaveGameCommon(cstring filename, int slot, cstring text)
{
	io::CreateDirectory("saves");
	io::CreateDirectory(Net::IsOnline() ? "saves/multi" : "saves/single");

	CreateSaveImage();

	LocalString tmpFilename = Format("%s.new", filename);
	GameWriter f(tmpFilename);
	if(!f)
	{
		Error("Failed to save game '%s' (%d).", tmpFilename.c_str(), GetLastError());
		gui->SimpleDialog(txSaveFailed, gameGui->saveload->visible ? gameGui->saveload : nullptr);
		return false;
	}

	SaveSlot* ss = nullptr;
	if(slot != -1)
	{
		ss = &gameGui->saveload->GetSaveSlot(slot, Net::IsOnline());
		ss->text = text;
	}

	SaveGame(f, ss);
	f.Close();

	if(io::FileExists(filename))
		io::MoveFile(filename, Format("%s.bak", filename));
	io::MoveFile(tmpFilename, filename);

	cstring msg = Format("Game saved '%s'.", filename);
	gameGui->console->AddMsg(msg);
	Info(msg);
	if(slot != -1 && !Net::IsOnline())
		SetLastSave(slot);

	if(hardcoreMode)
	{
		Info("Hardcore mode, exiting to menu.");
		ExitToMenu();
		return false;
	}

	return true;
}

//=================================================================================================
void Game::CreateSaveImage()
{
	int old_flags = drawFlags;
	if(gameState == GS_LEVEL)
		drawFlags = (0xFFFFFFFF & ~DF_GUI & ~DF_MENU);
	else
		drawFlags = (0xFFFFFFFF & ~DF_MENU);
	render->SetRenderTarget(rtSave);
	DrawGame();
	render->SetRenderTarget(nullptr);
	drawFlags = old_flags;
}

//=================================================================================================
void Game::LoadGameSlot(int slot)
{
	assert(InRange(slot, 1, SaveSlot::MAX_SLOTS));
	bool online = (net->mpLoad || Net::IsServer());
	cstring filename = Format(online ? "saves/multi/%d.sav" : "saves/single/%d.sav", slot);
	LoadGameCommon(filename, slot);
}

//=================================================================================================
void Game::LoadGameFilename(const string& name)
{
	string filename = "saves/";
	if(Net::IsOnline())
		filename += "multi/";
	else
		filename += "single/";
	filename += name;
	if(!EndsWith(name, ".sav"))
		filename += ".sav";

	return LoadGameCommon(filename.c_str(), -1);
}

//=================================================================================================
void Game::SetLastSave(int slot)
{
	if(slot != lastSave && !Net::IsOnline())
	{
		lastSave = slot;
		cfg.Add("lastSave", lastSave);
		SaveCfg();
	}
}

//=================================================================================================
void Game::LoadGameCommon(cstring filename, int slot)
{
	GameReader f(filename);
	if(!f)
	{
		DWORD lastError = GetLastError();
		throw SaveException(Format(txLoadOpenError, filename, lastError), Format("Failed to open file '%s' (%d).", filename, lastError),
			lastError == ERROR_FILE_NOT_FOUND);
	}

	Info("Loading save '%s'.", filename);

	net->mpQuickload = (Net::IsOnline() && Any(gameState, GS_LEVEL, GS_WORLDMAP));
	if(net->mpQuickload)
	{
		bool onWorldmap;
		try
		{
			onWorldmap = ValidateNetSaveForLoading(f, slot);
		}
		catch(SaveException& ex)
		{
			throw SaveException(Format(txCantLoadMultiplayer, ex.localizedMsg), Format("Can't quick load mp game: %s", ex.msg));
		}

		BitStreamWriter f;
		f << ID_LOADING;
		f << onWorldmap;
		net->SendAll(f, IMMEDIATE_PRIORITY, RELIABLE);

		net->mpLoad = true;
		net->ClearChanges();
	}

	prevGameState = gameState;
	if(net->mpLoad && !net->mpQuickload)
		gameGui->multiplayer->visible = false;
	else
	{
		gameGui->mainMenu->visible = false;
		gameGui->levelGui->visible = false;
		gameGui->gameMenu->CloseDialog();
		gameGui->worldMap->Hide();
	}

	try
	{
		inLoad = true;
		LoadGame(f);
		inLoad = false;
	}
	catch(const SaveException&)
	{
		inLoad = false;
		prevGameState = GS_LOAD;
		ExitToMenu();
		throw;
	}
	catch(cstring msg)
	{
		inLoad = false;
		prevGameState = GS_LOAD;
		ExitToMenu();
		throw SaveException(nullptr, msg);
	}

	f.Close();
	prevGameState = GS_LOAD;

	if(hardcoreMode)
	{
		Info("Hardcore mode, deleting save.");

		if(slot != -1)
		{
			gameGui->saveload->RemoveHardcoreSave(slot);
			io::DeleteFile(Format(Net::IsOnline() ? "saves/multi/%d.sav" : "saves/single/%d.sav", slot));
			if(!Net::IsOnline())
				SetLastSave(-1);
		}
		else
			io::DeleteFile(filename);
	}
	else if(slot != -1 && !Net::IsOnline())
		SetLastSave(slot);

	if(net->mpQuickload)
	{
		for(PlayerInfo& info : net->players)
		{
			if(info.left == PlayerInfo::LEFT_NO)
				info.loaded = true;
		}
		gameState = GS_LOAD;
		gameLevel->ready = false;
		netMode = NM_TRANSFER_SERVER;
		netTimer = mpTimeout;
		netState = NetState::Server_Starting;
		gameGui->infoBox->Show("");
	}
	else if(!net->mpLoad)
	{
		if(gameState == GS_LEVEL)
		{
			gameGui->levelGui->visible = true;
			gameGui->worldMap->Hide();
		}
		else
		{
			gameGui->levelGui->visible = false;
			gameGui->worldMap->Show();
		}
		gameGui->Setup(pc);
		SetTitle("SINGLE");
	}
	else
	{
		gameGui->multiplayer->visible = true;
		gameGui->mainMenu->Show();
		gameGui->createServer->Show();
	}
}

//=================================================================================================
bool Game::ValidateNetSaveForLoading(GameReader& f, int slot)
{
	SaveSlot ss;
	if(!LoadGameHeader(f, ss))
		throw SaveException(txLoadSignature, "Invalid file signature.");
	f.SetPos(0);
	for(PlayerInfo& info : net->players)
	{
		bool ok = false;
		if(info.pc->name == ss.playerName)
			continue;
		for(string& str : ss.mpPlayers)
		{
			if(str == info.name)
			{
				ok = true;
				break;
			}
		}
		if(!ok)
			throw SaveException(Format(txMissingPlayerInSave, info.name.c_str()), Format("Missing player '%s' in save.", info.name.c_str()));
	}
	return ss.onWorldmap;
}

//=================================================================================================
void Game::SaveGame(GameWriter& f, SaveSlot* slot)
{
	Info("Saving...");

	// before saving update minimap, finish unit warps
	if(Net::IsOnline())
		net->ProcessLeftPlayers();
	gameLevel->UpdateDungeonMinimap(false);
	gameLevel->ProcessUnitWarps();
	gameLevel->ProcessRemoveUnits(false);

	// signature
	byte sign[4] = { 'C','R','S','V' };
	f << sign;

	// version
	f << VERSION;
	f << V_CURRENT;
	f << startVersion;
	f << content.version;

	// save flags
	byte flags = 0;
	if(Net::IsOnline())
		flags |= SF_ONLINE;
	if(IsDebug())
		flags |= SF_DEBUG;
	if(hardcoreMode)
		flags |= SF_HARDCORE;
	if(gameState == GS_WORLDMAP)
		flags |= SF_ON_WORLDMAP;
	f << flags;

	// info
	f << (slot ? slot->text : "");
	f << pc->name;
	f << pc->unit->GetClass()->id;
	if(Net::IsOnline())
	{
		f.WriteCasted<byte>(net->activePlayers - 1);
		for(PlayerInfo& info : net->players)
		{
			if(!info.pc->isLocal && info.left == PlayerInfo::LEFT_NO)
				f << info.pc->name;
		}
	}
	f << time(nullptr);
	f << world->GetDateValue();
	f << gameLevel->GetCurrentLocationText();
	if(slot)
	{
		slot->valid = true;
		slot->loadVersion = V_CURRENT;
		slot->playerName = pc->name;
		slot->playerClass = pc->unit->data->clas;
		slot->mpPlayers.clear();
		if(Net::IsOnline())
		{
			for(PlayerInfo& info : net->players)
			{
				if(!info.pc->isLocal && info.left == PlayerInfo::LEFT_NO)
					slot->mpPlayers.push_back(info.pc->name);
			}
		}
		slot->saveDate = time(nullptr);
		slot->gameDate = world->GetDateValue();
		slot->hardcore = hardcoreMode;
		slot->onWorldmap = (gameState == GS_WORLDMAP);
		slot->location = gameLevel->GetCurrentLocationText();
		slot->imgOffset = f.GetPos() + 4;
	}
	uint imgSize = app::render->SaveToFile(rtSave->tex, f);
	if(slot)
		slot->imgSize = imgSize;

	// ids
	f << ParticleEmitter::impl.idCounter;
	f << TrailParticleEmitter::impl.idCounter;
	f << Unit::impl.idCounter;
	f << GroundItem::impl.idCounter;
	f << Chest::impl.idCounter;
	f << Usable::impl.idCounter;
	f << Trap::impl.idCounter;
	f << Door::impl.idCounter;
	f << Electro::impl.idCounter;
	f << Bullet::impl.idCounter;

	gameStats->Save(f);

	f << gameState;
	aiMgr->Save(f);
	world->Save(f);

	byte checkId = 0;

	if(gameState == GS_LEVEL)
		f << (gameLevel->eventHandler ? gameLevel->eventHandler->GetLocationEventHandlerQuestId() : -1);
	else
		team->SaveOnWorldmap(f);
	f << gameLevel->enterFrom;
	f << gameLevel->lightAngle;

	// camera
	f << gameLevel->camera.realRot.y;
	f << gameLevel->camera.dist;
	f << gameLevel->camera.drunkAnim;

	// vars
	f << devmode;
	f << noai;
	f << dontWander;
	f << sceneMgr->useFog;
	f << sceneMgr->useLighting;
	f << drawParticleSphere;
	f << drawUnitRadius;
	f << drawHitbox;
	f << drawPhy;
	f << drawCol;
	f << gameSpeed;
	f << nextSeed;
	f << drawFlags;
	f << pc->unit->id;
	f << gameLevel->dungeonLevel;
	f << portalAnim;
	f << ais.size();
	for(AIController* ai : ais)
		ai->Save(f);
	f << gameLevel->boss;

	gameGui->Save(f);

	checkId = (byte)world->GetLocations().size();
	f << checkId++;

	// save team
	team->Save(f);

	// save quests
	questMgr->Save(f);
	scriptMgr->Save(f);

	f << checkId++;

	if(Net::IsOnline())
	{
		net->Save(f);

		f << checkId++;

		Net::PushChange(NetChange::GAME_SAVED);
		gameGui->mpBox->Add(txGameSaved);
	}

	f.Write("EOS", 3);
}

//=================================================================================================
bool Game::LoadGameHeader(GameReader& f, SaveSlot& slot)
{
	if(!f)
		return false;

	// signature
	byte sign[4] = { 'C','R','S','V' };
	byte sign2[4];
	f >> sign2;
	for(int i = 0; i < 4; ++i)
	{
		if(sign2[i] != sign[i])
			return false;
	}

	// version
	f.Skip<int>();

	// save version
	f >> slot.loadVersion;
	if(slot.loadVersion < MIN_SUPPORT_LOAD_VERSION)
		return false;

	// start version
	f.Skip<int>();

	// content version
	f.Skip<uint>();

	// info
	byte flags;
	f >> flags;
	slot.hardcore = IsSet(flags, SF_HARDCORE);
	slot.onWorldmap = IsSet(flags, SF_ON_WORLDMAP);
	f >> slot.text;
	f >> slot.playerName;
	slot.playerClass = Class::TryGet(f.ReadString1());
	if(IsSet(flags, SF_ONLINE))
		f.ReadStringArray<byte, byte>(slot.mpPlayers);
	else
		slot.mpPlayers.clear();
	f >> slot.saveDate;
	f >> slot.gameDate;
	f >> slot.location;
	f >> slot.imgSize;
	slot.imgOffset = f.GetPos();

	return true;
}

//=================================================================================================
void Game::LoadGame(GameReader& f)
{
	LoadingStart(8);
	ClearGame();
	ClearGameVars(false);
	StopAllSounds();
	questMgr->questTutorial->inTutorial = false;
	arena->Reset();
	pc->data.autowalk = false;
	aiBowTargets.clear();
	loadLocationQuest.clear();
	loadUnitHandler.clear();
	loadChestHandler.clear();
	unitsMeshLoad.clear();
	gameLevel->ready = false;

	byte checkId = 0, readId;

	// signature
	byte sign[4] = { 'C','R','S','V' };
	byte sign2[4];
	f >> sign2;
	for(int i = 0; i < 4; ++i)
	{
		if(sign2[i] != sign[i])
			throw SaveException(txLoadSignature, "Invalid file signature.");
	}

	// version
	int version;
	f >> version;
	if(version > VERSION)
	{
		cstring ver_str = VersionToString(version);
		throw SaveException(Format(txLoadVersion, ver_str, VERSION_STR), Format("Invalid file version %s, current %s.", ver_str, VERSION_STR));
	}

	// save version
	f >> LOAD_VERSION;
	if(LOAD_VERSION < MIN_SUPPORT_LOAD_VERSION)
	{
		cstring ver_str = VersionToString(version);
		throw SaveException(Format(txLoadSaveVersionOld, ver_str), Format("Unsupported version '%s'.", ver_str));
	}
	f >> startVersion;

	// content version
	uint contentVersion;
	f >> contentVersion;
	content.requireUpdate = (content.version != contentVersion);
	if(content.requireUpdate)
		Info("Loading old system version. Content update required.");

	// save flags
	byte flags;
	f >> flags;
	bool online_save = IsSet(flags, SF_ONLINE);
	if(net->mpLoad)
	{
		if(!online_save)
			throw SaveException(txLoadSP, "Save is from singleplayer mode.");
	}
	else
	{
		if(online_save)
			throw SaveException(txLoadMP, "Save is from multiplayer mode.");
	}

	Info("Loading save. Version %s, start %s, format %d, mp %d, debug %d.", VersionToString(version), VersionToString(startVersion), LOAD_VERSION,
		online_save ? 1 : 0, IsSet(flags, SF_DEBUG) ? 1 : 0);

	// info
	f.SkipString1(); // text
	f.SkipString1(); // player name
	f.SkipString1(); // player class
	if(net->mpLoad) // mp players
		f.SkipStringArray<byte, byte>();
	f.Skip<time_t>(); // saveDate
	f.Skip<int>(); // game_year
	f.Skip<int>(); // game_month
	f.Skip<int>(); // game_day
	f.SkipString1(); // location
	f.SkipData<uint>(); // image

	// ids
	if(LOAD_VERSION >= V_0_12)
	{
		f >> ParticleEmitter::impl.idCounter;
		f >> TrailParticleEmitter::impl.idCounter;
		f >> Unit::impl.idCounter;
		f >> GroundItem::impl.idCounter;
		f >> Chest::impl.idCounter;
		f >> Usable::impl.idCounter;
		f >> Trap::impl.idCounter;
		f >> Door::impl.idCounter;
		f >> Electro::impl.idCounter;
	}
	if(LOAD_VERSION >= V_0_16)
		f >> Bullet::impl.idCounter;

	LoadingHandler loading;
	GAME_STATE game_state2;
	hardcoreMode = IsSet(flags, SF_HARDCORE);

	gameStats->Load(f);

	// game state
	f >> game_state2;
	gameLevel->isOpen = game_state2 == GS_LEVEL;

	if(LOAD_VERSION >= V_0_17)
		aiMgr->Load(f);

	// world map
	LoadingStep(txLoadingLocations);
	world->Load(f, loading);

	int locationEventHandlerQuestId;
	if(game_state2 == GS_LEVEL)
		f >> locationEventHandlerQuestId;
	else
	{
		locationEventHandlerQuestId = -1;
		// load team
		uint count;
		f >> count;
		for(uint i = 0; i < count; ++i)
		{
			Unit* u = new Unit;
			u->Load(f);
			u->locPart = nullptr;
			u->CreateMesh(Unit::CREATE_MESH::ON_WORLDMAP);

			if(!u->IsPlayer())
			{
				u->ai = new AIController;
				u->ai->Init(u);
				ais.push_back(u->ai);
			}
		}
	}
	f >> gameLevel->enterFrom;
	f >> gameLevel->lightAngle;

	// apply entity requests
	LoadingStep(txLoadingData);
	Unit::ApplyRequests();
	Usable::ApplyRequests();

	// camera
	f >> gameLevel->camera.realRot.y;
	f >> gameLevel->camera.dist;
	if(LOAD_VERSION >= V_0_14)
		f >> gameLevel->camera.drunkAnim;
	gameLevel->camera.Reset();
	pc->data.rotBuf = 0.f;

	// vars
	f >> devmode;
	f >> noai;
	if(IsDebug())
		noai = true;
	f >> dontWander;
	f >> sceneMgr->useFog;
	f >> sceneMgr->useLighting;
	f >> drawParticleSphere;
	f >> drawUnitRadius;
	f >> drawHitbox;
	f >> drawPhy;
	f >> drawCol;
	f >> gameSpeed;
	f >> nextSeed;
	f >> drawFlags;
	Unit* player;
	f >> player;
	pc = player->player;
	if(!net->mpLoad)
		pc->id = 0;
	gameLevel->camera.target = pc->unit;
	gameLevel->camera.realRot.x = pc->unit->rot;
	pc->dialogCtx = &dialogContext;
	dialogContext.dialogMode = false;
	dialogContext.isLocal = true;
	f >> gameLevel->dungeonLevel;
	f >> portalAnim;
	if(LOAD_VERSION < V_0_14)
		f >> gameLevel->camera.drunkAnim;
	ais.resize(f.Read<uint>());
	for(AIController*& ai : ais)
	{
		ai = new AIController;
		ai->Load(f);
	}
	if(LOAD_VERSION >= V_0_17)
	{
		f >> gameLevel->boss;
		if(gameLevel->boss)
			gameGui->levelGui->SetBoss(gameLevel->boss, true);
	}

	gameGui->Load(f);

	checkId = (byte)world->GetLocations().size();
	if(LOAD_VERSION < V_0_14)
		--checkId; // added offscreen location to old saves
	f >> readId;
	if(readId != checkId++)
		throw "Error reading data before team.";

	// load team
	team->Load(f);

	// load quests
	LoadingStep(txLoadingQuests);
	questMgr->Load(f);

	scriptMgr->Load(f);

	f >> readId;
	if(readId != checkId++)
		throw "Error reading data after news.";

	LoadingStep(txLoadingLevel);

	if(game_state2 == GS_LEVEL)
	{
		LocationGenerator* locGen = locGenFactory->Get(gameLevel->location);
		locGen->OnLoad();

		if(LOAD_VERSION < V_0_11)
		{
			gameLevel->localPart->lvlPart->Load(f);

			f >> readId;
			if(readId != checkId++)
				throw "Failed to read level data.";
		}

		RemoveUnusedAiAndCheck();
	}

	// gui
	gameGui->levelGui->PositionPanels();

	// set ai bow targets
	if(!aiBowTargets.empty())
	{
		BaseObject* bow_target = BaseObject::Get("bow_target");
		for(vector<AIController*>::iterator it = aiBowTargets.begin(), end = aiBowTargets.end(); it != end; ++it)
		{
			AIController& ai = **it;
			Object* ptr = nullptr;
			float dist, best_dist;
			for(Object* obj : ai.unit->locPart->objects)
			{
				if(obj->base == bow_target)
				{
					dist = Vec3::Distance(obj->pos, ai.st.idle.pos);
					if(!ptr || dist < best_dist)
					{
						ptr = obj;
						best_dist = dist;
					}
				}
			}
			assert(ptr);
			ai.st.idle.obj.ptr = ptr;
		}
	}

	// location quests
	for(Location* loc : loadLocationQuest)
	{
		loc->activeQuest = static_cast<Quest_Dungeon*>(questMgr->FindAnyQuest((int)loc->activeQuest));
		assert(loc->activeQuest);
	}

	// unit event handlers
	for(Unit* unit : loadUnitHandler)
	{
		unit->eventHandler = dynamic_cast<UnitEventHandler*>(questMgr->FindAnyQuest((int)unit->eventHandler));
		assert(unit->eventHandler);
	}

	// chest event handlers
	for(Chest* chest : loadChestHandler)
	{
		chest->handler = dynamic_cast<ChestEventHandler*>(questMgr->FindAnyQuest((int)chest->handler));
		assert(chest->handler);
	}

	// current location event handler
	if(locationEventHandlerQuestId != -1)
	{
		Quest* quest = questMgr->FindAnyQuest(locationEventHandlerQuestId);
		if(quest->isNew)
			gameLevel->eventHandler = nullptr;
		else
		{
			gameLevel->eventHandler = dynamic_cast<LocationEventHandler*>(quest);
			assert(gameLevel->eventHandler);
		}
	}
	else
		gameLevel->eventHandler = nullptr;

	questMgr->ProcessQuestRequests();
	questMgr->UpgradeQuests();

	dialogContext.dialogMode = false;
	team->Clear(false);
	fallbackType = FALLBACK::NONE;
	fallbackTimer = -0.5f;
	gameGui->inventory->mode = I_NONE;
	gameGui->ability->Refresh();
	pc->data.beforePlayer = BP_NONE;
	pc->data.selectedUnit = pc->unit;
	dialogContext.pc = pc;
	dialogContext.dialogMode = false;

	if(net->mpLoad)
	{
		net->Load(f);

		f >> readId;
		if(readId != checkId++)
			throw "Failed to read multiplayer data.";
	}
	else
		pc->isLocal = true;

	// end of save
	char eos[3];
	f.Read(eos);
	if(eos[0] != 'E' || eos[1] != 'O' || eos[2] != 'S')
		throw "Missing EOS.";

	gameRes->LoadCommonMusic();

	LoadResources(txEndOfLoading, game_state2 == GS_WORLDMAP);
	if(game_state2 == GS_LEVEL)
	{
		for(LocationPart& locPart : gameLevel->ForEachPart())
			locPart.BuildScene();
	}
	if(!net->mpQuickload)
		gameGui->loadScreen->visible = false;

	Info("Game loaded.");

	if(net->mpLoad)
	{
		gameState = GS_MAIN_MENU;
		gameLevel->ready = false;
		return;
	}

	gameState = game_state2;
	if(gameState == GS_LEVEL)
	{
		gameLevel->ready = true;
		SetMusic();
		if(pc->unit->usable && pc->unit->action == A_USE_USABLE && Any(pc->unit->animationState, AS_USE_USABLE_USING, AS_USE_USABLE_USING_SOUND)
			&& IsSet(pc->unit->usable->base->useFlags, BaseUsable::ALCHEMY))
			gameGui->craft->Show();
	}
	else
	{
		SetMusic(MusicType::Travel);
		gameLevel->ready = false;
	}
}

//=================================================================================================
bool Game::TryLoadGame(int slot, bool quickload, bool fromConsole)
{
	try
	{
		LoadGameSlot(slot);
		return true;
	}
	catch(const SaveException& ex)
	{
		if(quickload && ex.missingFile)
		{
			if(fromConsole)
				gameGui->console->AddMsg("Missing quicksave.");
			else
				gui->SimpleDialog(txMissingQuicksave, nullptr);
			return false;
		}

		cstring msg = Format("Failed to load game: %s", ex.msg);
		Error(msg);
		if(fromConsole)
			gameGui->console->AddMsg(msg);
		else
		{
			cstring dialogText;
			if(ex.localizedMsg)
				dialogText = Format("%s%s", txLoadError, ex.localizedMsg);
			else
				dialogText = txLoadErrorGeneric;

			Control* parent = nullptr;
			if(gameGui->gameMenu->visible)
				parent = gameGui->gameMenu;
			gui->SimpleDialog(dialogText, parent, "fatal");
		}
		net->mpLoad = false;
		return false;
	}
}

//=================================================================================================
void Game::Quicksave()
{
	ActionResult actionResult = CanSaveGame();
	if(actionResult == ActionResult::Yes)
	{
		if(SaveGameSlot(SaveSlot::MAX_SLOTS, txQuickSave))
			gameGui->messages->AddGameMsg3(GMS_GAME_SAVED);
	}
	else if(actionResult == ActionResult::No)
		gui->SimpleDialog(Net::IsClient() ? txOnlyServerCanSave : txCantSaveNow, nullptr);
}

//=================================================================================================
void Game::Quickload()
{
	ActionResult actionResult = CanLoadGame();
	if(actionResult == ActionResult::Yes)
		TryLoadGame(SaveSlot::MAX_SLOTS, true, false);
	else if(actionResult == ActionResult::No)
		gui->SimpleDialog(Net::IsClient() ? txOnlyServerCanLoad : txCantLoadGame, nullptr);
}

//=================================================================================================
void Game::RemoveUnusedAiAndCheck()
{
	uint prev_size = ais.size();
	bool deleted = false;

	for(vector<AIController*>::iterator it = ais.begin(), end = ais.end(); it != end; ++it)
	{
		bool ok = false;
		for(LocationPart& locPart : gameLevel->ForEachPart())
		{
			for(Unit* unit : locPart.units)
			{
				if(unit->ai == *it)
				{
					ok = true;
					break;
				}
			}
			if(ok)
				break;
		}
		if(!ok)
		{
			delete *it;
			*it = nullptr;
			deleted = true;
		}
	}

	if(deleted)
	{
		RemoveNullElements(ais);
		ReportError(5, Format("Removed unused ais: %u.", prev_size - ais.size()));
	}

	if(IsDebug())
	{
		int errors = 0;
		for(LocationPart& locPart : gameLevel->ForEachPart())
			CheckUnitsAi(locPart, errors);
		if(errors)
			gameGui->messages->AddGameMsg(Format("CheckUnitsAi: %d errors!", errors), 10.f);
	}
}

//=================================================================================================
void Game::CheckUnitsAi(LocationPart& locPart, int& errors)
{
	for(vector<Unit*>::iterator it = locPart.units.begin(), end = locPart.units.end(); it != end; ++it)
	{
		Unit& u = **it;
		if(u.player && u.ai)
		{
			++errors;
			Error("Unit %s is player 0x%p and ai 0x%p.", u.data->id.c_str(), u.player, u.ai);
		}
		else if(u.player && u.hero)
		{
			++errors;
			Error("Unit %s is player 0x%p and hero 0x%p.", u.data->id.c_str(), u.player, u.hero);
		}
		else if(!u.player && !u.ai)
		{
			++errors;
			Error("Unit %s is neither player or ai.", u.data->id.c_str());
		}
	}
}
