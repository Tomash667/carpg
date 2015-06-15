// przedmiot
#include "Pch.h"
#include "Base.h"
#include "Item.h"

//-----------------------------------------------------------------------------
WeaponTypeInfo weapon_type_info[] = {
	NULL, 0.5f, 0.5f, 0.4f, 1.1f, 0.002f, Skill::SHORT_BLADE, // WT_SHORT
	NULL, 0.75f, 0.25f, 0.33f, 1.f, 0.0015f, Skill::LONG_BLADE, // WT_LONG
	NULL, 0.85f, 0.15f, 0.29f, 0.9f, 0.00075f, Skill::BLUNT, // WT_MACE
	NULL, 0.8f, 0.2f, 0.31f, 0.95f, 0.001f, Skill::AXE // WT_AXE
};

//=================================================================================================
Weapon g_weapons[] = {
	//		ID						WEIGHT	PRICE	MESH						DMG	SI£A	TYPE		MAT				DMG_TYPE		FLAGS	LEVEL
	Weapon("dagger_short",			6,		2,		"sztylet.qmsh",				10,	20,		WT_SHORT,	MAT_IRON,		DMG_PIERCE,		0, 1),
	Weapon("dagger_sword",			9,		10,		"kmiecz.qmsh",				15,	25,		WT_SHORT,	MAT_IRON,		DMG_PIERCE,		0, 3),
	Weapon("dagger_rapier",			9,		20,		"rapier.qmsh",				25,	22,		WT_SHORT,	MAT_IRON,		DMG_PIERCE,		0, 5),
	Weapon("dagger_assassin",		6,		1000,	"sztylet_zabojcy.qmsh",		40,	20,		WT_SHORT,	MAT_IRON,		DMG_PIERCE,		0, 10),
	Weapon("dagger_sword_a",		10,		5000,	"adam_kmiecz.qmsh",			60,	26,		WT_SHORT,	MAT_IRON,		DMG_PIERCE,		0, 15),
	Weapon("dagger_spinesheath",	5,		25000,	"grzbietokluj.qmsh",		80,	20,		WT_SHORT,	MAT_IRON,		DMG_PIERCE,		ITEM_NOT_CHEST|ITEM_NOT_SHOP|ITEM_BACKSTAB|ITEM_NOT_RANDOM, 20),

	Weapon("sword_long",			18,		15,		"miecz.qmsh",				20,	40,		WT_LONG,	MAT_IRON,		DMG_SLASH,		0, 1),
	Weapon("sword_scimitar",		18,		15,		"sejmitar.qmsh",			30,	45,		WT_LONG,	MAT_IRON,		DMG_SLASH,		0, 3), // sejmitar
	Weapon("sword_orcish",			22,		25,		"orkowy_miecz.qmsh",		40,	55,		WT_LONG,	MAT_IRON,		DMG_SLASH,		0, 5), // orkowy miecz
	Weapon("sword_serrated",		20,		1200,	"zabkowany_miecz.qmsh",		55,	45,		WT_LONG,	MAT_IRON,		DMG_SLASH,		0, 10), // z¹bkowany miecz
	Weapon("sword_adamantine",		20,		10000,	"adam_miecz.qmsh",			75,	50,		WT_LONG,	MAT_IRON,		DMG_SLASH,		0, 15), // czerwonawy d³ugi miecz
	Weapon("sword_unique",			18,		30000,	"semur.qmsh",				100, 45,	WT_LONG,	MAT_IRON,		DMG_SLASH,		ITEM_NOT_CHEST|ITEM_NOT_SHOP|ITEM_MAGE|ITEM_POWER_4|ITEM_NOT_RANDOM, 20), // z³oty sejmitar

	Weapon("axe_small",				13,		6,		"toporek.qmsh",				20,	45,		WT_AXE,		MAT_IRON,		DMG_SLASH,		0, 1), // toporek
	Weapon("axe_battle",			27,		10,		"topor_bojowy.qmsh",		30,	55,		WT_AXE,		MAT_IRON,		DMG_SLASH,		0, 3), // topór
	Weapon("axe_orcish",			30,		25,		"topor_orkowy.qmsh",		35,	75,		WT_AXE,		MAT_IRON,		DMG_SLASH,		0, 5), // prymitywny topór
	Weapon("axe_crystal",			35,		2000,	"topor_krysztalowy.qmsh",	55,	70,		WT_AXE,		MAT_CRYSTAL,	DMG_SLASH,		0, 10), // kryszta³owy prymitywny topór
	Weapon("axe_giant",				35,		8000,	"topor_giganta.qmsh",		85, 85,		WT_AXE,		MAT_IRON,		DMG_SLASH,		0, 15), // podwójny topór
	Weapon("axe_ripper",			35,		40000,	"rozpruwacz.qmsh",			110, 80,	WT_AXE,		MAT_IRON,		DMG_SLASH|DMG_PIERCE,	ITEM_NOT_CHEST|ITEM_NOT_SHOP|ITEM_NOT_RANDOM, 20), // krwawy podwójny topór

	Weapon("blunt_club",			13,		1,		"maczuga.qmsh",				10,	45,		WT_MACE,	MAT_WOOD,		DMG_BLUNT,		0, 1), // maczuga
	Weapon("blunt_mace",			36,		12,		"buzdygan.qmsh",			35,	65,		WT_MACE,	MAT_IRON,		DMG_BLUNT,		0, 3), // buzdygan
	Weapon("blunt_orcish",			40,		6,		"orkowy_mlot.qmsh",			40, 70,		WT_MACE,	MAT_ROCK,		DMG_BLUNT,		0, 4), // prymitywny m³ot
	Weapon("blunt_orcish_shaman",	40,		9,		"ozdobny_orkowy_mlot.qmsh",	20, 50,		WT_MACE,	MAT_ROCK,		DMG_BLUNT,		ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_MAGE, 4), // ozdobny prymitywny m³ot
	Weapon("blunt_morningstar",		27,		8,		"morgensztern.qmsh",		40,	70,		WT_MACE,	MAT_IRON,		DMG_BLUNT|DMG_PIERCE,	0, 5), // morgensztern
	Weapon("blunt_dwarven",			44,		500,	"krasnoludzki_mlot.qmsh",	60,	85,		WT_MACE,	MAT_IRON,		DMG_BLUNT,		0, 10), // du¿y m³ot
	Weapon("blunt_adamantine",		40,		5000,	"adam_buzdygan.qmsh",		90, 100,	WT_MACE,	MAT_IRON,		DMG_BLUNT,		0, 15), // czerwonawy buzdygan
	Weapon("blunt_skullsmasher",	50,		25000,	"rozlupywacz.qmsh",			120, 100,	WT_MACE,	MAT_IRON,		DMG_BLUNT|DMG_PIERCE, ITEM_NOT_CHEST|ITEM_NOT_SHOP|ITEM_NOT_RANDOM, 20), // czarny morgensztern

	Weapon("blunt_blacksmith",		30,		15,		"mlot_kowala.qmsh",			30, 50,		WT_MACE,	MAT_IRON,		DMG_BLUNT,		0, 2), // m³ot
	Weapon("pickaxe",				25,		10,		"kilof.qmsh",				25, 55,		WT_MACE,	MAT_IRON,		DMG_BLUNT,		0, 2),

	Weapon("wand_1",				6,		75,		"rozdzka1.qmsh",			5,	15,		WT_MACE,	MAT_WOOD,		DMG_BLUNT,		ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_MAGE|ITEM_POWER_1, 1),
	Weapon("wand_2",				6,		150,	"rozdzka2.qmsh",			10,	15,		WT_MACE,	MAT_WOOD,		DMG_BLUNT,		ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_MAGE|ITEM_POWER_2, 1),
	Weapon("wand_3",				6,		225,	"rozdzka3.qmsh",			15,	15,		WT_MACE,	MAT_WOOD,		DMG_BLUNT,		ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_MAGE|ITEM_POWER_3, 1),

	Weapon("sword_forbidden",		20,		500000,	"sny.qmsh",					125, 45,	WT_LONG,	MAT_CRYSTAL,	DMG_SLASH|DMG_BLUNT|DMG_PIERCE,	ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_SECRET|ITEM_POWER_2|ITEM_NOT_RANDOM, 25)
};
const uint n_weapons = countof(g_weapons);

//=================================================================================================
Bow g_bows[] = {
	//	ID					WEIGHT	PRICE	MESH					DMG	SI£ FLAGS LVL
	Bow("bow_short",		9,		30,		"luk_krotki.qmsh",		30,	25, 0, 1),
	Bow("bow_long",			13,		75,		"luk_dlugi.qmsh",		40,	35, 0, 3),
	Bow("bow_composite",	13,		250,	"luk_kompozytowy.qmsh",	50,	40, 0, 5),
	Bow("bow_elven",		11,		3000,	"luk_elfi.qmsh",		70,	35, 0, 10),
	Bow("bow_dragonbone",	15,		10000,	"luk_smoczy.qmsh",		90,	50, 0, 15),
	Bow("bow_unique",		9,		30000,	"luk_anielski.qmsh",	120, 25, ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_POWER_1|ITEM_NOT_RANDOM, 20),
	Bow("q_gobliny_luk",	13,		120,	"luk_stary.qmsh",		40,	45, ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_DONT_DROP|ITEM_IMPORTANT|ITEM_NOT_RANDOM, 3)
};
const uint n_bows = countof(g_bows);

//=================================================================================================
Shield g_shields[] = {
	//		ID					WEIGHT	PRICE	MESH						DEF	MAT			SI£
	Shield("shield_wood",		22,		5,		"drewniana_tarcza.qmsh",	10,	MAT_WOOD,	25, 0, 1), // okr¹g³a drewniana tarcza
	Shield("shield_iron",		27,		20,		"zelazna_tarcza.qmsh",		20,	MAT_IRON,	35, 0, 3), // okr¹g³a metalowa tarcza
	Shield("shield_steel",		27,		200,	"stalowa_tarcza.qmsh",		30,	MAT_IRON,	40,  0, 5), // U metalowa tarcza
	Shield("shield_mithril",	13,		3000,	"tarcza_mit.qmsh",			50,	MAT_IRON,	20, 0, 10), // okr¹g³a bia³o metalowa tarcza
	Shield("shield_adamantine",	30,		10000,	"tarcza_adam.qmsh",			80,	MAT_IRON,	45, ITEM_MAGIC_RESISTANCE_10, 15), // U czerwonawa metalowa tarcza
	Shield("shield_unique",		35,		30000,	"tarcza_mur.qmsh",			100, MAT_ROCK,  60, ITEM_NOT_CHEST|ITEM_NOT_SHOP|ITEM_MAGIC_RESISTANCE_25|ITEM_NOT_RANDOM, 20) // prostok¹tna kamienna tarcza
};
const uint n_shields = countof(g_shields);

//=================================================================================================
Armor g_armors[] = {
	//		ID						WEIGHT	PRICE	MESH						SKILL					TYPE					MAT				DEF	SI£	ZRÊ
	Armor("armor_leather",			70,		10,		"skorznia.qmsh",			Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_SKIN,		10,	30,	60, 0, 1), // skórznia
	Armor("armor_studded",			90,		25,		"cwiekowana.qmsh",			Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_SKIN,		15,	35,	55,  0, 3), // skórznia z æwiekami
	Armor("armor_chain_shirt",		115,	100,	"koszulka_kolcza.qmsh",		Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		25,	40,	50, 0, 5), // jak kolczuga tylko mniejsze oczka i skórzane elemnty
	Armor("armor_mithril_shirt",	55,		3000,	"mithrilowa_koszulka.qmsh",	Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		40,	25,	75, 0, 10), // bia³awa koszulka kolcza
	Armor("armor_dragonskin",		80,		8000,	"smocza_skora.qmsh",		Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_SKIN,		60,	30,	60, ITEM_MAGIC_RESISTANCE_10, 15), // czerwonawa skórznia
	Armor("armor_unique2",			30,		20000,	"anielska_skora.qmsh",		Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		80,	20,	100, ITEM_NOT_CHEST|ITEM_NOT_SHOP|ITEM_POWER_1|ITEM_MAGIC_RESISTANCE_25|ITEM_NOT_RANDOM, 20), // niebieskawa koszulka kolcza

	Armor("armor_chainmail",		180,	50,		"kolczuga.qmsh",			Skill::HEAVY_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		20,	50,	40, 0, 1), // kolczuga
	Armor("armor_breastplate",		135,	200,	"napiersnik.qmsh",			Skill::HEAVY_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		30,	45,	45, 0, 3), // napierœnik
	Armor("armor_plate",			225,	1500,	"plytowka.qmsh",			Skill::HEAVY_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		50,	60,	30, 0, 5), // zbroja p³ytowa
	Armor("armor_crystal",			280,	6000,	"krysztalowa.qmsh",			Skill::HEAVY_ARMOR,		ArmorUnitType::HUMAN,	MAT_CRYSTAL,	65,	80,	15, 0, 10), // krysztalowy toporny napierœnik
	Armor("armor_adamantine",		230,	15000,	"ada_plytowka.qmsh",		Skill::HEAVY_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		80,	65,	25, ITEM_MAGIC_RESISTANCE_10, 15), // czerwonawa zbroja p³ytowa
	Armor("armor_unique",			250,	30000,	"czarna_zbroja.qmsh",		Skill::HEAVY_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		100, 75, 25, ITEM_NOT_CHEST|ITEM_NOT_SHOP|ITEM_MAGIC_RESISTANCE_25|ITEM_NOT_RANDOM, 20), // czarna zbroja p³ytowa

	Armor("armor_blacksmith",		20,		50,		"ubranie_kowala.qmsh",		Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_SKIN,		10,	30,	60, ITEM_NOT_BLACKSMITH, 1), // jak skórznia ale z fartuchem i narzêdziami
	Armor("armor_innkeeper",		15,		50,		"ubr_karczmarza.qmsh",		Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_CLOTH,		5,	20,	60, ITEM_NOT_BLACKSMITH, 1),
	Armor("armor_goblin",			40,		5,		"goblinski_pancerz.qmsh",	Skill::LIGHT_ARMOR,		ArmorUnitType::GOBLIN,	MAT_SKIN,		5,	20,	75, ITEM_NOT_SHOP|ITEM_NOT_CHEST, 1), // zielonkawa brunatna skórznia
	Armor("armor_orcish_leather",	90,		5,		"orkowa_zbroja.qmsh",		Skill::LIGHT_ARMOR,		ArmorUnitType::ORC,		MAT_SKIN,		15,	40,	55, ITEM_NOT_SHOP|ITEM_NOT_CHEST, 1), // skórznia z kawa³kami metalu
	Armor("armor_orcish_chainmail",	220,	30,		"orkowa_zbroja2.qmsh",		Skill::HEAVY_ARMOR,		ArmorUnitType::ORC,		MAT_IRON,		25,	60,	35, ITEM_NOT_SHOP|ITEM_NOT_CHEST, 1), // kolczuga
	Armor("armor_orcish_shaman",	90,		10,		"orkowa_zbroja_o.qmsh",		Skill::LIGHT_ARMOR,		ArmorUnitType::ORC,		MAT_SKIN,		15,	40,	55, ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_MAGE|ITEM_POWER_1, 1), // ozdobna skórznia
	Armor("armor_mage_1",			15,		30,		"toga.qmsh",				Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_CLOTH,		5,	20,	70, ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_MAGE|ITEM_POWER_1, 1), // br¹zowa szata
	Armor("armor_mage_2",			15,		100,	"toga2.qmsh",				Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_CLOTH,		10,	20,	70, ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_MAGE|ITEM_POWER_2|ITEM_MAGIC_RESISTANCE_10, 2), // jw ale czerwone fragmenty
	Armor("armor_mage_3",			15,		350,	"toga3.qmsh",				Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_CLOTH,		20,	20,	70, ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_MAGE|ITEM_POWER_3|ITEM_MAGIC_RESISTANCE_10, 4), // jw czerwona
	Armor("armor_mage_4",			15,		500,	"toga4.qmsh",				Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_CLOTH,		30,	20,	70, ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_MAGE|ITEM_POWER_4|ITEM_MAGIC_RESISTANCE_25|ITEM_NOT_RANDOM, 4), // jw czerwona
	Armor("armor_necromancer",		15,		80,		"toga_nekromanty.qmsh",		Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_CLOTH,		15, 25, 70, ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_POWER_2|ITEM_MAGIC_RESISTANCE_10, 3), // czarna szata
	Armor("clothes",				10,		5,		"ubranie.qmsh",				Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_CLOTH,		5, 20, 75, ITEM_NOT_BLACKSMITH|ITEM_NOT_CHEST, 1),
	Armor("clothes2",				10,		15,		"ubranie2.qmsh",			Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_CLOTH,		5, 20, 75, ITEM_NOT_BLACKSMITH|ITEM_NOT_CHEST, 1),
	Armor("clothes3",				10,		15,		"ubranie3.qmsh",			Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_CLOTH,		5, 20, 75, ITEM_NOT_BLACKSMITH|ITEM_NOT_CHEST, 1),
	Armor("clothes4",				10,		100,	"drogie_ubranie.qmsh",		Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_CLOTH,		5, 20, 75, ITEM_NOT_BLACKSMITH|ITEM_NOT_CHEST, 1),
	Armor("clothes5",				10,		100,	"drogie_ubranie2.qmsh",		Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_CLOTH,		5, 20, 75, ITEM_NOT_BLACKSMITH|ITEM_NOT_CHEST, 1)
};
const uint n_armors = countof(g_armors);

//=================================================================================================
Consumeable g_consumeables[] = {
	//			ID						TYPE	WG	PRICE	MESH					EFFECT			POWER	TIME	FLAGS
	Consumeable("potion_smallnreg",		Potion,	2,	10,		"naturalny.qmsh",		E_NATURAL,		2.f,	10.f,	ITEM_GROUND_MESH),
	Consumeable("potion_smallheal",		Potion,	1,	25,		"mikstura_hp.qmsh",		E_HEAL,			200.f,	0.f,	ITEM_GROUND_MESH),
	Consumeable("potion_mediumheal",	Potion,	1,	100,	"mikstura_hp2.qmsh",	E_HEAL,			400.f,	0.f,	ITEM_GROUND_MESH),
	Consumeable("potion_bigheal",		Potion,	1,	400,	"mikstura_hp3.qmsh",	E_HEAL,			600.f,	0.f,	ITEM_GROUND_MESH),
	Consumeable("potion_bignreg",		Potion,	2,	100,	"naturalny2.qmsh",		E_NATURAL,		2.f,	20.f,	ITEM_GROUND_MESH),
	Consumeable("potion_reg",			Potion,	1,	250,	"mikstura_reg.qmsh",	E_REGENERATE,	5.f,	360.f,	ITEM_GROUND_MESH),
	Consumeable("potion_antidote",		Potion,	1,	50,		"antidotum.qmsh",		E_ANTIDOTE,		0.f,	0.f,	ITEM_GROUND_MESH),
	Consumeable("potion_str",			Potion,	4,	500,	"wodka.qmsh",			E_STR,			0.f,	0.f,	ITEM_NOT_ALCHEMIST|ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_GROUND_MESH),
	Consumeable("potion_end",			Potion,	4,	500,	"wodka.qmsh",			E_END,			0.f,	0.f,	ITEM_NOT_ALCHEMIST|ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_GROUND_MESH),
	Consumeable("potion_dex",			Potion,	4,	500,	"wodka.qmsh",			E_DEX,			0.f,	0.f,	ITEM_NOT_ALCHEMIST|ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_GROUND_MESH),
	Consumeable("q_magowie_potion",		Potion,	1,	150,	"antidotum.qmsh",		E_NONE,			0.f,	0.f,	ITEM_NOT_ALCHEMIST|ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_GROUND_MESH|ITEM_NOT_RANDOM),
	Consumeable("potion_antimagic",		Potion,	2,	200,	"mikstura_reg.qmsh",	E_ANTIMAGIC,	0.f,	300.f,	ITEM_GROUND_MESH),

	Consumeable("potion_beer",			Drink,	5,	2,		"piwo.qmsh",			E_ALCOHOL,		50.f,	1.f,	ITEM_NOT_ALCHEMIST|ITEM_GROUND_MESH),
	Consumeable("potion_vodka",			Drink,	4,	10,		"wodka.qmsh",			E_ALCOHOL,		100.f,	2.f,	ITEM_NOT_ALCHEMIST|ITEM_GROUND_MESH),
	Consumeable("potion_spirit",		Drink,	4,	20,		"wodka.qmsh",			E_ALCOHOL,		200.f,	4.f,	ITEM_NOT_ALCHEMIST|ITEM_GROUND_MESH),
	Consumeable("potion_water",			Drink,	4,	1,		"woda.qmsh",			E_NONE,			0.f,	0.f,	ITEM_NOT_ALCHEMIST|ITEM_GROUND_MESH),

	Consumeable("bread",				Food,	8,	5,		"chleb.qmsh",			E_FOOD,			20.f,	0.f,	ITEM_NOT_ALCHEMIST|ITEM_GROUND_MESH|ITEM_NOT_MERCHANT),
	Consumeable("meat",					Food,	5,	10,		"mieso.qmsh",			E_FOOD,			25.f,	0.f,	ITEM_NOT_ALCHEMIST|ITEM_GROUND_MESH|ITEM_NOT_MERCHANT),
	Consumeable("raw_meat",				Food,	5,	5,		"surowe_mieso.qmsh",	E_FOOD,			10.f,	0.f,	ITEM_NOT_ALCHEMIST|ITEM_GROUND_MESH|ITEM_NOT_MERCHANT),
	Consumeable("apple",				Food,	1,	3,		"jablko.qmsh",			E_FOOD,			15.f,	0.f,	ITEM_NOT_ALCHEMIST|ITEM_GROUND_MESH|ITEM_NOT_MERCHANT),
	Consumeable("cheese",				Food,	5,	8,		"ser.qmsh",				E_FOOD,			20.f,	0.f,	ITEM_NOT_ALCHEMIST|ITEM_GROUND_MESH|ITEM_NOT_MERCHANT),
	Consumeable("honeycomb",			Food,	2,	15,		"plaster_miodu.qmsh",	E_FOOD,			30.f,	0.f,	ITEM_NOT_ALCHEMIST|ITEM_GROUND_MESH|ITEM_NOT_MERCHANT),
	Consumeable("rice",					Food,	5,	5,		"ryz.qmsh",				E_FOOD,			15.f,	0.f,	ITEM_NOT_ALCHEMIST|ITEM_GROUND_MESH|ITEM_NOT_MERCHANT),
	Consumeable("mushroom",				Food,	2,	4,		"grzyb.qmsh",			E_FOOD,			10.f,	0.f,	ITEM_NOT_ALCHEMIST|ITEM_GROUND_MESH|ITEM_NOT_MERCHANT),
};
const uint n_consumeables = countof(g_consumeables);

//=================================================================================================
OtherItem g_others[] = {
	OtherItem("vs_emerald", ValueableStone, "szmaragd.png", 1, 250, ITEM_CRYSTAL_SOUND|ITEM_TEX_ONLY),
	OtherItem("vs_ruby", ValueableStone, "rubin.png", 1, 500, ITEM_CRYSTAL_SOUND|ITEM_TEX_ONLY),
	OtherItem("vs_diamond", ValueableStone, "diament.png", 1, 750, ITEM_CRYSTAL_SOUND|ITEM_TEX_ONLY),
	OtherItem("vs_krystal", ValueableStone, "krystal.png", 1, 1000, ITEM_NOT_SHOP|ITEM_CRYSTAL_SOUND|ITEM_TEX_ONLY),

	OtherItem("ladle", Tool, "chochla.qmsh", 5, 5, 0),

	// kolejnoœæ jest wa¿na dla Quest_ZnajdzArtefakt
	OtherItem("a_amulet", OtherItems, "a_amulet.png", 1, 100, ITEM_QUEST|ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("a_amulet2", OtherItems, "a_amulet2.png", 1, 100, ITEM_QUEST|ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("a_amulet3", OtherItems, "a_amulet3.png", 1, 100, ITEM_QUEST|ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("a_brosza", OtherItems, "a_brosza.png", 1, 100, ITEM_QUEST|ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("a_butelka", OtherItems, "a_butelka.png", 5, 50, ITEM_QUEST|ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("a_cos", OtherItems, "a_cos.png", 5, 50, ITEM_QUEST|ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("a_czaszka", OtherItems, "a_czaszka.png", 10, 50, ITEM_QUEST|ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("a_figurka", OtherItems, "a_figurka.png", 2, 100, ITEM_QUEST|ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("a_figurka2", OtherItems, "a_figurka2.png", 2, 100, ITEM_QUEST|ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("a_figurka3", OtherItems, "a_figurka3.png", 2, 100, ITEM_QUEST|ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("a_ksiega", OtherItems, "a_ksiega.png", 5, 50, ITEM_QUEST|ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("a_maska", OtherItems, "a_maska.png", 5, 100, ITEM_QUEST|ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("a_maska2", OtherItems, "a_maska2.png", 5, 100, ITEM_QUEST|ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("a_misa", OtherItems, "a_misa.png", 5, 50, ITEM_QUEST|ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("a_moneta", OtherItems, "a_moneta.png", 1, 50, ITEM_QUEST|ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("a_pierscien", OtherItems, "a_pierscien.png", 1, 100, ITEM_QUEST|ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("a_pierscien2", OtherItems, "a_pierscien2.png", 1, 100, ITEM_QUEST|ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("a_pierscien3", OtherItems, "a_pierscien3.png", 1, 100, ITEM_QUEST|ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("a_talizman", OtherItems, "a_talizman.png", 1, 75, ITEM_QUEST|ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("a_talizman2", OtherItems, "a_talizman2.png", 1, 75, ITEM_QUEST|ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),

	// questowe przedmioty, ale tylko dla tych jednorazowych bo nie potrzbuj¹ refid
	OtherItem("key_kopalnia", OtherItems, "old-key.png", 1, 0, ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("q_bandyci_paczka", OtherItems, "paczka.png", 50, 0, ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_DONT_DROP|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("q_bandyci_list", OtherItems, "list.png", 1, 0, ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("q_magowie_kula", OtherItems, "magic_sphere.png", 10, 0, ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_CRYSTAL_SOUND|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("q_magowie_kula2", OtherItems, "magic_sphere.png", 10, 5000, ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_CRYSTAL_SOUND|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("q_orkowie_klucz", OtherItems, "rusty-key.png", 1, 0, ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("q_zlo_ksiega", OtherItems, "summoning.png", 10, 100, ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("key_lunar", OtherItems, "moon_key.png", 1, 100, ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("q_szaleni_kamien", OtherItems, "kamien2.png", 50, 0, ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("sekret_kartka", OtherItems, "piece_of_paper.png", 1, 0, ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),
	OtherItem("sekret_kartka2", OtherItems, "piece_of_paper.png", 1, 0, ITEM_NOT_SHOP|ITEM_NOT_CHEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_RANDOM),

	// szablony dla questowych przedmiotów
	OtherItem("letter", OtherItems, "list.png", 1, 0, ITEM_QUEST|ITEM_DONT_DROP|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_SHOP|ITEM_NOT_RANDOM),
	OtherItem("parcel", OtherItems, "paczka.png", 10, 0, ITEM_QUEST|ITEM_DONT_DROP|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_SHOP|ITEM_NOT_RANDOM),
	OtherItem("wanted_letter", OtherItems, "wanted.png", 1, 0, ITEM_QUEST|ITEM_IMPORTANT|ITEM_TEX_ONLY|ITEM_NOT_SHOP|ITEM_NOT_RANDOM),

};
const uint n_others = countof(g_others);

//=================================================================================================
// LIST PRZEDMIOTÓW
#define I(x) x, NULL

ItemEntry list_normal_food[] = {
	I("bread"),
	I("meat"),
	I("apple"),
	I("cheese"),
	I("honeycomb"),
	I("rice")
};

ItemEntry list_orc_food[] = {
	I("bread"),
	I("meat"),
	I("raw_meat"),
	I("mushroom")
};

ItemEntry list_food_and_drink[] = {
	I("bread"),
	I("meat"),
	I("apple"),
	I("cheese"),
	I("honeycomb"),
	I("rice"),
	I("raw_meat"),
	I("mushroom"),
	I("potion_water"),
	I("potion_beer"),
	I("potion_vodka"),
	I("potion_spirit")
};

ItemList g_item_lists[] = {
	"normal_food", list_normal_food, countof(list_normal_food),
	"orc_food", list_orc_food, countof(list_orc_food),
	"food_and_drink", list_food_and_drink, countof(list_food_and_drink)
};
const uint n_item_lists = countof(g_item_lists);

void SetItemLists()
{
	for(uint i=0; i<n_item_lists; ++i)
	{
		ItemList& lis = g_item_lists[i];
		for(uint j=0; j<lis.count; ++j)
			lis.items[j].item = FindItem(lis.items[j].name);
	}
}

//=================================================================================================
const Item* FindItem(cstring name, bool report, const ItemList** o_lis)
{
	assert(name);

	if(name[0] == '!')
	{
		for(uint i=0; i<n_item_lists; ++i)
		{
			ItemList& lis = g_item_lists[i];
			if(strcmp(name+1, lis.name) == 0)
			{
				if(o_lis)
					*o_lis = &lis;
				return lis.Get();
			}
		}
		
		if(report)
			WARN(Format("Missing item list '%s'.", name+1));

		return NULL;
	}

	for(uint i=0; i<n_weapons; ++i)
	{
		if(strcmp(g_weapons[i].id, name) == 0)
			return &g_weapons[i];
	}

	for(uint i=0; i<n_bows; ++i)
	{
		if(strcmp(g_bows[i].id, name) == 0)
			return &g_bows[i];
	}

	for(uint i=0; i<n_shields; ++i)
	{
		if(strcmp(g_shields[i].id, name) == 0)
			return &g_shields[i];
	}

	for(uint i=0; i<n_armors; ++i)
	{
		if(strcmp(g_armors[i].id, name) == 0)
			return &g_armors[i];
	}

	for(uint i=0; i<n_consumeables; ++i)
	{
		if(strcmp(g_consumeables[i].id, name) == 0)
			return &g_consumeables[i];
	}

	for(uint i=0; i<n_others; ++i)
	{
		if(strcmp(g_others[i].id, name) == 0)
			return &g_others[i];
	}

	if(strcmp(name, "vs_emerals") == 0)
		return FindItem("vs_emerald");

	if(report)
		WARN(Format("Missing item '%s'.", name));

	return NULL;
}

//=================================================================================================
const ItemList* FindItemList(cstring name, bool report)
{
	assert(name);

	for(uint i=0; i<n_item_lists; ++i)
	{
		ItemList& lis = g_item_lists[i];
		if(strcmp(name, lis.name) == 0)
			return &lis;
	}

	if(report)
		WARN(Format("Missing item list '%s'.", name));

	return NULL;
}

//=================================================================================================
Item* CreateItemCopy(const Item* item)
{
	switch(item->type)
	{
	case IT_OTHER:
		{
			OtherItem* o = new OtherItem;
			const OtherItem& o2 = item->ToOther();
			o->ani = o2.ani;
			o->desc = o2.desc;
			o->flags = o2.flags;
			o->id = o2.id;
			o->level = o2.level;
			o->mesh = o2.mesh;
			o->name = o2.name;
			o->other_type = o2.other_type;
			o->refid = o2.refid;
			o->tex = o2.tex;
			o->type = o2.type;
			o->value = o2.value;
			o->weight = o2.weight;
			return o;
		}
		break;
	case IT_WEAPON:
	case IT_BOW:
	case IT_SHIELD:
	case IT_ARMOR:
	case IT_CONSUMEABLE:
	case IT_GOLD:
	case IT_LETTER:
	default:
		// nie zrobione bo i po co
		assert(0);
		return NULL;
	}
}
