#pragma once

#include <App.h>
#include <Config.h>
#include <Timer.h>
#include "BaseLocation.h"
#include "BaseObject.h"
#include "Blood.h"
#include "Const.h"
#include "DialogContext.h"
#include "DrawBatch.h"
#include "GameCommon.h"
#include "GameKeys.h"
#include "LevelQuad.h"
#include "MusicType.h"
#include "Net.h"
#include "Settings.h"

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

enum class FALLBACK
{
	NO = -1,
	TRAIN, // fallbackValue (train what: 0-attribute, 1-skill, 2-tournament, 3-perk, 4-ability), fallbackValue2 (skill/attrib id)
	REST, // fallbackValue (days)
	ARENA,
	ENTER, // enter/exit building - fallbackValue (inside building index), fallbackValue2 (warp to building index or -1)
	EXIT,
	CHANGE_LEVEL, // fallbackValue (direction +1/-1)
	NONE,
	ARENA_EXIT,
	USE_PORTAL, // fallbackValue (portal index)
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

enum class ActionResult
{
	Ignore,
	No,
	Yes
};

class Game final : public App
{
public:
	Game();
	~Game();

	bool OnInit() override;
	void OnCleanup() override;
	void OnDraw() override;
	void OnUpdate(float dt) override;
	void OnResize() override;
	void OnFocus(bool focus, const Int2& activationPoint) override;

	void SetTitle(cstring mode, bool initial = false);
	void CreateRenderTargets();
	void ReportError(int id, cstring text, bool once = false);
	void CrashCallback();

	// initialization & loading
	void PreconfigureGame();
	void PreloadLanguage();
	void PreloadData();
	void LoadSystem();
	void AddFilesystem();
	void LoadDatafiles();
	void LoadLanguageFiles();
	void SetGameText();
	void SetStatsText();
	void ConfigureGame();
	void LoadData();
	void PostconfigureGame();
	void StartGameMode();

	//-----------------------------------------------------------------
	// DRAWING
	//-----------------------------------------------------------------
	void DrawGame();
	void ForceRedraw();
	void ListDrawObjects(LocationPart& locPart, FrustumPlanes& frustum);
	void ListDrawObjectsUnit(FrustumPlanes& frustum, Unit& u);
	void AddObjectToDrawBatch(FrustumPlanes& frustum, const Object& o);
	void ListAreas(LocationPart& locPart);
	void ListEntry(EntryType type, const Int2& pt, GameDirection dir);
	void PrepareAreaPath();
	void PrepareAreaPathCircle(Area2& area, float radius, float range, float rot);
	void PrepareAreaPathCircle(Area2& area, const Vec3& pos, float radius);
	void GatherDrawBatchLights(SceneNode* node);
	void GatherDrawBatchLights(SceneNode* node, float x, float z, float radius, int sub, array<Light*, 3>& lights);
	void DrawScene();
	void DrawDungeon(const vector<DungeonPart>& parts, const vector<DungeonPartGroup>& groups);
	void DrawBloods(const vector<Blood*>& bloods);
	void DrawAreas(const vector<Area>& areas, float range, const vector<Area2*>& areas2);
	void UvModChanged();
	void InitQuadTree();
	void DrawGrass();
	void ListGrass();
	void SetTerrainTextures();
	void ClearQuadtree();
	void ClearGrass();
	void CalculateQuadtree();
	void ListQuadtreeNodes();
	void SetDungeonParamsToMeshes();

	//-----------------------------------------------------------------
	// SOUND & MUSIC
	//-----------------------------------------------------------------
	void SetMusic(MusicType type = MusicType::Default);
	void PlayAttachedSound(Unit& unit, Sound* sound, float distance);
	void UpdateAttachedSounds(float dt);
	void StopAllSounds();

	DialogContext* FindDialogContext(Unit* talker);
	void LoadCfg();
	void SaveCfg();
	cstring GetShortcutText(GAME_KEYS key, cstring action = nullptr);
	void PauseGame();
	void ExitToMenu();
	void DoExitToMenu();
	void TakeScreenshot(bool noGui = false);
	void UpdateGame(float dt);
	void UpdateFallback(float dt);
	void UpdateAi(float dt);
	void UpdateCamera(float dt);
	uint ValidateGameData(bool major);
	uint TestGameData(bool major);
	ActionResult CanLoadGame() const;
	ActionResult CanSaveGame() const;
	void ChangeLevel(int where);
	void ExitToMap();
	void SaveGame(GameWriter& f, SaveSlot* slot);
	void CreateSaveImage();
	bool LoadGameHeader(GameReader& f, SaveSlot& slot);
	void LoadGame(GameReader& f);
	bool TryLoadGame(int slot, bool quickload, bool fromConsole);
	void RemoveUnusedAiAndCheck();
	void CheckUnitsAi(LocationPart& locPart, int& errors);
	bool SaveGameSlot(int slot, cstring text);
	void SaveGameFilename(const string& name);
	bool SaveGameCommon(cstring filename, int slot, cstring text);
	void LoadGameSlot(int slot);
	void LoadGameFilename(const string& name);
	void LoadGameCommon(cstring filename, int slot);
	void LoadLastSave() { LoadGameSlot(lastSave); }
	void SetLastSave(int slot);
	bool ValidateNetSaveForLoading(GameReader& f, int slot);
	void Quicksave();
	void Quickload();
	void ClearGameVars(bool newGame);
	void ClearGame();
	void EnterLevel(LocationGenerator* locGen);
	void LeaveLevel(bool clear = false);
	void LeaveLevel(LocationPart& locPart, bool clear);
	// loading
	void LoadingStart(int steps);
	void LoadingStep(cstring text = nullptr, int end = 0);
	void LoadResources(cstring text, bool worldmap, bool postLoad = true);
	void PreloadResources(bool worldmap);
	void PreloadUnit(Unit* unit);
	void PreloadItems(vector<ItemSlot>& items);
	void VerifyResources();
	void VerifyUnitResources(Unit* unit);
	void VerifyItemResources(const Item* item);
	void DeleteUnit(Unit* unit);
	void RemoveUnit(Unit* unit);
	void OnCloseInventory();
	void CloseInventory();
	bool CanShowEndScreen();
	void UpdateGameNet(float dt);
	void OnEnterLocation();
	void OnEnterLevel();
	void OnEnterLevelOrLocation();
private:
	void GetPostEffects(vector<PostEffect>& postEffects);
public:
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
	void RandomCharacter(Class*& clas, int& hairIndex, HumanData& hd, CreatedCharacter& cc);
	void OnEnterIp(int id);
	void GenericInfoBoxUpdate(float dt);
	void UpdateClientConnectingIp(float dt);
	void UpdateClientTransfer(float dt);
	void UpdateClientQuiting(float dt);
	void UpdateServerTransfer(float dt);
	void UpdateServerSend(float dt);
	void UpdateServerQuiting(float dt);
	void QuickJoinIp();
	void OnEnterPassword(int id);
	void Quit();
	void OnCreateCharacter(int id);
	void OnPlayTutorial(int id);
	void OnPickServer(int id);
	void EndConnecting(cstring msg, bool wait = false);
	void CloseConnection(VoidF f);
	void DoQuit();
	void RestartGame();
	void ClearAndExitToMenu(cstring msg);
	void OnLoadProgress(float progress, cstring str);

	//-----------------------------------------------------------------
	// WORLD MAP
	//-----------------------------------------------------------------
	void EnterLocation(int level = -2, int fromPortal = -1, bool closePortal = false);
	void GenerateWorld();
	void LeaveLocation(bool clear = false, bool takesTime = true);
	void Event_RandomEncounter(int id);

	//-----------------------------------------------------------------
	// COMPONENTS
	//-----------------------------------------------------------------
	LocationGeneratorFactory* locGenFactory;
	Arena* arena;
	BasicShader* basicShader;
	GlowShader* glowShader;
	GrassShader* grassShader;
	ParticleShader* particleShader;
	PostfxShader* postfxShader;
	SkyboxShader* skyboxShader;
	TerrainShader* terrainShader;

	//-----------------------------------------------------------------
	// GAME
	//-----------------------------------------------------------------
	GAME_STATE gameState, prevGameState;
	PlayerController* pc;
	string savePath;
	bool testing, endOfGame, deathSolo, cutscene, inLoad;
	int deathScreen;
	float deathFade, gameSpeed;
	vector<AIController*> ais;
	uint nextSeed;
	int startVersion;
	uint loadErrors, loadWarnings;
	std::set<const Item*> itemsLoad;
	bool hardcoreMode, hardcoreOption, checkUpdates, skipTutorial, changeTitle;
	// quickstart
	QUICKSTART quickstart;
	int quickstartSlot;
	// fallback
	FALLBACK fallbackType;
	int fallbackValue, fallbackValue2;
	float fallbackTimer;
	// dialogs
	DialogContext dialogContext, idleContext;

	//-----------------------------------------------------------------
	// LOADING
	//-----------------------------------------------------------------
	float loadingDt, loadingCap;
	Timer loadingTimer;
	int loadingSteps, loadingIndex;
	bool loadingFirstStep, loadingResources;
	// used temporary at loading
	vector<AIController*> aiBowTargets;
	vector<Location*> loadLocationQuest;
	vector<Unit*> loadUnitHandler;
	vector<Chest*> loadChestHandler;
	vector<pair<Unit*, bool>> unitsMeshLoad;

	//-----------------------------------------------------------------
	// MULTIPLAYER
	//-----------------------------------------------------------------
	string playerName, serverIp, enterPswd;
	enum NET_MODE
	{
		NM_CONNECTING,
		NM_QUITTING,
		NM_QUITTING_SERVER,
		NM_TRANSFER,
		NM_TRANSFER_SERVER,
		NM_SERVER_SEND
	} netMode;
	NetState netState;
	int netTries;
	VoidF netCallback;
	float netTimer, mpTimeout;
	BitStream preparedStream;
	int skipIdCounter;
	float trainMove; // used by client to training by walking
	bool paused;
	vector<ItemSlot> chestTrade; // used by clients when trading

	//-----------------------------------------------------------------
	// DRAWING
	//-----------------------------------------------------------------
	int drawFlags;
	bool drawParticleSphere, drawUnitRadius, drawHitbox, drawPhy, drawCol;
	float portalAnim;
	// scene
	bool useGlow, usePostfx;
	DrawBatch drawBatch;
	int uvMod;
	QuadTree quadtree;
	LevelQuads levelQuads;
	vector<const vector<Matrix>*> grassPatches[2];
	uint grassCount[2];
	// screenshot
	time_t lastScreenshot;
	uint screenshotCount;
	ImageFormat screenshotFormat;

	//-----------------------------------------------------------------
	// SOUND & MUSIC
	//-----------------------------------------------------------------
	vector<AttachedSound> attachedSounds;
	MusicType musicType;

	//-----------------------------------------------------------------
	// CONSOLE & COMMANDS
	//-----------------------------------------------------------------
	Config cfg;
	Settings settings;
	int lastSave;
	bool inactiveUpdate, noai, devmode, defaultDevmode, defaultPlayerDevmode, dontWander;

	//-----------------------------------------------------------------
	// RESOURCES
	//-----------------------------------------------------------------
	RenderTarget* rtSave, *rtItemRot;
	DynamicTexture* tMinimap;

	//-----------------------------------------------------------------
	// LOCALIZED TEXTS
	//-----------------------------------------------------------------
	cstring txPreloadAssets, txCreatingListOfFiles, txConfiguringGame, txLoadingAbilities, txLoadingBuildings, txLoadingClasses, txLoadingDialogs,
		txLoadingItems, txLoadingLocations, txLoadingMusics, txLoadingObjects, txLoadingPerks, txLoadingQuests, txLoadingRequired, txLoadingUnits,
		txLoadingShaders, txLoadingLanguageFiles;
	cstring txAiNoHpPot[2], txAiNoMpPot[2], txAiCity[2], txAiVillage[2], txAiVillageEmpty, txAiForest, txAiMoonwell, txAiAcademy, txAiCampEmpty, txAiCampFull,
		txAiFort, txAiDwarfFort, txAiTower, txAiArmory, txAiHideout, txAiVault, txAiCrypt, txAiTemple, txAiNecromancerBase, txAiLabyrinth, txAiNoEnemies,
		txAiNearEnemies, txAiCave;
	cstring txEnteringLocation, txGeneratingMap, txGeneratingBuildings, txGeneratingObjects, txGeneratingUnits, txGeneratingItems, txGeneratingPhysics,
		txRecreatingObjects, txGeneratingMinimap, txLoadingComplete, txWaitingForPlayers, txLoadingResources;
	cstring txTutPlay, txTutTick;
	cstring txCantSaveGame, txSaveFailed, txLoadFailed, txQuickSave, txGameSaved, txLoadingData, txEndOfLoading, txCantSaveNow, txOnlyServerCanSave,
		txCantLoadGame, txOnlyServerCanLoad, txLoadSignature, txLoadVersion, txLoadSaveVersionOld, txLoadMP, txLoadSP, txLoadOpenError, txCantLoadMultiplayer,
		txTooOldVersion, txMissingPlayerInSave, txGameLoaded, txLoadError, txLoadErrorGeneric, txMissingQuicksave;
	cstring txPvpRefuse, txWin, txWinHardcore, txWinMp, txLevelUp, txLevelDown, txRegeneratingLevel, txNeedItem;
	cstring txRumor[29], txRumorDrunk[7];
	cstring txQuestAlreadyGiven[2], txMayorNoQ[2], txCaptainNoQ[2], txLocationDiscovered[2], txAllDiscovered[2], txCampDiscovered[2], txAllCampDiscovered[2],
		txNoQRumors[2], txNeedMoreGold, txNoNearLoc, txNearLoc, txNearLocEmpty[2], txNearLocCleared, txNearLocEnemy[2], txNoNews[2], txAllNews[2],
		txAllNearLoc, txLearningPoint, txLearningPoints, txNeedLearningPoints, txTeamTooBig, txHeroJoined, txCantLearnAbility, txSpell, txAbility,
		txCantLearnSkill;
	cstring txNear, txFar, txVeryFar, txELvlVeryWeak[2], txELvlWeak[2], txELvlAverage[2], txELvlQuiteStrong[2], txELvlStrong[2];
	cstring txMineBuilt, txAncientArmory, txPortalClosed, txPortalClosedNews, txHiddenPlace, txOrcCamp, txPortalClose, txPortalCloseLevel,
		txXarDanger, txGorushDanger, txGorushCombat, txMageHere, txMageEnter, txMageFinal;
	cstring txEnterIp, txConnecting, txInvalidIp, txWaitingForPswd, txEnterPswd, txConnectingTo, txConnectingProxy, txConnectTimeout, txConnectInvalid,
		txConnectVersion, txConnectSLikeNet, txCantJoin, txLostConnection, txInvalidPswd, txCantJoin2, txServerFull, txInvalidData, txNickUsed, txInvalidVersion,
		txInvalidVersion2, txInvalidNick, txGeneratingWorld, txLoadedWorld, txWorldDataError, txLoadedPlayer, txPlayerDataError, txGeneratingLocation,
		txLoadingLocation, txLoadingLocationError, txLoadingChars, txLoadingCharsError, txSendingWorld, txMpNPCLeft, txLoadingLevel, txDisconnecting,
		txPreparingWorld, txInvalidCrc, txConnectionFailed, txLoadingSaveByServer, txServerFailedToLoadSave;
	cstring txYouAreLeader, txRolledNumber, txPcIsLeader, txReceivedGold, txYouDisconnected, txYouKicked, txGamePaused, txGameResumed, txDevmodeOn,
		txDevmodeOff, txPlayerDisconnected, txPlayerQuit, txPlayerKicked, txServerClosed;
	cstring txYell[3];
	cstring txHaveErrors;

private:
	vector<int> reportedErrors;
	asIScriptContext* cutsceneScript;
	DungeonMeshBuilder* dungeonMeshBuilder;
};
