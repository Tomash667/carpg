#pragma once

#include "ContentLoader.h"

struct BuildingScript;

class BuildingLoader final : public ContentLoader
{
public:
	BuildingLoader();
	~BuildingLoader();
	
	virtual void Init() override;
	virtual void Cleanup() override;
	virtual int Load() override;
	virtual int LoadText() override;

private:
	struct Var
	{
		string name;
		int index;
		bool is_const;
	};
	vector<Var> vars;
	string tmp_str;

	bool LoadBuilding();
	bool LoadBuildingGroups();
	bool LoadBuildingScript();
	void StartVariant(BuildingScript* script, vector<int>*& code);
	void AddVar(AnyString id, bool is_const = false);
	Var* FindVar(const string& id);
	Var& GetVar(bool can_be_const = false);
	void GetVarOrInt(int& type, int& value);
};
