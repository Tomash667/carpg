#pragma once

#include "ContentLoader.h"

struct BuildingScript;

class BuildingLoader : public ContentLoader
{
public:
	BuildingLoader();
	~BuildingLoader();
	
	void Init() override;
	void Cleanup() override;
	int Load() override;
	int LoadText() override;

	bool LoadBuilding();
	bool LoadBuildingGroups();
	bool LoadBuildingScript();

	struct Var
	{
		string name;
		int index;
		bool is_const;
	};
	vector<Var> vars;
	string tmp_str;

	void StartVariant(BuildingScript* script, vector<int>*& code);
	void AddVar(AnyString id, bool is_const = false);
	Var* FindVar(const string& id);
	Var& GetVar(bool can_be_const = false);
	void GetVarOrInt(int& type, int& value);
};
