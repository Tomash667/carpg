#pragma once

#include "Engine.h"
#include "Const.h"
#include "GameCommon.h"
#include "Object.h"
#include "ConsoleCommands.h"
#include "Net.h"
#include "Building.h"
#include "Dialog.h"
#include "BaseLocation.h"
#include "GroundItem.h"
#include "ParticleSystem.h"
#include "GameKeys.h"
#include "SceneNode.h"
#include "QuadTree.h"
#include "Music.h"
#include "PlayerInfo.h"
#include "Camera.h"
#include "Config.h"
#include "UnitEventHandler.h"
#include "LevelArea.h"
#include "SaveSlot.h"
#include "Mapa2.h"
#include "Location.h"
#include "Unit.h"
#include "ResourceManager.h"

enum TRAP_TYPE;

struct CameraCollider;
struct City;
struct CityBuilding;
struct Encounter;
struct InsideBuilding;
struct InsideLocationLevel;
struct Quest_Event;
struct TerrainTile;

class Console;
class Controls;
class CreateCharacterPanel;
class CreateServerPanel;
class DialogBox;
class GameGui;
class GameMenu;
class InfoBox;
class MainMenu;
class MultiplayerPanel;
class Options;
class PickServerPanel;
class SaveLoad;
class ServerPanel;
class WorldMapGui;

//#define DRAW_LOCAL_PATH
#ifdef DRAW_LOCAL_PATH
#	ifndef _DEBUG
#		error "DRAW_LOCAL_PATH in release!"
#	endif
#endif

struct APoint
{
	int odleglosc, koszt, suma, stan;
	Int2 prev;

	bool IsLower(int suma2) const
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
	GS_LOAD,
	GS_EXIT_TO_MENU,
	GS_QUIT
};

//-----------------------------------------------------------------------------
// Stan mapy �wiata
enum WORLDMAP_STATE
{
	WS_MAIN,
	WS_TRAVEL,
	WS_ENCOUNTER
};

struct EntityInterpolator;

enum AllowInput
{
	ALLOW_NONE = 0,	// 00
	ALLOW_KEYBOARD = 1,	// 01
	ALLOW_MOUSE = 2,	// 10
	ALLOW_INPUT = 3	// 11
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
	CG_TERRAIN = 1 << 7,
	CG_BUILDING = 1 << 8,
	CG_UNIT = 1 << 9,
	CG_OBJECT = 1 << 10,
	CG_DOOR = 1 << 11,
	CG_COLLIDER = 1 << 12,
	CG_CAMERA_COLLIDER = 1 << 13,
	CG_BARRIER = 1 << 14, // blocks only units
	// max 1 << 15
};

extern const float ATTACK_RANGE;
extern const Vec2 ALERT_RANGE;
extern const float PICKUP_RANGE;
extern const float ARROW_TIMER;
extern const float MIN_H;

struct AttachedSound
{
	FMOD::Channel* channel;
	Unit* unit;
};

static_assert(sizeof(time_t) == sizeof(__int64), "time_t needs to be 64 bit");

struct UnitView
{
	Unit* unit;
	Vec3 last_pos;
	float time;
	bool valid;
};

enum GMS
{
	GMS_NEED_WEAPON = 1,
	GMS_NEED_KEY,
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
	GMS_PICK_CHARACTER,
	GMS_ADDED_ITEM,
	GMS_TOO_COMPLICATED
};

struct UnitWarpData
{
	Unit* unit;
	int where;
};

enum class FALLBACK
{
	NO = -1,
	TRAIN,
	REST,
	ARENA,
	ENTER,
	EXIT,
	CHANGE_LEVEL,
	NONE,
	ARENA_EXIT,
	USE_PORTAL,
	WAIT_FOR_WARP,
	ARENA2,
	CLIENT,
	CLIENT2
};

enum InventoryMode
{
	I_NONE,
	I_INVENTORY,
	I_LOOT_BODY,
	I_LOOT_CHEST,
	I_TRADE,
	I_SHARE,
	I_GIVE,
	I_LOOT_CONTAINER
};

struct TeamShareItem
{
	Unit* from, *to;
	const Item* item;
	int index, value, priority;
};

enum DRAW_FLAGS
{
	DF_TERRAIN = 1 << 0,
	DF_OBJECTS = 1 << 1,
	DF_UNITS = 1 << 2,
	DF_PARTICLES = 1 << 3,
	DF_SKYBOX = 1 << 4,
	DF_BULLETS = 1 << 5,
	DF_BLOOD = 1 << 6,
	DF_ITEMS = 1 << 7,
	DF_USABLES = 1 << 8,
	DF_TRAPS = 1 << 9,
	DF_AREA = 1 << 10,
	DF_EXPLOS = 1 << 11,
	DF_LIGHTINGS = 1 << 12,
	DF_PORTALS = 1 << 13,
	DF_GUI = 1 << 14,
	DF_MENU = 1 << 15,
};

struct TutorialText
{
	cstring text;
	Vec3 pos;
	int state; // 0 - nie aktywny, 1 - aktywny, 2 - uruchomiony
	int id;
};

typedef delegate<void(cstring)> PrintMsgFunc;

struct EntityInterpolator
{
	static const int MAX_ENTRIES = 4;
	struct Entry
	{
		Vec3 pos;
		float rot;
		float timer;

		void operator = (const Entry& e)
		{
			pos = e.pos;
			rot = e.rot;
			timer = e.timer;
		}
	} entries[MAX_ENTRIES];
	int valid_entries;

	void Reset(const Vec3& pos, float rot);
	void Add(const Vec3& pos, float rot);
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
	Vec4 skill;
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
	Stream_None,
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
	Stream_UpdateGameClient,
	Stream_Chat
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

typedef std::map<Mesh*, TEX> ItemTextureMap;

struct Game final : public Engine, public UnitEventHandler
{
	Game();
	~Game();

	struct ObjectEntity
	{
		enum Type
		{
			NONE,
			OBJECT,
			USABLE,
			CHEST
		} type;
		union
		{
			Object* object;
			Usable* usable;
			Chest* chest;
		};

		ObjectEntity(nullptr_t) : object(nullptr), type(NONE) {}
		ObjectEntity(Object* object) : object(object), type(OBJECT) {}
		ObjectEntity(Usable* usable) : usable(usable), type(USABLE) {}
		ObjectEntity(Chest* chest) : chest(chest), type(CHEST) {}

		operator bool()
		{
			return type != NONE;
		}
		operator Object* ()
		{
			assert(type == OBJECT || type == NONE);
			return object;
		}
		operator Usable* ()
		{
			assert(type == USABLE || type == NONE);
			return usable;
		}
		operator Chest* ()
		{
			assert(type == CHEST || type == NONE);
			return chest;
		}

		bool IsChest() const { return type == CHEST; }
		bool IsObject() const { return type == OBJECT; }
		bool IsUsable() const { return type == USABLE; }
	};

	void OnCleanup();
	void OnDraw();
	void OnDraw(bool normal = true);
	void OnTick(float dt);
	void OnChar(char c);
	void OnReload();
	void OnReset();
	void OnResize();
	void OnFocus(bool focus, const Int2& activation_point);

	bool Start0(StartupOptions& options);
	void GetTitle(LocalString& s);
	void ChangeTitle();
	void ClearPointers();
	void CreateTextures();
	void PreloadData();
	void SetMeshSpecular();

	// initialization
	bool InitGame() override;
	void PreconfigureGame();
	void PreloadLanguage();
	void CreatePlaceholderResources();

	// loading system
	void LoadSystem();
	void AddFilesystem();
	void LoadDatafiles();
	bool LoadRequiredStats(uint& errors);
	void LoadLanguageFiles();
	void SetHeroNames();
	void SetGameText();
	void SetStatsText();
	void ConfigureGame();

	// loading data
	void LoadData();
	void AddLoadTasks();

	// after loading data
	void PostconfigureGame();
	void StartGameMode();

	QUICKSTART quickstart;
	bool disable_net_stats;

	// supershader
	string sshader_code;
	FILETIME sshader_edit_time;
	ID3DXEffectPool* sshader_pool;
	vector<SuperShader> sshaders;
	D3DXHANDLE hSMatCombined, hSMatWorld, hSMatBones, hSTint, hSAmbientColor, hSFogColor, hSFogParams, hSLightDir, hSLightColor, hSLights, hSSpecularColor, hSSpecularIntensity,
		hSSpecularHardness, hSCameraPos, hSTexDiffuse, hSTexNormal, hSTexSpecular;
	void InitSuperShader();
	uint GetSuperShaderId(bool animated, bool have_binormals, bool fog, bool specular, bool normal, bool point_light, bool dir_light) const;
	SuperShader* GetSuperShader(uint id);
	SuperShader* CompileSuperShader(uint id);

	float light_angle;
	bool dungeon_tex_wrap;

	void SetupSuperShader();
	void ReloadShaders();
	void ReleaseShaders();
	void ShaderVersionChanged();

	// scene
	bool cl_normalmap, cl_specularmap, cl_glow;
	DrawBatch draw_batch;
	VDefault blood_v[4];
	VParticle billboard_v[4];
	Vec3 billboard_ext[4];
	VParticle portal_v[4];
	IDirect3DVertexDeclaration9* vertex_decl[VDI_MAX];
	int uv_mod;
	QuadTree quadtree;
	float grass_range;
	LevelParts level_parts;
	VB vbInstancing;
	uint vb_instancing_max;
	vector< const vector<Matrix>* > grass_patches[2];
	uint grass_count[2];
	float lights_dt;

	void InitScene();
	void CreateVertexDeclarations();
	void BuildDungeon();
	void ChangeDungeonTexWrap();
	void FillDungeonPart(Int2* dungeon_part, word* faces, int& index, word offset);
	void CleanScene();
	void ListDrawObjects(LevelContext& ctx, FrustumPlanes& frustum, bool outside);
	void ListDrawObjectsUnit(LevelContext* ctx, FrustumPlanes& frustum, bool outside, Unit& u);
	void AddObjectToDrawBatch(LevelContext& ctx, const Object& o, FrustumPlanes& frustum);
	void ListAreas(LevelContext& ctx);
	void PrepareAreaPath();
	void PrepareAreaPathCircle(Area2& area, float radius, float range, float rot, bool outside);
	void FillDrawBatchDungeonParts(FrustumPlanes& frustum);
	void AddOrSplitSceneNode(SceneNode* node, int exclude_subs = 0);
	int GatherDrawBatchLights(LevelContext& ctx, SceneNode* node, float x, float z, float radius, int sub = 0);
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
	void DrawStunEffects(const vector<StunEffect>& stuns);
	void DrawAreas(const vector<Area>& areas, float range, const vector<Area2*>& areas2);
	void DrawPortals(const vector<Portal*>& portals);
	void UvModChanged();
	void InitQuadTree();
	void DrawGrass();
	void ListGrass();
	void SetTerrainTextures();
	void ClearQuadtree();
	void ClearGrass();
	void VerifyObjects();
	void VerifyObjects(vector<Object*>& objects, int& errors);
	void CalculateQuadtree();
	void ListQuadtreeNodes();

	// profiler
	int profiler_mode;

	//-----------------------------------------------------------------
	// ZASOBY
	//-----------------------------------------------------------------
	MeshPtr aHumanBase, aHair[5], aBeard[5], aMustache[2], aEyebrows;
	MeshPtr aBox, aCylinder, aSphere, aCapsule;
	MeshPtr aArrow, aSkybox, aBag, aChest, aGrating, aDoorWall, aDoorWall2, aStairsDown, aStairsDown2, aStairsUp, aSpellball, aPressurePlate, aDoor, aDoor2, aStun;
	VertexDataPtr vdSchodyGora, vdSchodyDol, vdNaDrzwi;
	TEX tItemRegion, tItemRegionRot, tMinimap, tChar, tSave;
	TEX tCzern, tEmerytura, tPortal, tLightingLine, tRip, tEquipped, tMiniSave, tWarning, tError;
	TexturePtr tKrew[BLOOD_MAX], tKrewSlad[BLOOD_MAX], tFlare, tFlare2, tIskra, tWoda, tSpawn;
	TexturePack tFloor[2], tWall[2], tCeil[2], tFloorBase, tWallBase, tCeilBase;
	ID3DXEffect* eMesh, *eParticle, *eSkybox, *eTerrain, *eArea, *eGui, *ePostFx, *eGlow, *eGrass;
	D3DXHANDLE techAnim, techHair, techAnimDir, techHairDir, techMesh, techMeshDir, techMeshSimple, techMeshSimple2, techMeshExplo, techParticle, techSkybox,
		techTerrain, techArea, techTrail, techGlowMesh, techGlowAni, techGrass;
	D3DXHANDLE hAniCombined, hAniWorld, hAniBones, hAniTex, hAniFogColor, hAniFogParam, hAniTint, hAniHairColor, hAniAmbientColor, hAniLightDir, hAniLightColor, hAniLights,
		hMeshCombined, hMeshWorld, hMeshTex, hMeshFogColor, hMeshFogParam, hMeshTint, hMeshAmbientColor, hMeshLightDir, hMeshLightColor, hMeshLights,
		hParticleCombined, hParticleTex, hSkyboxCombined, hSkyboxTex, hAreaCombined, hAreaColor, hAreaPlayerPos, hAreaRange,
		hTerrainCombined, hTerrainWorld, hTerrainTexBlend, hTerrainTex[5], hTerrainColorAmbient, hTerrainColorDiffuse, hTerrainLightDir, hTerrainFogColor, hTerrainFogParam,
		hGuiSize, hGuiTex, hPostTex, hPostPower, hPostSkill, hGlowCombined, hGlowBones, hGlowColor, hGlowTex, hGrassViewProj, hGrassTex, hGrassFogColor, hGrassFogParams, hGrassAmbientColor;
	SOUND sGulp, sCoins, sBow[2], sDoor[3], sDoorClosed[2], sDoorClose, sItem[8], sTalk[4], sChestOpen, sChestClose, sDoorBudge, sRock, sWood, sCrystal,
		sMetal, sBody[5], sBone, sSkin, sArenaFight, sArenaWin, sArenaLost, sUnlock, sEvil, sXarTalk, sOrcTalk, sGoblinTalk, sGolemTalk, sEat, sSummon;
	VB vbParticle;
	SURFACE sChar, sSave, sItemRegion, sItemRegionRot;
	static cstring txGoldPlus, txQuestCompletedGold;
	cstring txLoadGuiTextures, txLoadParticles, txLoadPhysicMeshes, txLoadModels, txLoadSpells, txLoadSounds, txLoadMusic, txGenerateWorld;
	TexturePtr tTrawa, tTrawa2, tTrawa3, tDroga, tZiemia, tPole;

	//-----------------------------------------------------------------
	// Localized texts
	//-----------------------------------------------------------------
	cstring txCreatingListOfFiles, txConfiguringGame, txLoadingItems, txLoadingObjects, txLoadingSpells, txLoadingUnits, txLoadingMusics, txLoadingBuildings,
		txLoadingRequires, txLoadingShaders, txLoadingDialogs, txLoadingLanguageFiles, txPreloadAssets;
	cstring txAiNoHpPot[2], txAiJoinTour[4], txAiCity[2], txAiVillage[2], txAiMoonwell, txAiForest, txAiCampEmpty, txAiCampFull, txAiFort, txAiDwarfFort, txAiTower, txAiArmory, txAiHideout,
		txAiVault, txAiCrypt, txAiTemple, txAiNecromancerBase, txAiLabirynth, txAiNoEnemies, txAiNearEnemies, txAiCave, txAiInsaneText[11], txAiDefaultText[9], txAiOutsideText[3],
		txAiInsideText[2], txAiHumanText[2], txAiOrcText[7], txAiGoblinText[5], txAiMageText[4], txAiSecretText[3], txAiHeroDungeonText[4], txAiHeroCityText[5], txAiBanditText[6],
		txAiHeroOutsideText[2], txAiDrunkMageText[3], txAiDrunkText[5], txAiDrunkmanText[4];
	cstring txRandomEncounter, txCamp;
	cstring txEnteringLocation, txGeneratingMap, txGeneratingBuildings, txGeneratingObjects, txGeneratingUnits, txGeneratingItems, txGeneratingPhysics, txRecreatingObjects, txGeneratingMinimap,
		txLoadingComplete, txWaitingForPlayers, txLoadingResources;
	cstring txContestNoWinner, txContestStart, txContestTalk[14], txContestWin, txContestWinNews, txContestDraw, txContestPrize, txContestNoPeople;
	cstring txTut[10], txTutNote, txTutLoc, txTour[23], txTutPlay, txTutTick;
	cstring txCantSaveGame, txSaveFailed, txSavedGameN, txLoadFailed, txQuickSave, txGameSaved, txLoadingLocations, txLoadingData, txLoadingQuests, txEndOfLoading, txCantSaveNow,
		txCantLoadGame, txLoadSignature, txLoadVersion, txLoadSaveVersionOld, txLoadMP, txLoadSP, txLoadError, txLoadErrorGeneric, txLoadOpenError;
	cstring txPvpRefuse, txWin, txWinMp, txINeedWeapon, txNoHpp, txCantDo, txDontLootFollower, txDontLootArena, txUnlockedDoor,
		txNeedKey, txLevelUp, txLevelDown, txLocationText, txLocationTextMap, txRegeneratingLevel, txGmsLooted, txGmsRumor, txGmsJournalUpdated, txGmsUsed,
		txGmsUnitBusy, txGmsGatherTeam, txGmsNotLeader, txGmsNotInCombat, txGainTextAttrib, txGainTextSkill, txNeedItem, txReallyQuit, txSecretAppear,
		txGmsAddedItem, txGmsAddedItems, txGmsTooComplicated;
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
	cstring txCreateServerFailed, txInitConnectionFailed, txServer, txYouAreLeader, txRolledNumber, txPcIsLeader, txReceivedGold, txYouDisconnected, txYouKicked,
		txGamePaused, txGameResumed, txDevmodeOn, txDevmodeOff, txPlayerLeft, txPlayerDisconnected, txPlayerQuit, txPlayerKicked, txServerClosed;
	cstring txYell[3];
	cstring txHaveErrors;

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
	ItemTextureMap item_texture_map;
	uint load_errors, load_warnings;
	TEX missing_texture;
	vector<std::pair<Unit*, bool>> units_mesh_load;
	std::set<const Item*> items_load;

	//---------------------------------
	// GUI / HANDEL
	InventoryMode inventory_mode;
	vector<ItemSlot> chest_merchant, chest_blacksmith, chest_alchemist, chest_innkeeper, chest_food_seller, chest_trade;
	bool* trader_buy;

	void StartTrade(InventoryMode mode, Unit& unit);
	void StartTrade(InventoryMode mode, vector<ItemSlot>& items, Unit* unit = nullptr);

	//---------------------------------
	// RYSOWANIE
	Matrix mat;
	int particle_count;
	Terrain* terrain;
	VB vbDungeon;
	IB ibDungeon;
	Int2 dungeon_part[16], dungeon_part2[16], dungeon_part3[16], dungeon_part4[16];
	vector<ParticleEmitter*> pes2;
	Vec4 fog_color, fog_params, ambient_color;
	int alpha_test_state;
	bool cl_fog, cl_lighting, draw_particle_sphere, draw_unit_radius, draw_hitbox, draw_phy, draw_col;
	BaseObject obj_alpha;
	float portal_anim, drunk_anim;
	// post effect u�ywa 3 tekstur lub je�li jest w��czony multisampling 3 surface i 1 tekstury
	SURFACE sPostEffect[3];
	TEX tPostEffect[3];
	VB vbFullscreen;
	vector<PostEffect> post_effects;
	SURFACE sCustom;

	//---------------------------------
	// KONSOLA I KOMENDY
	bool have_console, inactive_update, nosound, noai, devmode, default_devmode, default_player_devmode, debug_info, debug_info2, dont_wander,
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
	LocalPlayerData pc_data;
	PlayerController* pc;
	AllowInput allow_input;
	bool testing, force_seed_all, koniec_gry, local_ctx_valid, target_loc_is_camp, death_solo;
	int death_screen, dungeon_level;
	float death_fade, game_speed;
	vector<MeshInstance*> bow_instances;
	Pak* pak;
	vector<AIController*> ais;
	const Item* gold_item_ptr;
	uint force_seed, next_seed;
	vector<AttachedSound> attached_sounds;
	SaveSlot single_saves[MAX_SAVE_SLOTS], multi_saves[MAX_SAVE_SLOTS];
	vector<UnitView> unit_views;
	vector<UnitWarpData> unit_warp_data;
	LevelContext local_ctx;
	ObjectPool<TmpLevelContext> tmp_ctx_pool;
	City* city_ctx; // je�eli jest w mie�cie/wiosce to ten wska�nik jest ok, takto nullptr
	vector<Unit*> to_remove;
	CityGenerator* gen;

	MeshInstance* GetBowInstance(Mesh* mesh);

	//---------------------------------
	// SCREENSHOT
	time_t last_screenshot;
	uint screenshot_count;
	D3DXIMAGE_FILEFORMAT screenshot_format;

	//---------------------------------
	// DIALOGI
	DialogContext dialog_context;
	DialogContext* current_dialog;
	Int2 dialog_cursor_pos; // ostatnia pozycja myszki przy wyborze dialogu
	vector<string> dialog_choices; // u�ywane w MP u klienta
	string predialog;

	//---------------------------------
	// PATHFINDING
	vector<APoint> a_map;
#ifdef DRAW_LOCAL_PATH
	vector<std::pair<Vec2, int> > test_pf;
	Unit* marked;
	bool test_pf_outside;
#endif
	vector<bool> local_pfmap;

	//---------------------------------
	// FIZYKA
	btCollisionShape* shape_wall, *shape_low_ceiling, *shape_arrow, *shape_ceiling, *shape_floor, *shape_door, *shape_block, *shape_schody, *shape_schody_c[2],
		*shape_summon, *shape_barrier;
	btHeightfieldTerrainShape* terrain_shape;
	btCollisionObject* obj_arrow, *obj_terrain, *obj_spell;
	vector<CollisionObject> global_col; // wektor na tymczasowe obiekty, cz�sto u�ywany przy zbieraniu obiekt�w do kolizji
	vector<btCollisionShape*> shapes;
	vector<CameraCollider> cam_colliders;

	//---------------------------------
	// WCZYTYWANIE
	float loading_dt, loading_cap;
	Timer loading_t;
	int loading_steps, loading_index;
	DWORD clear_color2;

	//---------------------------------
	// MINIMAPA
	bool minimap_opened_doors;
	vector<Int2> minimap_reveal;

	//---------------------------------
	// FALLBACK
	FALLBACK fallback_co;
	int fallback_1, fallback_2;
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
	int arena_poziom; // poziom trudno�ci walki na arenie [1-3]
	int arena_wynik; // wynik walki na arenia [0 - gracz wygra�, 1 - gracz przegra�]
	vector<Unit*> at_arena; // jednostki na arenie
	float arena_t; // licznik czasu na arenie
	bool arena_free; // czy arena jest wolna
	Unit* arena_fighter; // posta� z kt�r� walczy si� na arenie w pvp
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
	// DRU�YNA
	int take_item_id; // u�ywane przy wymianie ekwipunku ai [tymczasowe]
	int team_share_id; // u�ywane przy wymianie ekwipunku ai [tymczasowe]
	vector<TeamShareItem> team_shares; // u�ywane przy wymianie ekwipunku ai [tymczasowe]
	vector<Int2> tmp_path;

	void AddTeamMember(Unit* unit, bool free);
	void RemoveTeamMember(Unit* unit);

	//--------------------------------------
	// QUESTS
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
	Item* GetSecretNote()
	{
		return Item::Get("sekret_kartka");
	}
	int secret_where, secret_where2;

	// tutorial
	int tut_state;
	vector<TutorialText> ttexts;
	Vec3 tut_dummy;
	Object* tut_shield, *tut_shield2;
	void UpdateTutorial();
	void TutEvent(int id);
	void EndOfTutorial(int);

	//
	vector<Unit*> warp_to_inn;

	// game news
	vector<News*> news;
	void AddNews(cstring text);

	bool show_mp_panel;
	int draw_flags;
	bool in_tutorial, finished_tutorial;

	// muzyka
	MusicType music_type;
	Music* last_music;
	vector<Music*> tracks;
	int track_id;
	MusicType GetLocationMusic();
	void LoadMusic(MusicType type, bool new_load_screen = true, bool task = false);
	void SetMusic();
	void SetMusic(MusicType type);
	void SetupTracks();
	void UpdateMusic();

	// u�ywane przy wczytywaniu gry
	vector<AIController*> ai_bow_targets, ai_cast_targets;
	vector<Location*> load_location_quest;
	vector<Unit*> load_unit_handler;
	vector<Chest*> load_chest_handler;

	bool hardcore_mode, hardcore_option;
	string hardcore_savename;
	const Item* crazy_give_item; // dawany przedmiot, nie trzeba zapisywa�
	int total_kills;
	float grayout;
	bool cl_postfx;

	bool WantAttackTeam(Unit& u)
	{
		if(Net::IsLocal())
			return u.attack_team;
		else
			return IS_SET(u.ai_mode, 0x08);
	}

	// zwraca losowy przedmiot o maksymalnej cenie, ta funkcja jest powolna!
	// mo�e zwr�ci� questowy przedmiot je�li b�dzie wystarczaj�co tani, lub unikat!
	const Item* GetRandomItem(int max_value);

	// klawisze
	void InitGameKeys();
	void ResetGameKeys();
	void SaveGameKeys();
	void LoadGameKeys();
	bool KeyAllowed(byte k)
	{
		return IS_SET(allow_input, KeyAllowState(k));
	}
	byte KeyDoReturn(GAME_KEYS gk, KeyF f)
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
	byte KeyDoReturn(GAME_KEYS gk, KeyFC f)
	{
		return KeyDoReturn(gk, (KeyF)f);
	}
	byte KeyDoReturnIgnore(GAME_KEYS gk, KeyF f, byte ignored_key)
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
	byte KeyDoReturnIgnore(GAME_KEYS gk, KeyFC f, byte ignored_key)
	{
		return KeyDoReturnIgnore(gk, (KeyF)f, ignored_key);
	}
	bool KeyDo(GAME_KEYS gk, KeyF f)
	{
		return KeyDoReturn(gk, f) != VK_NONE;
	}
	bool KeyDo(GAME_KEYS gk, KeyFC f)
	{
		return KeyDo(gk, (KeyF)f);
	}
	bool KeyDownAllowed(GAME_KEYS gk)
	{
		return KeyDo(gk, &KeyStates::Down);
	}
	bool KeyPressedReleaseAllowed(GAME_KEYS gk)
	{
		return KeyDo(gk, &KeyStates::PressedRelease);
	}
	bool KeyDownUpAllowed(GAME_KEYS gk)
	{
		return KeyDo(gk, &KeyStates::DownUp);
	}
	bool KeyDownUp(GAME_KEYS gk)
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
	bool KeyPressedUpAllowed(GAME_KEYS gk)
	{
		return KeyDo(gk, &KeyStates::PressedUp);
	}
	// Zwraca czy dany klawisze jest wyci�ni�ty, je�li nie jest to dozwolone to traktuje jak wyci�ni�ty
	bool KeyUpAllowed(byte key)
	{
		if(KeyAllowed(key))
			return Key.Up(key);
		else
			return true;
	}
	bool KeyPressedReleaseSpecial(GAME_KEYS gk, bool special)
	{
		if(special)
		{
			GameKey& k = GKey[gk];
			for(int i = 0; i < 2; ++i)
			{
				if(k.key[i] >= VK_F1 && k.key[i] <= VK_F12)
				{
					if(Key.PressedRelease(k.key[i]))
						return true;
				}
			}
		}
		return KeyPressedReleaseAllowed(gk);
	}
	// przedmioty w czasie grabienia itp s� tu przechowywane indeksy
	// ujemne warto�ci odnosz� si� do slot�w (SLOT_WEAPON = -SLOT_WEAPON-1), pozytywne do zwyk�ych przedmiot�w
	vector<int> tmp_inventory[2];
	int tmp_inventory_shift[2];

	void BuildTmpInventory(int index);
	int GetItemPrice(const Item* item, Unit& unit, bool buy);

	enum class BREAK_ACTION_MODE
	{
		NORMAL,
		FALL,
		INSTANT
	};
	void BreakUnitAction(Unit& unit, BREAK_ACTION_MODE mode = BREAK_ACTION_MODE::NORMAL, bool notify = false, bool allow_animation = false);
	void Draw();
	void ExitToMenu();
	void DoExitToMenu();
	void GenerateItemImage(TaskData& task_data);
	SURFACE DrawItemImage(const Item& item, TEX tex, SURFACE surface, float rot);
	void SetupObject(BaseObject& obj);
	void SetupCamera(float dt);
	void LoadShaders();
	void SetupShaders();
	void TakeScreenshot(bool no_gui = false);
	void UpdateGame(float dt);
	void UpdateFallback(float dt);
	void UpdatePlayer(LevelContext& ctx, float dt);
	void UseAction(PlayerController* p, bool from_server, const Vec3* pos_data = nullptr);
	void SpawnUnitEffect(Unit& unit);
	void PlayerCheckObjectDistance(Unit& u, const Vec3& pos, void* ptr, float& best_dist, BeforePlayer type);

	int CheckMove(Vec3& pos, const Vec3& dir, float radius, Unit* me, bool* is_small = nullptr);
	int CheckMovePhase(Vec3& pos, const Vec3& dir, float radius, Unit* me, bool* is_small = nullptr);

	struct IgnoreObjects
	{
		// nullptr lub tablica jednostek zako�czona nullptr
		const Unit** ignored_units;
		// nullptr lub tablica obiekt�w [u�ywalnych lub nie] zako�czona nullptr
		const void** ignored_objects;
		// czy ignorowa� bloki
		bool ignore_blocks;
		// czy ignorowa� obiekty
		bool ignore_objects;
	};
	void GatherCollisionObjects(LevelContext& ctx, vector<CollisionObject>& objects, const Vec3& pos, float radius, const IgnoreObjects* ignore = nullptr);
	void GatherCollisionObjects(LevelContext& ctx, vector<CollisionObject>& objects, const Box2d& box, const IgnoreObjects* ignore = nullptr);
	bool Collide(const vector<CollisionObject>& objects, const Vec3& pos, float radius);
	bool Collide(const vector<CollisionObject>& objects, const Box2d& box, float margin = 0.f);
	bool Collide(const vector<CollisionObject>& objects, const Box2d& box, float margin, float rot);
	void ParseCommand(const string& str, PrintMsgFunc print_func = nullptr, PARSE_SOURCE ps = PS_UNKNOWN);
	void CmdList(Tokenizer& t);
	void AddCommands();
	void AddConsoleMsg(cstring msg);
	void UpdateAi(float dt);
	void CheckAutoTalk(Unit& unit, float dt);
	void StartDialog(DialogContext& ctx, Unit* talker, GameDialog* dialog = nullptr);
	void StartDialog2(PlayerController* player, Unit* talker, GameDialog* dialog = nullptr);
	void StartNextDialog(DialogContext& ctx, GameDialog* dialog, int& if_level, Quest* quest = nullptr);
	void EndDialog(DialogContext& ctx);
	void UpdateGameDialog(DialogContext& ctx, float dt);
	void GenerateStockItems();
	void GenerateMerchantItems(vector<ItemSlot>& items, int price_limit);
	void ApplyLocationTexturePack(TexturePack& floor, TexturePack& wall, TexturePack& ceil, LocationTexturePack& tex);
	void ApplyLocationTexturePack(TexturePack& pack, LocationTexturePack::Entry& e, TexturePack& pack_def);
	void SetDungeonParamsAndTextures(BaseLocation& base);
	void SetDungeonParamsToMeshes();
	void MoveUnit(Unit& unit, bool warped = false, bool dash = false);
	bool CollideWithStairs(const CollisionObject& co, const Vec3& pos, float radius) const;
	bool CollideWithStairsRect(const CollisionObject& co, const Box2d& box) const;
	uint ValidateGameData(bool major);
	uint TestGameData(bool major);
	void TestUnitSpells(const SpellList& spells, string& errors, uint& count);
	enum CreateUnitFlags
	{
		CUF_NO_PHYSICS = 1 << 0,
		CUF_CUSTOM = 1 << 1,
		CUF_UNIQUE_STATSX = 1 << 2
	};
	Unit* CreateUnit(UnitData& base, int level = -1, Human* human_data = nullptr, Unit* test_unit = nullptr, int flags = 0);
	void ParseItemScript(Unit& unit, const ItemScript* script);
	bool IsEnemy(Unit& u1, Unit& u2, bool ignore_dont_attack = false);
	bool IsFriend(Unit& u1, Unit& u2);
	bool CanSee(Unit& unit, Unit& unit2);
	// nie dzia�a dla budynk�w bo nie uwzgl�dnia obiekt�w
	bool CanSee(const Vec3& v1, const Vec3& v2);
	bool CheckForHit(LevelContext& ctx, Unit& unit, Unit*& hitted, Vec3& hitpoint);
	bool CheckForHit(LevelContext& ctx, Unit& unit, Unit*& hitted, Mesh::Point& hitbox, Mesh::Point* bone, Vec3& hitpoint);
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
	enum DamageFlags
	{
		DMG_NO_BLOOD = 1 << 0,
		DMG_MAGICAL = 1 << 1
	};
	void GiveDmg(LevelContext& ctx, Unit* giver, float dmg, Unit& taker, const Vec3* hitpoint = nullptr, int dmg_flags = 0);
	void UpdateUnits(LevelContext& ctx, float dt);
	void UpdateUnitInventory(Unit& unit, bool notify = true);
	bool FindPath(LevelContext& ctx, const Int2& start_tile, const Int2& target_tile, vector<Int2>& path, bool can_open_doors = true, bool wedrowanie = false, vector<Int2>* blocked = nullptr);
	Int2 RandomNearTile(const Int2& tile);
	bool CanLoadGame() const;
	bool CanSaveGame() const;
	int FindLocalPath(LevelContext& ctx, vector<Int2>& path, const Int2& my_tile, const Int2& target_tile, const Unit* me, const Unit* other, const void* usable = nullptr, bool is_end_point = false);
	bool DoShieldSmash(LevelContext& ctx, Unit& attacker);
	Vec4 GetFogColor();
	Vec4 GetFogParams();
	Vec4 GetAmbientColor();
	Vec4 GetLightColor();
	Vec4 GetLightDir();
	void UpdateBullets(LevelContext& ctx, float dt);
	void SpawnDungeonColliders();
	void SpawnDungeonCollider(const Vec3& pos);
	void RemoveColliders();
	void CreateCollisionShapes();
	bool AllowKeyboard() const { return IS_SET(allow_input, ALLOW_KEYBOARD); }
	bool AllowMouse() const { return IS_SET(allow_input, ALLOW_MOUSE); }
	Vec3 PredictTargetPos(const Unit& me, const Unit& target, float bullet_speed) const;
	bool CanShootAtLocation(const Unit& me, const Unit& target, const Vec3& pos) const { return CanShootAtLocation2(me, &target, pos); }
	bool CanShootAtLocation(const Vec3& from, const Vec3& to) const;
	bool CanShootAtLocation2(const Unit& me, const void* ptr, const Vec3& to) const;
	void LoadItemsData();
	void SpawnTerrainCollider();
	void GenerateDungeonObjects();
	ObjectEntity GenerateDungeonObject(InsideLocationLevel& lvl, Room& room, BaseObject* base, vector<Vec3>& on_wall, vector<Int2>& blocks, int flags);
	void AddRoomColliders(InsideLocationLevel& lvl, Room& room, vector<Int2>& blocks);
	void GenerateDungeonTreasure(vector<Chest*>& chests, int level, bool extra = false);
	void GenerateDungeonUnits();
	Unit* SpawnUnitInsideRoom(Room& room, UnitData& unit, int level = -1, const Int2& pt = Int2(-1000, -1000), const Int2& pt2 = Int2(-1000, -1000));
	Unit* SpawnUnitInsideRoomOrNear(InsideLocationLevel& lvl, Room& room, UnitData& unit, int level = -1, const Int2& pt = Int2(-1000, -1000), const Int2& pt2 = Int2(-1000, -1000));
	Unit* SpawnUnitNearLocation(LevelContext& ctx, const Vec3& pos, UnitData& unit, const Vec3* look_at = nullptr, int level = -1, float extra_radius = 2.f);
	Unit* SpawnUnitInsideArea(LevelContext& ctx, const Box2d& area, UnitData& unit, int level = -1);
	Unit* SpawnUnitInsideInn(UnitData& unit, int level = -1, InsideBuilding* inn = nullptr, bool temporary = false);
	Unit* CreateUnitWithAI(LevelContext& ctx, UnitData& unit, int level = -1, Human* human_data = nullptr, const Vec3* pos = nullptr, const float* rot = nullptr, AIController** ai = nullptr);
	void ChangeLevel(int gdzie);
	void AddPlayerTeam(const Vec3& pos, float rot, bool reenter, bool hide_weapon);
	void OpenDoorsByTeam(const Int2& pt);
	void ExitToMap();
	void SetExitWorldDir();
	void RespawnObjectColliders(bool spawn_pes = true);
	void RespawnObjectColliders(LevelContext& ctx, bool spawn_pes = true);
	void SetRoomPointers();
	SOUND GetMaterialSound(MATERIAL_TYPE m1, MATERIAL_TYPE m2);
	void PlayAttachedSound(Unit& unit, SOUND sound, float smin, float smax = 0.f);
	ATTACK_RESULT DoGenericAttack(LevelContext& ctx, Unit& attacker, Unit& hitted, const Vec3& hitpoint, float dmg, int dmg_type, bool bash);
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
	void SpellHitEffect(LevelContext& ctx, Bullet& bullet, const Vec3& pos, Unit* hitted);
	void UpdateExplosions(LevelContext& ctx, float dt);
	void UpdateTraps(LevelContext& ctx, float dt);
	// zwraca tymczasowy wska�nik na stworzon� pu�apk� lub nullptr (mo�e si� nie uda� tylko dla ARROW i POISON)
	Trap* CreateTrap(Int2 pt, TRAP_TYPE type, bool timed = false);
	void PreloadTraps(vector<Trap*>& traps);
	bool RayTest(const Vec3& from, const Vec3& to, Unit* ignore, Vec3& hitpoint, Unit*& hitted);
	enum LINE_TEST_RESULT
	{
		LT_IGNORE,
		LT_COLLIDE,
		LT_END
	};
	bool LineTest(btCollisionShape* shape, const Vec3& from, const Vec3& dir, delegate<LINE_TEST_RESULT(btCollisionObject*, bool)> clbk, float& t,
		vector<float>* t_list = nullptr, bool use_clbk2 = false, float* end_t = nullptr);
	bool ContactTest(btCollisionObject* obj, delegate<bool(btCollisionObject*, bool)> clbk, bool use_clbk2 = false);
	void UpdateElectros(LevelContext& ctx, float dt);
	void UpdateDrains(LevelContext& ctx, float dt);
	void AI_Shout(LevelContext& ctx, AIController& ai);
	void AI_DoAttack(AIController& ai, Unit* target, bool w_biegu = false);
	void AI_HitReaction(Unit& unit, const Vec3& pos);
	void UpdateAttachedSounds(float dt);
	void BuildRefidTables();
	bool SaveGameSlot(int slot, cstring text);
	void LoadGameSlot(int slot);
	void LoadSaveSlots();
	void Quicksave(bool from_console);
	bool Quickload(bool from_console);
	void ClearGameVarsOnNewGameOrLoad();
	void ClearGameVarsOnNewGame();
	void ClearGameVarsOnLoad();
	// zwraca losowe miasto lub wiosk� kt�ra nie jest this_city
	int GetRandomSettlement(int this_city = -1);
	// zwraca losowe miasto lub wiosk� kt�ra nie jest this_city i nie ma aktywnego questa
	int GetRandomFreeSettlement(int this_city = -1);
	// zwraca losowe miasto kt�re nie jest this_city
	int GetRandomCity(int this_city = -1);
	void ClearGame();
	void FormatDialogText(DialogContext& ctx, string& str, cstring msg);
	cstring FormatString(DialogContext& ctx, const string& str_part);
	int GetNearestLocation(const Vec2& pos, bool not_quest, bool not_city);
	int GetNearestLocation2(const Vec2& pos, int flags, bool not_quest, int flagi_cel = -1);
	int GetNearestSettlement(const Vec2& pos) { return GetNearestLocation2(pos, (1 << L_CITY), false); }
	void AddGameMsg(cstring msg, float time);
	void AddGameMsg2(cstring msg, float time, int id = -1);
	void AddGameMsg3(GMS id);
	int CalculateQuestReward(int gold);
	void AddReward(int gold) { AddGold(CalculateQuestReward(gold), nullptr, true, txQuestCompletedGold, 4.f, false); }
	void CreateCityMinimap();
	void CreateDungeonMinimap();
	void RebuildMinimap();
	void UpdateDungeonMinimap(bool send);
	void DungeonReveal(const Int2& tile);
	void SaveStock(HANDLE file, vector<ItemSlot>& cnt);
	void LoadStock(HANDLE file, vector<ItemSlot>& cnt);
	Door* FindDoor(LevelContext& ctx, const Int2& pt);
	void AddGroundItem(LevelContext& ctx, GroundItem* item);
	void GenerateDungeonObjects2();
	SOUND GetItemSound(const Item* item);
	cstring GetCurrentLocationText();
	void Unit_StopUsingUsable(LevelContext& ctx, Unit& unit, bool send = true);
	void OnReenterLevel(LevelContext& ctx);
	void EnterLevel(bool first, bool reenter, bool from_lower, int from_portal, bool from_outside);
	void LeaveLevel(bool clear = false);
	void LeaveLevel(LevelContext& ctx, bool clear);
	void CreateBlood(LevelContext& ctx, const Unit& unit, bool fully_created = false);
	void WarpUnit(Unit& unit, const Vec3& pos);
	void ProcessUnitWarps();
	void ProcessRemoveUnits();
	void ApplyContext(ILevel* level, LevelContext& ctx);
	void UpdateContext(LevelContext& ctx, float dt);
	LevelContext& GetContext(Unit& unit);
	LevelContext& GetContext(const Vec3& pos);
	// dru�yna
	bool HaveTeamMemberNPC();
	bool HaveTeamMemberPC();
	bool IsLeader()
	{
		if(Net::IsSingleplayer())
			return true;
		else
			return leader_id == my_id;
	}
	void AddGold(int ile, vector<Unit*>* to = nullptr, bool show = false, cstring msg = txGoldPlus, float time = 3.f, bool defmsg = true);
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
	bool IsAnyoneTalking() const;
	bool FindQuestItem2(Unit* unit, cstring id, Quest** quest, int* i_index, bool not_active = false);
	bool RemoveQuestItem(const Item* item, int refid = -1);
	bool RemoveItemFromWorld(const Item* item);
	bool IsBetterItem(Unit& unit, const Item* item, int* value = nullptr);
	SPAWN_GROUP RandomSpawnGroup(const BaseLocation& base);
	// to by mog�o by� globalna funkcj�
	void GenerateTreasure(int level, int count, vector<ItemSlot>& items, int& gold, bool extra);
	void SplitTreasure(vector<ItemSlot>& items, int gold, Chest** chests, int count);
	void PlayHitSound(MATERIAL_TYPE mat_bron, MATERIAL_TYPE mat_cialo, const Vec3& hitpoint, float range, bool dmg);
	// wczytywanie
	void LoadingStart(int steps);
	void LoadingStep(cstring text = nullptr, int end = 0);
	void LoadResources(cstring text, bool worldmap);
	bool RequireLoadingResources(Location* loc);
	void PreloadResources(bool worldmap);
	void PreloadUsables(vector<Usable*>& usable);
	void PreloadUnits(vector<Unit*>& units);
	void PreloadUnit(Unit* unit);
	void PreloadItems(vector<ItemSlot>& items);
	void PreloadItem(const Item* item);
	void VerifyResources();
	void VerifyUnitResources(Unit* unit);
	void VerifyItemResources(const Item* item);
	//
	void StartArenaCombat(int level);
	InsideBuilding* GetArena();
	bool WarpToArea(LevelContext& ctx, const Box2d& area, float radius, Vec3& pos, int tries = 10);
	void RemoveUnit(Unit* unit, bool notify = true);
	void DeleteUnit(Unit* unit);
	void DialogTalk(DialogContext& ctx, cstring msg);
	void GenerateHeroName(HeroData& hero);
	void GenerateHeroName(Class klasa, bool szalony, string& name);
	bool WantExitLevel()
	{
		return !KeyDownAllowed(GK_WALK);
	}
	Vec2 GetMapPosition(Unit& unit);
	void EventTakeItem(int id);
	const Item* GetBetterItem(const Item* item);
	void CheckIfLocationCleared();
	void SpawnArenaViewers(int count);
	void RemoveArenaViewers();
	bool CanWander(Unit& u);
	float PlayerAngleY();
	Vec3 GetExitPos(Unit& u, bool force_border = false);
	void AttackReaction(Unit& attacked, Unit& attacker);
	enum class CanLeaveLocationResult
	{
		Yes,
		TeamTooFar,
		InCombat
	};
	CanLeaveLocationResult CanLeaveLocation(Unit& unit);
	void GenerateTraps();
	void RegenerateTraps();
	void SpawnHeroesInsideDungeon();
	GroundItem* SpawnGroundItemInsideAnyRoom(InsideLocationLevel& lvl, const Item* item);
	GroundItem* SpawnGroundItemInsideRoom(Room& room, const Item* item);
	GroundItem* SpawnGroundItemInsideRadius(const Item* item, const Vec2& pos, float radius, bool try_exact = false);
	GroundItem* SpawnGroundItemInsideRegion(const Item* item, const Vec2& pos, const Vec2& region_size, bool try_exact);
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
	// zwraca losowe miasto/wiosk� pomijaj�c te ju� u�yte, 0-wioska/miasto, 1-miasto, 2-wioska
	int GetRandomSettlement(const vector<int>& used, int type = 0) const;
	bool GenerateMine();
	void HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit);
	int GetUnitEventHandlerQuestRefid();
	Room& GetRoom(InsideLocationLevel& lvl, RoomTarget target, bool down_stairs);
	void UpdateGame2(float dt);
	bool IsUnitDontAttack(Unit& u)
	{
		if(Net::IsLocal())
			return u.dont_attack;
		else
			return IS_SET(u.ai_mode, 0x01);
	}
	bool IsUnitAssist(Unit& u)
	{
		if(Net::IsLocal())
			return u.assist;
		else
			return IS_SET(u.ai_mode, 0x02);
	}
	bool IsUnitIdle(Unit& u);
	void SetUnitWeaponState(Unit& unit, bool wyjmuje, WeaponType co);
	void UpdatePlayerView();
	void OnCloseInventory();
	void CloseInventory(bool do_close = true);
	void CloseAllPanels(bool close_mp_box = false);
	bool CanShowEndScreen();
	void UpdateGameDialogClient();
	LevelContext& GetContextFromInBuilding(int in_building);
	bool Cheat_KillAll(int typ, Unit& unit, Unit* ignore);
	void Event_Pvp(int id);
	void Cheat_Reveal();
	void Cheat_ShowMinimap();
	void StartPvp(PlayerController* player, Unit* unit);
	void UpdateGameNet(float dt);
	void CheckCredit(bool require_update = false, bool ignore = false);
	void CreateUnitPhysics(Unit& unit, bool position = false);
	void UpdateUnitPhysics(Unit& unit, const Vec3& pos);
	void WarpNearLocation(LevelContext& ctx, Unit& uint, const Vec3& pos, float extra_radius, bool allow_exact, int tries = 20);
	void ShowStatGain(bool is_skill, int what, int value);
	void ActivateChangeLeaderButton(bool activate);
	void RespawnTraps();
	void WarpToInn(Unit& unit);
	void PayCredit(PlayerController* player, int ile);
	void CreateSaveImage(cstring filename);
	void PlayerUseUsable(Usable* u, bool after_action);
	SOUND GetTalkSound(Unit& u);
	void UnitTalk(Unit& u, cstring text);
	void OnEnterLocation();
	void OnEnterLevel();
	void OnEnterLevelOrLocation();
	Unit* FindUnitByIdLocal(UnitData* ud)
	{
		return local_ctx.FindUnitById(ud);
	}
	Unit* FindUnitByIdLocal(cstring id)
	{
		return FindUnitByIdLocal(UnitData::Get(id));
	}
	Object* FindObjectByIdLocal(BaseObject* obj)
	{
		return local_ctx.FindObject(obj);
	}
	Object* FindObjectByIdLocal(cstring id)
	{
		return FindObjectByIdLocal(BaseObject::Get(id));
	}
	Unit* GetRandomArenaHero();
	cstring GetRandomIdleText(Unit& u);
	UnitData* GetRandomHeroData();
	UnitData* GetUnitDataFromClass(Class clas, bool crazy);
	void HandleQuestEvent(Quest_Event* event);

	void DropGold(int count);

	// dodaje przedmiot do ekwipunku postaci (obs�uguje z�oto, otwarty ekwipunek i multiplayer)
	void AddItem(Unit& unit, const Item* item, uint count, uint team_count, bool send_msg = true);
	void AddItem(Unit& unit, const Item* item, uint count = 1, bool is_team = true, bool send_msg = true)
	{
		AddItem(unit, item, count, is_team ? count : 0, send_msg);
	}
	void AddItem(Unit& unit, const GroundItem& item, bool send_msg = true)
	{
		AddItem(unit, item.item, item.count, item.team_count, send_msg);
	}
	// dodaje przedmiot do skrzyni (obs�uguje otwarty ekwipunek i multiplayer)
	void AddItem(Chest& chest, const Item* item, uint count, uint team_count, bool send_msg = true);
	void AddItem(Chest& chest, const Item* item, uint count = 1, bool is_team = true, bool send_msg = true)
	{
		AddItem(chest, item, count, is_team ? count : 0, send_msg);
	}
	// usuwa przedmiot z ekwipunku (obs�uguje otwarty ekwipunek, lock i multiplayer), dla 0 usuwa wszystko
	void RemoveItem(Unit& unit, int i_index, uint count);
	bool RemoveItem(Unit& unit, const Item* item, uint count);

	// szuka gracza kt�ry u�ywa skrzyni, je�li u�ywa nie-gracz to zwraca nullptr (aktualnie tylko gracz mo�e ale w przysz�o�ci nie)
	Unit* FindChestUserIfPlayer(Chest* chest);

	Unit* FindPlayerTradingWithUnit(Unit& u);
	Int2 GetSpawnPoint();
	InsideLocationLevel* TryGetLevelData();
	bool ValidateTarget(Unit& u, Unit* target);

	void UpdateLights(vector<Light>& lights);

	bool IsDrunkman(Unit& u);
	void PlayUnitSound(Unit& u, SOUND snd, float range = 1.f);
	void UnitFall(Unit& u);
	void UnitDie(Unit& u, LevelContext* ctx, Unit* killer);
	void UnitTryStandup(Unit& u, float dt);
	void UnitStandup(Unit& u);

	void UpdatePostEffects(float dt);

	void SpawnDrunkmans();
	void PlayerYell(Unit& u);
	bool CanBuySell(const Item* item);
	void SetOutsideParams();
	UnitData& GetHero(Class clas, bool crazy = false);
	const Item* GetRandomBook();

	// level area
	LevelAreaContext* ForLevel(int loc, int level = -1);
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
	DialogBox* dialog_enc;
	DialogBox* dialog_pvp;
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
	Class quickstart_class, autopick_class; // mo�na po��czy�
	string quickstart_name;
	bool check_updates, skip_tutorial;
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
	void ShowCreateCharacterPanel(bool enter_name, bool redo = false);
	void StartQuickGame();
	void MultiplayerPanelEvent(int id);
	void CreateServerEvent(int id);
	// set for Random player character (clas is in/out)
	void RandomCharacter(Class& clas, int& hair_index, HumanData& hd, CreatedCharacter& cc);
	void OnEnterIp(int id);
	void GenericInfoBoxUpdate(float dt);
	void UpdateClientConnectingIp(float dt);
	void UpdateClientTransfer(float dt);
	void UpdateClientQuiting(float dt);
	void UpdateServerTransfer(float dt);
	void UpdateServerSend(float dt);
	void UpdateServerQuiting(float dt);
	void QuickJoinIp();
	void AddMultiMsg(cstring msg);
	void Quit();
	bool ValidateNick(cstring nick);
	void UpdateLobbyNet(float dt);
	void UpdateLobbyNetClient(float dt);
	void UpdateLobbyNetServer(float dt);
	bool DoLobbyUpdate(BitStream& stream);
	void OnCreateCharacter(int id);
	void OnPlayTutorial(int id);
	void OnQuit(int);
	void OnExit(int);
	void ShowQuitDialog();
	void OnPickServer(int id);
	void EndConnecting(cstring msg, bool wait = false);
	void CloseConnection(VoidF f);
	void DoQuit();
	void RestartGame();
	void ClearAndExitToMenu(cstring msg);

	//-----------------------------------------------------------------
	// MULTIPLAYER
	SLNet::RakPeerInterface* peer;
	string server_name, player_name, server_pswd, server_ip, enter_pswd, server_name2;
	int autostart_count;//, kick_timer;
	int players; // aktualna liczba graczy w grze
	int max_players, max_players2; // maksymalna liczba graczy w grze
	int my_id; // moje unikalne id
	int last_id;
	int last_startup_id;
	bool sv_startup, was_client, players_left;
	BitStream server_info;
	vector<byte> packet_data;
	vector<PlayerInfo*> game_players, old_players;
	SystemAddress server;
	int leader_id, kick_id;
	float startup_timer;
	enum NET_MODE
	{
		NM_CONNECT_IP, // ��czenie serwera z klientem (0 - pingowanie, 1 - podawanie has�a, 2 - ��czenie)
		NM_QUITTING,
		NM_QUITTING_SERVER,
		NM_TRANSFER,
		NM_TRANSFER_SERVER,
		NM_SERVER_SEND
	} net_mode;
	NetState net_state;
	int net_tries;
	VoidF net_callback;
	string net_adr;
	float net_timer, update_timer, mp_timeout;
	vector<Int2> lobby_updates;
	void AddLobbyUpdate(const Int2& u);
	BitStream net_stream, net_stream2;
	bool change_title_a;
	bool level_generated;
	int netid_counter, item_netid_counter, chest_netid_counter, usable_netid_counter, skip_id_counter, trap_netid_counter, door_netid_counter, electro_netid_counter;
	vector<string*> net_talk;
	struct WarpData
	{
		Unit* u;
		int where; // -1-z budynku, >=0-do budynku
		float timer;
	};
	vector<WarpData> mp_warps;
	vector<Item*> quest_items;
	bool anyone_talking;
	// u�ywane u klienta kt�ry nie zapami�tuje zmiennej 'pc'
	bool godmode, noclip, invisible;
	vector<Int2> minimap_reveal_mp;
	bool boss_level_mp; // u�ywane u klienta zamiast boss_levels
	bool mp_load, mp_load_worldmap, mp_use_interp;
	float mp_interp;
	ObjectPool<EntityInterpolator> interpolators;
	float interpolate_timer;
	int mp_port;
	bool paused, pick_autojoin;

	// zwraca czy pozycja si� zmieni�a
	void UpdateInterpolator(EntityInterpolator* e, float dt, Vec3& pos, float& rot);
	void InterpolateUnits(float dt);
	void InterpolatePlayers(float dt);

	void InitServer();
	void InitClient();
	void UpdateServerInfo();
	int FindPlayerIndex(cstring nick, bool not_left = false);
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
	Unit* FindUnit(delegate<bool(Unit*)> pred);
	void UpdateServer(float dt);
	bool ProcessControlMessageServer(BitStream& stream, PlayerInfo& info);
	void WriteServerChanges(BitStream& stream);
	void WriteServerChangesForPlayer(BitStream& stream, PlayerInfo& info);
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
	GroundItem* FindItemNetid(int netid, LevelContext** ctx = nullptr);
	PlayerInfo& GetPlayerInfo(int id);
	PlayerInfo* GetPlayerInfoTry(int id);
	void UpdateWarpData(float dt);
	void Net_AddQuest(int refid)
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::ADD_QUEST;
		c.id = refid;
	}
	void Net_RegisterItem(const Item* item, const Item* base_item)
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::REGISTER_ITEM;
		c.item2 = item;
		c.base_item = base_item;
	}
	void Net_AddItem(PlayerController* player, const Item* item, bool is_team)
	{
		NetChangePlayer& c = Add1(player->player_info->changes);
		c.type = NetChangePlayer::ADD_ITEMS;
		c.item = item;
		c.id = (is_team ? 1 : 0);
		c.ile = 1;
	}
	void Net_AddedItemMsg(PlayerController* player)
	{
		NetChangePlayer& c = Add1(player->player_info->changes);
		c.type = NetChangePlayer::GAME_MESSAGE;
		c.id = GMS_ADDED_ITEM;
	}
	void Net_AddItems(PlayerController* player, const Item* item, int ile, bool is_team)
	{
		NetChangePlayer& c = Add1(player->player_info->changes);
		c.type = NetChangePlayer::ADD_ITEMS;
		c.item = item;
		c.id = (is_team ? ile : 0);
		c.ile = ile;
	}
	void Net_UpdateQuest(int refid)
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::UPDATE_QUEST;
		c.id = refid;
	}
	void Net_UpdateQuestMulti(int refid, int ile)
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::UPDATE_QUEST_MULTI;
		c.id = refid;
		c.ile = ile;
	}
	void Net_RenameItem(const Item* item)
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::RENAME_ITEM;
		c.base_item = item;
	}
	void Net_RemoveQuestItem(PlayerController* player, int refid)
	{
		NetChangePlayer& c = Add1(player->player_info->changes);
		c.type = NetChangePlayer::REMOVE_QUEST_ITEM;
		c.id = refid;
	}
	void Net_ChangeLocationState(int id, bool visited)
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CHANGE_LOCATION_STATE;
		c.id = id;
		c.ile = (visited ? 1 : 0);
	}
	void Net_RecruitNpc(Unit* unit)
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::RECRUIT_NPC;
		c.unit = unit;
	}
	void Net_RemoveUnit(Unit* unit)
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::REMOVE_UNIT;
		c.id = unit->netid;
	}
	void Net_KickNpc(Unit* unit)
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::KICK_NPC;
		c.id = unit->netid;
	}
	void Net_SpawnUnit(Unit* unit)
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::SPAWN_UNIT;
		c.unit = unit;
	}
	void Net_PrepareWarp(PlayerController* player)
	{
		NetChangePlayer& c = Add1(player->player_info->changes);
		c.type = NetChangePlayer::PREPARE_WARP;
	}
	void Net_StartDialog(PlayerController* player, Unit* talker)
	{
		NetChangePlayer& c = Add1(player->player_info->changes);
		c.type = NetChangePlayer::START_DIALOG;
		c.id = talker->netid;
	}
	enum Where
	{
		WHERE_LEVEL_UP = -1,
		WHERE_LEVEL_DOWN = -2,
		WHERE_OUTSIDE = -3,
		WHERE_PORTAL = 0
	};
	void Net_LeaveLocation(int where)
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::LEAVE_LOCATION;
		c.id = where;
	}
	void Net_OnNewGameServer();
	void Net_OnNewGameClient();
	// szuka questowych przedmiot�w u klienta
	const Item* FindQuestItemClient(cstring id, int refid) const;
	//void ConvertPlayerToAI(PlayerInfo& info);
	Usable* FindUsable(int netid);
	// read item id and return it (can be quest item or gold), results: -2 read error, -1 not found, 0 empty, 1 ok
	int ReadItemAndFind(BitStream& stream, const Item*& item) const;
	bool ReadItemList(BitStream& stream, vector<ItemSlot>& items);
	bool ReadItemListTeam(BitStream& stream, vector<ItemSlot>& items, bool skip = false);
	Door* FindDoor(int netid);
	Trap* FindTrap(int netid);
	bool RemoveTrap(int netid);
	Chest* FindChest(int netid);
	void ReequipItemsMP(Unit& unit); // zak�ada przedmioty kt�re ma w ekipunku, dostaje bro� je�li nie ma, podnosi z�oto
	Electro* FindElectro(int netid);
	void UseDays(PlayerController* player, int count);
	PlayerInfo* FindOldPlayer(cstring nick);
	void PrepareWorldData(BitStream& stream);
	bool ReadWorldData(BitStream& stream);
	void WriteNetVars(BitStream& stream);
	bool ReadNetVars(BitStream& stream);
	void WritePlayerStartData(BitStream& stream, PlayerInfo& info);
	bool ReadPlayerStartData(BitStream& stream);
	bool CheckMoveNet(Unit& unit, const Vec3& pos);
	void Net_PreSave();
	bool FilterOut(NetChange& c);
	bool FilterOut(NetChangePlayer& c);
	void Net_FilterServerChanges();
	void Net_FilterClientChanges();
	void ProcessLeftPlayers();
	void RemovePlayer(PlayerInfo& info);
	void ClosePeer(bool wait = false);
	void DeleteOldPlayers();
	NetChangePlayer& AddChange(NetChangePlayer::TYPE type, PlayerController* _pc)
	{
		assert(_pc);
		NetChangePlayer& c = Add1(_pc->player_info->changes);
		c.type = type;
		return c;
	}

	BitStream& StreamStart(Packet* packet, StreamLogType type);
	void StreamEnd();
	void StreamError();
	void StreamWrite(vector<byte>& data, StreamLogType type, const SystemAddress& adr)
	{
		StreamWrite(data.data(), data.size(), type, adr);
	}
	void StreamWrite(BitStream& data, StreamLogType type, const SystemAddress& adr)
	{
		StreamWrite(data.GetData(), data.GetNumberOfBytesUsed(), type, adr);
	}
	void StreamWrite(Packet* packet, StreamLogType type, const SystemAddress& adr)
	{
		StreamWrite(packet->data, packet->length, type, adr);
	}
	void StreamWrite(const void* data, uint size, StreamLogType type, const SystemAddress& adr);

	BitStream current_stream;
	Packet* current_packet;

	//-----------------------------------------------------------------
	// WORLD MAP
	typedef std::pair<LOCATION, bool>(*AddLocationsCallback)(uint index);
	void AddLocations(uint count, AddLocationsCallback clbk, float valid_dist, bool unique_name);
	bool EnterLocation(int level = 0, int from_portal = -1, bool close_portal = false);
	void GenerateWorld();
	void GenerateCityBuildings(City& city, vector<Building*>& buildings, bool required);
	void ApplyTiles(float* h, TerrainTile* tiles);
	void SpawnBuildings(vector<CityBuilding>& buildings);
	void SpawnUnits(City* city);
	void RespawnUnits();
	void RespawnUnits(LevelContext& ctx);
	void LeaveLocation(bool clear = false, bool end_buffs = true);
	void GenerateDungeon(Location& loc);
	void SpawnCityPhysics();
	// for object rot must be 0, PI/2, PI or PI*3/2
	ObjectEntity SpawnObjectEntity(LevelContext& ctx, BaseObject* base, const Vec3& pos, float rot, float scale = 1.f, int flags = 0,
		Vec3* out_point = nullptr, int variant = -1);
	void RespawnBuildingPhysics();
	void SpawnCityObjects();
	// roti jest u�ywane tylko do ustalenia czy k�t jest zerowy czy nie, mo�na przerobi� t� funkcj� �eby tego nie u�ywa�a wog�le
	void ProcessBuildingObjects(LevelContext& ctx, City* city, InsideBuilding* inside, Mesh* mesh, Mesh* inside_mesh, float rot, int roti,
		const Vec3& shift, Building* type, CityBuilding* building, bool recreate = false, Vec3* out_point = nullptr);
	void GenerateForest(Location& loc);
	void SpawnForestObjects(int road_dir = -1); //-1 brak, 0 -, 1 |
	void SpawnForestItems(int count_mod);
	void CreateForestMinimap();
	void SpawnOutsideBariers();
	void GetOutsideSpawnPoint(Vec3& pos, float& dir);
	void SpawnForestUnits(const Vec3& team_pos);
	void RepositionCityUnits();
	void Event_RandomEncounter(int id);
	void GenerateEncounterMap(Location& loc);
	void SpawnEncounterUnits(GameDialog*& dialog, Unit*& talker, Quest*& quest);
	void SpawnEncounterObjects();
	void SpawnEncounterTeam();
	Encounter* AddEncounter(int& id);
	void RemoveEncounter(int id);
	Encounter* GetEncounter(int id);
	Encounter* RecreateEncounter(int id);
	int GetRandomSpawnLocation(const Vec2& pos, SPAWN_GROUP group, float range = 160.f);
	void DoWorldProgress(int days);
	Location* CreateLocation(LOCATION type, int levels = -1, bool is_village = false);
	void UpdateLocation(LevelContext& ctx, int days, int open_chance, bool reset);
	void UpdateLocation(int days, int open_chance, bool reset);
	void GenerateCamp(Location& loc);
	void SpawnCampObjects();
	void SpawnCampUnits();
	ObjectEntity SpawnObjectNearLocation(LevelContext& ctx, BaseObject* obj, const Vec2& pos, float rot, float range = 2.f, float margin = 0.3f,
		float scale = 1.f);
	ObjectEntity SpawnObjectNearLocation(LevelContext& ctx, BaseObject* obj, const Vec2& pos, const Vec2& rot_target, float range = 2.f, float margin = 0.3f,
		float scale = 1.f);
	int GetClosestLocation(LOCATION type, const Vec2& pos, int target = -1);
	int GetClosestLocationNotTarget(LOCATION type, const Vec2& pos, int not_target);
	int CreateCamp(const Vec2& pos, SPAWN_GROUP group, float range = 64.f, bool allow_exact = true);
	void SpawnTmpUnits(City* city);
	void RemoveTmpUnits(City* city);
	void RemoveTmpUnits(LevelContext& ctx);
	int AddLocation(Location* loc);
	// tworzy lokacj� (je�li range<0 to pozycja jest dowolna a range=-range, level=-1 - losowy poziom, =0 - minimalny, =9 maksymalny, =liczba - okre�lony)
	int CreateLocation(LOCATION type, const Vec2& pos, float range = 64.f, int target = -1, SPAWN_GROUP spawn = SG_LOSOWO, bool allow_exact = true,
		int levels = -1);
	bool FindPlaceForLocation(Vec2& pos, float range = 64.f, bool allow_exact = true);
	int FindLocationId(Location* loc);
	void Event_StartEncounter(int id);
	void GenerateMoonwell(Location& loc);
	void SpawnMoonwellObjects();
	void SpawnMoonwellUnits(const Vec3& team_pos);
	enum SpawnObjectExtrasFlags
	{
		SOE_DONT_SPAWN_PARTICLES = 1 << 0,
		SOE_MAGIC_LIGHT = 1 << 1,
		SOE_DONT_CREATE_LIGHT = 1 << 2
	};
	void SpawnObjectExtras(LevelContext& ctx, BaseObject* obj, const Vec3& pos, float rot, void* user_ptr, float scale = 1.f, int flags = 0);
	void GenerateSecretLocation(Location& loc);
	void SpawnSecretLocationObjects();
	void SpawnSecretLocationUnits();
	void SpawnTeamSecretLocation();
	void GenerateMushrooms(int days_since = 10);
	void GenerateCityPickableItems();
	void PickableItemBegin(LevelContext& ctx, Object& o);
	void PickableItemAdd(const Item* item);
	void GenerateDungeonFood();
	void GenerateCityMap(Location& loc);
	void GenerateVillageMap(Location& loc);
	void PrepareCityBuildings(City& city, vector<ToBuild>& tobuild);
	void GetCityEntry(Vec3& pos, float& rot);
	void AbadonLocation(Location* loc);
	void SetLocationVisited(Location& loc);

	vector<Location*> locations; // lokacje w grze, mo�e by� nullptr
	Location* location; // wska�nik na aktualn� lokacj� [odtwarzany]
	int current_location; // aktualna lokacja lub -1
	int picked_location; // zaznaczona lokacja na mapie �wiata, ta do kt�rej si� w�druje lub -1 [tylko je�li world_state==WS_TRAVEL]
	int open_location; // aktualnie otwarta lokacja (w sensie wczytanych zasob�w, utworzonych jednostek itp) lub -1 [odtwarzany]
	int travel_day; // liczba dni w podr�y [tylko je�li world_state==WS_TRAVEL]
	int enc_kierunek; // kierunek z kt�rej strony nadesz�a dru�yna w czasie spotkania [tymczasowe]
	int spotkanie; // rodzaj losowego spotkania [tymczasowe]
	int enc_tryb; // 0 - losowa walka, 1 - specjalne spotkanie, 2 - questowe spotkanie [tymczasowe]
	int empty_locations; // liczba pustych lokacji
	int create_camp; // licznik do stworzenia nowego obozu
	WORLDMAP_STATE world_state; // stan na mapie �wiata (stoi, podr�uje)
	Vec2 world_pos; // pozycja na mapie �wiata
	Vec2 travel_start; // punkt startu podr�y na mapie �wiata [tylko je�li world_state==WS_TRAVEL]
	float travel_time; // czas podr�y na mapie [tylko je�li world_state==WS_TRAVEL]
	float travel_time2; // licznik aktualizacji szansy na spotkanie
	float world_dir; // kierunek podr�y/wej�cia na map�, to jest nowy k�t (0 w prawo), wskazuje od �rodka do kraw�dzi mapy
	float szansa_na_spotkanie; // szansa na spotkanie na mapie �wiata
	bool far_encounter; // czy dru�yna gracza jest daleko w czasie spotkania [tymczasowe]
	bool guards_enc_reward; // czy odebrano nagrod� za uratowanie stra�nik�w w czasie spotkania
	uint settlements; // liczba miast i wiosek
	uint encounter_loc; // id lokacji spotkania losowego
	SPAWN_GROUP losowi_wrogowie; // wrogowie w czasie spotkania [tymczasowe]
	vector<Encounter*> encs; // specjalne spotkania na mapie �wiata [odtwarzane przy wczytywaniu quest�w]
	Encounter* game_enc; // spotkanie w czasie podr�y [tymczasowe]
	LocationEventHandler* location_event_handler; // obs�uga wydarze� lokacji
	bool first_city;
	vector<Int2> boss_levels; // dla oznaczenia gdzie gra� muzyk� (x-lokacja, y-poziom)
	enum EnterFrom
	{
		ENTER_FROM_PORTAL = 0,
		ENTER_FROM_OUTSIDE = -1,
		ENTER_FROM_UP_LEVEL = -2,
		ENTER_FROM_DOWN_LEVEL = -3,
		ENTER_FROM_UNKNOWN = -4
	};
	int enter_from; // sk�d si� przysz�o (u�ywane przy wczytywanie w MP gdy do��cza nowa posta�)
	bool g_have_well;
	Int2 g_well_pt;

	//-----------------------------------------------------------------
	// WORLD STATE
	enum WorldProgressMode
	{
		WPM_NORMAL,
		WPM_TRAVEL,
		WPM_SKIP
	};
	void WorldProgress(int days, WorldProgressMode mode);
	void OnTravel(float dist);

	int year; // rok w grze [zaczyna si� od 100, ustawiane w NewGame]
	int month; // miesi�c w grze [od 0 do 11, ustawiane w NewGame]
	int day; // dzie� w grze [od 0 do 29, ustawiane w NewGame]
	int worldtime; // licznik dni [od 0, ustawiane w NewGame]

	Config cfg;
	void SaveCfg();
	cstring GetShortcutText(GAME_KEYS key, cstring action = nullptr);
};
