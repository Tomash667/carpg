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
GameResources::GameResources() : scene(nullptr), node(nullptr), camera(nullptr), peHit(nullptr), peSpellHit(nullptr), peElectroHit(nullptr), peTorch(nullptr),
peMagicTorch(nullptr), peCampfire(nullptr), peAltarBlood(nullptr), peWater(nullptr), peMagicfire(nullptr), peSmoke(nullptr), peSpawn(nullptr),
peRaise(nullptr), peHeal(nullptr), peSpellOther(nullptr), peSpellBall(nullptr), peBlood(nullptr)
{
}

//=================================================================================================
GameResources::~GameResources()
{
	delete scene;
	delete camera;
	for(auto& item : itemTextureMap)
		delete item.second;
	for(Texture* tex : overrideItemTextures)
		delete tex;
	delete peHit;
	delete peSpellHit;
	delete peElectroHit;
	delete peTorch;
	delete peMagicTorch;
	delete peCampfire;
	delete peAltarBlood;
	delete peWater;
	delete peMagicfire;
	delete peSmoke;
	delete peSpawn;
	delete peRaise;
	delete peHeal;
	delete peSpellOther;
	delete peSpellBall;
	delete peBlood;
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

	aHuman = resMgr->Load<Mesh>("human.qmsh");
	rtItem = render->CreateRenderTarget(Int2(ITEM_IMAGE_SIZE, ITEM_IMAGE_SIZE), RenderTarget::F_NO_DRAW);
	missingItemTexture = CreatePlaceholderTexture(Int2(ITEM_IMAGE_SIZE, ITEM_IMAGE_SIZE));

	InitEffects();
}

//=================================================================================================
void GameResources::InitEffects()
{
	peHit = new ParticleEffect;
	peHit->tex = resMgr->Get<Texture>("iskra.png");
	peHit->life = 5.f;
	peHit->particleLife = 0.5f;
	peHit->emissionInterval = 0.f;
	peHit->emissions = 1;
	peHit->spawn = Int2(10, 15);
	peHit->maxParticles = 15;
	peHit->speedMin = Vec3(-1, 0, -1);
	peHit->speedMax = Vec3(1, 1, 1);
	peHit->posMin = Vec3(-0.1f, -0.1f, -0.1f);
	peHit->posMax = Vec3(0.1f, 0.1f, 0.1f);
	peHit->alpha = Vec2(0.9f, 0.f);
	peHit->size = Vec2(0.3f, 0.f);

	peSpellHit = new ParticleEffect;
	//peSpellHit->tex = ability->texParticle;
	peSpellHit->life = -1;
	peSpellHit->particleLife = 0.5f;
	peSpellHit->emissionInterval = 0.1f;
	peSpellHit->emissions = -1;
	peSpellHit->spawn = Int2(3, 4);
	peSpellHit->maxParticles = 50;
	peSpellHit->speedMin = Vec3(-1, -1, -1);
	peSpellHit->speedMax = Vec3(1, 1, 1);
	//peSpellHit->posMin = Vec3(-ability->size, -ability->size, -ability->size);
	//peSpellHit->posMax = Vec3(ability->size, ability->size, ability->size);
	peSpellHit->alpha = Vec2(1.f, 0.f);
	//peSpellHit->size = Vec2(ability->sizeParticle, 0.f);
	peSpellHit->mode = 1;

	peElectroHit = new ParticleEffect;
	//peElectroHit->tex = ability->texParticle;
	peElectroHit->life = 0.f;
	peElectroHit->particleLife = 0.5f;
	peElectroHit->emissionInterval = 0.f;
	peElectroHit->emissions = 1;
	peElectroHit->spawn = Int2(8, 12);
	peElectroHit->maxParticles = 12;
	peElectroHit->speedMin = Vec3(-1.5f, -1.5f, -1.5f);
	peElectroHit->speedMax = Vec3(1.5f, 1.5f, 1.5f);
	//peElectroHit->posMin = Vec3(-ability->size, -ability->size, -ability->size);
	//peElectroHit->posMax = Vec3(ability->size, ability->size, ability->size);
	peElectroHit->alpha = Vec2(1.f, 0.f);
	//peElectroHit->size = Vec2(ability->sizeParticle, 0.f);
	peElectroHit->mode = 1;

	peTorch = new ParticleEffect;
	peTorch->tex = resMgr->Get<Texture>("flare.png");
	peTorch->life = -1;
	peTorch->particleLife = 0.5f;
	peTorch->emissionInterval = 0.1f;
	peTorch->emissions = -1;
	peTorch->spawn = Int2(1, 3);
	peTorch->maxParticles = 50;
	peTorch->posMin = Vec3(0, 0, 0);
	peTorch->posMax = Vec3(0, 0, 0);
	peTorch->speedMin = Vec3(-1, 3, -1);
	peTorch->speedMax = Vec3(1, 4, 1);
	peTorch->alpha = Vec2(0.8f, 0.f);
	peTorch->size = Vec2(0.5f, 0.f);
	peTorch->mode = 1;

	peMagicTorch = new ParticleEffect(*peTorch);
	peMagicTorch->tex = resMgr->Get<Texture>("flare2.png");

	peCampfire = new ParticleEffect(*peTorch);
	peCampfire->size = Vec2(0.7f, 0.f);

	peAltarBlood = new ParticleEffect;
	peAltarBlood->tex = resMgr->Get<Texture>("krew.png");
	peAltarBlood->life = -1;
	peAltarBlood->particleLife = 0.5f;
	peAltarBlood->emissionInterval = 0.1f;
	peAltarBlood->emissions = -1;
	peAltarBlood->spawn = Int2(1, 3);
	peAltarBlood->maxParticles = 50;
	peAltarBlood->posMin = Vec3(0, 0, 0);
	peAltarBlood->posMax = Vec3(0, 0, 0);
	peAltarBlood->speedMin = Vec3(-1, 4, -1);
	peAltarBlood->speedMax = Vec3(1, 6, 1);
	peAltarBlood->alpha = Vec2(0.8f, 0.f);
	peAltarBlood->size = Vec2(0.5f, 0.f);

	peWater = new ParticleEffect;
	peWater->tex = resMgr->Get<Texture>("water.png");
	peWater->life = -1;
	peWater->particleLife = 3.f;
	peWater->emissionInterval = 0.1f;
	peWater->emissions = -1;
	peWater->spawn = Int2(4, 8);
	peWater->maxParticles = 500;
	peWater->posMin = Vec3(0, 0, 0);
	peWater->posMax = Vec3(0, 0, 0);
	peWater->speedMin = Vec3(-0.6f, 4, -0.6f);
	peWater->speedMax = Vec3(0.6f, 7, 0.6f);
	peWater->alpha = Vec2(0.8f, 0.f);
	peWater->size = Vec2(0.05f, 0.f);

	peMagicfire = new ParticleEffect;
	peMagicfire->tex = resMgr->Get<Texture>("flare2.png");
	peMagicfire->life = -1;
	peMagicfire->particleLife = 0.5f;
	peMagicfire->emissionInterval = 0.1f;
	peMagicfire->emissions = -1;
	peMagicfire->spawn = Int2(2, 4);
	peMagicfire->maxParticles = 50;
	peMagicfire->posMin = Vec3(0, 0, 0);
	peMagicfire->posMax = Vec3(0, 0, 0);
	peMagicfire->speedMin = Vec3(-1, 3, -1);
	peMagicfire->speedMax = Vec3(1, 4, 1);
	peMagicfire->alpha = Vec2(1.0f, 0.f);
	peMagicfire->size = Vec2(1.0f, 0.f);
	peMagicfire->mode = 1;

	peSmoke = new ParticleEffect;
	peSmoke->tex = resMgr->Get<Texture>("smoke.png");
	peSmoke->life = -1;
	peSmoke->particleLife = 2.5f;
	peSmoke->emissionInterval = 0.1f;
	peSmoke->emissions = -1;
	peSmoke->spawn = Int2(2, 4);
	peSmoke->maxParticles = 100;
	peSmoke->posMin = Vec3(-0.5f, 0, -0.5f);
	peSmoke->posMax = Vec3(0.5f, 0, 0.5f);
	peSmoke->speedMin = Vec3(-0.5f, 3, -0.5f);
	peSmoke->speedMax = Vec3(0.5f, 4, 0.5f);
	peSmoke->alpha = Vec2(0.25f, 0.f);
	peSmoke->size = Vec2(1.0f, 5.f);
	peSmoke->gravity = false;

	peSpawn = new ParticleEffect;
	peSpawn->tex = resMgr->Get<Texture>("spawn_fog.png");
	peSpawn->life = 5.f;
	peSpawn->particleLife = 0.5f;
	peSpawn->emissionInterval = 0.1f;
	peSpawn->emissions = 5;
	peSpawn->spawn = Int2(10, 15);
	peSpawn->maxParticles = 15 * 5;
	peSpawn->speedMin = Vec3(-1, 0, -1);
	peSpawn->speedMax = Vec3(1, 1, 1);
	peSpawn->posMin = Vec3(-0.75f, 0, -0.75f);
	peSpawn->posMax = Vec3(0.75f, 1.f, 0.75f);
	peSpawn->alpha = Vec2(0.5f, 0.f);
	peSpawn->size = Vec2(0.3f, 0.f);

	peRaise = new ParticleEffect;
	//peRaise->tex = ability->texParticle;
	peRaise->life = 0.f;
	peRaise->particleLife = 1.f;
	peRaise->emissionInterval = 0.f;
	peRaise->emissions = 1;
	peRaise->spawn = Int2(16, 25);
	peRaise->maxParticles = 25;
	peRaise->speedMin = Vec3(0, 4, 0);
	peRaise->speedMax = Vec3(0, 5, 0);
	//peRaise->posMin = Vec3(-bounds.x, -bounds.y / 2, -bounds.x);
	//peRaise->posMax = Vec3(bounds.x, bounds.y / 2, bounds.x);
	peRaise->alpha = Vec2(1.f, 0.f);
	//peRaise->size = Vec2(ability->sizeParticle, 0.f);

	peHeal = new ParticleEffect;
	//peHeal->tex = ability->texParticle;
	peHeal->life = 0.f;
	peHeal->particleLife = 0.5f;
	peHeal->emissionInterval = 0.f;
	peHeal->emissions = 1;
	peHeal->spawn = Int2(16, 25);
	peHeal->maxParticles = 25;
	peHeal->speedMin = Vec3(-1.5f, -1.5f, -1.5f);
	peHeal->speedMax = Vec3(1.5f, 1.5f, 1.5f);
	//peHeal->posMin = Vec3(-bounds.x, -bounds.y / 2, -bounds.x);
	//peHeal->posMax = Vec3(bounds.x, bounds.y / 2, bounds.x);
	peHeal->alpha = Vec2(0.9f, 0.f);
	//peHeal->size = Vec2(ability->sizeParticle, 0.f);
	peHeal->mode = 1;

	peSpellOther = new ParticleEffect;
	//peSpellOther->tex = ability->texParticle;
	peSpellOther->life = 0.f;
	peSpellOther->particleLife = 0.5f;
	peSpellOther->emissionInterval = 0.f;
	peSpellOther->emissions = 1;
	peSpellOther->spawn = Int2(12);
	peSpellOther->maxParticles = 12;
	peSpellOther->speedMin = Vec3(-0.5f, 1.5f, -0.5f);
	peSpellOther->speedMax = Vec3(0.5f, 3.0f, 0.5f);
	peSpellOther->posMin = Vec3(-0.5f, 0, -0.5f);
	peSpellOther->posMax = Vec3(0.5f, 0, 0.5f);
	peSpellOther->alpha = Vec2(1.f, 0.f);
	//peSpellOther->size = Vec2(ability->sizeParticle / 2, 0.f);
	peSpellOther->mode = 1;

	peSpellBall = new ParticleEffect;
	//peSpellBall->tex = ability.texParticle;
	peSpellBall->life = 0.f;
	peSpellBall->particleLife = 0.5f;
	peSpellBall->emissionInterval = 0.f;
	peSpellBall->emissions = 1;
	peSpellBall->spawn = Int2(8, 12);
	peSpellBall->maxParticles = 12;
	peSpellBall->speedMin = Vec3(-1.5f, -1.5f, -1.5f);
	peSpellBall->speedMax = Vec3(1.5f, 1.5f, 1.5f);
	//peSpellBall->posMin = Vec3(-ability.size, -ability.size, -ability.size);
	//peSpellBall->posMax = Vec3(ability.size, ability.size, ability.size);
	peSpellBall->alpha = Vec2(1.f, 0.f);
	//peSpellBall->size = Vec2(ability.size / 2, 0.f);
	peSpellBall->mode = 1;

	peBlood = new ParticleEffect;
	//peBlood->tex = gameRes->tBlood[type];
	peBlood->life = 5.f;
	peBlood->particleLife = 0.5f;
	peBlood->emissionInterval = 0.f;
	peBlood->emissions = 1;
	peBlood->spawn = Int2(10, 15);
	peBlood->maxParticles = 15;
	peBlood->speedMin = Vec3(-1, 0, -1);
	peBlood->speedMax = Vec3(1, 1, 1);
	peBlood->posMin = Vec3(-0.1f, -0.1f, -0.1f);
	peBlood->posMax = Vec3(0.1f, 0.1f, 0.1f);
	peBlood->alpha = Vec2(0.9f, 0.f);
	peBlood->size = Vec2(0.3f, 0.f);
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
	resMgr->Load(peHit->tex);
	resMgr->Load(peTorch->tex);
	resMgr->Load(peMagicTorch->tex);
	resMgr->Load(peAltarBlood->tex);
	resMgr->Load(peWater->tex);
	resMgr->Load(peSmoke->tex);
	resMgr->Load(peSpawn->tex);

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
	for(pair<const int, BaseObject*>& p : BaseObject::items)
	{
		BaseObject& obj = *p.second;
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
