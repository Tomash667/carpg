#pragma once

//-----------------------------------------------------------------------------
class LevelArea
{
public:
	vector<Unit*> units;
	vector<GroundItem*> items;
};

//-----------------------------------------------------------------------------
class LevelAreaContext;

//-----------------------------------------------------------------------------
extern ObjectPool<LevelAreaContext> LevelAreaContextPool;

//-----------------------------------------------------------------------------
class LevelAreaContext
{
public:
	struct Entry
	{
		LevelArea* area;
		int loc, level;
		bool active;
	};

	vector<Entry> entries;
	int refs;

	inline void AddRef()
	{
		++refs;
	}
	inline void Free()
	{
		if(--refs == 0)
			LevelAreaContextPool.Free(this);
	}
};
