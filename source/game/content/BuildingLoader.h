#pragma once

//-----------------------------------------------------------------------------
#include "ContentLoader.h"

//-----------------------------------------------------------------------------
struct BuildingScript;

//-----------------------------------------------------------------------------
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
	vector<int>* code;

	bool LoadBuilding(Tokenizer& t);
	bool LoadBuildingGroups(Tokenizer& t);
	bool LoadBuildingScript(Tokenizer& t);
	void StartVariant(BuildingScript* script);
	void AddVar(AnyString id, bool is_const = false);
	Var* FindVar(const string& id);
	Var& GetVar(Tokenizer& t, bool can_be_const = false);
	void GetExpr(Tokenizer& t);
	void GetItem(Tokenizer& t);
	int CharToOp(char c);
};
