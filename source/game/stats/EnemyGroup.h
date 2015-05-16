// grupa jednostek
#pragma once

//-----------------------------------------------------------------------------
struct UnitData;

//-----------------------------------------------------------------------------
struct EnemyEntry
{
	cstring id;
	UnitData* unit;
	int count;
};

//-----------------------------------------------------------------------------
struct EnemyGroup
{
	cstring id;
	EnemyEntry* enemies;
	int count;
};
extern EnemyGroup g_enemy_groups[];
extern int n_enemy_groups;

//-----------------------------------------------------------------------------
inline EnemyGroup& FindEnemyGroup(cstring id)
{
	assert(id);

	for(int i=0; i<n_enemy_groups; ++i)
	{
		if(strcmp(id, g_enemy_groups[i].id) == 0)
			return g_enemy_groups[i];
	}

	assert(0);
	return g_enemy_groups[0];
}

//-----------------------------------------------------------------------------
inline int FindEnemyGroupId(cstring id)
{
	assert(id);

	for(int i=0; i<n_enemy_groups; ++i)
	{
		if(strcmp(id, g_enemy_groups[i].id) == 0)
			return i;
	}

	assert(0);
	return -1;
}
