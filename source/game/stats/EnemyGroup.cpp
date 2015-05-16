// grupa jednostek
#include "Pch.h"
#include "Base.h"
#include "EnemyGroup.h"

//-----------------------------------------------------------------------------
EnemyEntry ee_goblins[] = {
	"goblin", NULL, 10,
	"goblin_hunter", NULL, 7,
	"goblin_fighter", NULL, 7,
	"goblin_chief", NULL, 2
};

//-----------------------------------------------------------------------------
EnemyEntry ee_orcs[] = {
	"orc", NULL, 8,
	"orc_hunter", NULL, 6,
	"orc_fighter", NULL, 6,
	"orc_chief", NULL, 2,
	"orc_shaman", NULL, 3
};

//-----------------------------------------------------------------------------
EnemyEntry ee_bandits[] = {
	"bandit", NULL, 10,
	"bandit_archer", NULL, 5,
	"bandit_hegemon", NULL, 2
};

//-----------------------------------------------------------------------------
EnemyEntry ee_crazies[] = {
	"crazy_mage", NULL, 1,
	"crazy_warrior", NULL, 2,
	"crazy_hunter", NULL, 2,
	"crazy_rogue", NULL, 2
};

//-----------------------------------------------------------------------------
EnemyEntry ee_necros[] = {
	"necromancer", NULL, 3,
	"undead_guard", NULL, 6,
	"undead_archer", NULL, 6,
	"skeleton", NULL, 8,
	"skeleton_archer", NULL, 6,
	"skeleton_fighter", NULL, 6,
	"skeleton_mage", NULL, 2,
	"zombie", NULL, 10,
	"zombie_rotting", NULL, 8,
	"zombie_ancient", NULL, 6
};

//-----------------------------------------------------------------------------
EnemyEntry ee_undead[] = {
	"skeleton", NULL, 8,
	"skeleton_archer", NULL, 6,
	"skeleton_fighter", NULL, 6,
	"skeleton_mage", NULL, 2,
	"zombie", NULL, 10,
	"zombie_rotting", NULL, 8,
	"zombie_ancient", NULL, 6
};

//-----------------------------------------------------------------------------
EnemyEntry ee_evils[] = {
	"evil_cleric", NULL, 3,
	"skeleton", NULL, 8,
	"skeleton_archer", NULL, 6,
	"skeleton_fighter", NULL, 6,
	"skeleton_mage", NULL, 2,
	"zombie", NULL, 10,
	"zombie_rotting", NULL, 8,
	"zombie_ancient", NULL, 6
};

//-----------------------------------------------------------------------------
EnemyEntry ee_mages[] = {
	"mage", NULL, 5,
	"mage_guard", NULL, 10
};

//-----------------------------------------------------------------------------
EnemyEntry ee_golems[] = {
	"golem_stone", NULL, 2, 
	"golem_iron", NULL, 1
};

//-----------------------------------------------------------------------------
EnemyEntry ee_mages_n_golems[] = {
	"mage", NULL, 5,
	"mage_guard", NULL, 10,
	"golem_stone", NULL, 2, 
	"golem_iron", NULL, 1
};

//-----------------------------------------------------------------------------
EnemyEntry ee_animals[] = {
	"wolf", NULL, 5,
	"worg", NULL, 2,
	"rat", NULL, 4,
	"spider", NULL, 5,
	"spider_big", NULL, 3
};

//-----------------------------------------------------------------------------
EnemyEntry ee_hunters[] = {
	"wolf", NULL, 5,
	"worg", NULL, 2,
	"rat", NULL, 4,
	"spider", NULL, 5,
	"spider_big", NULL, 3,
	"wild_hunter", NULL, 2
};

//-----------------------------------------------------------------------------
EnemyEntry ee_labirynth[] = {
	"skeleton", NULL, 8,
	"skeleton_archer", NULL, 6,
	"skeleton_fighter", NULL, 6,
	"skeleton_mage", NULL, 2,
	"zombie", NULL, 10,
	"zombie_rotting", NULL, 8,
	"zombie_ancient", NULL, 6
};

//-----------------------------------------------------------------------------
EnemyEntry ee_cave_wolfs[] = {
	"wolf", NULL, 5,
	"worg", NULL, 2
};

//-----------------------------------------------------------------------------
EnemyEntry ee_cave_spiders[] = {
	"spider", NULL, 5,
	"spider_big", NULL, 3
};

//-----------------------------------------------------------------------------
EnemyEntry ee_cave_rats[] = {
	"rat", NULL, 1
};

//-----------------------------------------------------------------------------
EnemyEntry ee_heroes[] = {
	"hero_mage", NULL, 1,
	"hero_warrior", NULL, 2,
	"hero_hunter", NULL, 2,
	"hero_rogue", NULL, 2
};

//-----------------------------------------------------------------------------
EnemyEntry ee_guards[] = {
	"guard3", NULL, 1
};

//-----------------------------------------------------------------------------
EnemyEntry ee_guards2[] = {
	"guard2", NULL, 1
};

//-----------------------------------------------------------------------------
EnemyEntry ee_unk[] = {
	"unk", NULL, 1
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
