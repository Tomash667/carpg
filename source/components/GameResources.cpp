#include "Pch.h"
#include "GameResources.h"

#include "Ability.h"
#include "BaseTrap.h"
#include "Building.h"
#include "Game.h"
#include "GameGui.h"
#include "Item.h"
#include "Language.h"
#include "Level.h"
#include "ParticleEffect.h"
#include "Tile.h"

#include <Mesh.h>
#include <Render.h>
#include <RenderTarget.h>
#include <ResourceManager.h>
#include <SoundManager.h>
#include <Scene.h>
#include <SceneManager.h>

GameResources* gameRes;

//=================================================================================================
GameResources::GameResources() : scene(nullptr), node(nullptr), camera(nullptr)
{
}

//=================================================================================================
GameResources::~GameResources()
{
	delete scene;
	delete camera;
	delete tmpMeshInst;
	for(auto& item : itemTextureMap)
		delete item.second;
	for(Texture* tex : overrideItemTextures)
		delete tex;
	DeleteElements(particleEffects);
}

//=================================================================================================
void GameResources::Init()
{
	scene = new Scene;
	scene->clearColor = Color::None;

	Light light;
	light.range = 10.f;
	light.color = Vec4::One;
	scene->lights.push_back(light);

	node = SceneNode::Get();
	scene->Add(node);

	camera = new Camera;
	camera->aspect = 1.f;
	camera->znear = 0.1f;
	camera->zfar = 25.f;

	tmpMeshInst = new MeshInstance(nullptr);

	aHuman = resMgr->Load<Mesh>("human.qmsh");
	rtItem = render->CreateRenderTarget(Int2(ITEM_IMAGE_SIZE, ITEM_IMAGE_SIZE), RenderTarget::F_NO_DRAW);
	missingItemTexture = CreatePlaceholderTexture(Int2(ITEM_IMAGE_SIZE, ITEM_IMAGE_SIZE));

	InitEffects();
}

//=================================================================================================
void GameResources::InitEffects()
{
	ParticleEffect* effect = new ParticleEffect;
	effect->id = "hit";
	effect->tex = resMgr->Get<Texture>("iskra.png");
	effect->life = 5.f;
	effect->particleLife = 0.5f;
	effect->emissionInterval = 0.f;
	effect->emissions = 1;
	effect->spawn = Int2(10, 15);
	effect->maxParticles = 15;
	effect->speedMin = Vec3(-1, 0, -1);
	effect->speedMax = Vec3(1, 1, 1);
	effect->posMin = Vec3(-0.1f, -0.1f, -0.1f);
	effect->posMax = Vec3(0.1f, 0.1f, 0.1f);
	effect->alpha = Vec2(0.9f, 0.f);
	effect->size = Vec2(0.3f, 0.f);
	particleEffects.push_back(effect);
	peHit = effect;

	effect = new ParticleEffect;
	effect->id = "spellHit";
	//effect->tex = ability->texParticle;
	effect->tex = nullptr;
	effect->life = -1;
	effect->particleLife = 0.5f;
	effect->emissionInterval = 0.1f;
	effect->emissions = -1;
	effect->spawn = Int2(3, 4);
	effect->maxParticles = 50;
	effect->speedMin = Vec3(-1, -1, -1);
	effect->speedMax = Vec3(1, 1, 1);
	//effect->posMin = Vec3(-ability->size, -ability->size, -ability->size);
	//effect->posMax = Vec3(ability->size, ability->size, ability->size);
	effect->alpha = Vec2(1.f, 0.f);
	//effect->size = Vec2(ability->sizeParticle, 0.f);
	effect->mode = 1;
	particleEffects.push_back(effect);
	peSpellHit = effect;

	effect = new ParticleEffect;
	effect->id = "electroHit";
	//effect->tex = ability->texParticle;
	effect->tex = nullptr;
	effect->life = 0.f;
	effect->particleLife = 0.5f;
	effect->emissionInterval = 0.f;
	effect->emissions = 1;
	effect->spawn = Int2(8, 12);
	effect->maxParticles = 12;
	effect->speedMin = Vec3(-1.5f, -1.5f, -1.5f);
	effect->speedMax = Vec3(1.5f, 1.5f, 1.5f);
	//effect->posMin = Vec3(-ability->size, -ability->size, -ability->size);
	//effect->posMax = Vec3(ability->size, ability->size, ability->size);
	effect->alpha = Vec2(1.f, 0.f);
	//effect->size = Vec2(ability->sizeParticle, 0.f);
	effect->mode = 1;
	particleEffects.push_back(effect);
	peElectroHit = effect;

	effect = new ParticleEffect;
	effect->id = "torch";
	effect->tex = resMgr->Get<Texture>("flare.png");
	effect->life = -1;
	effect->particleLife = 0.5f;
	effect->emissionInterval = 0.1f;
	effect->emissions = -1;
	effect->spawn = Int2(1, 3);
	effect->maxParticles = 50;
	effect->posMin = Vec3(0, 0, 0);
	effect->posMax = Vec3(0, 0, 0);
	effect->speedMin = Vec3(-1, 3, -1);
	effect->speedMax = Vec3(1, 4, 1);
	effect->alpha = Vec2(0.8f, 0.f);
	effect->size = Vec2(0.5f, 0.f);
	effect->mode = 1;
	particleEffects.push_back(effect);
	peTorch = effect;

	effect = new ParticleEffect(*peTorch);
	effect->id = "torchCeiling";
	effect->size = Vec2(1.f, 0.f);
	particleEffects.push_back(effect);
	peTorchCeiling = effect;

	effect = new ParticleEffect(*peTorch);
	effect->id = "magicTorch";
	effect->tex = resMgr->Get<Texture>("flare2.png");
	particleEffects.push_back(effect);
	peMagicTorch = effect;

	effect = new ParticleEffect(*peTorch);
	effect->id = "campfire";
	effect->size = Vec2(0.7f, 0.f);
	particleEffects.push_back(effect);
	peCampfire = effect;

	effect = new ParticleEffect;
	effect->id = "altarBlood";
	effect->tex = resMgr->Get<Texture>("krew.png");
	effect->life = -1;
	effect->particleLife = 0.5f;
	effect->emissionInterval = 0.1f;
	effect->emissions = -1;
	effect->spawn = Int2(1, 3);
	effect->maxParticles = 50;
	effect->posMin = Vec3(0, 0, 0);
	effect->posMax = Vec3(0, 0, 0);
	effect->speedMin = Vec3(-1, 4, -1);
	effect->speedMax = Vec3(1, 6, 1);
	effect->alpha = Vec2(0.8f, 0.f);
	effect->size = Vec2(0.5f, 0.f);
	particleEffects.push_back(effect);

	effect = new ParticleEffect;
	effect->id = "water";
	effect->tex = resMgr->Get<Texture>("water.png");
	effect->life = -1;
	effect->particleLife = 3.f;
	effect->emissionInterval = 0.1f;
	effect->emissions = -1;
	effect->spawn = Int2(4, 8);
	effect->maxParticles = 500;
	effect->posMin = Vec3(0, 0, 0);
	effect->posMax = Vec3(0, 0, 0);
	effect->speedMin = Vec3(-0.6f, 4, -0.6f);
	effect->speedMax = Vec3(0.6f, 7, 0.6f);
	effect->alpha = Vec2(0.8f, 0.f);
	effect->size = Vec2(0.05f, 0.f);
	particleEffects.push_back(effect);

	effect = new ParticleEffect;
	effect->id = "magicfire";
	effect->tex = resMgr->Get<Texture>("flare2.png");
	effect->life = -1;
	effect->particleLife = 0.5f;
	effect->emissionInterval = 0.1f;
	effect->emissions = -1;
	effect->spawn = Int2(2, 4);
	effect->maxParticles = 50;
	effect->posMin = Vec3(0, 0, 0);
	effect->posMax = Vec3(0, 0, 0);
	effect->speedMin = Vec3(-1, 3, -1);
	effect->speedMax = Vec3(1, 4, 1);
	effect->alpha = Vec2(1.0f, 0.f);
	effect->size = Vec2(1.0f, 0.f);
	effect->mode = 1;
	particleEffects.push_back(effect);

	effect = new ParticleEffect;
	effect->id = "smoke";
	effect->tex = resMgr->Get<Texture>("smoke.png");
	effect->life = -1;
	effect->particleLife = 2.5f;
	effect->emissionInterval = 0.1f;
	effect->emissions = -1;
	effect->spawn = Int2(2, 4);
	effect->maxParticles = 100;
	effect->posMin = Vec3(-0.5f, 0, -0.5f);
	effect->posMax = Vec3(0.5f, 0, 0.5f);
	effect->speedMin = Vec3(-0.5f, 3, -0.5f);
	effect->speedMax = Vec3(0.5f, 4, 0.5f);
	effect->alpha = Vec2(0.25f, 0.f);
	effect->size = Vec2(1.0f, 5.f);
	effect->gravity = false;
	particleEffects.push_back(effect);

	effect = new ParticleEffect;
	effect->id = "spawn";
	effect->tex = resMgr->Get<Texture>("spawn_fog.png");
	effect->life = 5.f;
	effect->particleLife = 0.5f;
	effect->emissionInterval = 0.1f;
	effect->emissions = 5;
	effect->spawn = Int2(10, 15);
	effect->maxParticles = 15 * 5;
	effect->speedMin = Vec3(-1, 0, -1);
	effect->speedMax = Vec3(1, 1, 1);
	effect->posMin = Vec3(-0.75f, 0, -0.75f);
	effect->posMax = Vec3(0.75f, 1.f, 0.75f);
	effect->alpha = Vec2(0.5f, 0.f);
	effect->size = Vec2(0.3f, 0.f);
	particleEffects.push_back(effect);
	peSpawn = effect;

	effect = new ParticleEffect;
	effect->id = "raiseEffect";
	effect->tex = resMgr->Get<Texture>("czarna_iskra.png");
	effect->life = 0.f;
	effect->particleLife = 1.f;
	effect->emissionInterval = 0.f;
	effect->emissions = 1;
	effect->spawn = Int2(16, 25);
	effect->maxParticles = 25;
	effect->speedMin = Vec3(0, 4, 0);
	effect->speedMax = Vec3(0, 5, 0);
	effect->alpha = Vec2(1.f, 0.f);
	effect->size = Vec2(0.2f, 0.f);
	particleEffects.push_back(effect);

	effect = new ParticleEffect;
	effect->id = "healEffect";
	effect->tex = resMgr->Get<Texture>("heal_effect.png");
	effect->life = 0.f;
	effect->particleLife = 0.5f;
	effect->emissionInterval = 0.f;
	effect->emissions = 1;
	effect->spawn = Int2(16, 25);
	effect->maxParticles = 25;
	effect->speedMin = Vec3(-1.5f, -1.5f, -1.5f);
	effect->speedMax = Vec3(1.5f, 1.5f, 1.5f);
	effect->alpha = Vec2(0.9f, 0.f);
	effect->size = Vec2(0.1f, 0.f);
	effect->mode = 1;
	particleEffects.push_back(effect);

	effect = new ParticleEffect;
	effect->id = "fireTrap";
	effect->tex = resMgr->Get<Texture>("fire_spark.png");
	effect->life = 0.f;
	effect->particleLife = 0.5f;
	effect->emissionInterval = 0.f;
	effect->emissions = 1;
	effect->spawn = Int2(12);
	effect->maxParticles = 12;
	effect->speedMin = Vec3(-0.5f, 1.5f, -0.5f);
	effect->speedMax = Vec3(0.5f, 3.0f, 0.5f);
	effect->posMin = Vec3(-0.5f, 0, -0.5f);
	effect->posMax = Vec3(0.5f, 0, 0.5f);
	effect->alpha = Vec2(1.f, 0.f);
	effect->size = Vec2(0.0375f, 0.f);
	effect->mode = 1;
	particleEffects.push_back(effect);

	effect = new ParticleEffect(*effect);
	effect->id = "bearTrap";
	effect->tex = resMgr->Get<Texture>("dust.png");
	particleEffects.push_back(effect);

	effect = new ParticleEffect;
	effect->id = "spellBall";
	//effect->tex = ability.texParticle;
	effect->tex = nullptr;
	effect->life = 0.f;
	effect->particleLife = 0.5f;
	effect->emissionInterval = 0.f;
	effect->emissions = 1;
	effect->spawn = Int2(8, 12);
	effect->maxParticles = 12;
	effect->speedMin = Vec3(-1.5f, -1.5f, -1.5f);
	effect->speedMax = Vec3(1.5f, 1.5f, 1.5f);
	//effect->posMin = Vec3(-ability.size, -ability.size, -ability.size);
	//effect->posMax = Vec3(ability.size, ability.size, ability.size);
	effect->alpha = Vec2(1.f, 0.f);
	//effect->size = Vec2(ability.size / 2, 0.f);
	effect->mode = 1;
	particleEffects.push_back(effect);
	peSpellBall = effect;

	effect = new ParticleEffect;
	effect->id = "blood";
	//effect->tex = gameRes->tBlood[type];
	effect->tex = nullptr;
	effect->life = 5.f;
	effect->particleLife = 0.5f;
	effect->emissionInterval = 0.f;
	effect->emissions = 1;
	effect->spawn = Int2(10, 15);
	effect->maxParticles = 15;
	effect->speedMin = Vec3(-1, 0, -1);
	effect->speedMax = Vec3(1, 1, 1);
	effect->posMin = Vec3(-0.1f, -0.1f, -0.1f);
	effect->posMax = Vec3(0.1f, 0.1f, 0.1f);
	effect->alpha = Vec2(0.9f, 0.f);
	effect->size = Vec2(0.3f, 0.f);
	particleEffects.push_back(effect);
	peBlood = effect;
}

//=================================================================================================
Texture* GameResources::CreatePlaceholderTexture(const Int2& size)
{
	cstring name = Format("placeholder;%d;%d", size.x, size.y);
	Texture* tex = resMgr->TryGet<Texture>(name);
	if(tex)
		return tex;

	Buf buf;
	Color* texData = buf.Get<Color>(sizeof(Color) * size.x * size.y);
	const uint col[2] = { Color(255, 0, 255), Color(0, 255, 0) };
	for(int y = 0; y < size.y; ++y)
	{
		for(int x = 0; x < size.x; ++x)
			texData[x + y * size.x] = col[(x >= size.x / 2 ? 1 : 0) + (y >= size.y / 2 ? 1 : 0) % 2];
	}
	TEX t = render->CreateImmutableTexture(size, texData);

	tex = new Texture;
	tex->path = name;
	tex->filename = tex->path.c_str();
	tex->tex = t;
	tex->state = ResourceState::Loaded;
	resMgr->AddResource(tex);

	return tex;
}

//=================================================================================================
void GameResources::LoadLanguage()
{
	txLoadGuiTextures = Str("loadGuiTextures");
	txLoadTerrainTextures = Str("loadTerrainTextures");
	txLoadParticles = Str("loadParticles");
	txLoadModels = Str("loadModels");
	txLoadSounds = Str("loadSounds");
	txLoadMusic = Str("loadMusic");
}

//=================================================================================================
void GameResources::LoadData()
{
	// gui textures
	resMgr->AddTaskCategory(txLoadGuiTextures);
	tBlack = resMgr->Load<Texture>("czern.bmp");
	tWarning = resMgr->Load<Texture>("warning.png");
	tError = resMgr->Load<Texture>("error.png");
	gameGui->LoadData();
	net->LoadData();

	// particles
	resMgr->AddTaskCategory(txLoadParticles);
	tBlood[BLOOD_RED] = resMgr->Load<Texture>("krew.png");
	tBlood[BLOOD_GREEN] = resMgr->Load<Texture>("krew2.png");
	tBlood[BLOOD_BLACK] = resMgr->Load<Texture>("krew3.png");
	tBlood[BLOOD_BONE] = resMgr->Load<Texture>("iskra.png");
	tBlood[BLOOD_ROCK] = resMgr->Load<Texture>("kamien.png");
	tBlood[BLOOD_IRON] = resMgr->Load<Texture>("iskra.png");
	tBlood[BLOOD_SLIME] = resMgr->Load<Texture>("slime_part.png");
	tBlood[BLOOD_YELLOW] = resMgr->Load<Texture>("krew4.png");
	tBloodSplat[BLOOD_RED] = resMgr->Load<Texture>("krew_slad.png");
	tBloodSplat[BLOOD_GREEN] = resMgr->Load<Texture>("krew_slad2.png");
	tBloodSplat[BLOOD_BLACK] = resMgr->Load<Texture>("krew_slad3.png");
	tBloodSplat[BLOOD_BONE] = nullptr;
	tBloodSplat[BLOOD_ROCK] = nullptr;
	tBloodSplat[BLOOD_IRON] = nullptr;
	tBloodSplat[BLOOD_SLIME] = nullptr;
	tBloodSplat[BLOOD_YELLOW] = resMgr->Load<Texture>("krew_slad4.png");
	tLightingLine = resMgr->Load<Texture>("lighting_line.png");
	tVignette = resMgr->Load<Texture>("vignette.jpg");
	for(ParticleEffect* effect : particleEffects)
	{
		if(effect->tex)
			resMgr->Load(effect->tex);
	}

	// preload terrain textures
	resMgr->AddTaskCategory(txLoadTerrainTextures);
	tGrass = resMgr->Load<Texture>("trawa.jpg");
	tGrass2 = resMgr->Load<Texture>("Grass0157_5_S.jpg");
	tGrass3 = resMgr->Load<Texture>("LeavesDead0045_1_S.jpg");
	tRoad = resMgr->Load<Texture>("droga.jpg");
	tFootpath = resMgr->Load<Texture>("ziemia.jpg");
	tField = resMgr->Load<Texture>("pole.jpg");
	tFloorBase.diffuse = resMgr->Load<Texture>("droga.jpg");
	tFloorBase.normal = nullptr;
	tFloorBase.specular = nullptr;
	tWallBase.diffuse = resMgr->Load<Texture>("sciana.jpg");
	tWallBase.normal = resMgr->Load<Texture>("sciana_nrm.jpg");
	tWallBase.specular = resMgr->Load<Texture>("sciana_spec.jpg");
	tCeilBase.diffuse = resMgr->Load<Texture>("sufit.jpg");
	tCeilBase.normal = nullptr;
	tCeilBase.specular = nullptr;
	BaseLocation::PreloadTextures();
	tFloor[1] = tFloorBase;
	tCeil[1] = tCeilBase;
	tWall[1] = tWallBase;

	// meshes
	resMgr->AddTaskCategory(txLoadModels);
	resMgr->Load(aHuman);
	aHair[0] = resMgr->Load<Mesh>("hair1.qmsh");
	aHair[1] = resMgr->Load<Mesh>("hair2.qmsh");
	aHair[2] = resMgr->Load<Mesh>("hair3.qmsh");
	aHair[3] = resMgr->Load<Mesh>("hair4.qmsh");
	aHair[4] = resMgr->Load<Mesh>("hair5.qmsh");
	aEyebrows = resMgr->Load<Mesh>("eyebrows.qmsh");
	aMustache[0] = resMgr->Load<Mesh>("mustache1.qmsh");
	aMustache[1] = resMgr->Load<Mesh>("mustache2.qmsh");
	aBeard[0] = resMgr->Load<Mesh>("beard1.qmsh");
	aBeard[1] = resMgr->Load<Mesh>("beard2.qmsh");
	aBeard[2] = resMgr->Load<Mesh>("beard3.qmsh");
	aBeard[3] = resMgr->Load<Mesh>("beard4.qmsh");
	aBeard[4] = resMgr->Load<Mesh>("beardm1.qmsh");
	aArrow = resMgr->Load<Mesh>("arrow.qmsh");
	aSkybox = resMgr->Load<Mesh>("skybox.qmsh");
	aBag = resMgr->Load<Mesh>("worek.qmsh");
	aGrating = resMgr->Load<Mesh>("kratka.qmsh");
	aDoorWall = resMgr->Load<Mesh>("nadrzwi.qmsh");
	aDoorWall2 = resMgr->Load<Mesh>("nadrzwi2.qmsh");
	aStairsDown = resMgr->Load<Mesh>("schody_dol.qmsh");
	aStairsDown2 = resMgr->Load<Mesh>("schody_dol2.qmsh");
	aStairsUp = resMgr->Load<Mesh>("schody_gora.qmsh");
	aSpellball = resMgr->Load<Mesh>("spellball.qmsh");
	aDoor = resMgr->Load<Mesh>("drzwi.qmsh");
	aDoor2 = resMgr->Load<Mesh>("drzwi2.qmsh");
	aStun = resMgr->Load<Mesh>("stunned.qmsh");
	aPortal = resMgr->Load<Mesh>("dark_portal.qmsh");
	aDungeonDoor = resMgr->Load<Mesh>("dungeon_door.qmsh");
	mVine = resMgr->Load<Mesh>("vine.qmsh");

	PreloadBuildings();
	PreloadTraps();
	PreloadAbilities();
	PreloadObjects();
	PreloadItem(Item::Get("gold"));

	// physic meshes
	vdStairsUp = resMgr->Load<VertexData>("schody_gora.phy");
	vdStairsDown = resMgr->Load<VertexData>("schody_dol.phy");
	vdDoorHole = resMgr->Load<VertexData>("nadrzwi.phy");

	// sounds
	resMgr->AddTaskCategory(txLoadSounds);
	sGulp = resMgr->Load<Sound>("gulp.mp3");
	sCoins = resMgr->Load<Sound>("moneta2.mp3");
	sBow[0] = resMgr->Load<Sound>("bow1.mp3");
	sBow[1] = resMgr->Load<Sound>("bow2.mp3");
	sDoor[0] = resMgr->Load<Sound>("drzwi-02.mp3");
	sDoor[1] = resMgr->Load<Sound>("drzwi-03.mp3");
	sDoor[2] = resMgr->Load<Sound>("drzwi-04.mp3");
	sDoorClose = resMgr->Load<Sound>("104528__skyumori__door-close-sqeuak-02.mp3");
	sDoorClosed[0] = resMgr->Load<Sound>("wont_budge.mp3");
	sDoorClosed[1] = resMgr->Load<Sound>("wont_budge2.mp3");
	sItem[0] = resMgr->Load<Sound>("bottle.wav"); // potion
	sItem[1] = resMgr->Load<Sound>("armor-light.wav"); // light armor
	sItem[2] = resMgr->Load<Sound>("chainmail1.wav"); // heavy armor
	sItem[3] = resMgr->Load<Sound>("metal-ringing.wav"); // crystal
	sItem[4] = resMgr->Load<Sound>("wood-small.wav"); // bow
	sItem[5] = resMgr->Load<Sound>("cloth-heavy.wav"); // shield
	sItem[6] = resMgr->Load<Sound>("sword-unsheathe.wav"); // weapon
	sItem[7] = resMgr->Load<Sound>("interface3.wav"); // other
	sItem[8] = resMgr->Load<Sound>("amulet.mp3"); // amulet
	sItem[9] = resMgr->Load<Sound>("ring.mp3"); // ring
	sItem[10] = resMgr->Load<Sound>("splat.mp3"); // meat
	sChestOpen = resMgr->Load<Sound>("chest_open.mp3");
	sChestClose = resMgr->Load<Sound>("chest_close.mp3");
	sDoorBudge = resMgr->Load<Sound>("door_budge.mp3");
	sRock = resMgr->Load<Sound>("atak_kamien.mp3");
	sWood = resMgr->Load<Sound>("atak_drewno.mp3");
	sCrystal = resMgr->Load<Sound>("atak_krysztal.mp3");
	sMetal = resMgr->Load<Sound>("atak_metal.mp3");
	sBody[0] = resMgr->Load<Sound>("atak_cialo.mp3");
	sBody[1] = resMgr->Load<Sound>("atak_cialo2.mp3");
	sBody[2] = resMgr->Load<Sound>("atak_cialo3.mp3");
	sBody[3] = resMgr->Load<Sound>("atak_cialo4.mp3");
	sBody[4] = resMgr->Load<Sound>("atak_cialo5.mp3");
	sBone = resMgr->Load<Sound>("atak_kosci.mp3");
	sSkin = resMgr->Load<Sound>("atak_skora.mp3");
	sSlime = resMgr->Load<Sound>("slime_hit.wav");
	sArenaFight = resMgr->Load<Sound>("arena_fight.mp3");
	sArenaWin = resMgr->Load<Sound>("arena_wygrana.mp3");
	sArenaLost = resMgr->Load<Sound>("arena_porazka.mp3");
	sUnlock = resMgr->Load<Sound>("unlock.mp3");
	sEvil = resMgr->Load<Sound>("TouchofDeath.mp3");
	sEat = resMgr->Load<Sound>("eat.mp3");
	sSummon = resMgr->Load<Sound>("whooshy-puff.wav");
	sZap = resMgr->Load<Sound>("zap.mp3");
	sCancel = resMgr->Load<Sound>("cancel.mp3");
	sCoughs = resMgr->Load<Sound>("coughs.mp3");

	// music
	LoadMusic(MusicType::Title);
}

//=================================================================================================
void GameResources::PreloadBuildings()
{
	for(Building* b : Building::buildings)
	{
		if(b->mesh)
		{
			resMgr->LoadMeshMetadata(b->mesh);
			if(!b->mesh->FindPoint("o_s_point"))
				Warn("Missing building '%s' point.", b->id.c_str());
		}
		if(b->insideMesh)
			resMgr->LoadMeshMetadata(b->insideMesh);
	}
}

//=================================================================================================
void GameResources::PreloadTraps()
{
	for(uint i = 0; i < BaseTrap::nTraps; ++i)
	{
		BaseTrap& t = BaseTrap::traps[i];
		if(t.meshId)
		{
			t.mesh = resMgr->Get<Mesh>(t.meshId);
			resMgr->LoadMeshMetadata(t.mesh);

			Mesh::Point* pt = t.mesh->FindPoint("hitbox");
			assert(pt);
			if(pt->type == Mesh::Point::BOX)
			{
				t.rw = pt->size.x;
				t.h = pt->size.z;
			}
			else
				t.h = t.rw = pt->size.x;
		}
		if(t.soundId)
			t.sound = resMgr->Get<Sound>(t.soundId);
		if(t.soundId2)
			t.sound2 = resMgr->Get<Sound>(t.soundId2);
		if(t.soundId3)
			t.sound3 = resMgr->Get<Sound>(t.soundId3);
	}
}

//=================================================================================================
void GameResources::PreloadAbilities()
{
	for(Ability* ptrAbility : Ability::abilities)
	{
		Ability& ability = *ptrAbility;

		if(ability.soundCast)
			resMgr->Load(ability.soundCast);
		if(ability.soundHit)
			resMgr->Load(ability.soundHit);
		if(ability.tex)
			resMgr->Load(ability.tex);
		if(ability.texParticle)
			resMgr->Load(ability.texParticle);
		if(ability.texExplode.diffuse)
			resMgr->Load(ability.texExplode.diffuse);
		if(ability.texIcon)
			resMgr->Load(ability.texIcon);
		if(ability.mesh)
			resMgr->Load(ability.mesh);

		if(ability.type == Ability::Ball || ability.type == Ability::Point)
			ability.shape = new btSphereShape(ability.size);
	}
}

//=================================================================================================
void GameResources::PreloadObjects()
{
	for(BaseObject* pObj : BaseObject::items)
	{
		BaseObject& obj = *pObj;
		if(obj.mesh || obj.variants)
		{
			if(obj.mesh && !IsSet(obj.flags, OBJ_SCALEABLE | OBJ_NO_PHYSICS | OBJ_TMP_PHYSICS) && obj.type == OBJ_CYLINDER)
				obj.shape = new btCylinderShape(btVector3(obj.r, obj.h, obj.r));

			Mesh::Point* point;

			if(IsSet(obj.flags, OBJ_PRELOAD))
			{
				if(obj.variants)
				{
					for(Mesh* mesh : obj.variants->meshes)
						resMgr->Load(mesh);
				}
				else if(obj.mesh)
					resMgr->Load(obj.mesh);
			}

			if(obj.variants)
			{
				assert(!IsSet(obj.flags, OBJ_DOUBLE_PHYSICS | OBJ_MULTI_PHYSICS)); // not supported for variant mesh yet
				Mesh* mesh = obj.variants->meshes[0];
				resMgr->LoadMeshMetadata(mesh);
				point = mesh->FindPoint("hit");
			}
			else
			{
				resMgr->LoadMeshMetadata(obj.mesh);
				point = obj.mesh->FindPoint("hit");
			}

			if(!point || !point->IsBox() || IsSet(obj.flags, OBJ_BUILDING | OBJ_SCALEABLE) || obj.type == OBJ_CYLINDER)
			{
				obj.size = Vec2::Zero;
				obj.matrix = nullptr;
			}
			else
			{
				assert(point->size.x >= 0 && point->size.y >= 0 && point->size.z >= 0);
				obj.matrix = &point->mat;
				obj.size = point->size.XZ();
				obj.h = point->size.y;

				if(!IsSet(obj.flags, OBJ_NO_PHYSICS | OBJ_TMP_PHYSICS))
					obj.shape = new btBoxShape(ToVector3(point->size));

				if(IsSet(obj.flags, OBJ_PHY_ROT))
					obj.type = OBJ_HITBOX_ROT;

				if(IsSet(obj.flags, OBJ_MULTI_PHYSICS))
				{
					LocalVector<Mesh::Point*> points;
					Mesh::Point* prevPoint = point;

					while(true)
					{
						Mesh::Point* newPoint = obj.mesh->FindNextPoint("hit", prevPoint);
						if(newPoint)
						{
							assert(newPoint->IsBox() && newPoint->size.x >= 0 && newPoint->size.y >= 0 && newPoint->size.z >= 0);
							points.push_back(newPoint);
							prevPoint = newPoint;
						}
						else
							break;
					}

					assert(points.size() > 1u);
					obj.nextObj = new BaseObject[points.size() + 1];
					for(uint i = 0, size = points.size(); i < size; ++i)
					{
						BaseObject& o2 = obj.nextObj[i];
						o2.shape = new btBoxShape(ToVector3(points[i]->size));
						if(IsSet(obj.flags, OBJ_PHY_BLOCKS_CAM))
							o2.flags = OBJ_PHY_BLOCKS_CAM;
						o2.matrix = &points[i]->mat;
						o2.size = points[i]->size.XZ();
						o2.type = obj.type;
					}
					obj.nextObj[points.size()].shape = nullptr;
				}
				else if(IsSet(obj.flags, OBJ_DOUBLE_PHYSICS))
				{
					Mesh::Point* point2 = obj.mesh->FindNextPoint("hit", point);
					if(point2 && point2->IsBox())
					{
						assert(point2->size.x >= 0 && point2->size.y >= 0 && point2->size.z >= 0);
						obj.nextObj = new BaseObject;
						if(!IsSet(obj.flags, OBJ_NO_PHYSICS | OBJ_TMP_PHYSICS))
						{
							btBoxShape* shape = new btBoxShape(ToVector3(point2->size));
							obj.nextObj->shape = shape;
							if(IsSet(obj.flags, OBJ_PHY_BLOCKS_CAM))
								obj.nextObj->flags = OBJ_PHY_BLOCKS_CAM;
						}
						else
							obj.nextObj->shape = nullptr;
						obj.nextObj->matrix = &point2->mat;
						obj.nextObj->size = point2->size.XZ();
						obj.nextObj->type = obj.type;
					}
				}
			}
		}
	}
}

//=================================================================================================
void GameResources::PreloadItem(const Item* ptrItem)
{
	Item& item = *(Item*)ptrItem;
	if(item.state == ResourceState::Loaded)
		return;

	if(resMgr->IsLoadScreen())
	{
		if(item.state != ResourceState::Loading)
		{
			if(item.type == IT_ARMOR)
			{
				Armor& armor = item.ToArmor();
				if(!armor.texOverride.empty())
				{
					for(TexOverride& texOverride : armor.texOverride)
					{
						if(texOverride.diffuse)
							resMgr->Load(texOverride.diffuse);
					}
				}
			}
			else if(item.type == IT_BOOK)
			{
				Book& book = item.ToBook();
				resMgr->Load(book.scheme->tex);
			}

			if(item.mesh)
				resMgr->Load(item.mesh);
			else if(item.tex)
				resMgr->Load(item.tex);
			resMgr->AddTask(&item, TaskCallback(this, &GameResources::GenerateItemIconTask));

			item.state = ResourceState::Loading;
		}
	}
	else
	{
		// instant loading
		if(item.type == IT_ARMOR)
		{
			Armor& armor = item.ToArmor();
			if(!armor.texOverride.empty())
			{
				for(TexOverride& texOverride : armor.texOverride)
				{
					if(texOverride.diffuse)
						resMgr->Load(texOverride.diffuse);
				}
			}
		}
		else if(item.type == IT_BOOK)
		{
			Book& book = item.ToBook();
			resMgr->Load(book.scheme->tex);
		}

		if(item.mesh)
			resMgr->Load(item.mesh);
		else if(item.tex)
			resMgr->Load(item.tex);
		GenerateItemIcon(item);

		item.state = ResourceState::Loaded;
	}
}

//=================================================================================================
void GameResources::GenerateItemIconTask(TaskData& taskData)
{
	Item& item = *(Item*)taskData.ptr;
	GenerateItemIcon(item);
}

//=================================================================================================
void GameResources::GenerateItemIcon(Item& item)
{
	item.state = ResourceState::Loaded;

	// use missing texture if no mesh/texture
	if(!item.mesh && !item.tex)
	{
		item.icon = missingItemTexture;
		item.flags &= ~ITEM_GROUND_MESH;
		return;
	}

	// if item use image, set it as icon
	if(item.tex)
	{
		item.icon = item.tex;
		return;
	}

	// try to find icon using same mesh
	bool useTexOverride = false;
	if(item.type == IT_ARMOR)
		useTexOverride = !item.ToArmor().texOverride.empty();
	ItemTextureMap::iterator it;
	if(!useTexOverride)
	{
		it = itemTextureMap.lower_bound(item.mesh);
		if(it != itemTextureMap.end() && !(itemTextureMap.key_comp()(item.mesh, it->first)))
		{
			item.icon = it->second;
			return;
		}
	}
	else
		it = itemTextureMap.end();

	DrawItemIcon(item, rtItem, 0.f);
	Texture* tex = render->CopyToTexture(rtItem);

	item.icon = tex;
	if(it != itemTextureMap.end())
		itemTextureMap.insert(it, ItemTextureMap::value_type(item.mesh, tex));
	else
		overrideItemTextures.push_back(tex);
}

//=================================================================================================
void GameResources::DrawItemIcon(const Item& item, RenderTarget* target, float rot)
{
	// setup node
	Mesh& mesh = *item.mesh;
	mesh.EnsureIsLoaded();
	Mesh::Point* point = mesh.FindPoint("cam_rot");
	if(point)
		node->mat = Matrix::CreateFromAxisAngle(point->rot, rot);
	else
		node->mat = Matrix::RotationY(rot);
	node->SetMesh(&mesh);
	node->center = Vec3::Zero;
	node->texOverride = nullptr;
	if(item.type == IT_ARMOR)
	{
		if(const Armor& armor = item.ToArmor(); !armor.texOverride.empty())
		{
			node->texOverride = armor.GetTextureOverride();
			assert(armor.texOverride.size() == mesh.head.nSubs);
		}
	}

	// light
	Vec3& lightPos = scene->lights.back().pos;
	point = mesh.FindPoint("light");
	if(point)
		lightPos = Vec3::TransformZero(point->mat);
	else
		lightPos = mesh.head.camPos;

	// setup camera
	camera->from = mesh.head.camPos;
	camera->to = mesh.head.camTarget;
	camera->up = mesh.head.camUp;
	camera->UpdateMatrix();

	// draw
	sceneMgr->SetScene(scene, camera);
	sceneMgr->Draw(target);
}

//=================================================================================================
Sound* GameResources::GetMaterialSound(MATERIAL_TYPE attackMat, MATERIAL_TYPE hitMat)
{
	switch(hitMat)
	{
	case MAT_BODY:
		return sBody[Rand() % 5];
	case MAT_BONE:
		return sBone;
	case MAT_CLOTH:
	case MAT_SKIN:
		return sSkin;
	case MAT_IRON:
		return sMetal;
	case MAT_WOOD:
		return sWood;
	case MAT_ROCK:
		return sRock;
	case MAT_CRYSTAL:
		return sCrystal;
	case MAT_SLIME:
		return sSlime;
	default:
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
Sound* GameResources::GetItemSound(const Item* item)
{
	assert(item);

	switch(item->type)
	{
	case IT_WEAPON:
		return sItem[6];
	case IT_BOW:
		return sItem[4];
	case IT_SHIELD:
		return sItem[5];
	case IT_ARMOR:
		if(item->ToArmor().armorType != AT_LIGHT)
			return sItem[2];
		else
			return sItem[1];
	case IT_AMULET:
		return sItem[8];
	case IT_RING:
		return sItem[9];
	case IT_CONSUMABLE:
		if(Any(item->ToConsumable().subtype, Consumable::Subtype::Food, Consumable::Subtype::Herb))
			return sItem[7];
		else
			return sItem[0];
	case IT_OTHER:
		if(IsSet(item->flags, ITEM_CRYSTAL_SOUND))
			return sItem[3];
		else if(IsSet(item->flags, ITEM_MEAT_SOUND))
			return sItem[10];
		else
			return sItem[7];
	case IT_GOLD:
		return sCoins;
	default:
		return sItem[7];
	}
}

//=================================================================================================
Mesh* GameResources::GetEntryMesh(EntryType type)
{
	switch(type)
	{
	default:
		assert(0);
	case ENTRY_STAIRS_UP:
		return aStairsUp;
	case ENTRY_STAIRS_DOWN:
		return aStairsDown;
	case ENTRY_STAIRS_DOWN_IN_WALL:
		return aStairsDown2;
	case ENTRY_DOOR:
		return aDungeonDoor;
	}
}

//=================================================================================================
void GameResources::LoadMusic(MusicType type, bool newLoadScreen, bool instant)
{
	if(soundMgr->IsDisabled())
		return;

	MusicList* list = musicLists[(int)type];
	if(list->IsLoaded())
		return;

	if(newLoadScreen)
		resMgr->AddTaskCategory(txLoadMusic);

	for(Music* music : list->musics)
	{
		if(instant)
			resMgr->LoadInstant(music);
		else
			resMgr->Load(music);
	}
}

//=================================================================================================
void GameResources::LoadCommonMusic()
{
	LoadMusic(MusicType::Boss, false);
	LoadMusic(MusicType::Death, false);
	LoadMusic(MusicType::Travel, false);
}

//=================================================================================================
void GameResources::LoadTrap(BaseTrap* trap)
{
	assert(trap);
	if(trap->state != ResourceState::NotLoaded)
		return;

	if(trap->mesh)
		resMgr->Load(trap->mesh);
	if(trap->sound)
		resMgr->Load(trap->sound);
	if(trap->sound2)
		resMgr->Load(trap->sound2);
	if(trap->sound3)
		resMgr->Load(trap->sound3);

	trap->state = ResourceState::Loaded;
}

//=================================================================================================
ParticleEffect* GameResources::GetParticleEffect(const string& id)
{
	for(ParticleEffect* effect : particleEffects)
	{
		if(effect->id == id)
			return effect;
	}
	return nullptr;
}
