#pragma once

#include "Engine.h"
#include "Const.h"
#include "GameCommon.h"
#include "BitStreamFunc.h"
#include "Object.h"
#include "ConsoleCommands.h"
#include "Quest.h"
#include "Net.h"
#include "Building.h"
#include "Dialog.h"
#include "BaseTrap.h"
#include "SpawnGroup.h"
#include "BaseLocation.h"
#include "Useable.h"
#include "Spell.h"
#include "Door.h"
#include "Bullet.h"
#include "SpellEffects.h"
#include "GroundItem.h"
#include "Trap.h"
#include "ParticleSystem.h"
#include "GameKeys.h"
#include "SceneNode.h"
#include "QuadTree.h"
#include "Music.h"
#include "PlayerInfo.h"
#include "QuestManager.h"
#include "Camera.h"
#include "Config.h"

// gui
#include "MainMenu.h"
#include "Dialog2.h"
#include "Console.h"
#include "GameMenu.h"
#include "Options.h"
#include "SaveLoadPanel.h"
#include "GetTextDialog.h"
#include "GetNumberDialog.h"
#include "GameGui.h"
#include "WorldMapGui.h"
#include "CreateCharacterPanel.h"
#include "MultiplayerPanel.h"
#include "CreateServerPanel.h"
#include "PickServerPanel.h"
#include "ServerPanel.h"
#include "InfoBox.h"
#include "MpBox.h"
#include "LoadScreen.h"
#include "Controls.h"
#include "GameMessages.h"

// postacie
#include "Unit.h"
#include "HeroData.h"
#include "PlayerController.h"
#include "AIController.h"

// lokacje
#include "Location.h"
#include "OutsideLocation.h"
#include "City.h"
#include "Village.h"
#include "InsideLocation.h"
#include "SingleInsideLocation.h"
#include "MultiInsideLocation.h"
#include "CaveLocation.h"
#include "Camp.h"

//#define DRAW_LOCAL_PATH
#ifdef DRAW_LOCAL_PATH
#	ifndef _DEBUG
#		error "DRAW_LOCAL_PATH in release!"
#	endif
#endif

struct APoint
{
	int odleglosc, koszt, suma, stan;
	INT2 prev;

	inline bool IsLower(int suma2) const
	{
		return stan == 0 || suma2 < suma;
	}
};

//-----------------------------------------------------------------------------
// Tryb szybkiego uruchamiania gry
enum QUICKSTART
{
	QUICKSTART_NONE,
	QUICKSTART_SINGLE,
	QUICKSTART_HOST,
	QUICKSTART_JOIN_LAN,
	QUICKSTART_JOIN_IP
};

//-----------------------------------------------------------------------------
// Stan gry
enum GAME_STATE
{
	GS_MAIN_MENU,
	GS_WORLDMAP,
	GS_LEVEL,
	GS_LOAD
};

//-----------------------------------------------------------------------------
// Stan mapy œwiata
enum WORLDMAP_STATE
{
	WS_MAIN,
	WS_TRAVEL,
	WS_ENCOUNTER
};

struct SpeechBubble;
struct EntityInterpolator;

enum AllowInput
{
	ALLOW_NONE =		0,	// 00	
	ALLOW_KEYBOARD =	1,	// 01
	ALLOW_MOUSE =		2,	// 10
	ALLOW_INPUT =		3	// 11
};

inline int KeyAllowState(byte k)
{
	if(k > 7)
		return ALLOW_KEYBOARD;
	else if(k != 0)
		return ALLOW_MOUSE;
	else
		return ALLOW_NONE;
}

enum COLLISION_GROUP
{
	CG_WALL = 1<<8,
	CG_UNIT = 1<<9
};

enum BeforePlayer
{
	BP_NONE,
	BP_UNIT,
	BP_CHEST,
	BP_DOOR,
	BP_ITEM,
	BP_USEABLE
};

union BeforePlayerPtr
{
	Unit* unit;
	Chest* chest;
	Door* door;
	GroundItem* item;
	Useable* useable;
	void* any;
};

extern const float ATTACK_RANGE;
extern const VEC2 ALERT_RANGE;
extern const float PICKUP_RANGE;
extern const float ARROW_TIMER;
extern const float MIN_H;

struct AttachedSound
{
	FMOD::Channel* channel;
	Unit* unit;
};

COMPILE_ASSERT(sizeof(time_t) == sizeof(__int64));

struct UnitView
{
	Unit* unit;
	VEC3 last_pos;
	float time;
	bool valid;
};

enum GMS
{
	GMS_NEED_WEAPON = 1,
	GMS_NEED_KEY,
	GMS_NEED_LADLE,
	GMS_NEED_HAMMER,
	GMS_DONT_LOOT_FOLLOWER,
	GMS_JOURNAL_UPDATED,
	GMS_GATHER_TEAM,
	GMS_ADDED_RUMOR,
	GMS_UNLOCK_DOOR,
	GMS_CANT_DO,
	GMS_UNIT_BUSY,
	GMS_NOT_NOW,
	GMS_NOT_IN_COMBAT,
	GMS_IS_LOOTED,
	GMS_USED,
	GMS_DONT_LOOT_ARENA,
	GMS_NOT_LEADER,
	GMS_ONLY_LEADER_CAN_TRAVEL,
	GMS_NO_POTION,
	GMS_GAME_SAVED,
	GMS_NEED_PICKAXE,
	GMS_PICK_CHARACTER,
	GMS_ADDED_ITEM
};

struct UnitWarpData
{
	Unit* unit;
	int where;
};

#define FALLBACK_TRAIN 0
#define FALLBACK_REST 1
#define FALLBACK_ARENA 2
#define FALLBACK_ENTER 3
#define FALLBACK_EXIT 4
#define FALLBACK_CHANGE_LEVEL 5
#define FALLBACK_NONE 6
#define FALLBACK_ARENA_EXIT 7
#define FALLBACK_USE_PORTAL 8
#define FALLBACK_WAIT_FOR_WARP 9
#define FALLBACK_ARENA2 10
#define FALLBACK_CLIENT 11
#define FALLBACK_CLIENT2 12

enum InventoryMode
{
	I_NONE,
	I_INVENTORY,
	I_LOOT_BODY,
	I_LOOT_CHEST,
	I_TRADE,
	I_SHARE,
	I_GIVE
};

inline PlayerController::Action InventoryModeToActionRequired(InventoryMode imode)
{
	switch(imode)
	{
	case I_NONE:
	case I_INVENTORY:
		return PlayerController::Action_None;
	case I_LOOT_BODY:
		return PlayerController::Action_LootUnit;
	case I_LOOT_CHEST:
		return PlayerController::Action_LootChest;
	case I_TRADE:
		return PlayerController::Action_Trade;
	case I_SHARE:
		return PlayerController::Action_ShareItems;
	case I_GIVE:
		return PlayerController::Action_GiveItems;
	default:
		assert(0);
		return PlayerController::Action_None;
	}
}

struct TeamShareItem
{
	Unit* from, *to;
	const Item* item;
	int index, value, priority;
};

typedef bool (*BoolFunc)();

struct Encounter
{
	VEC2 pos;
	int szansa;
	float zasieg;
	bool dont_attack, timed;
	DialogEntry* dialog;
	SPAWN_GROUP grupa;
	cstring text;
	Quest_Encounter* quest; // tak naprawdê nie musi to byæ Quest_Encounter, mo¿e byæ zwyk³y Quest, chyba ¿e jest to czasowy encounter!
	LocationEventHandler* location_event_handler;
	// nowe pola
	BoolFunc check_func;

	// dla kompatybilnoœci ze starym kodem, ustawia tylko nowe pola
	Encounter() : check_func(nullptr)
	{

	}
};

struct QuestItemRequest
{
	const Item** item;
	string name;
	int quest_refid;
	vector<ItemSlot>* items;
	Unit* unit;
};

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

enum DRAW_FLAGS
{
	DF_TERRAIN = 1<<0,
	DF_OBJECTS = 1<<1,
	DF_UNITS = 1<<2,
	DF_PARTICLES = 1<<3,
	DF_SKYBOX =  1<<4,
	DF_BULLETS = 1<<5,
	DF_BLOOD = 1<<6,
	DF_ITEMS = 1<<7,
	DF_USEABLES = 1<<8,
	DF_TRAPS = 1<<9,
	DF_AREA = 1<<10,
	DF_EXPLOS = 1<<11,
	DF_LIGHTINGS = 1<<12,
	DF_PORTALS = 1<<13,
	DF_GUI = 1<<14,
	DF_MENU = 1<<15,
};

struct TutorialText
{
	cstring text;
	VEC3 pos;
	int state; // 0 - nie aktywny, 1 - aktywny, 2 - uruchomiony
	int id;
};

typedef fastdelegate::FastDelegate1<cstring> PrintMsgFunc;

struct EntityInterpolator
{
	static const int MAX_ENTRIES = 4;
	struct Entry
	{
		VEC3 pos;
		float rot;
		float timer;

		inline void operator = (const Entry& e)
		{
			pos = e.pos;
			rot = e.rot;
			timer = e.timer;
		}
	} entries[MAX_ENTRIES];
	int valid_entries;

	void Reset(const VEC3& pos, float rot);
	void Add(const VEC3& pos, float rot);
};

const float UNIT_VIEW_A = 0.2f;
const float UNIT_VIEW_B = 0.4f;
const int UNIT_VIEW_MUL = 5;

struct Terrain;

struct PostEffect
{
	int id;
	D3DXHANDLE tech;
	float power;
	VEC4 skill;
};

enum SuperShaderSwitches
{
	SSS_ANIMATED,
	SSS_HAVE_BINORMALS,
	SSS_FOG,
	SSS_SPECULAR,
	SSS_NORMAL,
	SSS_POINT_LIGHT,
	SSS_DIR_LIGHT
};

struct SuperShader
{
	ID3DXEffect* e;
	uint id;
};

class CityGenerator;

class Quest_Sawmill;
class Quest_Mine;
class Quest_Bandits;
class Quest_Mages;
class Quest_Mages2;
class Quest_Orcs;
class Quest_Orcs2;
class Quest_Goblins;
class Quest_Evil;
class Quest_Crazies;

enum StreamLogType
{
	Stream_PickServer,
	Stream_PingIp,
	Stream_Connect,
	Stream_Quitting,
	Stream_QuittingServer,
	Stream_Transfer,
	Stream_TransferServer,
	Stream_ServerSend,
	Stream_UpdateLobbyServer,
	Stream_UpdateLobbyClient,
	Stream_UpdateGameServer,
	Stream_UpdateGameClient
};

enum class AnyVarType
{
	Bool
};

union AnyVar
{
	bool _bool;
};

struct ConfigVar
{
	cstring name;
	AnyVarType type;
	AnyVar* ptr;
	AnyVar new_value;
	bool have_new_value, need_save;

	ConfigVar(cstring name, bool& _bool) : name(name), type(AnyVarType::Bool), ptr((AnyVar*)&_bool), have_new_value(false), need_save(false) {}
};

struct Game : public Engine, public UnitEventHandler
{
	Game();
	~Game();	
	
	void OnCleanup();
	void OnDraw();
	void OnDraw(bool normal=true);
	void OnTick(float dt);
	void OnChar(char c);
	void OnReload();
	void OnReset();
	void OnResize();
	void OnFocus(bool focus);

	bool Start0(bool fullscreen, int w, int h);
	void GetTitle(LocalString& s);
	void ChangeTitle();
	void ClearPointers();
	void CreateTextures();
	void PreloadData();
	void SetMeshSpecular();

	// initialization
	void InitGame();
	void PreconfigureGame();
	void PreloadLanguage();

	// loading system
	void LoadSystem();
	void AddFilesystem();
	void LoadDatafiles();
	void LoadRequiredStats();
	void LoadLanguageFiles();
	void SetHeroNames();
	void SetGameText();
	void SetStatsText();
	void ConfigureGame();
	
	// loading data
	void LoadData();
	void AddLoadTasks();

	// after loading data
	void AfterLoadData();
	void StartGameMode();

	QUICKSTART quickstart;
	HANDLE mutex;

	// supershader
	string sshader_code;
	FILETIME sshader_edit_time;
	ID3DXEffectPool* sshader_pool;
	vector<SuperShader> sshaders;
	D3DXHANDLE hSMatCombined, hSMatWorld, hSMatBones, hSTint, hSAmbientColor, hSFogColor, hSFogParams, hSLightDir, hSLightColor, hSLights, hSSpecularColor, hSSpecularIntensity,
		hSSpecularHardness, hSCameraPos, hSTexDiffuse, hSTexNormal, hSTexSpecular;
	void InitSuperShader();
	SuperShader* GetSuperShader(uint id);
	SuperShader* CompileSuperShader(uint id);
	inline uint GetSuperShaderId(bool animated, bool have_binormals, bool fog, bool specular, bool normal, bool point_light, bool dir_light) const
	{
		uint id = 0;
		if(animated)
			id |= (1<<SSS_ANIMATED);
		if(have_binormals)
			id |= (1<<SSS_HAVE_BINORMALS);
		if(fog)
			id |= (1<<SSS_FOG);
		if(specular)
			id |= (1<<SSS_SPECULAR);
		if(normal)
			id |= (1<<SSS_NORMAL);
		if(point_light)
			id |= (1<<SSS_POINT_LIGHT);
		if(dir_light)
			id |= (1<<SSS_DIR_LIGHT);
		return id;
	}

	float light_angle;
	bool dungeon_tex_wrap;

	void SetupSuperShader();
	void ReloadShaders();
	void ReleaseShaders();
	void ShaderVersionChanged();

	// scene
	bool cl_normalmap, cl_specularmap, cl_glow;
	bool r_alphatest, r_nozwrite, r_nocull, r_alphablend;
	DrawBatch draw_batch;
	VDefault blood_v[4];
	VParticle billboard_v[4];
	VEC3 billboard_ext[4];
	VParticle portal_v[4];
	IDirect3DVertexDeclaration9* vertex_decl[VDI_MAX];
	int uv_mod;
	QuadTree quadtree;
	float grass_range;
	LevelParts level_parts;
	VB vbInstancing;
	uint vb_instancing_max;
	vector< const vector<MATRIX>* > grass_patches[2];
	uint grass_count[2];
	float lights_dt;

	void InitScene();
	void CreateVertexDeclarations();
	void BuildDungeon();
	void ChangeDungeonTexWrap();
	void FillDungeonPart(INT2* dungeon_part, word* faces, int& index, word offset);
	void CleanScene();
	void ListDrawObjects(LevelContext& ctx, FrustumPlanes& frustum, bool outside);
	void ListDrawObjectsUnit(LevelContext* ctx, FrustumPlanes& frustum, bool outside, Unit& u);
	void FillDrawBatchDungeonParts(FrustumPlanes& frustum);
	void AddOrSplitSceneNode(SceneNode* node, int exclude_subs=0);
	int GatherDrawBatchLights(LevelContext& ctx, SceneNode* node, float x, float z, float radius, int sub=0);
	void DrawScene(bool outside);
	void DrawGlowingNodes(bool use_postfx);
	void DrawSkybox();
	void DrawTerrain(const vector<uint>& parts);
	void DrawDungeon(const vector<DungeonPart>& parts, const vector<Lights>& lights, const vector<NodeMatrix>& matrices);
	void DrawSceneNodes(const vector<SceneNode*>& nodes, const vector<Lights>& lights, bool outside);
	void DrawDebugNodes(const vector<DebugSceneNode*>& nodes);
	void DrawBloods(bool outside, const vector<Blood*>& bloods, const vector<Lights>& lights);
	void DrawBillboards(const vector<Billboard>& billboards);
	void DrawExplosions(const vector<Explo*>& explos);
	void DrawParticles(const vector<ParticleEmitter*>& pes);
	void DrawTrailParticles(const vector<TrailParticleEmitter*>& tpes);
	void DrawLightings(const vector<Electro*>& electros);
	void DrawAreas(const vector<Area>& areas, float range);
	void DrawPortals(const vector<Portal*>& portals);
	inline void SetAlphaTest(bool use_alphatest)
	{
		if(use_alphatest != r_alphatest)
		{
			r_alphatest = use_alphatest;
			V( device->SetRenderState(D3DRS_ALPHATESTENABLE, r_alphatest ? TRUE : FALSE) );
		}
	}
	inline void SetNoZWrite(bool use_nozwrite)
	{
		if(use_nozwrite != r_nozwrite)
		{
			r_nozwrite = use_nozwrite;
			V( device->SetRenderState(D3DRS_ZWRITEENABLE, r_nozwrite ? FALSE : TRUE) );
		}
	}
	inline void SetNoCulling(bool use_nocull)
	{
		if(use_nocull != r_nocull)
		{
			r_nocull = use_nocull;
			V( device->SetRenderState(D3DRS_CULLMODE, r_nocull ? D3DCULL_NONE : D3DCULL_CCW) );
		}
	}
	inline void SetAlphaBlend(bool use_alphablend)
	{
		if(use_alphablend != r_alphablend)
		{
			r_alphablend = use_alphablend;
			V( device->SetRenderState(D3DRS_ALPHABLENDENABLE, r_alphablend ? TRUE : FALSE) );
		}
	}
	void UvModChanged();
	void InitQuadTree();
	void DrawGrass();
	void ListGrass();
	void SetTerrainTextures();
	void ClearQuadtree();
	void ClearGrass();
	void VerifyObjects();
	void VerifyObjects(vector<Object>& objects, int& errors);

	// profiler
	int profiler_mode;

	//-----------------------------------------------------------------
	// ZASOBY
	//-----------------------------------------------------------------
	Animesh* aHumanBase, *aHair[5], *aBeard[5], *aMustache[2], *aEyebrows;
	Animesh* aBox, *aCylinder, *aSphere, *aCapsule;
	Animesh* aArrow, *aSkybox, *aWorek, *aSkrzynia, *aKratka, *aNaDrzwi, *aNaDrzwi2, *aSchodyDol, *aSchodyGora, *aSchodyDol2, *aSpellball, *aPrzycisk, *aBeczka, *aDrzwi, *aDrzwi2;
	VertexData* vdSchodyGora, *vdSchodyDol, *vdNaDrzwi;
	TEX tItemRegion, tMinimap, tChar, tSave;
	TEX tCzern, tEmerytura, tPortal, tLightingLine, tKlasaCecha, tRip, tCelownik, tObwodkaBolu, tEquipped,
		tDialogUp, tDialogDown, tBubble, tMiniunit, tMiniunit2, tSchodyDol, tSchodyGora, tIcoHaslo, tIcoZapis, tGotowy, tNieGotowy, tTrawa, tTrawa2, tTrawa3, tZiemia,
		tDroga, tMiniSave, tMiniunit3, tMiniunit4, tMiniunit5, tMinibag, tMinibag2, tMiniportal, tPole;
	TextureResourcePtr tKrew[BLOOD_MAX], tKrewSlad[BLOOD_MAX], tFlare, tFlare2, tIskra, tWoda;
	TexturePack tFloor[2], tWall[2], tCeil[2], tFloorBase, tWallBase, tCeilBase;
	ID3DXEffect* eMesh, *eParticle, *eSkybox, *eTerrain, *eArea, *eGui, *ePostFx, *eGlow, *eGrass;
	D3DXHANDLE techAnim, techHair, techAnimDir, techHairDir, techMesh, techMeshDir, techMeshSimple, techMeshSimple2, techMeshExplo, techParticle, techSkybox, techTerrain, techArea, techTrail,
		techGui, techGlowMesh, techGlowAni, techGrass;
	D3DXHANDLE hAniCombined, hAniWorld, hAniBones, hAniTex, hAniFogColor, hAniFogParam, hAniTint, hAniHairColor, hAniAmbientColor, hAniLightDir, hAniLightColor, hAniLights,
		hMeshCombined, hMeshWorld, hMeshTex, hMeshFogColor, hMeshFogParam, hMeshTint, hMeshAmbientColor, hMeshLightDir, hMeshLightColor, hMeshLights,
		hParticleCombined, hParticleTex, hSkyboxCombined, hSkyboxTex, hAreaCombined, hAreaColor, hAreaPlayerPos, hAreaRange,
		hTerrainCombined, hTerrainWorld, hTerrainTexBlend, hTerrainTex[5], hTerrainColorAmbient, hTerrainColorDiffuse, hTerrainLightDir, hTerrainFogColor, hTerrainFogParam,
		hGuiSize, hGuiTex, hPostTex, hPostPower, hPostSkill, hGlowCombined, hGlowBones, hGlowColor, hGrassViewProj, hGrassTex, hGrassFogColor, hGrassFogParams, hGrassAmbientColor;
	SOUND sGulp, sCoins, sBow[2], sDoor[3], sDoorClosed[2], sDoorClose, sItem[8], sTalk[4], sChestOpen, sChestClose, sDoorBudge, sRock, sWood, sCrystal,
		sMetal, sBody[5], sBone, sSkin, sArenaFight, sArenaWin, sArenaLost, sUnlock, sEvil, sXarTalk, sOrcTalk, sGoblinTalk, sGolemTalk, sEat;
	VB vbParticle;
	SURFACE sChar, sSave, sItemRegion;
	static cstring txGoldPlus, txQuestCompletedGold;
	cstring txCreateListOfFiles, txLoadItemsDatafile, txLoadMusicDatafile, txLoadLanguageFiles, txLoadShaders, txConfigureGame, txLoadGuiTextures,
		txLoadTerrainTextures, txLoadParticles, txLoadPhysicMeshes, txLoadModels, txLoadBuildings, txLoadTraps, txLoadSpells, txLoadObjects, txLoadUnits,
		txLoadItems, txLoadSounds, txLoadMusic, txGenerateWorld, txInitQuests, txLoadUnitDatafile, txLoadSpellDatafile, txLoadRequires;
	cstring txAiNoHpPot[2], txAiJoinTour[4], txAiCity[2], txAiVillage[2], txAiMoonwell, txAiForest, txAiCampEmpty, txAiCampFull, txAiFort, txAiDwarfFort, txAiTower, txAiArmory, txAiHideout,
		txAiVault, txAiCrypt, txAiTemple, txAiNecromancerBase, txAiLabirynth, txAiNoEnemies, txAiNearEnemies, txAiCave, txAiInsaneText[11], txAiDefaultText[9], txAiOutsideText[3],
		txAiInsideText[2], txAiHumanText[2], txAiOrcText[7], txAiGoblinText[5], txAiMageText[4], txAiSecretText[3], txAiHeroDungeonText[4], txAiHeroCityText[5], txAiBanditText[6],
		txAiHeroOutsideText[2], txAiDrunkMageText[3], txAiDrunkText[5], txAiDrunkmanText[4];
	cstring txRandomEncounter, txCamp;
	cstring txEnteringLocation, txGeneratingMap, txGeneratingBuildings, txGeneratingObjects, txGeneratingUnits, txGeneratingItems, txGeneratingPhysics, txRecreatingObjects, txGeneratingMinimap,
		txLoadingComplete, txWaitingForPlayers, txGeneratingTerrain;
	cstring txContestNoWinner, txContestStart, txContestTalk[14], txContestWin, txContestWinNews, txContestDraw, txContestPrize, txContestNoPeople;
	cstring txTut[10], txTutNote, txTutLoc, txTour[23], txTutPlay, txTutTick;
	cstring txCantSaveGame, txSaveFailed, txSavedGameN, txLoadFailed, txQuickSave, txGameSaved, txLoadingLocations, txLoadingData, txLoadingQuests, txEndOfLoading, txCantSaveNow, txCantLoadGame,
		txLoadSignature, txLoadVersion, txLoadSaveVersionNew, txLoadSaveVersionOld, txLoadMP, txLoadSP, txLoadError, txLoadOpenError;
	cstring txPvpRefuse, txSsFailed, txSsDone, txWin, txWinMp, txINeedWeapon, txNoHpp, txCantDo, txDontLootFollower, txDontLootArena, txUnlockedDoor,
		txNeedKey, txLevelUp, txLevelDown, txLocationText, txLocationTextMap, txRegeneratingLevel, txGmsLooted, txGmsRumor, txGmsJournalUpdated, txGmsUsed,
		txGmsUnitBusy, txGmsGatherTeam, txGmsNotLeader, txGmsNotInCombat, txGainTextAttrib, txGainTextSkill, txNeedLadle, txNeedPickaxe, txNeedHammer,
		txNeedUnk, txReallyQuit, txSecretAppear, txGmsAddedItem, txGmsAddedItems;
	cstring txRumor[28], txRumorD[7];
	cstring txMayorQFailed[3], txQuestAlreadyGiven[2], txMayorNoQ[2], txCaptainQFailed[2], txCaptainNoQ[2], txLocationDiscovered[2], txAllDiscovered[2], txCampDiscovered[2],
		txAllCampDiscovered[2], txNoQRumors[2], txRumorQ[9], txNeedMoreGold, txNoNearLoc, txNearLoc, txNearLocEmpty[2], txNearLocCleared, txNearLocEnemy[2], txNoNews[2], txAllNews[2], txPvpTooFar,
		txPvp, txPvpWith, txNewsCampCleared, txNewsLocCleared, txArenaText[3], txArenaTextU[5], txAllNearLoc;
	cstring txNear, txFar, txVeryFar, txELvlVeryWeak[2], txELvlWeak[2], txELvlAverage[2], txELvlQuiteStrong[2], txELvlStrong[2];
	cstring txSGOOrcs, txSGOGoblins, txSGOBandits, txSGOEnemies, txSGOUndead, txSGOMages, txSGOGolems, txSGOMagesAndGolems, txSGOUnk, txSGOPowerfull;
	cstring txArthur, txMineBuilt, txAncientArmory, txPortalClosed, txPortalClosedNews, txHiddenPlace, txOrcCamp, txPortalClose, txPortalCloseLevel, txXarDanger, txGorushDanger, txGorushCombat,
		txMageHere, txMageEnter, txMageFinal, txQuest[279], txForMayor, txForSoltys;
	cstring txEnterIp, txConnecting, txInvalidIp, txWaitingForPswd, txEnterPswd, txConnectingTo, txConnectTimeout, txConnectInvalid, txConnectVersion, txConnectRaknet, txCantJoin, txLostConnection,
		txInvalidPswd, txCantJoin2, txServerFull, txInvalidData, txNickUsed, txInvalidVersion, txInvalidVersion2, txInvalidNick, txGeneratingWorld, txLoadedWorld, txWorldDataError, txLoadedPlayer,
		txPlayerDataError, txGeneratingLocation, txLoadingLocation, txLoadingLocationError, txLoadingChars, txLoadingCharsError, txSendingWorld, txMpNPCLeft, txLoadingLevel, txDisconnecting,
		txLost, txLeft, txLost2, txUnconnected, txDisconnected, txClosing, txKicked, txUnknown, txUnknown2, txWaitingForServer, txStartingGame, txPreparingWorld, txInvalidCrc;
	cstring txCreateServerFailed, txInitConnectionFailed, txServer, txPlayerKicked, txYouAreLeader, txRolledNumber, txPcIsLeader, txReceivedGold, txYouDisconnected, txYouKicked, txPcWasKicked,
		txPcLeftGame, txGamePaused, txGameResumed, txDevmodeOn, txDevmodeOff, txPlayerLeft;
	cstring txDialog[1312], txYell[3];

private:
	static Game* game;
public:
	static Game& Get()
	{
		return *game;
	}

	//-----------------------------------------------------------------
	// GAME
	//---------------------------------
	Camera cam;
	int start_version;

	//---------------------------------
	// GUI / HANDEL
	InventoryMode inventory_mode;
	vector<ItemSlot> chest_merchant, chest_blacksmith, chest_alchemist, chest_innkeeper, chest_food_seller, chest_trade;
	bool* trader_buy;

	void StartTrade(InventoryMode mode, Unit& unit);
	void StartTrade(InventoryMode mode, vector<ItemSlot>& items, Unit* unit=nullptr);

	//---------------------------------
	// RYSOWANIE
	MATRIX mat;
	int particle_count;
	Terrain* terrain;
	VB vbDungeon;
	IB ibDungeon;
	INT2 dungeon_part[16], dungeon_part2[16], dungeon_part3[16], dungeon_part4[16];
	vector<ParticleEmitter*> pes2;
	VEC4 fog_color, fog_params, ambient_color;
	int alpha_test_state;
	bool cl_fog, cl_lighting, draw_particle_sphere, draw_unit_radius, draw_hitbox, draw_phy, draw_col;
	Obj obj_alpha;
	float portal_anim, drunk_anim;
	// post effect u¿ywa 3 tekstur lub jeœli jest w³¹czony multisampling 3 surface i 1 tekstury
	SURFACE sPostEffect[3];
	TEX tPostEffect[3];
	VB vbFullscreen;
	vector<PostEffect> post_effects;
	SURFACE sCustom;

	//---------------------------------
	// KONSOLA I KOMENDY
	bool have_console, console_open, inactive_update, nosound, noai, devmode, default_devmode, default_player_devmode, debug_info, debug_info2, dont_wander,
		nomusic;
	string cfg_file;
	vector<ConsoleCommand> cmds;
	int sound_volume, music_volume, mouse_sensitivity;
	float mouse_sensitivity_f;
	vector<ConfigVar> config_vars;

	void SetupConfigVars();
	void ParseConfigVar(cstring var);
	void SetConfigVarsFromFile();
	void ApplyConfigVars();

	//---------------------------------
	// GRA
	GAME_STATE game_state, prev_game_state;
	PlayerController* pc;
	float player_rot_buf;
	AllowInput allow_input;
	bool testing, force_seed_all, koniec_gry, local_ctx_valid, target_loc_is_camp, exit_mode, exit_to_menu;
	int death_screen, dungeon_level;
	bool death_solo;
	float death_fade, speed;
	vector<AnimeshInstance*> bow_instances;
	Pak* pak;
	Unit* selected_unit, *selected_target;
	vector<AIController*> ais;
	const Item* gold_item_ptr;
	BeforePlayer before_player;
	BeforePlayerPtr before_player_ptr;
	uint force_seed, next_seed;
	bool next_seed_extra;
	int next_seed_val[3];
	vector<AttachedSound> attached_sounds;
	SaveSlot single_saves[MAX_SAVE_SLOTS], multi_saves[MAX_SAVE_SLOTS];
	vector<UnitView> unit_views;
	vector<UnitWarpData> unit_warp_data;
	LevelContext local_ctx;
	ObjectPool<TmpLevelContext> tmp_ctx_pool;
	City* city_ctx; // je¿eli jest w mieœcie/wiosce to ten wskaŸnik jest ok, takto nullptr
	vector<Unit*> to_remove;
	CityGenerator* gen;
	uint crc_items, crc_units, crc_dialogs, crc_spells;

	AnimeshInstance* GetBowInstance(Animesh* mesh);

	//---------------------------------
	// SCREENSHOT
	time_t last_screenshot;
	uint screenshot_count;
	D3DXIMAGE_FILEFORMAT screenshot_format;

	//---------------------------------
	// DIALOGI
	DialogContext dialog_context;
	DialogContext* current_dialog;
	INT2 dialog_cursor_pos; // ostatnia pozycja myszki przy wyborze dialogu
	vector<string> dialog_choices; // u¿ywane w MP u klienta
	string predialog;

	//---------------------------------
	// PATHFINDING
	vector<APoint> a_map;
#ifdef DRAW_LOCAL_PATH
	vector<std::pair<VEC2, int> > test_pf;
	Unit* marked;
	bool test_pf_outside;
#endif
	vector<bool> local_pfmap;

	//---------------------------------
	// FIZYKA
	btCollisionShape* shape_wall, *shape_low_ceiling, *shape_arrow, *shape_ceiling, *shape_floor, *shape_door, *shape_block, *shape_schody, *shape_schody_c[2];
	btHeightfieldTerrainShape* terrain_shape;
	btCollisionObject* obj_arrow, *obj_terrain, *obj_spell;
	vector<CollisionObject> global_col; // wektor na tymczasowe obiekty, czêsto u¿ywany przy zbieraniu obiektów do kolizji
	vector<btCollisionShape*> shapes;
	vector<CameraCollider> cam_colliders;

	//---------------------------------
	// WIADOMOŒCI / NOTATKI / PLOTKI
	vector<string> notes;
	vector<string> rumors;

	//---------------------------------
	// WCZYTYWANIE
	float loading_dt;
	Timer loading_t;
	int loading_steps, loading_index;
	DWORD clear_color2;
	
	//---------------------------------
	// MINIMAPA
	bool minimap_opened_doors;
	vector<INT2> minimap_reveal;

	//---------------------------------
	// FALLBACK
	int fallback_co, fallback_1, fallback_2;
	float fallback_t;

	//--------------------------------------
	// ARENA
	// etapy: 0-brak, 1-walka wchodzenie, 2-walka oczekiwanie, 3-walka, 4-walka oczekiwanie po, 5-walka koniec
	// 10-pvp wchodzenie, 11-pvp oczekiwanie, 12-pvp 13-pvp oczekiwanie po, 14-pvp koniec
	enum TrybArena
	{
		Arena_Brak,
		Arena_Walka,
		Arena_Pvp,
		Arena_Zawody
	} arena_tryb;
	enum EtapAreny
	{
		Arena_OdliczanieDoPrzeniesienia,
		Arena_OdliczanieDoStartu,
		Arena_TrwaWalka,
		Arena_OdliczanieDoKonca,
		Arena_OdliczanieDoWyjscia
	} arena_etap;
	int arena_poziom; // poziom trudnoœci walki na arenie [1-3]
	int arena_wynik; // wynik walki na arenia [0 - gracz wygra³, 1 - gracz przegra³]
	vector<Unit*> at_arena; // jednostki na arenie
	float arena_t; // licznik czasu na arenie
	bool arena_free; // czy arena jest wolna
	Unit* arena_fighter; // postaæ z któr¹ walczy siê na arenie w pvp
	vector<Unit*> near_players;
	vector<string> near_players_str;
	Unit* pvp_unit;
	struct PvpResponse
	{
		Unit* from, *to;
		float timer;
		bool ok;
	} pvp_response;
	PlayerController* pvp_player;
	vector<Unit*> arena_viewers;

	void CleanArena();

	//--------------------------------------
	// TOURNAMENT
	enum TOURNAMENT_STATE
	{
		TOURNAMENT_NOT_DONE,
		TOURNAMENT_STARTING,
		TOURNAMENT_IN_PROGRESS
	} tournament_state;
	int tournament_year, tournament_city_year, tournament_city, tournament_state2, tournament_state3, tournament_round, tournament_arena;
	vector<Unit*> tournament_units;
	float tournament_timer;
	Unit* tournament_master, *tournament_skipped_unit, *tournament_other_fighter, *tournament_winner;
	vector<std::pair<Unit*, Unit*> > tournament_pairs;
	bool tournament_generated;

	void StartTournament(Unit* arena_master);
	bool IfUnitJoinTournament(Unit& u);
	void GenerateTournamentUnits();
	void UpdateTournament(float dt);
	void StartTournamentRound();
	void TournamentTalk(cstring text);
	void TournamentTrain(Unit& u);
	void CleanTournament();

	//--------------------------------------
	// DRU¯YNA
	vector<Unit*> team; // wszyscy cz³onkowie dru¿yny
	vector<Unit*> active_team; // cz³onkowie dru¿yny którzy maj¹ udzia³ w ³upach (bez questowych postaci)
	Unit* leader; // przywódca dru¿yny
	int team_gold; // niepodzielone z³oto dru¿ynowe
	int take_item_id; // u¿ywane przy wymianie ekwipunku ai [tymczasowe]
	int team_share_id; // u¿ywane przy wymianie ekwipunku ai [tymczasowe]
	vector<TeamShareItem> team_shares; // u¿ywane przy wymianie ekwipunku ai [tymczasowe]
	bool atak_szalencow, free_recruit, bandyta;
	vector<INT2> tmp_path;

	void AddTeamMember(Unit* unit, bool free);
	void RemoveTeamMember(Unit* unit);

	//--------------------------------------
	// QUESTS
	QuestManager quest_manager;
	vector<Quest*> unaccepted_quests;
	vector<Quest*> quests;
	vector<Quest_Dungeon*> quests_timeout;
	vector<Quest*> quests_timeout2;
	int quest_counter;
	vector<QuestItemRequest*> quest_item_requests;
	inline void AddQuestItemRequest(const Item** item, cstring name, int quest_refid, vector<ItemSlot>* items, Unit* unit=nullptr)
	{
		assert(item && name && quest_refid != -1);
		QuestItemRequest* q = new QuestItemRequest;
		q->item = item;
		q->name = name;
		q->quest_refid = quest_refid;
		q->items = items;
		q->unit = unit;
		quest_item_requests.push_back(q);
	}
	int quest_rumor_counter;
	bool quest_rumor[P_MAX];
	int unique_quests_completed;
	bool unique_completed_show;
	Quest_Sawmill* quest_sawmill;
	Quest_Mine* quest_mine;
	Quest_Bandits* quest_bandits;
	Quest_Mages* quest_mages;
	Quest_Mages2* quest_mages2;
	Quest_Orcs* quest_orcs;
	Quest_Orcs2* quest_orcs2;
	Quest_Goblins* quest_goblins;
	Quest_Evil* quest_evil;
	Quest_Crazies* quest_crazies;
	void CheckCraziesStone();
	void ShowAcademyText();

	// drinking contest
	enum ContestState
	{
		CONTEST_NOT_DONE,
		CONTEST_DONE,
		CONTEST_TODAY,
		CONTEST_STARTING,
		CONTEST_IN_PROGRESS,
		CONTEST_FINISH
	} contest_state;
	int contest_where, contest_state2;
	vector<Unit*> contest_units;
	float contest_time;
	bool contest_generated;
	Unit* contest_winner;
	void UpdateContest(float dt);

	// secret quest
	enum SecretState
	{
		SECRET_OFF,
		SECRET_NONE,
		SECRET_DROPPED_STONE,
		SECRET_GENERATED,
		SECRET_CLOSED,
		SECRET_GENERATED2,
		SECRET_TALKED,
		SECRET_FIGHT,
		SECRET_LOST,
		SECRET_WIN,
		SECRET_REWARD
	} secret_state;
	bool CheckMoonStone(GroundItem* item, Unit& unit);
	inline Item* GetSecretNote()
	{
		return (Item*)FindItem("sekret_kartka");
	}
	int secret_where, secret_where2;

	// tutorial
	int tut_state;
	vector<TutorialText> ttexts;
	VEC3 tut_dummy;
	Object* tut_shield, *tut_shield2;
	void UpdateTutorial();
	void TutEvent(int id);
	void EndOfTutorial(int);

	struct TutChestHandler : public ChestEventHandler
	{
		void HandleChestEvent(ChestEventHandler::Event event)
		{
			Game::Get().TutEvent(0);
		}
		int GetChestEventHandlerQuestRefid()
		{
			// w tutorialu nie mo¿na zapisywaæ
			return -1;
		}
	} tut_chest_handler;
	struct TutChestHandler2 : public ChestEventHandler
	{
		void HandleChestEvent(ChestEventHandler::Event event)
		{
			Game::Get().TutEvent(1);
		}
		int GetChestEventHandlerQuestRefid()
		{
			// w tutorialu nie mo¿na zapisywaæ
			return -1;
		}
	} tut_chest_handler2;
	struct TutUnitHandler : public UnitEventHandler
	{
		void HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit)
		{
			Game::Get().TutEvent(2);
		}
		int GetUnitEventHandlerQuestRefid()
		{
			// w tutorialu nie mo¿na zapisywaæ
			return -1;
		}
	} tut_unit_handler;

	//
	vector<Unit*> warp_to_inn;

	// game news
	vector<News*> news;
	void AddNews(cstring text);

	bool show_mp_panel;
	int draw_flags;
	bool in_tutorial;

	// muzyka
	MusicType music_type;
	Music* last_music;
	vector<Music*> musics, tracks;
	int track_id;
	MusicType GetLocationMusic();
	void LoadMusicDatafile();
	void LoadMusic(MusicType type, bool new_load_screen = true);
	void SetMusic();
	void SetMusic(MusicType type);
	void SetupTracks();
	void UpdateMusic();

	// u¿ywane przy wczytywaniu gry
	vector<AIController*> ai_bow_targets, ai_cast_targets;
	vector<Location*> load_location_quest;
	vector<Unit*> load_unit_handler;
	vector<Chest*> load_chest_handler;
	vector<Unit**> load_unit_refid;

	bool hardcore_mode, hardcore_option;
	string hardcore_savename;
	bool clearup_shutdown;
	const Item* crazy_give_item; // dawany przedmiot, nie trzeba zapisywaæ
	int total_kills;
	float grayout;
	bool cl_postfx;

	inline bool WantAttackTeam(Unit& u)
	{
		if(IsLocal())
			return u.attack_team;
		else
			return IS_SET(u.ai_mode, 0x08);
	}

	// zwraca losowy przedmiot o maksymalnej cenie, ta funkcja jest powolna!
	// mo¿e zwróciæ questowy przedmiot jeœli bêdzie wystarczaj¹co tani, lub unikat!
	const Item* GetRandomItem(int max_value);

	// klawisze
	void InitGameKeys();
	void ResetGameKeys();
	void SaveGameKeys();
	void LoadGameKeys();
	inline bool KeyAllowed(byte k)
	{
		return IS_SET(allow_input, KeyAllowState(k));
	}
	inline byte KeyDoReturn(GAME_KEYS gk, KeyF f)
	{
		GameKey& k = GKey[gk];
		if(k[0])
		{
			if(KeyAllowed(k[0]) && (Key.*f)(k[0]))
				return k[0];
		}
		if(k[1])
		{
			if(KeyAllowed(k[1]) && (Key.*f)(k[1]))
				return k[1];
		}
		return VK_NONE;
	}
	inline byte KeyDoReturn(GAME_KEYS gk, KeyFC f)
	{
		return KeyDoReturn(gk, (KeyF)f);
	}
	inline byte KeyDoReturnIgnore(GAME_KEYS gk, KeyF f, byte ignored_key)
	{
		GameKey& k = GKey[gk];
		if(k[0] && k[0] != ignored_key)
		{
			if(KeyAllowed(k[0]) && (Key.*f)(k[0]))
				return k[0];
		}
		if(k[1] && k[1] != ignored_key)
		{
			if(KeyAllowed(k[1]) && (Key.*f)(k[1]))
				return k[1];
		}
		return VK_NONE;
	}
	inline byte KeyDoReturnIgnore(GAME_KEYS gk, KeyFC f, byte ignored_key)
	{
		return KeyDoReturnIgnore(gk, (KeyF)f, ignored_key);
	}
	inline bool KeyDo(GAME_KEYS gk, KeyF f)
	{
		return KeyDoReturn(gk, f) != VK_NONE;
	}
	inline bool KeyDo(GAME_KEYS gk, KeyFC f)
	{
		return KeyDo(gk, (KeyF)f);
	}
	inline bool KeyDownAllowed(GAME_KEYS gk)
	{
		return KeyDo(gk, &KeyStates::Down);
	}
	inline bool KeyPressedReleaseAllowed(GAME_KEYS gk)
	{
		return KeyDo(gk, &KeyStates::PressedRelease);
	}
	inline bool KeyDownUpAllowed(GAME_KEYS gk)
	{
		return KeyDo(gk, &KeyStates::DownUp);
	}
	inline bool KeyDownUp(GAME_KEYS gk)
	{
		GameKey& k = GKey[gk];
		if(k[0])
		{
			if(Key.DownUp(k[0]))
				return true;
		}
		if(k[1])
		{
			if(Key.DownUp(k[1]))
				return true;
		}
		return false;
	}
	inline bool KeyPressedUpAllowed(GAME_KEYS gk)
	{
		return KeyDo(gk, &KeyStates::PressedUp);
	}
	// Zwraca czy dany klawisze jest wyciœniêty, jeœli nie jest to dozwolone to traktuje jak wyciœniêty
	inline bool KeyUpAllowed(byte key)
	{
		if(KeyAllowed(key))
			return Key.Up(key);
		else
			return true;
	}
	// przedmioty w czasie grabienia itp s¹ tu przechowywane indeksy
	// ujemne wartoœci odnosz¹ siê do slotów (SLOT_WEAPON = -SLOT_WEAPON-1), pozytywne do zwyk³ych przedmiotów
	vector<int> tmp_inventory[2];
	int tmp_inventory_shift[2];

	void BuildTmpInventory(int index);
	int GetItemPrice(const Item* item, Unit& unit, bool buy);

	void BreakAction(Unit& unit, bool fall=false, bool notify=false);
	void CreateTerrain();
	void Draw();
	void ExitToMenu();
	void DoExitToMenu();
	void GenerateImage(TaskData& task_data);
	void SetupTrap(TaskData& task_data);
	void SetupObject(TaskData& task_data);
	Unit* GetFollowTarget();
	void SetupCamera(float dt);
	void LoadShaders();
	void SetupShaders();
	void TakeScreenshot(bool text=false, bool no_gui=false);
	void UpdateGame(float dt);
	void UpdatePlayer(LevelContext& ctx, float dt);
	void PlayerCheckObjectDistance(Unit& u, const VEC3& pos, void* ptr, float& best_dist, BeforePlayer type);

	int CheckMove(VEC3& pos, const VEC3& dir, float radius, Unit* me, bool* is_small=nullptr);
	int CheckMovePhase(VEC3& pos, const VEC3& dir, float radius, Unit* me, bool* is_small=nullptr);

	struct IgnoreObjects
	{
		// nullptr lub tablica jednostek zakoñczona nullptr
		const Unit** ignored_units;
		// nullptr lub tablica obiektów [u¿ywalnych lub nie] zakoñczona nullptr
		const void** ignored_objects;
		// czy ignorowaæ bloki
		bool ignore_blocks;
		// czy ignorowaæ obiekty
		bool ignore_objects;
	};
	void GatherCollisionObjects(LevelContext& ctx, vector<CollisionObject>& objects, const VEC3& pos, float radius, const IgnoreObjects* ignore=nullptr);
	void GatherCollisionObjects(LevelContext& ctx, vector<CollisionObject>& objects, const BOX2D& box, const IgnoreObjects* ignore=nullptr);
	bool Collide(const vector<CollisionObject>& objects, const VEC3& pos, float radius);
	bool Collide(const vector<CollisionObject>& objects, const BOX2D& box, float margin=0.f);
	bool Collide(const vector<CollisionObject>& objects, const BOX2D& box, float margin, float rot);
	void ParseCommand(const string& str, PrintMsgFunc print_func=nullptr, PARSE_SOURCE ps=PS_UNKNOWN);
	void AddCommands();
	void AddConsoleMsg(cstring msg);
	void UpdateAi(float dt);
	void StartDialog(DialogContext& ctx, Unit* talker, DialogEntry* dialog = nullptr, bool is_new = false);
	void StartDialog2(PlayerController* player, Unit* talker, DialogEntry* dialog = nullptr, bool is_new = false);
	void EndDialog(DialogContext& ctx);
	void UpdateGameDialog(DialogContext& ctx, float dt);
	void GenerateStockItems();
	void GenerateMerchantItems(vector<ItemSlot>& items, int price_limit);
	void ApplyToTexturePack(TexturePack& tp, cstring diffuse, cstring normal, cstring specular);
	void SetDungeonParamsAndTextures(BaseLocation& base);
	void MoveUnit(Unit& unit, bool warped=false);
	bool RayToMesh(const VEC3& ray_pos, const VEC3& ray_dir, const VEC3& obj_pos, float obj_rot, VertexData* vd, float& dist);
	bool CollideWithStairs(const CollisionObject& co, const VEC3& pos, float radius) const;
	bool CollideWithStairsRect(const CollisionObject& co, const BOX2D& box) const;
	void ValidateGameData(bool popup);
	void TestGameData(bool major);
	void TestUnitSpells(const SpellList& spells, string& errors, uint& count);
	Unit* CreateUnit(UnitData& base, int level=-1, Human* human_data=nullptr, Unit* test_unit=nullptr, bool create_physics=true, bool custom=false);
	void ParseItemScript(Unit& unit, const int* script, bool is_new);
	bool IsEnemy(Unit& u1, Unit& u2, bool ignore_dont_attack=false);
	bool IsFriend(Unit& u1, Unit& u2);
	bool CanSee(Unit& unit, Unit& unit2);
	// nie dzia³a dla budynków bo nie uwzglêdnia obiektów
	bool CanSee(const VEC3& v1, const VEC3& v2);
	bool CheckForHit(LevelContext& ctx, Unit& unit, Unit*& hitted, VEC3& hitpoint);
	bool CheckForHit(LevelContext& ctx, Unit& unit, Unit*& hitted, Animesh::Point& hitbox, Animesh::Point* bone, VEC3& hitpoint);
	void UpdateParticles(LevelContext& ctx, float dt);
	// wykonuje atak postaci
	enum ATTACK_RESULT
	{
		ATTACK_NOT_HIT,
		ATTACK_BLOCKED,
		ATTACK_NO_DAMAGE,
		ATTACK_HIT,
		ATTACK_CLEAN_HIT
	};
	ATTACK_RESULT DoAttack(LevelContext& ctx, Unit& unit);
#define DMG_NO_BLOOD (1<<0)
#define DMG_MAGICAL (1<<1)
	void GiveDmg(LevelContext& ctx, Unit* giver, float dmg, Unit& taker, const VEC3* hitpoint=nullptr, int dmg_flags=0);
	void UpdateUnits(LevelContext& ctx, float dt);
	void UpdateUnitInventory(Unit& unit);
	bool FindPath(LevelContext& ctx, const INT2& start_tile, const INT2& target_tile, vector<INT2>& path, bool can_open_doors=true, bool wedrowanie=false, vector<INT2>* blocked=nullptr);
	INT2 RandomNearTile(const INT2& tile);
	bool CanLoadGame() const;
	bool CanSaveGame() const;
	bool IsAnyoneAlive() const;
	int FindLocalPath(LevelContext& ctx, vector<INT2>& path, const INT2& my_tile, const INT2& target_tile, const Unit* me, const Unit* other, const void* useable=nullptr, bool is_end_point=false);
	bool DoShieldSmash(LevelContext& ctx, Unit& attacker);
	VEC4 GetFogColor();
	VEC4 GetFogParams();
	VEC4 GetAmbientColor();
	VEC4 GetLightColor();
	VEC4 GetLightDir();
	void UpdateBullets(LevelContext& ctx, float dt);
	void SpawnDungeonColliders();
	void RemoveColliders();
	void CreateCollisionShapes();
	inline bool AllowKeyboard() const { return IS_SET(allow_input, ALLOW_KEYBOARD); }
	inline bool AllowMouse() const { return IS_SET(allow_input, ALLOW_MOUSE); }
	VEC3 PredictTargetPos(const Unit& me, const Unit& target, float bullet_speed) const;
	inline bool CanShootAtLocation(const Unit& me, const Unit& target, const VEC3& pos) const
	{
		return CanShootAtLocation2(me, &target, pos);
	}
	bool CanShootAtLocation(const VEC3& from, const VEC3& to) const;
	bool CanShootAtLocation2(const Unit& me, const void* ptr, const VEC3& to) const;
	void LoadItemsData();
	void SpawnTerrainCollider();
	void GenerateDungeonObjects();
	void GenerateDungeonUnits();
	Unit* SpawnUnitInsideRoom(Room& room, UnitData& unit, int level=-1, const INT2& pt=INT2(-1000,-1000), const INT2& pt2=INT2(-1000,-1000));
	Unit* SpawnUnitInsideRoomOrNear(InsideLocationLevel& lvl, Room& room, UnitData& unit, int level=-1, const INT2& pt=INT2(-1000,-1000), const INT2& pt2=INT2(-1000,-1000));
	Unit* SpawnUnitNearLocation(LevelContext& ctx, const VEC3& pos, UnitData& unit, const VEC3* look_at=nullptr, int level=-1, float extra_radius=2.f);
	Unit* SpawnUnitInsideArea(LevelContext& ctx, const BOX2D& area, UnitData& unit, int level=-1);
	Unit* SpawnUnitInsideInn(UnitData& unit, int level=-1, InsideBuilding* inn=nullptr);
	Unit* CreateUnitWithAI(LevelContext& ctx, UnitData& unit, int level=-1, Human* human_data=nullptr, const VEC3* pos=nullptr, const float* rot=nullptr, AIController** ai=nullptr);
	void ChangeLevel(int gdzie);
	void AddPlayerTeam(const VEC3& pos, float rot, bool reenter, bool hide_weapon);
	void OpenDoorsByTeam(const INT2& pt);
	void ExitToMap();
	void SetExitWorldDir();
	void RespawnObjectColliders(bool spawn_pes=true);
	void RespawnObjectColliders(LevelContext& ctx, bool spawn_pes=true);
	void SetRoomPointers();
	SOUND GetMaterialSound(MATERIAL_TYPE m1, MATERIAL_TYPE m2);
	void PlayAttachedSound(Unit& unit, SOUND sound, float smin, float smax);
	ATTACK_RESULT DoGenericAttack(LevelContext& ctx, Unit& attacker, Unit& hitted, const VEC3& hitpoint, float dmg, int dmg_type, bool bash);
	void GenerateLabirynthUnits();
	int GetDungeonLevel();
	int GetDungeonLevelChest();
	void GenerateCave(Location& l);
	void GenerateCaveObjects();
	void GenerateCaveUnits();
	void SaveGame(HANDLE file);
	void LoadGame(HANDLE file);
	void RemoveUnusedAiAndCheck();
	void CheckUnitsAi(LevelContext& ctx, int& err_count);
	void CastSpell(LevelContext& ctx, Unit& unit);
	void SpellHitEffect(LevelContext& ctx, Bullet& bullet, const VEC3& pos, Unit* hitted);
	void UpdateExplosions(LevelContext& ctx, float dt);
	void UpdateTraps(LevelContext& ctx, float dt);
	// zwraca tymczasowy wskaŸnik na stworzon¹ pu³apkê lub nullptr (mo¿e siê nie udaæ tylko dla ARROW i POISON)
	Trap* CreateTrap(INT2 pt, TRAP_TYPE type, bool timed=false);
	bool RayTest(const VEC3& from, const VEC3& to, Unit* ignore, VEC3& hitpoint, Unit*& hitted);
	void UpdateElectros(LevelContext& ctx, float dt);
	void UpdateDrains(LevelContext& ctx, float dt);
	void AI_Shout(LevelContext& ctx, AIController& ai);
	void AI_DoAttack(AIController& ai, Unit* target, bool w_biegu=false);
	void AI_HitReaction(Unit& unit, const VEC3& pos);
	void UpdateAttachedSounds(float dt);
	void BuildRefidTables();
	bool SaveGameSlot(int slot, cstring text);
	bool LoadGameSlot(int slot);
	void LoadSaveSlots();
	void Quicksave(bool from_console);
	void Quickload(bool from_console);
	void ClearGameVarsOnNewGameOrLoad();
	void ClearGameVarsOnNewGame();
	void ClearGameVarsOnLoad();
	Quest* FindQuest(int location, Quest::Type type);
	Quest* FindQuest(int refid, bool active=true);
	Quest* FindQuestById(QUEST quest_id);
	Quest* FindUnacceptedQuest(int location, Quest::Type type);
	Quest* FindUnacceptedQuest(int refid);
	// zwraca losowe miasto lub wioskê która nie jest this_city
	int GetRandomCityLocation(int this_city=-1);
	// zwraca losowe miasto lub wioskê która nie jest this_city i nie ma aktywnego questa
	int GetFreeRandomCityLocation(int this_city=-1);
	// zwraca losowe miasto które nie jest this_city
	int GetRandomCity(int this_city=-1);
	void LoadQuests(vector<Quest*>& v_quests, HANDLE file);
	void ClearGame();
	cstring FormatString(DialogContext& ctx, const string& str_part);
	int GetNearestLocation(const VEC2& pos, bool not_quest, bool not_city);
	int GetNearestLocation2(const VEC2& pos, int flags, bool not_quest, int flagi_cel=-1);
	inline int GetNearestCity(const VEC2& pos)
	{
		return GetNearestLocation2(pos, (1<<L_CITY)|(1<<L_VILLAGE), false);
	}
	void AddGameMsg(cstring msg, float time);
	void AddGameMsg2(cstring msg, float time, int id);
	void AddGameMsg3(GMS id);
	int CalculateQuestReward(int gold);
	void AddReward(int gold)
	{
		AddGold(CalculateQuestReward(gold), nullptr, true, txQuestCompletedGold, 4.f, false);
	}
	void CreateCityMinimap();
	void CreateDungeonMinimap();
	void RebuildMinimap();
	void UpdateDungeonMinimap(bool send);
	void DungeonReveal(const INT2& tile);
	const Item* FindQuestItem(cstring name, int refid);
	void SaveStock(HANDLE file, vector<ItemSlot>& cnt);
	void LoadStock(HANDLE file, vector<ItemSlot>& cnt);
	Door* FindDoor(LevelContext& ctx, const INT2& pt);
	void AddGroundItem(LevelContext& ctx, GroundItem* item);
	void GenerateDungeonObjects2();
	SOUND GetItemSound(const Item* item);
	cstring GetCurrentLocationText();
	void Unit_StopUsingUseable(LevelContext& ctx, Unit& unit, bool send=true);
	void OnReenterLevel(LevelContext& ctx);
	void EnterLevel(bool first, bool reenter, bool from_lower, int from_portal, bool from_outside);
	void LeaveLevel(bool clear=false);
	void LeaveLevel(LevelContext& ctx, bool clear);
	void CreateBlood(LevelContext& ctx, const Unit& unit, bool fully_created=false);
	void WarpUnit(Unit& unit, const VEC3& pos);
	void ProcessUnitWarps();
	void ProcessRemoveUnits();
	void ApplyContext(ILevel* level, LevelContext& ctx);
	void UpdateContext(LevelContext& ctx, float dt);
	inline LevelContext& GetContext(Unit& unit)
	{
		if(unit.in_building == -1)
			return local_ctx;
		else
		{
			assert(city_ctx);
			return city_ctx->inside_buildings[unit.in_building]->ctx;
		}
	}
	inline LevelContext& GetContext(const VEC3& pos)
	{
		if(!city_ctx)
			return local_ctx;
		else
		{
			INT2 offset(int((pos.x-256.f)/256.f), int((pos.z-256.f)/256.f));
			if(offset.x%2 == 1)
				++offset.x;
			if(offset.y%2 == 1)
				++offset.y;
			offset /= 2;
			for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
			{
				if((*it)->level_shift == offset)
					return (*it)->ctx;
			}
			return local_ctx;
		}
	}
	// dru¿yna
	inline int GetTeamSize() // zwraca liczbê osób w dru¿ynie
	{
		return team.size();
	}
	inline int GetActiveTeamSize() // liczba osób w dru¿ynie które nie s¹ questowe
	{
		return active_team.size();
	}
	bool HaveTeamMember();
	bool HaveTeamMemberNPC();
	bool HaveTeamMemberPC();
	bool IsTeamMember(Unit& unit);
	inline Unit* GetLeader() { return leader; }
	int GetPCShare();
	int GetPCShare(int pc, int npc);
	inline bool IsLeader(const Unit& unit)
	{
		return &unit == GetLeader();
	}
	inline bool IsLeader(const Unit* unit)
	{
		assert(unit);
		return unit == GetLeader();
	}
	inline bool IsLeader()
	{
		if(!IsOnline())
			return true;
		else
			return leader_id == my_id;
	}
	void AddGold(int ile, vector<Unit*>* to=nullptr, bool show=false, cstring msg=txGoldPlus, float time=3.f, bool defmsg=true);
	void AddGoldArena(int ile);
	void CheckTeamItemShares();
	bool CheckTeamShareItem(TeamShareItem& tsi);
	void UpdateTeamItemShares();
	void TeamShareGiveItemCredit(DialogContext& ctx);
	void TeamShareSellItem(DialogContext& ctx);
	void TeamShareDecline(DialogContext& ctx);
	void ValidateTeamItems();
	void BuyTeamItems();
	void CheckUnitOverload(Unit& unit);
	bool IsTeamNotBusy();
	bool IsAnyoneTalking() const;
	// szuka przedmiotu w dru¿ynie
	bool FindItemInTeam(const Item* item, int refid, Unit** unit, int* i_index, bool check_npc=true);
	bool FindQuestItem2(Unit* unit, cstring id, Quest** quest, int* i_index, bool not_active=false);
	inline bool HaveQuestItem(const Item* item, int refid=-1)
	{
		return FindItemInTeam(item, refid, nullptr, nullptr, true);
	}
	bool RemoveQuestItem(const Item* item, int refid=-1);
	bool RemoveItemFromWorld(const Item* item);
	bool IsBetterItem(Unit& unit, const Item* item, int* value=nullptr);
	SPAWN_GROUP RandomSpawnGroup(const BaseLocation& base);
	// to by mog³o byæ globalna funkcj¹
	void GenerateTreasure(int level, int count, vector<ItemSlot>& items, int& gold);
	void SplitTreasure(vector<ItemSlot>& items, int gold, Chest** chests, int count);
	void PlayHitSound(MATERIAL_TYPE mat_bron, MATERIAL_TYPE mat_cialo, const VEC3& hitpoint, float range, bool dmg);
	// wczytywanie
	void LoadingStart(int steps);
	void LoadingStep(cstring text = nullptr);
	//
	void StartArenaCombat(int level);
	InsideBuilding* GetArena();
	bool WarpToArea(LevelContext& ctx, const BOX2D& area, float radius, VEC3& pos, int tries=10);
	void DeleteUnit(Unit* unit);
	void DialogTalk(DialogContext& ctx, cstring msg);
	void GenerateHeroName(HeroData& hero);
	void GenerateHeroName(Class klasa, bool szalony, string& name);
	inline bool WantExitLevel()
	{
		return !KeyDownAllowed(GK_WALK);
	}
	VEC2 GetMapPosition(Unit& unit);
	void EventTakeItem(int id);
	const Item* GetBetterItem(const Item* item);
	void CheckIfLocationCleared();
	void SpawnArenaViewers(int count);
	void RemoveArenaViewers();
	inline bool CanWander(Unit& u)
	{
		if(city_ctx && u.ai->loc_timer <= 0.f && !dont_wander && IS_SET(u.data->flags, F_AI_WANDERS))
		{
			if(u.busy != Unit::Busy_No)
				return false;
			if(u.IsHero())
			{
				if(u.hero->team_member && u.hero->mode != HeroData::Wander)
					return false;
				else if(tournament_generated)
					return false;
				else
					return true;
			}
			else if(u.in_building == -1)
				return true;
			else
				return false;
		}
		else
			return false;
	}
	float PlayerAngleY();
	VEC3 GetExitPos(Unit& u, bool force_border=false);
	void AttackReaction(Unit& attacked, Unit& attacker);
	// czy mo¿na opuœciæ lokacjê (0-tak, 1-dru¿yna za daleko, 2-wrogowie w pobli¿u)
	int CanLeaveLocation(Unit& unit);
	void GenerateTraps();
	void RegenerateTraps();
	void SpawnHeroesInsideDungeon();
	GroundItem* SpawnGroundItemInsideAnyRoom(InsideLocationLevel& lvl, const Item* item);
	GroundItem* SpawnGroundItemInsideRoom(Room& room, const Item* item);
	GroundItem* SpawnGroundItemInsideRadius(const Item* item, const VEC2& pos, float radius, bool try_exact=false);
	void InitQuests();
	void GenerateQuestUnits();
	void GenerateQuestUnits2(bool on_enter);
	void UpdateQuests(int days);
	void SaveQuestsData(HANDLE file);
	void LoadQuestsData(HANDLE file);
	void RemoveQuestUnit(UnitData* ud, bool on_leave);
	void RemoveQuestUnits(bool on_leave);
	void GenerateSawmill(bool in_progress);
	int FindWorldUnit(Unit* unit, int hint_loc = -1, int hint_loc2 = -1, int* level = nullptr);
	// zwraca losowe miasto/wioskê pomijaj¹c te ju¿ u¿yte, 0-wioska/miasto, 1-miasto, 2-wioska
	int GetRandomCityLocation(const vector<int>& used, int type=0) const;
	bool GenerateMine();
	void HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit);
	int GetUnitEventHandlerQuestRefid();
	void EndUniqueQuest();
	Room& GetRoom(InsideLocationLevel& lvl, RoomTarget target, bool down_stairs);
	void UpdateGame2(float dt);
	inline bool IsUnitDontAttack(Unit& u)
	{
		if(IsLocal())
			return u.dont_attack;
		else
			return IS_SET(u.ai_mode, 0x01);
	}
	inline bool IsUnitAssist(Unit& u)
	{
		if(IsLocal())
			return u.assist;
		else
			return IS_SET(u.ai_mode, 0x02);
	}
	inline bool IsUnitIdle(Unit& u)
	{
		if(IsLocal())
			return u.ai->state == AIController::Idle;
		else
			return !IS_SET(u.ai_mode, 0x04);
	}
	void SetUnitWeaponState(Unit& unit, bool wyjmuje, WeaponType co);
	void UpdatePlayerView();
	void OnCloseInventory();
	void CloseInventory(bool do_close=true);
	void CloseAllPanels(bool close_mp_box=false);
	bool CanShowEndScreen();
	void UpdateGameDialogClient();
	LevelContext& GetContextFromInBuilding(int in_building);
	bool Cheat_KillAll(int typ, Unit& unit, Unit* ignore);
	void Event_Pvp(int id);
	void Cheat_Reveal();
	void Cheat_ShowMinimap();
	void StartPvp(PlayerController* player, Unit* unit);
	void UpdateGameNet(float dt);
	void CheckCredit(bool require_update=false, bool ignore=false);
	void UpdateUnitPhysics(Unit& unit, const VEC3& pos);
	Unit* FindTeamMember(int netid);
	void WarpNearLocation(LevelContext& ctx, Unit& uint, const VEC3& pos, float extra_radius, bool allow_exact, int tries=20);
	void Train(Unit& unit, bool is_skill, int co, int mode=0);
	void ShowStatGain(bool is_skill, int what, int value);
	void ActivateChangeLeaderButton(bool activate);
	void RespawnTraps();
	void WarpToInn(Unit& unit);
	void PayCredit(PlayerController* player, int ile);
	void CreateSaveImage(cstring filename);
	void PlayerUseUseable(Useable* u, bool after_action);
	SOUND GetTalkSound(Unit& u);
	void UnitTalk(Unit& u, cstring text);
	void OnEnterLocation();
	void OnEnterLevel();
	void OnEnterLevelOrLocation();
	Unit* FindTeamMemberById(cstring id);
	inline Unit* FindUnitByIdLocal(UnitData* ud)
	{
		return local_ctx.FindUnitById(ud);
	}
	inline Unit* FindUnitByIdLocal(cstring id)
	{
		return FindUnitByIdLocal(FindUnitData(id));
	}
	inline Object* FindObjectByIdLocal(Obj* obj)
	{
		return local_ctx.FindObjectById(obj);
	}
	inline Object* FindObjectByIdLocal(cstring id)
	{
		return FindObjectByIdLocal(FindObject(id));
	}
	inline Useable* FindUseableByIdLocal(int type)
	{
		return local_ctx.FindUseableById(type);
	}
	Unit* GetRandomArenaHero();
	cstring GetRandomIdleText(Unit& u);
	struct TeamInfo
	{
		int players;
		int npcs;
		int heroes;
		int sane_heroes;
		int insane_heroes;
		int free_members;
	};
	void GetTeamInfo(TeamInfo& info);
	Unit* GetRandomSaneHero();
	UnitData* GetRandomHeroData();
	UnitData* GetUnitDataFromClass(Class clas, bool crazy);
	void HandleQuestEvent(Quest_Event* event);

	enum BLOCK_RESULT
	{
		BLOCK_PERFECT,
		BLOCK_GOOD,
		BLOCK_MEDIUM,
		BLOCK_POOR,
		BLOCK_BREAK
	};
	BLOCK_RESULT CheckBlock(Unit& hitted, float angle_dif, float attack_power, float skill, float str);

	void DropGold(int count);

	// dodaje przedmiot do ekwipunku postaci (obs³uguje z³oto, otwarty ekwipunek i multiplayer)
	void AddItem(Unit& unit, const Item* item, uint count, uint team_count, bool send_msg=true);
	inline void AddItem(Unit& unit, const Item* item, uint count=1, bool is_team=true, bool send_msg=true)
	{
		AddItem(unit, item, count, is_team ? count : 0, send_msg);
	}
	inline void AddItem(Unit& unit, const GroundItem& item, bool send_msg=true)
	{
		AddItem(unit, item.item, item.count, item.team_count, send_msg);
	}
	// dodaje przedmiot do skrzyni (obs³uguje otwarty ekwipunek i multiplayer)
	void AddItem(Chest& chest, const Item* item, uint count, uint team_count, bool send_msg=true);
	inline void AddItem(Chest& chest, const Item* item, uint count=1, bool is_team=true, bool send_msg=true)
	{
		AddItem(chest, item, count, is_team ? count : 0, send_msg);
	}
	// dodaje przedmiot do skrzyni, nie sortuje
	void AddItemBare(Chest& chest, const Item* item, uint count, uint team_count);
	inline void AddItemBare(Chest& chest, const Item* item, uint count=1, bool is_team=true)
	{
		AddItemBare(chest, item, count, is_team ? count : 0);
	}
	// usuwa przedmiot z ekwipunku (obs³uguje otwarty ekwipunek, lock i multiplayer), dla 0 usuwa wszystko
	void RemoveItem(Unit& unit, int i_index, uint count);
	bool RemoveItem(Unit& unit, const Item* item, uint count);

	// szuka gracza który u¿ywa skrzyni, jeœli u¿ywa nie-gracz to zwraca nullptr (aktualnie tylko gracz mo¿e ale w przysz³oœci nie)
	Unit* FindChestUserIfPlayer(Chest* chest);

	Unit* FindPlayerTradingWithUnit(Unit& u);
	INT2 GetSpawnPoint();
	InsideLocationLevel* TryGetLevelData();
	bool ValidateTarget(Unit& u, Unit* target);

	void UpdateLights(vector<Light>& lights);

	bool IsDrunkman(Unit& u);
	void PlayUnitSound(Unit& u, SOUND snd, float range=1.f);
	void UnitFall(Unit& u);
	void UnitDie(Unit& u, LevelContext* ctx, Unit* killer);
	void UnitTryStandup(Unit& u, float dt);
	void UnitStandup(Unit& u);

	void UpdatePostEffects(float dt);
	
	void SpawnDrunkmans();
	void PlayerYell(Unit& u);
	bool CanBuySell(const Item* item);
	void ResetCollisionPointers();
	void SetOutsideParams();
	UnitData& GetHero(Class clas, bool crazy = false);

	// level area
	LevelAreaContext* ForLevel(int loc, int level=-1);
	GroundItem* FindQuestGroundItem(LevelAreaContext* lac, int quest_refid, LevelAreaContext::Entry** entry = nullptr, int* item_index = nullptr);
	Unit* FindUnitWithQuestItem(LevelAreaContext* lac, int quest_refid, LevelAreaContext::Entry** entry = nullptr, int* unit_index = nullptr, int* item_iindex = nullptr);
	bool FindUnit(LevelAreaContext* lac, Unit* unit, LevelAreaContext::Entry** entry = nullptr, int* unit_index = nullptr);
	Unit* FindUnit(LevelAreaContext* lac, UnitData* data, LevelAreaContext::Entry** entry = nullptr, int* unit_index = nullptr);
	bool RemoveQuestGroundItem(LevelAreaContext* lac, int quest_refid);
	bool RemoveQuestItemFromUnit(LevelAreaContext* lac, int quest_refid);
	bool RemoveUnit(LevelAreaContext* lac, Unit* unit);

	//-----------------------------------------------------------------
	// GUI
	// panele
	LoadScreen* load_screen;
	GameGui* game_gui;
	MainMenu* main_menu;
	WorldMapGui* world_map;
	// dialogi
	Console* console;
	GameMenu* game_menu;
	Options* options;
	SaveLoad* saveload;
	CreateCharacterPanel* create_character;
	MultiplayerPanel* multiplayer_panel;
	CreateServerPanel* create_server_panel;
	PickServerPanel* pick_server_panel;
	ServerPanel* server_panel;
	InfoBox* info_box;
	Controls* controls;
	// inne
	Dialog* dialog_enc;
	Dialog* dialog_pvp;
	bool cursor_allow_move;

	void UpdateGui(float dt);
	void CloseGamePanels();
	void SetGamePanels();
	void NullGui();
	void PreinitGui();
	void InitGui();
	void LoadGuiData();
	void RemoveGui();
	void LoadGui(FileReader& f);
	void ClearGui(bool reset_mpbox);

	//-----------------------------------------------------------------
	// MENU / MAIN MENU / OPTIONS
	Class quickstart_class, autopick_class; // mo¿na po³¹czyæ
	string quickstart_name;
	bool check_updates, skip_tutorial;
	uint skip_version;
	string save_input_text;
	int hair_redo_index;

	bool CanShowMenu();
	void MainMenuEvent(int index);
	void MenuEvent(int id);
	void OptionsEvent(int id);
	void SaveLoadEvent(int id);
	void SaveEvent(int id);
	void SaveOptions();
	void ShowOptions();
	void ShowMenu();
	void ShowSavePanel();
	void ShowLoadPanel();
	void StartNewGame();
	void StartTutorial();
	void NewGameCommon(Class clas, cstring name, HumanData& hd, CreatedCharacter& cc, bool tutorial);
	void ShowCreateCharacterPanel(bool enter_name, bool redo=false);
	void StartQuickGame();
	void DialogNewVersion(int);
	void MultiplayerPanelEvent(int id);
	void CreateServerEvent(int id);
	// set for random player character (clas is in/out)
	void RandomCharacter(Class& clas, int& hair_index, HumanData& hd, CreatedCharacter& cc);
	void OnEnterIp(int id);
	void GenericInfoBoxUpdate(float dt);
	void QuickJoinIp();
	void AddMultiMsg(cstring msg);
	void Quit();
	bool ValidateNick(cstring nick);
	void UpdateLobbyNet(float dt);
	bool DoLobbyUpdate(BitStream& stream);
	void OnCreateCharacter(int id);
	void OnPlayTutorial(int id);
	void OnQuit(int);
	void OnExit(int);
	void ShowQuitDialog();
	void OnPickServer(int id);
	void EndConnecting(cstring msg, bool wait=false);
	void CloseConnection(VoidF f);
	void DoQuit();
	void RestartGame();
	void ClearAndExitToMenu(cstring msg);

	//-----------------------------------------------------------------
	// MULTIPLAYER
	RakNet::RakPeerInterface* peer;
	string server_name, player_name, server_pswd, server_ip, enter_pswd, server_name2;
	int autostart_count;//, kick_timer;
	int players; // aktualna liczba graczy w grze
	int max_players, max_players2; // maksymalna liczba graczy w grze
	int my_id; // moje unikalne id
	int last_id;
	int last_startup_id;
	bool sv_server, sv_online, sv_startup, was_client;
	BitStream server_info;
	vector<byte> packet_data;
	vector<PlayerInfo> game_players, old_players;
	vector<int> players_left;
	SystemAddress server;
	int leader_id, kick_id;
	float startup_timer;
	enum NET_MODE
	{
		NM_CONNECT_IP, // ³¹czenie serwera z klientem (0 - pingowanie, 1 - podawanie has³a, 2 - ³¹czenie)
		NM_QUITTING,
		NM_QUITTING_SERVER,
		NM_TRANSFER,
		NM_TRANSFER_SERVER,
		NM_SERVER_SEND
	} net_mode;
	int net_state, net_tries;
	VoidF net_callback;
	string net_adr;
	float net_timer, update_timer, mp_timeout;
	vector<INT2> lobby_updates;
	inline void AddLobbyUpdate(const INT2& u)
	{
		for(vector<INT2>::iterator it = lobby_updates.begin(), end = lobby_updates.end(); it != end; ++it)
		{
			if(*it == u)
				break;
		}
		lobby_updates.push_back(u);
	}
	BitStream net_stream, net_stream2;
	bool change_title_a;
	bool level_generated;
	int netid_counter, item_netid_counter, chest_netid_counter, useable_netid_counter, skip_id_counter, trap_netid_counter, door_netid_counter, electro_netid_counter;
	vector<NetChange> net_changes;
	vector<NetChangePlayer> net_changes_player;
	vector<string*> net_talk;
	union
	{
		//Unit* loot_unit;
		//Chest* loot_chest;
		struct
		{
			GroundItem* picking_item;
			int picking_item_state;
		};
	};
	struct WarpData
	{
		Unit* u;
		int where; // -1-z budynku, >=0-do budynku
		float timer;
	};
	vector<WarpData> mp_warps;
	vector<Item*> quest_items;
	float train_move; // u¿ywane przez klienta do trenowania przez chodzenie
	bool anyone_talking;
	// u¿ywane u klienta który nie zapamiêtuje zmiennej 'pc'
	bool godmode, noclip, invisible;
	vector<INT2> minimap_reveal_mp;
	bool boss_level_mp; // u¿ywane u klienta zamiast boss_levels
	bool mp_load;
	float mp_interp;
	bool mp_use_interp;
	ObjectPool<EntityInterpolator> interpolators;
	float interpolate_timer;
	int mp_port;
	bool paused, pick_autojoin;

	// zwraca czy pozycja siê zmieni³a
	void UpdateInterpolator(EntityInterpolator* e, float dt, VEC3& pos, float& rot);
	void InterpolateUnits(float dt);
	void InterpolatePlayers(float dt);

	// sprawdza czy aktualna gra jest online
	inline bool IsOnline() const { return sv_online; }
	// sprawdza czy ja jestem serwerem
	inline bool IsServer() const { return sv_server; }
	// sprawdza czy ja jestem klientem
	inline bool IsClient() const { return !sv_server; }
	inline bool IsClient2() const { return sv_online && !sv_server; }
	// czy jest serwerem lub pojedyñczy gracz
	inline bool IsLocal() const { return !IsOnline() || IsServer(); }

	void InitServer();
	void InitClient();
	void UpdateServerInfo();
	int FindPlayerIndex(cstring nick, bool not_left=false);
	int FindPlayerIndex(const SystemAddress& adr);
	int GetPlayerIndex(int id);
	void AddServerMsg(cstring msg);
	void KickPlayer(int index);
	void ChangeReady();
	void CheckReady();
	void AddMsg(cstring msg);
	void OnEnterPassword(int id);
	void ForceRedraw();
	void PrepareLevelData(BitStream& stream);
	void WriteUnit(BitStream& stream, Unit& unit);
	void WriteDoor(BitStream& stream, Door& door);
	void WriteItem(BitStream& stream, GroundItem& item);
	void WriteChest(BitStream& stream, Chest& chest);
	void WriteTrap(BitStream& stream, Trap& trap);
	bool ReadLevelData(BitStream& stream);
	bool ReadUnit(BitStream& stream, Unit& unit);
	bool ReadDoor(BitStream& stream, Door& door);
	bool ReadItem(BitStream& stream, GroundItem& item);
	bool ReadChest(BitStream& stream, Chest& chest);
	bool ReadTrap(BitStream& stream, Trap& trap);
	void SendPlayerData(int index);
	bool ReadPlayerData(BitStream& stream);
	Unit* FindUnit(int netid);
	void UpdateServer(float dt);
	bool ProcessControlMessageServer(BitStream& stream, PlayerInfo& info);
	void WriteServerChanges(BitStream& stream);
	int WriteServerChangesForPlayer(BitStream& stream, PlayerInfo& info);
	void UpdateClient(float dt);
	bool ProcessControlMessageClient(BitStream& stream, bool& exit_from_server);
	bool ProcessControlMessageClientForMe(BitStream& stream);
	void WriteClientChanges(BitStream& stream);
	void Client_Say(BitStream& stream);
	void Client_Whisper(BitStream& stream);
	void Client_ServerSay(BitStream& stream);
	void Server_Say(BitStream& stream, PlayerInfo& info, Packet* packet);
	void Server_Whisper(BitStream& stream, PlayerInfo& info, Packet* packet);
	void ServerProcessUnits(vector<Unit*>& units);
	GroundItem* FindItemNetid(int netid, LevelContext** ctx=nullptr);
	inline PlayerInfo& GetPlayerInfo(int id)
	{
		for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
		{
			if(it->id == id)
				return *it;
		}
		assert(0);
		return game_players[0];
	}
	inline PlayerInfo* GetPlayerInfoTry(int id)
	{
		for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
		{
			if(it->id == id)
				return &*it;
		}
		return nullptr;
	}
	inline PlayerInfo& GetPlayerInfo(PlayerController* player)
	{
		return GetPlayerInfo(player->id);
	}
	inline PlayerInfo* GetPlayerInfoTry(PlayerController* player)
	{
		return GetPlayerInfoTry(player->id);
	}
	inline void PushNetChange(NetChange::TYPE type)
	{
		NetChange& c = Add1(net_changes);
		c.type = type;
	}
	void UpdateWarpData(float dt);
	inline void Net_AddQuest(int refid)
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::ADD_QUEST;
		c.id = refid;
	}
	inline void Net_RegisterItem(const Item* item, const Item* base_item)
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::REGISTER_ITEM;
		c.item2 = item;
		c.base_item = base_item;
	}
	inline void Net_AddItem(PlayerController* player, const Item* item, bool is_team)
	{
		NetChangePlayer& c = Add1(net_changes_player);
		c.type = NetChangePlayer::ADD_ITEMS;
		c.pc = player;
		c.item = item;
		c.id = (is_team ? 1 : 0);
		c.ile = 1;
		GetPlayerInfo(player).NeedUpdate();
	}
	inline void Net_AddedItemMsg(PlayerController* player)
	{
		NetChangePlayer& c = Add1(net_changes_player);
		c.pc = player;
		c.type = NetChangePlayer::ADDED_ITEM_MSG;
		GetPlayerInfo(player).NeedUpdate();
	}
	inline void Net_AddItems(PlayerController* player, const Item* item, int ile, bool is_team)
	{
		NetChangePlayer& c = Add1(net_changes_player);
		c.type = NetChangePlayer::ADD_ITEMS;
		c.pc = player;
		c.item = item;
		c.id = (is_team ? ile : 0);
		c.ile = ile;
		GetPlayerInfo(player).NeedUpdate();
	}
	inline void Net_UpdateQuest(int refid)
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::UPDATE_QUEST;
		c.id = refid;
	}
	inline void Net_UpdateQuestMulti(int refid, int ile)
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::UPDATE_QUEST_MULTI;
		c.id = refid;
		c.ile = ile;
	}
	inline void Net_RenameItem(const Item* item)
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::RENAME_ITEM;
		c.base_item = item;
	}
	inline void Net_RemoveQuestItem(PlayerController* player, int refid)
	{
		NetChangePlayer& c = Add1(net_changes_player);
		c.type = NetChangePlayer::REMOVE_QUEST_ITEM;
		c.pc = player;
		c.id = refid;
		GetPlayerInfo(player).NeedUpdate();
	}
	inline void Net_ChangeLocationState(int id, bool visited)
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::CHANGE_LOCATION_STATE;
		c.id = id;
		c.ile = (visited ? 1 : 0);
	}
	inline void Net_RecruitNpc(Unit* unit)
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::RECRUIT_NPC;
		c.unit = unit;
	}
	inline void Net_RemoveUnit(Unit* unit)
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::REMOVE_UNIT;
		c.id = unit->netid;
	}
	inline void Net_KickNpc(Unit* unit)
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::KICK_NPC;
		c.id = unit->netid;
	}
	inline void Net_SpawnUnit(Unit* unit)
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::SPAWN_UNIT;
		c.unit = unit;
	}
	inline void Net_PrepareWarp(PlayerController* player)
	{
		NetChangePlayer& c = Add1(net_changes_player);
		c.type = NetChangePlayer::PREPARE_WARP;
		c.pc = player;
		GetPlayerInfo(player).NeedUpdate();
	}
	inline void Net_StartDialog(PlayerController* player, Unit* talker)
	{
		NetChangePlayer& c = Add1(net_changes_player);
		c.type = NetChangePlayer::START_DIALOG;
		c.pc = player;
		c.id = talker->netid;
		GetPlayerInfo(player).NeedUpdate();
	}
#define WHERE_LEVEL_UP -1
#define WHERE_LEVEL_DOWN -2
#define WHERE_OUTSIDE -3
#define WHERE_PORTAL 0
	inline void Net_LeaveLocation(int where)
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::LEAVE_LOCATION;
		c.id = where;
	}
	void Net_OnNewGameServer();
	void Net_OnNewGameClient();
	// szuka questowych przedmiotów u klienta
	const Item* FindQuestItemClient(cstring id, int refid) const;
	//void ConvertPlayerToAI(PlayerInfo& info);
	Useable* FindUseable(int netid);
	// read item id and return it (can be quest item or gold), results: -2 read error, -1 not found, 0 empty, 1 ok
	int ReadItemAndFind(BitStream& stream, const Item*& item) const;
	bool ReadItemList(BitStream& stream, vector<ItemSlot>& items);
	bool ReadItemListTeam(BitStream& stream, vector<ItemSlot>& items);
	Door* FindDoor(int netid);
	Trap* FindTrap(int netid);
	bool RemoveTrap(int netid);
	Chest* FindChest(int netid);
	void ReequipItemsMP(Unit& unit); // zak³ada przedmioty które ma w ekipunku, dostaje broñ jeœli nie ma, podnosi z³oto
	Electro* FindElectro(int netid);
	void UseDays(PlayerController* player, int count);
	PlayerInfo* FindOldPlayer(cstring nick);
	void PrepareWorldData(BitStream& stream);
	bool ReadWorldData(BitStream& stream);
	void WriteNetVars(BitStream& stream);
	bool ReadNetVars(BitStream& stream);
	void WritePlayerStartData(BitStream& stream, PlayerInfo& info);
	bool ReadPlayerStartData(BitStream& stream);
	bool CheckMoveNet(Unit& unit, const VEC3& pos);
	void Net_PreSave();
	bool FilterOut(NetChange& c);
	bool FilterOut(NetChangePlayer& c);
	void Net_FilterServerChanges();
	void Net_FilterClientChanges();
	void ProcessLeftPlayers();
	void ClosePeer(bool wait=false);
	void DeleteOldPlayers();
	inline NetChangePlayer& AddChange(NetChangePlayer::TYPE type, PlayerController* pc)
	{
		assert(pc);
		NetChangePlayer& c = Add1(net_changes_player);
		c.type = type;
		c.pc = pc;
		pc->player_info->NeedUpdate();
		return c;
	}
	void RemovePlayerOnLoad(PlayerInfo& info);

	BitStream& StreamStart(Packet* packet, StreamLogType type);
	void StreamEnd();
	void StreamError();

	BitStream current_stream;
	Packet* current_packet;

	//-----------------------------------------------------------------
	// WORLD MAP
	void AddLocations(uint count, LOCATION type, float dist, bool unique_name);
	void AddLocations(uint count, const LOCATION* types, uint type_count, float dist, bool unique_name);
	void AddLocations(const LOCATION* types, uint count, float dist, bool unique_name);
	void EnterLocationCallback();
	bool EnterLocation(int level=0, int from_portal=-1, bool close_portal=false);
	void GenerateWorld();
	void ApplyTiles(float* h, TerrainTile* tiles);
	void SpawnBuildings(vector<CityBuilding>& buildings);
	void SpawnUnits(City* city);
	void RespawnUnits();
	void RespawnUnits(LevelContext& ctx);
	void LeaveLocation(bool clear = false, bool end_buffs = true);
	void GenerateDungeon(Location& loc);
	void SpawnCityPhysics();
	// zwraca Object lub Useable lub Chest!!!, w przypadku budynku rot musi byæ równe 0, PI/2, PI, 3*2/PI (w przeciwnym wypadku bêdzie 0)
	Object* SpawnObject(LevelContext& ctx, Obj* obj, const VEC3& pos, float rot, float scale=1.f, VEC3* out_point=nullptr, int variant=-1);
	void RespawnBuildingPhysics();
	void SpawnCityObjects();
	// roti jest u¿ywane tylko do ustalenia czy k¹t jest zerowy czy nie, mo¿na przerobiæ t¹ funkcjê ¿eby tego nie u¿ywa³a wogóle
	void ProcessBuildingObjects(LevelContext& ctx, City* city, InsideBuilding* inside, Animesh* mesh, Animesh* inside_mesh, float rot, int roti, const VEC3& shift, BUILDING type,
		CityBuilding* building, bool recreate=false, VEC3* out_point=nullptr);
	void GenerateForest(Location& loc);
	void SpawnForestObjects(int road_dir=-1); //-1 brak, 0 -, 1 |
	void CreateForestMinimap();
	void SpawnOutsideBariers();
	void GetOutsideSpawnPoint(VEC3& pos, float& dir);
	void SpawnForestUnits(const VEC3& team_pos);
	void RepositionCityUnits();
	void Event_RandomEncounter(int id);
	void GenerateEncounterMap(Location& loc);
	void SpawnEncounterUnits(DialogEntry*& dialog, Unit*& talker, Quest*& quest);
	void SpawnEncounterObjects();
	void SpawnEncounterTeam();
	Encounter* AddEncounter(int& id);
	void RemoveEncounter(int id);
	Encounter* GetEncounter(int id);
	Encounter* RecreateEncounter(int id);
	int GetRandomSpawnLocation(const VEC2& pos, SPAWN_GROUP group, float range=160.f);
	void DoWorldProgress(int days);
	Location* CreateLocation(LOCATION type, int levels=-1);
	void UpdateLocation(LevelContext& ctx, int days, int open_chance, bool reset);
	void UpdateLocation(int days, int open_chance, bool reset);
	void GenerateCamp(Location& loc);
	void SpawnCampObjects();
	void SpawnCampUnits();
	Object* SpawnObjectNearLocation(LevelContext& ctx, Obj* obj, const VEC2& pos, float rot, float range=2.f, float margin=0.3f, float scale=1.f);
	Object* SpawnObjectNearLocation(LevelContext& ctx, Obj* obj, const VEC2& pos, const VEC2& rot_target, float range=2.f, float margin=0.3f, float scale=1.f);
	int GetClosestLocation(LOCATION type, const VEC2& pos, int target = -1);
	int GetClosestLocationNotTarget(LOCATION type, const VEC2& pos, int not_target);
	int CreateCamp(const VEC2& pos, SPAWN_GROUP group, float range=64.f, bool allow_exact=true);
	void SpawnTmpUnits(City* city);
	void RemoveTmpUnits(City* city);
	void RemoveTmpUnits(LevelContext& ctx);
	int AddLocation(Location* loc);
	// tworzy lokacjê (jeœli range<0 to pozycja jest dowolna a range=-range, level=-1 - losowy poziom, =0 - minimalny, =9 maksymalny, =liczba - okreœlony)
	int CreateLocation(LOCATION type, const VEC2& pos, float range=64.f, int target=-1, SPAWN_GROUP spawn=SG_LOSOWO, bool allow_exact=true, int levels=-1);
	bool FindPlaceForLocation(VEC2& pos, float range=64.f, bool allow_exact=true);
	int FindLocationId(Location* loc);
	void Event_StartEncounter(int id);
	void GenerateMoonwell(Location& loc);
	void SpawnMoonwellObjects();
	void SpawnMoonwellUnits(const VEC3& team_pos);
#define SOE_DONT_SPAWN_PARTICLES (1<<0)
#define SOE_MAGIC_LIGHT (1<<1)
#define SOE_DONT_CREATE_LIGHT (1<<2)
	void SpawnObjectExtras(LevelContext& ctx, Obj* obj, const VEC3& pos, float rot, void* user_ptr, btCollisionObject** phy_result, float scale=1.f, int flags=0);
	void GenerateSecretLocation(Location& loc);
	void SpawnSecretLocationObjects();
	void SpawnSecretLocationUnits();
	void SpawnTeamSecretLocation();
	void GenerateMushrooms(int days_since=10);
	void GenerateCityPickableItems();
	void PickableItemBegin(LevelContext& ctx, Object& o);
	void PickableItemAdd(const Item* item);
	void GenerateDungeonFood();
	void GenerateCityMap(Location& loc);
	void GenerateVillageMap(Location& loc);
	void GetCityEntry(VEC3& pos, float& rot);
	void AbadonLocation(Location* loc);
	
	vector<Location*> locations; // lokacje w grze, mo¿e byæ nullptr
	Location* location; // wskaŸnik na aktualn¹ lokacjê [odtwarzany]
	int current_location; // aktualna lokacja lub -1
	int picked_location; // zaznaczona lokacja na mapie œwiata, ta do której siê wêdruje lub -1 [tylko jeœli world_state==WS_TRAVEL]
	int open_location; // aktualnie otwarta lokacja (w sensie wczytanych zasobów, utworzonych jednostek itp) lub -1 [odtwarzany]
	int travel_day; // liczba dni w podró¿y [tylko jeœli world_state==WS_TRAVEL]
	int enc_kierunek; // kierunek z której strony nadesz³a dru¿yna w czasie spotkania [tymczasowe]
	int spotkanie; // rodzaj losowego spotkania [tymczasowe]
	int enc_tryb; // 0 - losowa walka, 1 - specjalne spotkanie, 2 - questowe spotkanie [tymczasowe]
	int empty_locations; // liczba pustych lokacji
	int create_camp; // licznik do stworzenia nowego obozu
	WORLDMAP_STATE world_state; // stan na mapie œwiata (stoi, podró¿uje)
	VEC2 world_pos; // pozycja na mapie œwiata
	VEC2 travel_start; // punkt startu podró¿y na mapie œwiata [tylko jeœli world_state==WS_TRAVEL]
	float travel_time; // czas podró¿y na mapie [tylko jeœli world_state==WS_TRAVEL]
	float travel_time2; // licznik aktualizacji szansy na spotkanie
	float world_dir; // kierunek podró¿y/wejœcia na mapê, to jest nowy k¹t (0 w prawo), wskazuje od œrodka do krawêdzi mapy
	float szansa_na_spotkanie; // szansa na spotkanie na mapie œwiata
	bool far_encounter; // czy dru¿yna gracza jest daleko w czasie spotkania [tymczasowe]
	bool guards_enc_reward; // czy odebrano nagrodê za uratowanie stra¿ników w czasie spotkania
	uint cities; // liczba miast i wiosek
	uint encounter_loc; // id lokacji spotkania losowego
	SPAWN_GROUP losowi_wrogowie; // wrogowie w czasie spotkania [tymczasowe]
	vector<Encounter*> encs; // specjalne spotkania na mapie œwiata [odtwarzane przy wczytywaniu questów]
	Encounter* game_enc; // spotkanie w czasie podró¿y [tymczasowe]
	LocationEventHandler* location_event_handler; // obs³uga wydarzeñ lokacji
	bool first_city;
	vector<INT2> boss_levels; // dla oznaczenia gdzie graæ muzykê (x-lokacja, y-poziom)
#define ENTER_FROM_PORTAL 0
#define ENTER_FROM_OUTSIDE -1
#define ENTER_FROM_UP_LEVEL -2
#define ENTER_FROM_DOWN_LEVEL -3
#define ENTER_FROM_UNKNOWN -4
	int enter_from; // sk¹d siê przysz³o (u¿ywane przy wczytywanie w MP gdy do³¹cza nowa postaæ)
	bool g_have_well;
	INT2 g_well_pt;

	//-----------------------------------------------------------------
	// WORLD STATE
	enum WorldProgressMode
	{
		WPM_NORMAL,
		WPM_TRAVEL,
		WPM_SKIP
	};
	void WorldProgress(int days, WorldProgressMode mode);

	int year; // rok w grze [zaczyna siê od 100, ustawiane w NewGame]
	int month; // miesi¹c w grze [od 0 do 11, ustawiane w NewGame]
	int day; // dzieñ w grze [od 0 do 29, ustawiane w NewGame]
	int worldtime; // licznik dni [od 0, ustawiane w NewGame]
	int gt_hour; // iloœæ godzin jakie gracz gra
	int gt_minute; // iloœæ minut jakie gracz gra [0-59]
	int gt_second; // iloœæ sekund jakie gracz gra [0-59]
	float gt_tick; // licznik czasu do sekundy grania


	Config cfg;
	void SaveCfg();
};
