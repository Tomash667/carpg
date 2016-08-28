#pragma once

enum class GameSystemTypeId
{
	Building,
	BuildingGroup,
	BuildingScript,
	Unit
};

class GameSystemType
{
public:
	inline GameSystemType(GameSystemTypeId id, cstring name) : id(id), name(name) {}
	virtual ~GameSystemType() {}

protected:
	GameSystemTypeId id;
	string name;
};
