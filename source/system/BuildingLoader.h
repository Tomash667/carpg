#pragma once

//-----------------------------------------------------------------------------
#include "ContentLoader.h"
#include "BuildingScript.h"

//-----------------------------------------------------------------------------
class BuildingLoader : public ContentLoader
{
	friend class Content;
private:
	struct Var
	{
		string name;
		int index;
		bool isConst;
	};

	BuildingLoader();
	void DoLoading() override;
	static void Cleanup();
	void InitTokenizer() override;
	void LoadEntity(int top, const string& id) override;
	void Finalize() override;
	void ParseBuilding(const string& id);
	void ParseBuildingGroup(const string& id);
	void ParseBuildingScript(const string& id);
	void ParseCode(BuildingScript& script);
	void StartVariant(BuildingScript& script);
	void AddVar(Cstring id, bool isConst = false);
	Var* FindVar(const string& id);
	Var& GetVar(bool canBeConst = false);
	void GetExpr();
	void GetItem();
	int CharToOp(char c);
	void SetupBuildingGroups();
	void CalculateCrc();

	BuildingScript::Variant* variant;
	vector<int>* code;
	vector<Var> vars;
};
