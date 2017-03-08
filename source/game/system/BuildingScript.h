#pragma once

//-----------------------------------------------------------------------------
struct BuildingGroup;
class GameTypeManager;

//-----------------------------------------------------------------------------
// Building script
struct BuildingScript
{
	// script code
	enum Code
	{
		BS_BUILDING, // building identifier [Entry = BS_BUILDING, Building*]
		BS_GROUP, // buildings group identifier [Entry = BS_GROUP, BuildingGroup*]
		BS_ADD_BUILDING, // add building [Entry]
		BS_RANDOM, // pick random item from list [uint-count, Entry...]
		BS_SHUFFLE_START, // shuffle start
		BS_SHUFFLE_END, // shuffle end
		BS_REQUIRED_OFF, // end of required buildings
		BS_PUSH_INT, // push int to stack [int]
		BS_PUSH_VAR, // push var to stack [uint-var index]
		BS_SET_VAR, // set var value from stack [uint-var index]
		BS_INC, // increase var by 1 [uint-var index]
		BS_DEC, // decrease var by 1 [uint-var index]
		BS_IF, // if block [Op (BS_EQUAL, BS_NOT_EQUAL, BS_GREATER, BS_GREATER_EQUAL, BS_LESS, BS_LESS_EQUAL)], takes 2 values from stack
		BS_IF_RAND, // if rand block, takes value from stack
		BS_ELSE, // else
		BS_ENDIF, // end of if
		BS_EQUAL, // equal op
		BS_NOT_EQUAL, // not equal op
		BS_GREATER, // greater op
		BS_GREATER_EQUAL, // greater equal op
		BS_LESS, // less op
		BS_LESS_EQUAL, // less equal op
		BS_CALL, // call function (currentlty only random)
		BS_ADD, // add two values from stack and push result
		BS_SUB, // subtract two values from stack and push result
		BS_MUL, // multiply two values from stack and push result
		BS_DIV, // divide two values from stack and push result
		BS_NEG, // negate value on stack
	};

public:
	// builtin variables
	enum VarIndex
	{
		V_COUNT,
		V_COUNTER,
		V_CITIZENS,
		V_CITIZENS_WORLD
	};

	// script variant
	struct Variant
	{
		uint index, vars;
		vector<int> code;
	};

	// max variables used by single script (including builtin)
	static const uint MAX_VARS = 10;

	string id;
	vector<Variant*> variants;
	int vars[MAX_VARS];
	uint required_offset;

	~BuildingScript()
	{
		DeleteElements(variants);
	}

	// Checks if building script have building from selected building group
	bool HaveBuilding(const string& group_id) const;
	bool HaveBuilding(BuildingGroup* group, Variant* variant) const;

	// Register building script gametype
	static void Register(GameTypeManager& gt_mgr);

private:
	bool IsEntryGroup(const int*& code, BuildingGroup* group) const;
};
