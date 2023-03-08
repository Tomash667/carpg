#pragma once

//-----------------------------------------------------------------------------
#include "BloodType.h"
#include "Light.h"

//-----------------------------------------------------------------------------
class GameResources
{
public:
	static constexpr int ITEM_IMAGE_SIZE = 64;

	GameResources();
	~GameResources();
	void Init();
	void LoadLanguage();
	void LoadData();
	Texture* CreatePlaceholderTexture(const Int2& size);
	void GenerateItemIconTask(TaskData& taskData);
	void GenerateItemIcon(Item& item);
	void DrawItemIcon(const Item& item, RenderTarget* target, float rot);
	void PreloadItem(const Item* item);
	Sound* GetMaterialSound(MATERIAL_TYPE attackMat, MATERIAL_TYPE hitMat);
	Sound* GetItemSound(const Item* item);
	Mesh* GetEntryMesh(EntryType type);
	void LoadMusic(MusicType type, bool newLoadScreen = true, bool instant = false);
	void LoadCommonMusic();
	void LoadTrap(BaseTrap* trap);
	ParticleEffect* GetParticleEffect(const string& id);

	TexturePtr tBlack, tWarning, tError;
	TexturePtr tBlood[BLOOD_MAX], tBloodSplat[BLOOD_MAX], tLightingLine, tVignette;
	TexturePtr tGrass, tGrass2, tGrass3, tRoad, tFootpath, tField;
	TexOverride tFloor[2], tWall[2], tCeil[2], tFloorBase, tWallBase, tCeilBase;
	MeshPtr aHuman, aHair[5], aBeard[5], aMustache[2], aEyebrows;
	MeshPtr aArrow, aSkybox, aBag, aGrating, aDoorWall, aDoorWall2, aStairsDown, aStairsDown2, aStairsUp, aSpellball, aPressurePlate, aDoor, aDoor2, aStun,       
		aPortal, aDungeonDoor, mVine;
	MeshInstance* tmpMeshInst;
	VertexDataPtr vdStairsUp, vdStairsDown, vdDoorHole;
	SoundPtr sGulp, sCoins, sBow[2], sDoor[3], sDoorClosed[2], sDoorClose, sItem[11], sChestOpen, sChestClose, sDoorBudge, sRock, sWood, sCrystal, sMetal,
		sBody[5], sBone, sSkin, sSlime, sArenaFight, sArenaWin, sArenaLost, sUnlock, sEvil, sEat, sSummon, sZap, sCancel, sCoughs;
	ParticleEffect* peHit, *peSpellHit, *peElectroHit, *peSpawn, *peRaise, *peHeal, *peSpellOther, *peSpellBall, *peBlood;

private:
	void InitEffects();
	void PreloadBuildings();
	void PreloadTraps();
	void PreloadAbilities();
	void PreloadObjects();

	typedef std::map<Mesh*, Texture*> ItemTextureMap;

	Scene* scene;
	SceneNode* node;
	Camera* camera;
	ItemTextureMap itemTextureMap;
	vector<Texture*> overrideItemTextures;
	vector<ParticleEffect*> particleEffects;
	RenderTarget* rtItem;
	Texture* missingItemTexture;
	cstring txLoadGuiTextures, txLoadTerrainTextures, txLoadParticles, txLoadModels, txLoadSounds, txLoadMusic;
};
