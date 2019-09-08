#pragma once

#include <App.h>
#include <Config.h>
#include <Timer.h>
#include "Const.h"
#include "GameCommon.h"
#include "Net.h"
#include "DialogContext.h"
#include "BaseLocation.h"
#include "GameKeys.h"
#include "SceneNode.h"
#include "QuadTree.h"
#include "MusicTrack.h"
#include "Settings.h"
#include "Blood.h"
#include "BaseObject.h"
#include "PlayerController.h"

//-----------------------------------------------------------------------------
// quickstart mode
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
// game state
enum GAME_STATE
{
	GS_MAIN_MENU,
	GS_WORLDMAP,
	GS_LEVEL,
	GS_LOAD,
	GS_EXIT_TO_MENU,
	GS_QUIT,
	GS_LOAD_MENU
};

struct AttachedSound
{
	FMOD::Channel* channel;
	Entity<Unit> unit;
};

static_assert(sizeof(time_t) == sizeof(__int64), "time_t needs to be 64 bit");

enum class FALLBACK
{
	NO = -1,
	TRAIN, // fallback_1 (train what: 0-attribute, 1-skill, 2-tournament, 3-perk), fallback_2 (skill/attrib id)
	REST, // fallback_1 (days)
	ARENA,
	ENTER, // fallback_1 (inside building index)
	EXIT,
	CHANGE_LEVEL, // fallback_1 (direction +1/-1)
	NONE,
	ARENA_EXIT,
	USE_PORTAL, // fallback_1 (portal index)
	WAIT_FOR_WARP,
	ARENA2,
	CLIENT,
	CLIENT2,
	CUTSCENE,
	CUTSCENE_END
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

struct PostEffect
{
	int id;
	D3DXHANDLE tech;
	float power;
	Vec4 skill;
};

typedef std::map<Mesh*, Texture*> ItemTextureMap;

enum class ProfilerMode
{
	Disabled,
	Update,
	Rendering
};

class Game final : public App
{
public:
	Game();
	~Game();

	void OnCleanup() override;
	void OnDraw() override;
	void DrawGame(RenderTarget* target);
	void OnDebugDraw(DebugDrawer* dd);
	void OnUpdate(float dt) override;
	void OnReload() override;
	void OnReset() override;
	void OnResize() override;
	void OnFocus(bool focus, const Int2& activation_point) override;

	bool Start();
	void GetTitle(LocalString& s);
	void ChangeTitle();
	void ClearPointers();
	void CreateTextures();
	void CreateRenderTargets();
	void PreloadData();
	void ReportError(int id, cstring text, bool once = false);

	// initialization
	void BeforeInit();
	bool OnInit() override;
	void PreconfigureGame();
	void PreloadLanguage();
	void CreatePlaceholderResources();

	// loading system
	void LoadSystem();
	void AddFilesystem();
	void LoadDatafiles();
	bool LoadRequiredStats(uint& errors);
	void LoadLanguageFiles();
	void SetGameText();
	void SetStatsText();
	void ConfigureGame();

	// loading data
	void LoadData();
	void AddLoadTasks();

	// after loading data
	void PostconfigureGame();
	void StartGameMode();

	void ReloadShaders();
	void ReleaseShaders();
	void InitScene();
	void BuildDungeon();
	void ChangeDungeonTexWrap();
	void FillDungeonPart(Int2* dungeon_part, word* faces, int& index, word offset);
	void ListDrawObjects(LevelArea& area, FrustumPlanes& frustum, bool outside);
	void ListDrawObjectsUnit(LevelArea* area, FrustumPlanes& frustum, bool outside, Unit& u);
	void AddObjectToDrawBatch(LevelArea& area, const Object& o, FrustumPlanes& frustum);
	void ListAreas(LevelArea& area);
	void PrepareAreaPath();
	void PrepareAreaPathCircle(Area2& area, float radius, float range, float rot);
	void PrepareAreaPathCircle(Area2& area, const Vec3& pos, float radius);
	void FillDrawBatchDungeonParts(FrustumPlanes& frustum);
	void AddOrSplitSceneNode(SceneNode* node, int exclude_subs = 0);
	int GatherDrawBatchLights(LevelArea& area, SceneNode* node, float x, float z, float radius, int sub = 0);
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
	void CalculateQuadtree();
	void ListQuadtreeNodes();
	void SetupConfigVars();
	MeshInstance* GetBowInstance(Mesh* mesh);
	DialogContext* FindDialogContext(Unit* talker);
	void LoadMusic(MusicType type, bool new_load_screen = true, bool instant = false);
	void SetMusic();
	void SetMusic(MusicType type);
	void SetupTracks();
	void UpdateMusic();
	void SaveCfg();
	cstring GetShortcutText(GAME_KEYS key, cstring action = nullptr);
	void PauseGame();
	void Draw();
	void ExitToMenu();
	void DoExitToMenu();
	void GenerateItemImage(TaskData& task_data);
	void GenerateItemImageImpl(Item& item);
	Texture* TryGenerateItemImage(const Item& item);
	void DrawItemImage(const Item& item, RenderTarget* target, float rot);
	void SetupObject(BaseObject& obj);
	void SetupCamera(float dt);
	void LoadShaders();
	void SetupShaders();
	void TakeScreenshot(bool no_gui = false);
	void UpdateGame(float dt);
	void UpdateFallback(float dt);
	void UpdatePlayer(float dt, bool allow_rot);
	void UseAction(PlayerController* p, bool from_server, const Vec3* pos_data = nullptr, Unit* target = nullptr);
	void SpawnUnitEffect(Unit& unit);
	void PlayerCheckObjectDistance(Unit& u, const Vec3& pos, void* ptr, float& best_dist, BeforePlayer type);
	int CheckMove(Vec3& pos, const Vec3& dir, float radius, Unit* me, bool* is_small = nullptr);
	void UpdateAi(float dt);
	void CheckAutoTalk(Unit& unit, float dt);
	void ApplyLocationTexturePack(TexturePack& floor, TexturePack& wall, TexturePack& ceil, LocationTexturePack& tex);
	void ApplyLocationTexturePack(TexturePack& pack, LocationTexturePack::Entry& e, TexturePack& pack_def);
	void SetDungeonParamsAndTextures(BaseLocation& base);
	void SetDungeonParamsToMeshes();
	void MoveUnit(Unit& unit, bool warped = false, bool dash = false);
	uint ValidateGameData(bool major);
	uint TestGameData(bool major);
	void TestUnitSpells(const SpellList& spells, string& errors, uint& count);
	Unit* CreateUnit(UnitData& base, int level = -1, Human* human_data = nullptr, Unit* test_unit = nullptr, bool create_physics = true, bool custom = false);
	bool CheckForHit(LevelArea& area, Unit& unit, Unit*& hitted, Vec3& hitpoint);
	bool CheckForHit(LevelArea& area, Unit& unit, Unit*& hitted, Mesh::Point& hitbox, Mesh::Point* bone, Vec3& hitpoint);
	void UpdateParticles(LevelArea& area, float dt);
	// perform character attack
	enum ATTACK_RESULT
	{
		ATTACK_NOT_HIT,
		ATTACK_BLOCKED,
		ATTACK_NO_DAMAGE,
		ATTACK_HIT,
		ATTACK_CLEAN_HIT
	};
	ATTACK_RESULT DoAttack(LevelArea& area, Unit& unit);
	enum DamageFlags
	{
		DMG_NO_BLOOD = 1 << 0,
		DMG_MAGICAL = 1 << 1
	};
	void GiveDmg(Unit& taker, float dmg, Unit* giver = nullptr, const Vec3* hitpoint = nullptr, int dmg_flags = 0);
	void UpdateUnits(LevelArea& area, float dt);
	bool CanLoadGame() const;
	bool CanSaveGame() const;
	bool DoShieldSmash(LevelArea& area, Unit& attacker);
	void UpdateBullets(LevelArea& area, float dt);
	Vec3 PredictTargetPos(const Unit& me, const Unit& target, float bullet_speed) const;
	bool CanShootAtLocation(const Unit& me, const Unit& target, const Vec3& pos) const { return CanShootAtLocation2(me, &target, pos); }
	bool CanShootAtLocation(const Vec3& from, const Vec3& to) const;
	bool CanShootAtLocation2(const Unit& me, const void* ptr, const Vec3& to) const;
	void LoadItemsData();
	Unit* CreateUnitWithAI(LevelArea& area, UnitData& unit, int level = -1, Human* human_data = nullptr, const Vec3* pos = nullptr, const float* rot = nullptr, AIController** ai = nullptr);
	void ChangeLevel(int where);
	void ExitToMap();
	Sound* GetMaterialSound(MATERIAL_TYPE m1, MATERIAL_TYPE m2);
	void PlayAttachedSound(Unit& unit, Sound* sound, float distance);
	void StopAllSounds();
	ATTACK_RESULT DoGenericAttack(LevelArea& area, Unit& attacker, Unit& hitted, const Vec3& hitpoint, float attack, int dmg_type, bool bash);
	void SaveGame(GameWriter& f, SaveSlot* slot);
	void CreateSaveImage();
	bool LoadGameHeader(GameReader& f, SaveSlot& slot);
	void LoadGame(GameReader& f);
	bool TryLoadGame(int slot, bool quickload, bool from_console);
	void RemoveUnusedAiAndCheck();
	void CheckUnitsAi(LevelArea& area, int& err_count);
	void CastSpell(Unit& unit);
	void CastPlayerSpell(PlayerController& player);
	void SpellHitEffect(LevelArea& area, Bullet& bullet, const Vec3& pos, Unit* hitted);
	void UpdateExplosions(LevelArea& area, float dt);
	void UpdateTraps(LevelArea& area, float dt);
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
	void UpdateElectros(LevelArea& area, float dt);
	void UpdateDrains(LevelArea& area, float dt);
	void AI_Shout(LevelArea& area, AIController& ai);
	void AI_DoAttack(AIController& ai, Unit* target, bool running = false);
	void AI_HitReaction(Unit& unit, const Vec3& pos);
	void UpdateAttachedSounds(float dt);
	bool SaveGameSlot(int slot, cstring text);
	void SaveGameFilename(const string& name);
	bool SaveGameCommon(cstring filename, int slot, cstring text);
	void LoadGameSlot(int slot);
	void LoadGameFilename(const string& name);
	void LoadGameCommon(cstring filename, int slot);
	bool ValidateNetSaveForLoading(GameReader& f, int slot);
	void Quicksave(bool from_console);
	void Quickload(bool from_console);
	void ClearGameVars(bool new_game);
	void ClearGame();
	Sound* GetItemSound(const Item* item);
	void Unit_StopUsingUsable(LevelArea& area, Unit& unit, bool send = true);
	void EnterLevel(LocationGenerator* loc_gen);
	void LeaveLevel(bool clear = false);
	void LeaveLevel(LevelArea& area, bool clear);
	void UpdateArea(LevelArea& area, float dt);
	bool IsAnyoneTalking() const;
	// this could be a global function
	void PlayHitSound(MATERIAL_TYPE mat_weapon, MATERIAL_TYPE mat_body, const Vec3& hitpoint, float range, bool dmg);
	// loading
	void LoadingStart(int steps);
	void LoadingStep(cstring text = nullptr, int end = 0);
	void LoadResources(cstring text, bool worldmap);
	void PreloadResources(bool worldmap);
	void PreloadUsables(vector<Usable*>& usable);
	void PreloadUnits(vector<Unit*>& units);
	void PreloadUnit(Unit* unit);
	void PreloadItems(vector<ItemSlot>& items);
	void PreloadItem(const Item* item);
	void VerifyResources();
	void VerifyUnitResources(Unit* unit);
	void VerifyItemResources(const Item* item);
	void DeleteUnit(Unit* unit);
	bool WantExitLevel() { return !GKey.KeyDownAllowed(GK_WALK); }
	float PlayerAngleY();
	void AttackReaction(Unit& attacked, Unit& attacker);
	void GenerateQuestUnits();
	void GenerateQuestUnits2();
	void UpdateQuests(int days);
	void RemoveQuestUnit(UnitData* ud, bool on_leave);
	void RemoveQuestUnits(bool on_leave);
	void UpdateGame2(float dt);
	void OnCloseInventory();
	void CloseInventory();
	bool CanShowEndScreen();
	void UpdateGameDialogClient();
	void UpdateGameNet(float dt);
	void PlayerUseUsable(Usable* u, bool after_action);
	void OnEnterLocation();
	void OnEnterLevel();
	void OnEnterLevelOrLocation();
	cstring GetRandomIdleText(Unit& u);
	void HandleQuestEvent(Quest_Event* event);
	void UpdateLights(vector<Light>& lights);
	void UpdatePostEffects(float dt);
	// --- cutscene
	void CutsceneStart(bool instant);
	void CutsceneImage(const string& image, float time);
	void CutsceneText(const string& text, float time);
	void CutsceneEnd();
	void CutsceneEnded(bool cancel);
	bool CutsceneShouldSkip();

	//-----------------------------------------------------------------
	// MENU / MAIN MENU / OPTIONS
	//-----------------------------------------------------------------
	bool CanShowMenu();
	void SaveOptions();
	void StartNewGame();
	void NewGameCommon(Class* clas, cstring name, HumanData& hd, CreatedCharacter& cc, bool tutorial);
	void StartQuickGame();
	void MultiplayerPanelEvent(int id);
	void CreateServerEvent(int id);
	// set for Random player character (clas is in/out)
	void RandomCharacter(Class*& clas, int& hair_index, HumanData& hd, CreatedCharacter& cc);
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
	void OnCreateCharacter(int id);
	void OnPlayTutorial(int id);
	void OnPickServer(int id);
	void EndConnecting(cstring msg, bool wait = false);
	void CloseConnection(VoidF f);
	void DoQuit();
	void RestartGame();
	void ClearAndExitToMenu(cstring msg);

	//-----------------------------------------------------------------
	// MULTIPLAYER
	//-----------------------------------------------------------------
	void AddServerMsg(cstring msg);
	void AddMsg(cstring msg);
	void OnEnterPassword(int id);
	void ForceRedraw();
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
	void Server_Say(BitStreamReader& f, PlayerInfo& info, Packet* packet);
	void Server_Whisper(BitStreamReader& f, PlayerInfo& info, Packet* packet);
	void ServerProcessUnits(vector<Unit*>& units);
	void UpdateWarpData(float dt);
	void Net_LeaveLocation(int where)
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::LEAVE_LOCATION;
		c.id = where;
	}
	void Net_OnNewGameServer();
	void Net_OnNewGameClient();
	// read item id and return it (can be quest item or gold), results: -2 read error, -1 not found, 0 empty, 1 ok
	int ReadItemAndFind(BitStreamReader& f, const Item*& item) const;
	bool ReadItemList(BitStreamReader& f, vector<ItemSlot>& items);
	bool ReadItemListTeam(BitStreamReader& f, vector<ItemSlot>& items, bool skip = false);
	bool CheckMoveNet(Unit& unit, const Vec3& pos);
	void ProcessLeftPlayers();
	void RemovePlayer(PlayerInfo& info);

	//-----------------------------------------------------------------
	// WORLD MAP
	//-----------------------------------------------------------------
	void EnterLocation(int level = 0, int from_portal = -1, bool close_portal = false);
	void GenerateWorld();
	void LeaveLocation(bool clear = false, bool end_buffs = true);
	void Event_RandomEncounter(int id);

	//-----------------------------------------------------------------
	// COMPONENTS
	//-----------------------------------------------------------------
	LocationGeneratorFactory* loc_gen_factory;
	Arena* arena;
	DebugDrawer* debug_drawer;
	GrassShader* grass_shader;
	SuperShader* super_shader;
	TerrainShader* terrain_shader;

	//-----------------------------------------------------------------
	// GAME
	//-----------------------------------------------------------------
	GAME_STATE game_state, prev_game_state;
	LocalPlayerData pc_data;
	PlayerController* pc;
	bool testing, force_seed_all, end_of_game, target_loc_is_camp, death_solo, cutscene;
	int death_screen;
	float death_fade, game_speed;
	vector<MeshInstance*> bow_instances;
	vector<AIController*> ais;
	uint force_seed, next_seed;
	ProfilerMode profiler_mode;
	int start_version;
	ItemTextureMap item_texture_map;
	vector<Texture*> over_item_textures;
	uint load_errors, load_warnings;
	Texture missing_item_texture;
	std::set<const Item*> items_load;
	bool hardcore_mode, hardcore_option, check_updates, skip_tutorial;
	// quickstart
	QUICKSTART quickstart;
	int quickstart_slot;
	// fallback
	FALLBACK fallback_type;
	int fallback_1, fallback_2;
	float fallback_t;
	// dialogs
	DialogContext dialog_context;
	vector<string> dialog_choices; // used in client multiplayer mode
	string predialog;

	//-----------------------------------------------------------------
	// LOADING
	//-----------------------------------------------------------------
	float loading_dt, loading_cap;
	Timer loading_t;
	int loading_steps, loading_index;
	bool loading_first_step;
	// used temporary at loading
	vector<AIController*> ai_bow_targets;
	vector<Location*> load_location_quest;
	vector<Unit*> load_unit_handler;
	vector<Chest*> load_chest_handler;
	vector<pair<Unit*, bool>> units_mesh_load;

	//-----------------------------------------------------------------
	// MULTIPLAYER
	//-----------------------------------------------------------------
	string player_name, server_ip, enter_pswd;
	enum NET_MODE
	{
		NM_CONNECTING,
		NM_QUITTING,
		NM_QUITTING_SERVER,
		NM_TRANSFER,
		NM_TRANSFER_SERVER,
		NM_SERVER_SEND
	} net_mode;
	NetState net_state;
	int net_tries;
	VoidF net_callback;
	float net_timer, mp_timeout;
	BitStream prepared_stream;
	int skip_id_counter;
	struct WarpData
	{
		Unit* u;
		int where; // <-1 - get outside the building,  >=0 - get inside the building
		float timer;
	};
	vector<WarpData> mp_warps;
	float train_move; // used by client to training by walking
	bool anyone_talking;
	float interpolate_timer;
	bool paused;
	vector<ItemSlot> chest_trade; // used by clients when trading

	//-----------------------------------------------------------------
	// DRAWING
	//-----------------------------------------------------------------
	int draw_flags;
	Matrix mat;
	int particle_count;
	VB vbDungeon;
	IB ibDungeon;
	Int2 dungeon_part[16], dungeon_part2[16], dungeon_part3[16], dungeon_part4[16];
	bool draw_particle_sphere, draw_unit_radius, draw_hitbox, draw_phy, draw_col, cl_postfx;
	float portal_anim, drunk_anim, grayout;
	// post effect u¿ywa 3 tekstur lub jeœli jest w³¹czony multisampling 3 surface i 1 tekstury
	SURFACE sPostEffect[3];
	TEX tPostEffect[3];
	VB vbFullscreen;
	vector<PostEffect> post_effects;
	// scene
	Color clear_color, clear_color_next;
	bool dungeon_tex_wrap;
	bool cl_normalmap, cl_specularmap, cl_glow;
	DrawBatch draw_batch;
	VDefault blood_v[4];
	VParticle billboard_v[4];
	Vec3 billboard_ext[4];
	VParticle portal_v[4];
	int uv_mod;
	QuadTree quadtree;
	LevelParts level_parts;
	vector<const vector<Matrix>*> grass_patches[2];
	uint grass_count[2];
	// screenshot
	time_t last_screenshot;
	uint screenshot_count;
	ImageFormat screenshot_format;

	//-----------------------------------------------------------------
	// SOUND & MUSIC
	//-----------------------------------------------------------------
	vector<AttachedSound> attached_sounds;
	MusicType music_type;
	MusicTrack* last_music;
	vector<MusicTrack*> tracks;
	int track_id;

	//-----------------------------------------------------------------
	// CONSOLE & COMMANDS
	//-----------------------------------------------------------------
	Config cfg;
	Settings settings;
	bool inactive_update, noai, devmode, default_devmode, default_player_devmode, dont_wander;
	string cfg_file;

	//-----------------------------------------------------------------
	// RESOURCES
	//-----------------------------------------------------------------
	MeshPtr aHumanBase, aHair[5], aBeard[5], aMustache[2], aEyebrows;
	MeshPtr aBox, aCylinder, aSphere, aCapsule;
	MeshPtr aArrow, aSkybox, aBag, aChest, aGrating, aDoorWall, aDoorWall2, aStairsDown, aStairsDown2, aStairsUp, aSpellball, aPressurePlate, aDoor, aDoor2, aStun;
	VertexDataPtr vdStairsUp, vdStairsDown, vdDoorHole;
	RenderTarget* rt_save, *rt_item, *rt_item_rot;
	Texture tMinimap;
	TexturePtr tCzern, tPortal, tLightingLine, tRip, tEquipped, tWarning, tError;
	TexturePtr tKrew[BLOOD_MAX], tKrewSlad[BLOOD_MAX], tIskra, tSpawn;
	TexturePack tFloor[2], tWall[2], tCeil[2], tFloorBase, tWallBase, tCeilBase;
	ID3DXEffect* eMesh, *eParticle, *eSkybox, *eArea, *ePostFx, *eGlow;
	D3DXHANDLE techMesh, techMeshDir, techMeshSimple, techMeshSimple2, techMeshExplo, techParticle, techSkybox, techArea, techTrail, techGlowMesh, techGlowAni;
	D3DXHANDLE hMeshCombined, hMeshWorld, hMeshTex, hMeshFogColor, hMeshFogParam, hMeshTint, hMeshAmbientColor, hMeshLightDir, hMeshLightColor, hMeshLights,
		hParticleCombined, hParticleTex, hSkyboxCombined, hSkyboxTex, hAreaCombined, hAreaColor, hAreaPlayerPos, hAreaRange, hPostTex, hPostPower, hPostSkill,
		hGlowCombined, hGlowBones, hGlowColor, hGlowTex;
	SoundPtr sGulp, sCoins, sBow[2], sDoor[3], sDoorClosed[2], sDoorClose, sItem[10], sChestOpen, sChestClose, sDoorBudge, sRock, sWood, sCrystal, sMetal,
		sBody[5], sBone, sSkin, sArenaFight, sArenaWin, sArenaLost, sUnlock, sEvil, sEat, sSummon, sZap, sCancel;
	VB vbParticle;
	TexturePtr tGrass, tGrass2, tGrass3, tRoad, tFootpath, tField;

	//-----------------------------------------------------------------
	// LOCALIZED TEXTS
	//-----------------------------------------------------------------
	cstring txLoadGuiTextures, txLoadParticles, txLoadPhysicMeshes, txLoadModels, txLoadSpells, txLoadSounds, txLoadMusic, txGenerateWorld;
	cstring txCreatingListOfFiles, txConfiguringGame, txLoadingItems, txLoadingObjects, txLoadingSpells, txLoadingUnits, txLoadingMusics, txLoadingBuildings,
		txLoadingRequires, txLoadingShaders, txLoadingDialogs, txLoadingLanguageFiles, txPreloadAssets, txLoadingQuests, txLoadingClasses;
	cstring txAiNoHpPot[2], txAiCity[2], txAiVillage[2], txAiMoonwell, txAiForest, txAiCampEmpty, txAiCampFull, txAiFort, txAiDwarfFort, txAiTower, txAiArmory,
		txAiHideout, txAiVault, txAiCrypt, txAiTemple, txAiNecromancerBase, txAiLabyrinth, txAiNoEnemies, txAiNearEnemies, txAiCave, txAiInsaneText[11],
		txAiDefaultText[9], txAiOutsideText[3], txAiInsideText[2], txAiHumanText[2], txAiOrcText[7], txAiGoblinText[5], txAiMageText[4], txAiSecretText[3],
		txAiHeroDungeonText[4], txAiHeroCityText[5], txAiBanditText[6], txAiHeroOutsideText[2], txAiDrunkMageText[3], txAiDrunkText[6],
		txAiDrunkContestText[4], txAiWildHunterText[3];
	cstring txEnteringLocation, txGeneratingMap, txGeneratingBuildings, txGeneratingObjects, txGeneratingUnits, txGeneratingItems, txGeneratingPhysics,
		txRecreatingObjects, txGeneratingMinimap, txLoadingComplete, txWaitingForPlayers, txLoadingResources;
	cstring txTutPlay, txTutTick;
	cstring txCantSaveGame, txSaveFailed, txLoadFailed, txQuickSave, txGameSaved, txLoadingLocations, txLoadingData, txEndOfLoading, txCantSaveNow,
		txOnlyServerCanSave, txCantLoadGame, txOnlyServerCanLoad, txLoadSignature, txLoadVersion, txLoadSaveVersionOld, txLoadMP, txLoadSP, txLoadOpenError,
		txCantLoadMultiplayer, txTooOldVersion, txMissingPlayerInSave, txGameLoaded, txLoadError, txLoadErrorGeneric;
	cstring txPvpRefuse, txWin, txWinMp, txLevelUp, txLevelDown, txRegeneratingLevel, txNeedItem;
	cstring txRumor[29], txRumorD[7];
	cstring txMayorQFailed[3], txQuestAlreadyGiven[2], txMayorNoQ[2], txCaptainQFailed[2], txCaptainNoQ[2], txLocationDiscovered[2], txAllDiscovered[2],
		txCampDiscovered[2], txAllCampDiscovered[2], txNoQRumors[2], txNeedMoreGold, txNoNearLoc, txNearLoc, txNearLocEmpty[2], txNearLocCleared,
		txNearLocEnemy[2], txNoNews[2], txAllNews[2], txAllNearLoc, txLearningPoint, txLearningPoints, txNeedLearningPoints;
	cstring txNear, txFar, txVeryFar, txELvlVeryWeak[2], txELvlWeak[2], txELvlAverage[2], txELvlQuiteStrong[2], txELvlStrong[2];
	cstring txMineBuilt, txAncientArmory, txPortalClosed, txPortalClosedNews, txHiddenPlace, txOrcCamp, txPortalClose, txPortalCloseLevel,
		txXarDanger, txGorushDanger, txGorushCombat, txMageHere, txMageEnter, txMageFinal, txQuest[279], txForMayor, txForSoltys;
	cstring txEnterIp, txConnecting, txInvalidIp, txWaitingForPswd, txEnterPswd, txConnectingTo, txConnectingProxy, txConnectTimeout, txConnectInvalid,
		txConnectVersion, txConnectSLikeNet, txCantJoin, txLostConnection, txInvalidPswd, txCantJoin2, txServerFull, txInvalidData, txNickUsed, txInvalidVersion,
		txInvalidVersion2, txInvalidNick, txGeneratingWorld, txLoadedWorld, txWorldDataError, txLoadedPlayer, txPlayerDataError, txGeneratingLocation,
		txLoadingLocation, txLoadingLocationError, txLoadingChars, txLoadingCharsError, txSendingWorld, txMpNPCLeft, txLoadingLevel, txDisconnecting,
		txPreparingWorld, txInvalidCrc, txConnectionFailed, txLoadingSaveByServer, txServerFailedToLoadSave;
	cstring txServer, txYouAreLeader, txRolledNumber, txPcIsLeader, txReceivedGold, txYouDisconnected, txYouKicked, txGamePaused, txGameResumed, txDevmodeOn,
		txDevmodeOff, txPlayerDisconnected, txPlayerQuit, txPlayerKicked, txServerClosed;
	cstring txYell[3];
	cstring txHaveErrors;

private:
	Engine* engine;
	vector<int> reported_errors;
	asIScriptContext* cutscene_script;
};
