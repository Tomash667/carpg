// zewnêtrzna lokacja
#pragma once

//-----------------------------------------------------------------------------
#include "Location.h"
#include "LevelArea.h"
#include "TerrainTile.h"

//-----------------------------------------------------------------------------
struct OutsideLocation : public Location, public LevelArea
{
	vector<Object> objects;
	vector<Chest*> chests;
	vector<Useable*> useables;
	vector<Blood> bloods;
	TerrainTile* tiles;
	float* h;
	static const int size = 16 * 8;

	OutsideLocation() : Location(true), tiles(nullptr), h(nullptr)
	{

	}
	virtual ~OutsideLocation();

	inline bool IsInside(int _x, int _y) const
	{
		return _x >= 0 && _y >= 0 && _x < size && _y < size;
	}
	inline bool IsInside(const INT2& _pt) const
	{
		return IsInside(_pt.x, _pt.y);
	}
	inline VEC2 GetRandomPos() const
	{
		return VEC2(random(40.f,256.f-40.f), random(40.f,256.f-40.f));
	}

	void ApplyContext(LevelContext& ctx);
	virtual void Save(HANDLE file, bool local);
	virtual void Load(HANDLE file, bool local);

	virtual void BuildRefidTable();
	virtual bool FindUnit(Unit* unit, int* level);
	virtual Unit* FindUnit(UnitData* data, int& at_level);
	virtual LOCATION_TOKEN GetToken() const
	{
		return LT_OUTSIDE;
	}
};
