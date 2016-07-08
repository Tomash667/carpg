#pragma once

enum DatatypeId
{
	DT_BuildingGroup,
	DT_Building
};

struct Datatype
{
	struct Field
	{
		enum Type
		{
			STRING
		};
		
		string name;
		Type type;
		uint offset;
	};

	DatatypeId dtid;
	string id, multi_id;

	void AddId(uint offset);
};


