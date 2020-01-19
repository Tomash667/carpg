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
	void GenerateItemIconTask(TaskData& task_data);
	void GenerateItemIcon(Item& item);
	void DrawItemIcon(const Item& item, RenderTarget* target, float rot);
	void PreloadItem(const Item* item);
	Sound* GetMaterialSound(MATERIAL_TYPE attack_mat, MATERIAL_TYPE hit_mat);
	Sound* GetItemSound(const Item* item);
	void LoadMusic(MusicType type, bool new_load_screen = true, bool instant = false);
	void LoadCommonMusic();

	TexturePtr tBlack, tWarning, tError;
	TexturePtr tBlood[BLOOD_MAX], tBloodSplat[BLOOD_MAX], tSpark, tSpawn, tLightingLine;
	TexturePtr tGrass, tGrass2, tGrass3, tRoad, tFootpath, tField;
	TexOverride tFloor[2], tWall[2], tCeil[2], tFloorBase, tWallBase, tCeilBase;
	MeshPtr aBox, aCylinder, aSphere, aCapsule;
	MeshPtr aHuman, aHair[5], aBeard[5], aMustache[2], aEyebrows;
	MeshPtr aArrow, aSkybox, aBag, aChest, aGrating, aDoorWall, aDoorWall2, aStairsDown, aStairsDown2, aStairsUp, aSpellball, aPressurePlate, aDoor, aDoor2,
		aStun, aPortal;
	VertexDataPtr vdStairsUp, vdStairsDown, vdDoorHole;
	SoundPtr sGulp, sCoins, sBow[2], sDoor[3], sDoorClosed[2], sDoorClose, sItem[10], sChestOpen, sChestClose, sDoorBudge, sRock, sWood, sCrystal, sMetal,
		sBody[5], sBone, sSkin, sArenaFight, sArenaWin, sArenaLost, sUnlock, sEvil, sEat, sSummon, sZap, sCancel;

private:
	void PreloadBuildings();
	void PreloadTraps();
	void PreloadAbilities();
	void PreloadObjects();
	void PreloadItems();

	typedef std::map<Mesh*, Texture*> ItemTextureMap;

	Scene* scene;
	SceneNode* node;
	Camera* camera;
	ItemTextureMap item_texture_map;
	vector<Texture*> over_item_textures;
	RenderTarget* rt_item;
	Texture* missing_item_texture;
	cstring txLoadGuiTextures, txLoadTerrainTextures, txLoadParticles, txLoadModels, txLoadSounds, txLoadMusic;
};
