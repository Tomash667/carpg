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
	//		ID						WEIGHT	PRICE	MESH						SKILL					TYPE					MAT				DEF	SI£	ZRÊ FLAGS LVL
	Armor("al_padded",				45,		5,		"skorznia.qmsh",			Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_SKIN,		5, 25, 90, 0, 0),
	Armor("al_padded_hq",			45,		55,		"skorznia.qmsh",			Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_SKIN,		7, 25, 95, ITEM_HQ, 0),
	Armor("al_padded_m",			40,		505,	"skorznia.qmsh",			Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_SKIN,		15, 25, 95, ITEM_MAGICAL, 0),
	Armor("al_leather",				70,		15,		"skorznia.qmsh",			Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_SKIN,		10, 30, 75, 0, 1),
	Armor("al_leather_hq",			70,		65,		"skorznia.qmsh",			Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_SKIN,		12, 30, 80, ITEM_HQ, 1),
	Armor("al_leather_m",			60,		515,	"skorznia.qmsh",			Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_SKIN,		20, 30, 80, ITEM_MAGICAL, 0),
	// Resistance(Magic, 10), Resistance(Fire, 25)
	Armor("al_dragonskin",			80,		7015,	"smocza_skora.qmsh",		Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_SKIN,		55, 35, 70, ITEM_MAGIC_RESISTANCE_10 | ITEM_MAGICAL, 15),
	Armor("al_studded",				90,		30,		"cwiekowana.qmsh",			Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_SKIN,		15, 35, 60, 0, 3),
	Armor("al_studded_hq",			90,		80,		"cwiekowana.qmsh",			Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_SKIN,		18, 35, 65, ITEM_HQ, 3),
	Armor("al_studded_m",			80,		530,	"cwiekowana.qmsh",			Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_SKIN,		25, 35, 65, ITEM_MAGICAL, 3),
	// Attribute(Dex, 15), SkillGroup(Thieft, 15), Resistance(Magic, 20)
	Armor("al_shadow",				70,		12000,	"cwiekowana.qmsh",			Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_SKIN,		60, 30, 90, ITEM_UNIQUE, 3),
	Armor("al_chain_shirt",			115,	100,	"koszulka_kolcza.qmsh",		Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		25, 40, 60, 0, 5),
	Armor("al_chain_shirt_hq",		115,	200,	"koszulka_kolcza.qmsh",		Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		30, 40, 65, ITEM_HQ, 5),
	Armor("al_chain_shirt_m",		100,	1100,	"koszulka_kolcza.qmsh",		Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		35, 40, 65, ITEM_MAGICAL, 5),
	Armor("al_chain_shirt_mith",	55,		2600,	"mithrilowa_koszulka.qmsh",	Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		35, 30, 75, ITEM_MAGICAL, 10),
	// RegenAura(1), Resistance(Negative, 25), Resistance(Magic, 25)
	Armor("al_angelskin",			30,		15000,	"anielska_skora.qmsh",		Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		60, 20, 100, ITEM_NOT_CHEST | ITEM_NOT_SHOP | ITEM_POWER_1 | ITEM_MAGIC_RESISTANCE_25 | ITEM_NOT_RANDOM | ITEM_UNIQUE, 20),

	Armor("am_hide",				115,	15,		"skorznia.qmsh",			Skill::MEDIUM_ARMOR,	ArmorUnitType::HUMAN,	MAT_SKIN,		15, 40, 55, 0, 0),
	Armor("am_hide_hq",				115,	65,		"skorznia.qmsh",			Skill::MEDIUM_ARMOR,	ArmorUnitType::HUMAN,	MAT_SKIN,		18, 40, 60, ITEM_HQ, 0),
	Armor("am_hide_m",				100,	1100,	"skorznia.qmsh",			Skill::MEDIUM_ARMOR,	ArmorUnitType::HUMAN,	MAT_SKIN,		25, 40, 60, ITEM_MAGICAL, 0),
	// Attribute(Str, 10), Attribute(Con, 10), Regen(2)
	Armor("am_troll_hide",			125,	6000,	"skorznia.qmsh",			Skill::MEDIUM_ARMOR,	ArmorUnitType::HUMAN,	MAT_SKIN,		40, 40, 60, ITEM_MAGICAL, 0),
	Armor("am_chainmail",			180,	60,		"kolczuga.qmsh",			Skill::MEDIUM_ARMOR,	ArmorUnitType::HUMAN,	MAT_IRON,		20, 50, 50, 0, 1),
	Armor("am_chainmail_hq",		180,	160,	"kolczuga.qmsh",			Skill::MEDIUM_ARMOR,	ArmorUnitType::HUMAN,	MAT_IRON,		24, 50, 55, ITEM_HQ, 1),
	Armor("am_chainmail_m",			160,	1060,	"kolczuga.qmsh",			Skill::MEDIUM_ARMOR,	ArmorUnitType::HUMAN,	MAT_IRON,		30, 50, 55, ITEM_MAGICAL, 1),
	// Resistance(Magic, 10)
	Armor("am_chainmail_mith",		90,		2660,	"kolczuga.qmsh",			Skill::MEDIUM_ARMOR,	ArmorUnitType::HUMAN,	MAT_IRON,		30, 35, 65, ITEM_MAGICAL, 1),
	Armor("am_scale",				150,	150,	"kolczuga.qmsh",			Skill::MEDIUM_ARMOR,	ArmorUnitType::HUMAN,	MAT_IRON,		25, 50, 55, 0, 0),
	Armor("am_scale_hq",			150,	250,	"kolczuga.qmsh",			Skill::MEDIUM_ARMOR,	ArmorUnitType::HUMAN,	MAT_IRON,		29, 50, 60, ITEM_HQ, 0),
	Armor("am_scale_m",				135,	1150,	"kolczuga.qmsh",			Skill::MEDIUM_ARMOR,	ArmorUnitType::HUMAN,	MAT_IRON,		35, 45, 60, ITEM_MAGICAL, 0),
	// Resistance(Magic, 10), Resistance(Fire, 25)
	Armor("am_dragonscale",			165,	8150,	"kolczuga.qmsh",			Skill::MEDIUM_ARMOR,	ArmorUnitType::HUMAN,	MAT_IRON,		60, 50, 50, ITEM_MAGICAL, 0),
	Armor("am_breastplate",			135,	270,	"napiersnik.qmsh",			Skill::MEDIUM_ARMOR,	ArmorUnitType::HUMAN,	MAT_IRON,		30, 45, 60, 0, 3),
	Armor("am_breastplate_hq",		135,	420,	"napiersnik.qmsh",			Skill::MEDIUM_ARMOR,	ArmorUnitType::HUMAN,	MAT_IRON,		35, 45, 65, ITEM_HQ, 3),
	Armor("am_breastplate_m",		120,	1270,	"napiersnik.qmsh",			Skill::MEDIUM_ARMOR,	ArmorUnitType::HUMAN,	MAT_IRON,		40, 40, 65, ITEM_MAGICAL, 3),
	// Resistance(Magic, 10)
	Armor("am_breastplate_mith",	65,		3270,	"napiersnik.qmsh",			Skill::MEDIUM_ARMOR,	ArmorUnitType::HUMAN,	MAT_IRON,		40, 30, 75, ITEM_MAGICAL, 3),
	// Resistance(Magic, 15)
	Armor("am_breastplate_adam",	160,	6270,	"napiersnik.qmsh",			Skill::MEDIUM_ARMOR,	ArmorUnitType::HUMAN,	MAT_IRON,		55, 50, 55, ITEM_MAGICAL, 3),
	// Resistance(Magic, 25), Attribute(Str, 10), Attribute(Con, 10), Attribute(Dex, 10), SkillGroup(Weapon, 10), Skill(MediumArmor, 10)
	Armor("am_gladiator",			135,	22000,	"napiersnik.qmsh",			Skill::MEDIUM_ARMOR,	ArmorUnitType::HUMAN,	MAT_IRON,		80, 45, 65, ITEM_UNIQUE | ITEM_NOT_CHEST | ITEM_NOT_SHOP | ITEM_NOT_RANDOM, 3),

	Armor("ah_splint",				180,	100,	"napiersnik.qmsh",			Skill::HEAVY_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		25, 50, 45, 0, 0),
	Armor("ah_splint_hq",			180,	300,	"napiersnik.qmsh",			Skill::HEAVY_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		30, 50, 50, ITEM_HQ, 0),
	Armor("ah_splint_m",			160,	1100,	"napiersnik.qmsh",			Skill::HEAVY_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		35, 50, 50, ITEM_MAGICAL, 0),
	// Resistance(Magic, 10)
	Armor("ah_splint_mith",			90,		3100,	"napiersnik.qmsh",			Skill::HEAVY_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		35, 35, 65, ITEM_MAGICAL, 0),
	// Resistance(Magic, 50), ManaBurn(10)
	Armor("ah_antimage",			200,	18000,	"napiersnik.qmsh",			Skill::HEAVY_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		65, 55, 50, ITEM_UNIQUE | ITEM_NOT_SHOP | ITEM_NOT_CHEST | ITEM_NOT_RANDOM, 0),
	Armor("ah_plated",				225,	350,	"plytowka.qmsh",			Skill::HEAVY_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		35, 60, 40, 0, 0),
	Armor("ah_plated_hq",			225,	650,	"plytowka.qmsh",			Skill::HEAVY_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		40, 60, 45, ITEM_HQ, 0),
	Armor("ah_plated_m",			200,	1350,	"plytowka.qmsh",			Skill::HEAVY_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		45, 55, 45, ITEM_MAGICAL, 0),
	// Resistance(Magic, 10)
	Armor("ah_plated_mith",			115,	2950,	"plytowka.qmsh",			Skill::HEAVY_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		45, 40, 60, ITEM_MAGICAL, 0),
	Armor("ah_plate",				250,	1500,	"plytowka.qmsh",			Skill::HEAVY_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		50, 70, 30, 0, 5),
	Armor("ah_plate_hq",			250,	1700,	"plytowka.qmsh",			Skill::HEAVY_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		55, 70, 35, ITEM_HQ, 5),
	Armor("ah_plate_m",				250,	3500,	"plytowka.qmsh",			Skill::HEAVY_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		60, 65, 35, ITEM_MAGICAL, 5),
	// Resistance(Magic, 10)
	Armor("ah_plate_mith",			125,	6500,	"plytowka.qmsh",			Skill::HEAVY_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		60, 40, 55, ITEM_MAGICAL, 5),
	// Resistance(Magic, 15)
	Armor("ah_plate_adam",			300,	10500,	"ada_plytowka.qmsh",		Skill::HEAVY_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		80, 75, 25, ITEM_MAGIC_RESISTANCE_10|ITEM_MAGICAL, 15),
	// Resistance(Magic, 25), Attribute(Str, 15), Attribute(Con, 15), DamageBurn(15, Negative)
	Armor("ah_black_armor",			300,	30000,	"czarna_zbroja.qmsh",		Skill::HEAVY_ARMOR,		ArmorUnitType::HUMAN,	MAT_IRON,		100, 75, 25, ITEM_NOT_CHEST | ITEM_NOT_SHOP | ITEM_MAGIC_RESISTANCE_25 | ITEM_NOT_RANDOM | ITEM_UNIQUE, 20),
	// Resistance(Magic, 10)
	Armor("ah_crystal",				350,	6000,	"krysztalowa.qmsh",			Skill::HEAVY_ARMOR,		ArmorUnitType::HUMAN,	MAT_CRYSTAL,	65, 80, 20, ITEM_HQ, 10),
	// Resistance(Magic, 15)
	Armor("ah_crystal_m",			350,	10000,	"krysztalowa.qmsh",			Skill::HEAVY_ARMOR,		ArmorUnitType::HUMAN,	MAT_CRYSTAL,	70, 80, 20, ITEM_MAGICAL, 10),
	// Resistance(Magic, 20), Lifesteal(5), DamageToAttribute(5%, 30 max, 40 limit, 1/sec decrase, Str)
	Armor("ah_blood_crystal",		350,	24000,	"krysztalowa.qmsh",			Skill::HEAVY_ARMOR,		ArmorUnitType::HUMAN,	MAT_CRYSTAL,	90, 80, 20, ITEM_MAGICAL, 10),

	Armor("al_blacksmith",			20,		50,		"ubranie_kowala.qmsh",		Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_SKIN,		10, 30, 60, ITEM_NOT_BLACKSMITH, 1),
	Armor("al_innkeeper",			15,		50,		"ubr_karczmarza.qmsh",		Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_CLOTH,		5, 20, 60, ITEM_NOT_BLACKSMITH, 1),
	Armor("al_goblin",				40,		5,		"goblinski_pancerz.qmsh",	Skill::LIGHT_ARMOR,		ArmorUnitType::GOBLIN,	MAT_SKIN,		5, 20, 75, ITEM_NOT_SHOP | ITEM_NOT_CHEST, 1),
	Armor("al_orc",					90,		5,		"orkowa_zbroja.qmsh",		Skill::LIGHT_ARMOR,		ArmorUnitType::ORC,		MAT_SKIN,		15, 40, 55, ITEM_NOT_SHOP | ITEM_NOT_CHEST, 1),
	Armor("al_orc_shaman",			90,		10,		"orkowa_zbroja_o.qmsh",		Skill::LIGHT_ARMOR,		ArmorUnitType::ORC,		MAT_SKIN,		15, 40, 55, ITEM_NOT_SHOP | ITEM_NOT_CHEST | ITEM_MAGE | ITEM_POWER_1 | ITEM_MAGICAL, 1),
	Armor("am_orc",					220,	30,		"orkowa_zbroja2.qmsh",		Skill::MEDIUM_ARMOR,	ArmorUnitType::ORC,		MAT_IRON,		25, 60, 35, ITEM_NOT_SHOP | ITEM_NOT_CHEST, 1),
	Armor("al_mage_1",				15,		30,		"toga.qmsh",				Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_CLOTH,		5, 20, 70, ITEM_NOT_SHOP | ITEM_NOT_CHEST | ITEM_MAGE | ITEM_POWER_1 | ITEM_MAGICAL, 1),
	Armor("al_mage_2",				15,		100,	"toga2.qmsh",				Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_CLOTH,		10, 20, 70, ITEM_NOT_SHOP | ITEM_NOT_CHEST | ITEM_MAGE | ITEM_POWER_2 | ITEM_MAGIC_RESISTANCE_10, 2),
	Armor("al_mage_3",				15,		350,	"toga3.qmsh",				Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_CLOTH,		20, 20, 70, ITEM_NOT_SHOP | ITEM_NOT_CHEST | ITEM_MAGE | ITEM_POWER_3 | ITEM_MAGIC_RESISTANCE_10, 4),
	Armor("al_mage_4",				15,		500,	"toga4.qmsh",				Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_CLOTH,		30, 20, 70, ITEM_NOT_SHOP | ITEM_NOT_CHEST | ITEM_MAGE | ITEM_POWER_4 | ITEM_MAGIC_RESISTANCE_25 | ITEM_NOT_RANDOM, 4),
	Armor("al_necromancer",			15,		80,		"toga_nekromanty.qmsh",		Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_CLOTH,		15, 25, 70, ITEM_NOT_SHOP | ITEM_NOT_CHEST | ITEM_POWER_2 | ITEM_MAGIC_RESISTANCE_10, 3),
	Armor("al_clothes_1",			10,		5,		"ubranie.qmsh",				Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_CLOTH,		5, 20, 75, ITEM_NOT_BLACKSMITH | ITEM_NOT_CHEST, 1),
	Armor("al_clothes_2",			10,		15,		"ubranie2.qmsh",			Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_CLOTH,		5, 20, 75, ITEM_NOT_BLACKSMITH | ITEM_NOT_CHEST, 1),
	Armor("al_clothes_3",			10,		15,		"ubranie3.qmsh",			Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_CLOTH,		5, 20, 75, ITEM_NOT_BLACKSMITH | ITEM_NOT_CHEST, 1),
	Armor("al_clothes_4",			10,		100,	"drogie_ubranie.qmsh",		Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_CLOTH,		5, 20, 75, ITEM_NOT_BLACKSMITH | ITEM_NOT_CHEST | ITEM_HQ, 1),
	Armor("al_clothes_5",			10,		100,	"drogie_ubranie2.qmsh",		Skill::LIGHT_ARMOR,		ArmorUnitType::HUMAN,	MAT_CLOTH,		5, 20, 75, ITEM_NOT_BLACKSMITH | ITEM_NOT_CHEST | ITEM_HQ, 1)
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

//=================================================================================================
void Item::Validate(int& err)
{
	for(Weapon& w : g_weapons)
	{
		if(w.name.empty())
		{
			++err;
			ERROR(Format("Missing weapon '%s' name.", w.id));
		}
	}

	for(Bow& b : g_bows)
	{
		if(b.name.empty())
		{
			++err;
			ERROR(Format("Missing bow '%s' name.", b.id));
		}
	}

	for(Shield& s : g_shields)
	{
		if(s.name.empty())
		{
			++err;
			ERROR(Format("Missing shield '%s' name.", s.id));
		}
	}

	for(Armor& a : g_armors)
	{
		if(a.name.empty())
		{
			++err;
			ERROR(Format("Missing armor '%s' name.", a.id));
		}
	}

	for(Consumeable& c : g_consumeables)
	{
		if(c.name.empty())
		{
			++err;
			ERROR(Format("Missing consumeable '%s' name.", c.id));
		}
	}

	for(OtherItem& o : g_others)
	{
		if(o.name.empty())
		{
			++err;
			ERROR(Format("Missing other item '%s' name.", o.id));
		}
	}
}
