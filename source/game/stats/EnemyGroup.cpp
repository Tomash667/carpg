// grupa jednostek
#include "Pch.h"
#include "Base.h"
#include "EnemyGroup.h"

//-----------------------------------------------------------------------------
EnemyEntry ee_goblins[] = {
	"goblin", nullptr, 10,
	"goblin_hunter", nullptr, 7,
	"goblin_fighter", nullptr, 7,
	"goblin_chief", nullptr, 2
};

//-----------------------------------------------------------------------------
EnemyEntry ee_orcs[] = {
	"orc", nullptr, 8,
	"orc_hunter", nullptr, 6,
	"orc_fighter", nullptr, 6,
	"orc_chief", nullptr, 2,
	"orc_shaman", nullptr, 3
};

//-----------------------------------------------------------------------------
EnemyEntry ee_bandits[] = {
	"bandit", nullptr, 10,
	"bandit_archer", nullptr, 5,
	"bandit_hegemon", nullptr, 2
};

//-----------------------------------------------------------------------------
EnemyEntry ee_crazies[] = {
	"crazy_mage", nullptr, 1,
	"crazy_warrior", nullptr, 2,
	"crazy_hunter", nullptr, 2,
	"crazy_rogue", nullptr, 2
};

//-----------------------------------------------------------------------------
EnemyEntry ee_necros[] = {
	"necromancer", nullptr, 3,
	"undead_guard", nullptr, 6,
	"undead_archer", nullptr, 6,
	"skeleton", nullptr, 8,
	"skeleton_archer", nullptr, 6,
	"skeleton_fighter", nullptr, 6,
	"skeleton_mage", nullptr, 2,
	"zombie", nullptr, 10,
	"zombie_rotting", nullptr, 8,
	"zombie_ancient", nullptr, 6
};

//-----------------------------------------------------------------------------
EnemyEntry ee_undead[] = {
	"skeleton", nullptr, 8,
	"skeleton_archer", nullptr, 6,
	"skeleton_fighter", nullptr, 6,
	"skeleton_mage", nullptr, 2,
	"zombie", nullptr, 10,
	"zombie_rotting", nullptr, 8,
	"zombie_ancient", nullptr, 6
};

//-----------------------------------------------------------------------------
EnemyEntry ee_evils[] = {
	"evil_cleric", nullptr, 3,
	"skeleton", nullptr, 8,
	"skeleton_archer", nullptr, 6,
	"skeleton_fighter", nullptr, 6,
	"skeleton_mage", nullptr, 2,
	"zombie", nullptr, 10,
	"zombie_rotting", nullptr, 8,
	"zombie_ancient", nullptr, 6
};

//-----------------------------------------------------------------------------
EnemyEntry ee_mages[] = {
	"mage", nullptr, 5,
	"mage_guard", nullptr, 10
};

//-----------------------------------------------------------------------------
EnemyEntry ee_golems[] = {
	"golem_stone", nullptr, 2, 
	"golem_iron", nullptr, 1
};

//-----------------------------------------------------------------------------
EnemyEntry ee_mages_n_golems[] = {
	"mage", nullptr, 5,
	"mage_guard", nullptr, 10,
	"golem_stone", nullptr, 2, 
	"golem_iron", nullptr, 1
};

//-----------------------------------------------------------------------------
EnemyEntry ee_animals[] = {
	"wolf", nullptr, 5,
	"worg", nullptr, 2,
	"rat", nullptr, 4,
	"spider", nullptr, 5,
	"spider_big", nullptr, 3
};

//-----------------------------------------------------------------------------
EnemyEntry ee_hunters[] = {
	"wolf", nullptr, 5,
	"worg", nullptr, 2,
	"rat", nullptr, 4,
	"spider", nullptr, 5,
	"spider_big", nullptr, 3,
	"wild_hunter", nullptr, 2
};

//-----------------------------------------------------------------------------
EnemyEntry ee_labirynth[] = {
	"skeleton", nullptr, 8,
	"skeleton_archer", nullptr, 6,
	"skeleton_fighter", nullptr, 6,
	"skeleton_mage", nullptr, 2,
	"zombie", nullptr, 10,
	"zombie_rotting", nullptr, 8,
	"zombie_ancient", nullptr, 6
};

//-----------------------------------------------------------------------------
EnemyEntry ee_cave_wolfs[] = {
	"wolf", nullptr, 5,
	"worg", nullptr, 2
};

//-----------------------------------------------------------------------------
EnemyEntry ee_cave_spiders[] = {
	"spider", nullptr, 5,
	"spider_big", nullptr, 3
};

//-----------------------------------------------------------------------------
EnemyEntry ee_cave_rats[] = {
	"rat", nullptr, 1
};

//-----------------------------------------------------------------------------
EnemyEntry ee_heroes[] = {
	"hero_mage", nullptr, 1,
	"hero_warrior", nullptr, 2,
	"hero_hunter", nullptr, 2,
	"hero_rogue", nullptr, 2
};

//-----------------------------------------------------------------------------
EnemyEntry ee_guards[] = {
	"guard3", nullptr, 1
};

//-----------------------------------------------------------------------------
EnemyEntry ee_guards2[] = {
	"guard2", nullptr, 1
};

//-----------------------------------------------------------------------------
EnemyEntry ee_unk[] = {
	"unk", nullptr, 1
};

//-----------------------------------------------------------------------------
EnemyGroup g_enemy_groups[] = {
	"goblins", ee_goblins, countof(ee_goblins),
	"orcs", ee_orcs, countof(ee_orcs),
	"bandits", ee_bandits, countof(ee_bandits),
	"crazies", ee_crazies, countof(ee_crazies),
	"necros", ee_necros, countof(ee_necros),
	"undead", ee_undead, countof(ee_undead),
	"evils", ee_evils, countof(ee_evils),
	"mages", ee_mages, countof(ee_mages),
	"golems", ee_golems, countof(ee_golems),
	"mages_n_golems", ee_mages_n_golems, countof(ee_mages_n_golems),
	"animals", ee_animals, countof(ee_animals),
	"hunters", ee_hunters, countof(ee_hunters),
	"labirynth", ee_labirynth, countof(ee_labirynth),
	"cave_wolfs", ee_cave_wolfs, countof(ee_cave_wolfs),
	"cave_spiders", ee_cave_spiders, countof(ee_cave_spiders),
	"cave_rats", ee_cave_rats, countof(ee_cave_rats),
	"heroes", ee_heroes, countof(ee_heroes),
	"guards", ee_guards, countof(ee_guards),
	"guards2", ee_guards2, countof(ee_guards2),
	"unk", ee_unk, countof(ee_unk)
};
int n_enemy_groups = countof(g_enemy_groups);
