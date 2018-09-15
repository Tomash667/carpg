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
#include "LevelArea.h"
#include "SaveSlot.h"
#include "ResourceManager.h"
#include "ObjectEntity.h"

//-----------------------------------------------------------------------------
// Tryb szybkiego uruchamiania gry
enum QUICKSTART
{
	QUICKSTART_NONE,
	QUICKSTART_SINGLE,
	QUICKSTART_HOST,
	QUICKSTART_JOIN_LAN,
	QUICKSTART_JOIN_IP,
	QUICKSTART_LOAD,
	QUICKSTART_LOAD_MP
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
	SmartPtr<Unit> unit;
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
	GMS_GETTING_OUT_OF_RANGE,
	GMS_LEFT_EVENT
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

typedef delegate<void(cstring)> PrintMsgFunc;

const float UNIT_VIEW_A = 0.2f;
const float UNIT_VIEW_B = 0.4f;
const int UNIT_VIEW_MUL = 5;

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

class Game final : public Engine
{
public:
	Game();
	~Game();

	void OnCleanup() override;
	void CleanupSystems();
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
	void InitScripts();

	// loading data
	void LoadData();
	void AddLoadTasks();

	// after loading data
	void PostconfigureGame();
	void StartGameMode();

	QUICKSTART quickstart;
	int quickstart_slot;

	// supershader
	string sshader_code;
	FileTime sshader_edit_time;
	ID3DXEffectPool* sshader_pool;
	vector<SuperShader> sshaders;
	D3DXHANDLE hSMatCombined, hSMatWorld, hSMatBones, hSTint, hSAmbientColor, hSFogColor, hSFogParams, hSLightDir, hSLightColor, hSLights, hSSpecularColor, hSSpecularIntensity,
		hSSpecularHardness, hSCameraPos, hSTexDiffuse, hSTexNormal, hSTexSpecular;
	void InitSuperShader();
	uint GetSuperShaderId(bool animated, bool have_binormals, bool fog, bool specular, bool normal, bool point_light, bool dir_light) const;
	SuperShader* GetSuperShader(uint id);
	SuperShader* CompileSuperShader(uint id);

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
	TexturePtr tKrew[BLOOD_MAX], tKrewSlad[BLOOD_MAX], tIskra, tSpawn;
	TexturePack tFloor[2], tWall[2], tCeil[2], tFloorBase, tWallBase, tCeilBase;
	ID3DXEffect* eMesh, *eParticle, *eSkybox, *eTerrain, *eArea, *eGui, *ePostFx, *eGlow, *eGrass;
	D3DXHANDLE techMesh, techMeshDir, techMeshSimple, techMeshSimple2, techMeshExplo, techParticle, techSkybox, techTerrain, techArea, techTrail, techGlowMesh,
		techGlowAni, techGrass;
	D3DXHANDLE hAniCombined, hAniWorld, hAniBones, hAniTex, hAniFogColor, hAniFogParam, hAniTint, hAniHairColor, hAniAmbientColor, hAniLightDir,
		hAniLightColor, hAniLights, hMeshCombined, hMeshWorld, hMeshTex, hMeshFogColor, hMeshFogParam, hMeshTint, hMeshAmbientColor, hMeshLightDir,
		hMeshLightColor, hMeshLights, hParticleCombined, hParticleTex, hSkyboxCombined, hSkyboxTex, hAreaCombined, hAreaColor, hAreaPlayerPos, hAreaRange,
		hTerrainCombined, hTerrainWorld, hTerrainTexBlend, hTerrainTex[5], hTerrainColorAmbient, hTerrainColorDiffuse, hTerrainLightDir, hTerrainFogColor,
		hTerrainFogParam, hGuiSize, hGuiTex, hPostTex, hPostPower, hPostSkill, hGlowCombined, hGlowBones, hGlowColor, hGlowTex, hGrassViewProj, hGrassTex,
		hGrassFogColor, hGrassFogParams, hGrassAmbientColor;
	SOUND sGulp, sCoins, sBow[2], sDoor[3], sDoorClosed[2], sDoorClose, sItem[8], sChestOpen, sChestClose, sDoorBudge, sRock, sWood, sCrystal,
		sMetal, sBody[5], sBone, sSkin, sArenaFight, sArenaWin, sArenaLost, sUnlock, sEvil, sEat, sSummon;
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
	cstring txAiNoHpPot[2], txAiCity[2], txAiVillage[2], txAiMoonwell, txAiForest, txAiCampEmpty, txAiCampFull, txAiFort, txAiDwarfFort, txAiTower, txAiArmory, txAiHideout,
		txAiVault, txAiCrypt, txAiTemple, txAiNecromancerBase, txAiLabirynth, txAiNoEnemies, txAiNearEnemies, txAiCave, txAiInsaneText[11], txAiDefaultText[9], txAiOutsideText[3],
		txAiInsideText[2], txAiHumanText[2], txAiOrcText[7], txAiGoblinText[5], txAiMageText[4], txAiSecretText[3], txAiHeroDungeonText[4], txAiHeroCityText[5], txAiBanditText[6],
		txAiHeroOutsideText[2], txAiDrunkMageText[3], txAiDrunkText[5], txAiDrunkmanText[4];
	cstring txEnteringLocation, txGeneratingMap, txGeneratingBuildings, txGeneratingObjects, txGeneratingUnits, txGeneratingItems, txGeneratingPhysics, txRecreatingObjects, txGeneratingMinimap,
		txLoadingComplete, txWaitingForPlayers, txLoadingResources;
	cstring txTutPlay, txTutTick;
	cstring txCantSaveGame, txSaveFailed, txSavedGameN, txLoadFailed, txQuickSave, txGameSaved, txLoadingLocations, txLoadingData, txLoadingQuests, txEndOfLoading,
		txCantSaveNow, txOnlyServerCanSave, txCantLoadGame, txOnlyServerCanLoad, txLoadSignature, txLoadVersion, txLoadSaveVersionOld, txLoadMP, txLoadSP, txLoadError,
		txLoadErrorGeneric, txLoadOpenError;
	cstring txPvpRefuse, txWin, txWinMp, txINeedWeapon, txNoHpp, txCantDo, txDontLootFollower, txDontLootArena, txUnlockedDoor,
		txNeedKey, txLevelUp, txLevelDown, txLocationText, txLocationTextMap, txRegeneratingLevel, txGmsLooted, txGmsRumor, txGmsJournalUpdated, txGmsUsed,
		txGmsUnitBusy, txGmsGatherTeam, txGmsNotLeader, txGmsNotInCombat, txGainTextAttrib, txGainTextSkill, txNeedItem, txReallyQuit,
		txGmsAddedItem, txGmsAddedItems, txGmsGettingOutOfRange, txGmsLeftEvent;
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
	cstring txServer, txYouAreLeader, txRolledNumber, txPcIsLeader, txReceivedGold, txYouDisconnected, txYouKicked,
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
	// post effect u¿ywa 3 tekstur lub jeœli jest w³¹czony multisampling 3 surface i 1 tekstury
	SURFACE sPostEffect[3];
	TEX tPostEffect[3];
	VB vbFullscreen;
	vector<PostEffect> post_effects;
	SURFACE sCustom;

	//---------------------------------
	// KONSOLA I KOMENDY
	bool have_console, inactive_update, noai, devmode, default_devmode, default_player_devmode, debug_info, debug_info2, dont_wander;
	string cfg_file;
	vector<ConsoleCommand> cmds;
	int mouse_sensitivity;
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
	bool testing, force_seed_all, koniec_gry, target_loc_is_camp, death_solo;
	int death_screen;
	float death_fade, game_speed;
	vector<MeshInstance*> bow_instances;
	Pak* pak;
	vector<AIController*> ais;
	uint force_seed, next_seed;
	vector<AttachedSound> attached_sounds;
	SaveSlot single_saves[MAX_SAVE_SLOTS], multi_saves[MAX_SAVE_SLOTS];
	vector<UnitView> unit_views;

	MeshInstance* GetBowInstance(Mesh* mesh);

	//---------------------------------
	// SCREENSHOT
	time_t last_screenshot;
	uint screenshot_count;
	ImageFormat screenshot_format;

	//---------------------------------
	// DIALOGI
	DialogContext dialog_context;
	DialogContext* current_dialog;
	Int2 dialog_cursor_pos; // ostatnia pozycja myszki przy wyborze dialogu
	vector<string> dialog_choices; // u¿ywane w MP u klienta
	string predialog;

	DialogContext* FindDialogContext(Unit* talker);

	//---------------------------------
	// FIZYKA
	btCollisionShape* shape_wall, *shape_low_ceiling, *shape_arrow, *shape_ceiling, *shape_floor, *shape_door, *shape_block, *shape_schody, *shape_schody_c[2],
		*shape_summon, *shape_barrier;
	btHeightfieldTerrainShape* terrain_shape;
	btBvhTriangleMeshShape* dungeon_shape;
	btCollisionObject* obj_terrain, *obj_dungeon;
	vector<btCollisionShape*> shapes;
	vector<CameraCollider> cam_colliders;
	vector<Vec3> dungeon_shape_pos;
	vector<int> dungeon_shape_index;
	btTriangleIndexVertexArray* dungeon_shape_data;

	//---------------------------------
	// WCZYTYWANIE
	float loading_dt, loading_cap;
	Timer loading_t;
	int loading_steps, loading_index;
	Color clear_color2;

	//---------------------------------
	// MINIMAPA
	bool minimap_opened_doors;
	vector<Int2> minimap_reveal;
	uint minimap_size;

	//---------------------------------
	// FALLBACK
	FALLBACK fallback_type;
	int fallback_1, fallback_2;
	float fallback_t;

	//--------------------------------------
	// ARENA
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

	void UpdateArena(float dt);
	void CleanArena();

	//--------------------------------------
	// QUESTS
	void ShowAcademyText();

	//
	vector<Unit*> warp_to_inn;

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

	// u¿ywane przy wczytywaniu gry
	vector<AIController*> ai_bow_targets, ai_cast_targets;
	vector<Location*> load_location_quest;
	vector<Unit*> load_unit_handler;
	vector<Chest*> load_chest_handler;

	bool hardcore_mode, hardcore_option;
	string hardcore_savename;
	const Item* crazy_give_item; // dawany przedmiot, nie trzeba zapisywaæ
	float grayout;
	bool cl_postfx;

	// zwraca losowy przedmiot o maksymalnej cenie, ta funkcja jest powolna!
	// mo¿e zwróciæ questowy przedmiot jeœli bêdzie wystarczaj¹co tani, lub unikat!
	const Item* GetRandomItem(int max_value);

	// klawisze
	void InitGameKeys();
	void ResetGameKeys();
	void SaveGameKeys();
	void LoadGameKeys();

	// przedmioty w czasie grabienia itp s¹ tu przechowywane indeksy
	// ujemne wartoœci odnosz¹ siê do slotów (SLOT_WEAPON = -SLOT_WEAPON-1), pozytywne do zwyk³ych przedmiotów
	vector<int> tmp_inventory[2];
	int tmp_inventory_shift[2];

	void BuildTmpInventory(int index);
	int GetItemPrice(const Item* item, Unit& unit, bool buy);

	void Draw();
	void ExitToMenu();
	void DoExitToMenu();
	void GenerateItemImage(TaskData& task_data);
	TEX TryGenerateItemImage(const Item& item);
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
	bool ExecuteGameDialogSpecial(DialogContext& ctx, cstring msg, int& if_level);
	bool ExecuteGameDialogSpecialIf(DialogContext& ctx, cstring msg);
	void GenerateStockItems();
	void GenerateMerchantItems(vector<ItemSlot>& items, int price_limit);
	void ApplyLocationTexturePack(TexturePack& floor, TexturePack& wall, TexturePack& ceil, LocationTexturePack& tex);
	void ApplyLocationTexturePack(TexturePack& pack, LocationTexturePack::Entry& e, TexturePack& pack_def);
	void SetDungeonParamsAndTextures(BaseLocation& base);
	void SetDungeonParamsToMeshes();
	void MoveUnit(Unit& unit, bool warped = false, bool dash = false);
	uint ValidateGameData(bool major);
	uint TestGameData(bool major);
	void TestUnitSpells(const SpellList& spells, string& errors, uint& count);
	Unit* CreateUnit(UnitData& base, int level = -1, Human* human_data = nullptr, Unit* test_unit = nullptr, bool create_physics = true, bool custom = false);
	void ParseItemScript(Unit& unit, const ItemScript* script);
	bool IsEnemy(Unit& u1, Unit& u2, bool ignore_dont_attack = false);
	bool IsFriend(Unit& u1, Unit& u2);
	bool CanSee(Unit& unit, Unit& unit2);
	// nie dzia³a dla budynków bo nie uwzglêdnia obiektów
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
	bool CanLoadGame() const;
	bool CanSaveGame() const;
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
	Unit* CreateUnitWithAI(LevelContext& ctx, UnitData& unit, int level = -1, Human* human_data = nullptr, const Vec3* pos = nullptr, const float* rot = nullptr, AIController** ai = nullptr);
	void ChangeLevel(int where);
	void AddPlayerTeam(const Vec3& pos, float rot, bool reenter, bool hide_weapon);
	void OpenDoorsByTeam(const Int2& pt);
	void ExitToMap();
	void RespawnObjectColliders(bool spawn_pes = true);
	void SetRoomPointers();
	SOUND GetMaterialSound(MATERIAL_TYPE m1, MATERIAL_TYPE m2);
	void PlayAttachedSound(Unit& unit, SOUND sound, float smin, float smax = 0.f);
	void StopAllSounds();
	ATTACK_RESULT DoGenericAttack(LevelContext& ctx, Unit& attacker, Unit& hitted, const Vec3& hitpoint, float dmg, int dmg_type, bool bash);
	int GetDungeonLevel();
	int GetDungeonLevelChest();
	void SaveGame(GameWriter& f);
	void LoadGame(GameReader& f);
	void RemoveUnusedAiAndCheck();
	void CheckUnitsAi(LevelContext& ctx, int& err_count);
	void CastSpell(LevelContext& ctx, Unit& unit);
	void SpellHitEffect(LevelContext& ctx, Bullet& bullet, const Vec3& pos, Unit* hitted);
	void UpdateExplosions(LevelContext& ctx, float dt);
	void UpdateTraps(LevelContext& ctx, float dt);
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
	void SaveGameFilename(const string& name);
	bool SaveGameCommon(cstring filename, int slot, cstring text);
	void LoadGameSlot(int slot);
	void LoadGameFilename(const string& name);
	void LoadGameCommon(cstring filename, int slot);
	void LoadSaveSlots();
	void Quicksave(bool from_console);
	bool Quickload(bool from_console);
	void ClearGameVarsOnNewGameOrLoad();
	void ClearGameVarsOnNewGame();
	void ClearGameVarsOnLoad();
	void ClearGame();
	cstring FormatString(DialogContext& ctx, const string& str_part);
	void AddGameMsg(cstring msg, float time);
	void AddGameMsg2(cstring msg, float time, int id = -1);
	void AddGameMsg3(GMS id);
	void AddGameMsg3(PlayerController* player, GMS id);
	int CalculateQuestReward(int gold);
	void AddReward(int gold) { AddGold(CalculateQuestReward(gold), nullptr, true, txQuestCompletedGold, 4.f, false); }
	void UpdateDungeonMinimap(bool send);
	void DungeonReveal(const Int2& tile);
	void SaveStock(FileWriter& f, vector<ItemSlot>& cnt);
	void LoadStock(FileReader& f, vector<ItemSlot>& cnt);
	Door* FindDoor(LevelContext& ctx, const Int2& pt);
	void GenerateDungeonObjects2();
	SOUND GetItemSound(const Item* item);
	cstring GetCurrentLocationText();
	void Unit_StopUsingUsable(LevelContext& ctx, Unit& unit, bool send = true);
	void OnReenterLevel(LevelContext& ctx);
	void EnterLevel(LocationGenerator* loc_gen);
	void LeaveLevel(bool clear = false);
	void LeaveLevel(LevelContext& ctx, bool clear);
	void UpdateContext(LevelContext& ctx, float dt);
	// dru¿yna
	bool IsLeader()
	{
		if(Net::IsSingleplayer())
			return true;
		else
			return leader_id == my_id;
	}
	void AddGold(int count, vector<Unit*>* to = nullptr, bool show = false, cstring msg = txGoldPlus, float time = 3.f, bool defmsg = true);
	void AddGoldArena(int count);
	bool IsAnyoneTalking() const;
	bool FindQuestItem2(Unit* unit, cstring id, Quest** quest, int* i_index, bool not_active = false);
	bool RemoveQuestItem(const Item* item, int refid = -1);
	bool RemoveItemFromWorld(const Item* item);
	bool IsBetterItem(Unit& unit, const Item* item, int* value = nullptr);
	// to by mog³o byæ globalna funkcj¹
	void GenerateTreasure(int level, int count, vector<ItemSlot>& items, int& gold, bool extra);
	void SplitTreasure(vector<ItemSlot>& items, int gold, Chest** chests, int count);
	void PlayHitSound(MATERIAL_TYPE mat_bron, MATERIAL_TYPE mat_cialo, const Vec3& hitpoint, float range, bool dmg);
	// wczytywanie
	void LoadingStart(int steps);
	void LoadingStep(cstring text = nullptr, int end = 0);
	void LoadResources(cstring text, bool worldmap);
	bool RequireLoadingResources(Location* loc, bool* to_set);
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
	void DeleteUnit(Unit* unit);
	void DialogTalk(DialogContext& ctx, cstring msg);
	void GenerateHeroName(HeroData& hero);
	void GenerateHeroName(Class clas, bool crazy, string& name);
	bool WantExitLevel()
	{
		return !GKey.KeyDownAllowed(GK_WALK);
	}
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
	void SpawnHeroesInsideDungeon();
	void GenerateQuestUnits();
	void GenerateQuestUnits2(bool on_enter);
	void UpdateQuests(int days);
	void RemoveQuestUnit(UnitData* ud, bool on_leave);
	void RemoveQuestUnits(bool on_leave);
	void UpdateGame2(float dt);
	void SetUnitWeaponState(Unit& unit, bool wyjmuje, WeaponType co);
	void UpdatePlayerView();
	void OnCloseInventory();
	void CloseInventory();
	void CloseAllPanels(bool close_mp_box = false);
	bool CanShowEndScreen();
	void UpdateGameDialogClient();
	bool Cheat_KillAll(int typ, Unit& unit, Unit* ignore);
	void Event_Pvp(int id);
	void Cheat_ShowMinimap();
	void StartPvp(PlayerController* player, Unit* unit);
	void UpdateGameNet(float dt);
	void CheckCredit(bool require_update = false, bool ignore = false);
	void WarpNearLocation(LevelContext& ctx, Unit& uint, const Vec3& pos, float extra_radius, bool allow_exact, int tries = 20);
	void Train(Unit& unit, bool is_skill, int co, int mode = 0);
	void ShowStatGain(bool is_skill, int what, int value);
	void ActivateChangeLeaderButton(bool activate);
	void PayCredit(PlayerController* player, int ile);
	void CreateSaveImage(cstring filename);
	void PlayerUseUsable(Usable* u, bool after_action);
	void UnitTalk(Unit& u, cstring text);
	void OnEnterLocation();
	void OnEnterLevel();
	void OnEnterLevelOrLocation();
	Unit* GetRandomArenaHero();
	cstring GetRandomIdleText(Unit& u);
	void HandleQuestEvent(Quest_Event* event);

	// dodaje przedmiot do ekwipunku postaci (obs³uguje z³oto, otwarty ekwipunek i multiplayer)
	void AddItem(Unit& unit, const Item* item, uint count, uint team_count, bool send_msg = true);
	void AddItem(Unit& unit, const Item* item, uint count = 1, bool is_team = true, bool send_msg = true)
	{
		AddItem(unit, item, count, is_team ? count : 0, send_msg);
	}
	void AddItem(Unit& unit, const GroundItem& item, bool send_msg = true)
	{
		AddItem(unit, item.item, item.count, item.team_count, send_msg);
	}
	// usuwa przedmiot z ekwipunku (obs³uguje otwarty ekwipunek, lock i multiplayer), dla 0 usuwa wszystko
	void RemoveItem(Unit& unit, int i_index, uint count);
	bool RemoveItem(Unit& unit, const Item* item, uint count);

	Unit* FindPlayerTradingWithUnit(Unit& u);
	Int2 GetSpawnPoint();
	bool ValidateTarget(Unit& u, Unit* target);

	void UpdateLights(vector<Light>& lights);
	void UpdatePostEffects(float dt);

	void PlayerYell(Unit& u);
	bool CanBuySell(const Item* item);
	void SetOutsideParams();

	//-----------------------------------------------------------------
	// GUI
	// panele
	LoadScreen* load_screen;
	GameGui* game_gui;
	MainMenu* main_menu;
	WorldMapGui* world_map;
	GlobalGui* global_gui;
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
	Class quickstart_class, autopick_class; // mo¿na po³¹czyæ
	string quickstart_name;
	bool check_updates, skip_tutorial, autoready;
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
	bool DoLobbyUpdate(BitStreamReader& f);
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
	string player_name, server_ip, enter_pswd, server_name2;
	uint autostart_count;
	int max_players2;
	int my_id; // moje unikalne id
	int last_id;
	int last_startup_id;
	bool was_client, players_left;
	vector<PlayerInfo*> old_players;
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
	NetState net_state;
	int net_tries;
	VoidF net_callback;
	string net_adr;
	float net_timer, update_timer, mp_timeout;
	vector<Int2> lobby_updates;
	void AddLobbyUpdate(const Int2& u);
	BitStream prepared_stream;
	bool change_title_a;
	bool level_generated;
	int skip_id_counter;
	vector<string*> net_talk;
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
	vector<Int2> minimap_reveal_mp;
	bool mp_load, mp_load_worldmap;
	float interpolate_timer;
	bool paused, pick_autojoin;

	// zwraca czy pozycja siê zmieni³a
	void UpdateInterpolator(EntityInterpolator* e, float dt, Vec3& pos, float& rot);
	void InterpolateUnits(float dt);
	void InterpolatePlayers(float dt);

	void AddServerMsg(cstring msg);
	void KickPlayer(PlayerInfo& info);
	void ChangeReady();
	void CheckReady();
	void AddMsg(cstring msg);
	void OnEnterPassword(int id);
	void ForceRedraw();
	void PrepareLevelData(BitStream& stream, bool loaded_resources);
	bool ReadLevelData(BitStreamReader& f);
	void SendPlayerData(PlayerInfo& info);
	bool ReadPlayerData(BitStreamReader& stream);
	void UpdateServer(float dt);
	bool ProcessControlMessageServer(BitStreamReader& f, PlayerInfo& info);
	void WriteServerChanges(BitStreamWriter& f);
	void WriteServerChangesForPlayer(BitStreamWriter& f, PlayerInfo& info);
	void UpdateClient(float dt);
	bool ProcessControlMessageClient(BitStreamReader& f, bool& exit_from_server);
	bool ProcessControlMessageClientForMe(BitStreamReader& f);
	void WriteClientChanges(BitStreamWriter& f);
	void Client_Say(BitStreamReader& f);
	void Client_Whisper(BitStreamReader& f);
	void Client_ServerSay(BitStreamReader& f);
	void Server_Say(BitStream& stream, PlayerInfo& info, Packet* packet);
	void Server_Whisper(BitStreamReader& f, PlayerInfo& info, Packet* packet);
	void ServerProcessUnits(vector<Unit*>& units);
	GroundItem* FindItemNetid(int netid, LevelContext** ctx = nullptr);
	void UpdateWarpData(float dt);
	void Net_SpawnUnit(Unit* unit)
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::SPAWN_UNIT;
		c.unit = unit;
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
	// szuka questowych przedmiotów u klienta
	const Item* FindQuestItemClient(cstring id, int refid) const;
	// read item id and return it (can be quest item or gold), results: -2 read error, -1 not found, 0 empty, 1 ok
	int ReadItemAndFind(BitStreamReader& f, const Item*& item) const;
	bool ReadItemList(BitStreamReader& f, vector<ItemSlot>& items);
	bool ReadItemListTeam(BitStreamReader& f, vector<ItemSlot>& items, bool skip = false);
	void ReequipItemsMP(Unit& unit); // zak³ada przedmioty które ma w ekipunku, dostaje broñ jeœli nie ma, podnosi z³oto
	void UseDays(PlayerController* player, int count);
	PlayerInfo* FindOldPlayer(cstring nick);
	void PrepareWorldData(BitStreamWriter& f);
	bool ReadWorldData(BitStreamReader& f);
	void WritePlayerStartData(BitStreamWriter& f, PlayerInfo& info);
	bool ReadPlayerStartData(BitStreamReader& f);
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

	//-----------------------------------------------------------------
	// WORLD MAP
	bool EnterLocation(int level = 0, int from_portal = -1, bool close_portal = false);
	void GenerateWorld();
	void LeaveLocation(bool clear = false, bool end_buffs = true);
	void Event_RandomEncounter(int id);

	Config cfg;
	void SaveCfg();
	cstring GetShortcutText(GAME_KEYS key, cstring action = nullptr);
	void PauseGame();

	//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
	// NEW
	//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
	LocationGeneratorFactory* loc_gen_factory;
	Pathfinding* pathfinding;
};
