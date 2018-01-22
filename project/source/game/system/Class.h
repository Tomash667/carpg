#pragma once

//-----------------------------------------------------------------------------
struct Action;
struct UnitData;

//-----------------------------------------------------------------------------
struct Class
{
	int index;
	string id;
	// localized data
	string name, desc, about;
	vector<string> names, nicknames;
	TEX icon;
	UnitData* player_data, *hero_data, *crazy_data;
	Action* action;
	bool pickable;

	Class() : icon(nullptr), player_data(nullptr), hero_data(nullptr), crazy_data(nullptr), action(nullptr), pickable(false)
	{
	}

	/*ClassInfo(Class class_id, cstring id, cstring unit_data_id, cstring icon_file, bool pickable, cstring action_id) : class_id(class_id), id(id),
		unit_data_id(unit_data_id), icon_file(icon_file), icon(nullptr), pickable(pickable), unit_data(nullptr), action_id(action_id), action(nullptr)
	{
	}*/

	/*bool IsPickable() const
	{
		return pickable;
	}*/

	/*static ClassInfo* Find(const string& id);
	static bool IsPickable(Class c);
	static Class GetRandom();
	static Class GetRandomPlayer();
	static Class GetRandomEvil();
	static void Validate(uint &err);
	static Class OldToNew(Class c);*/

	static Class* TryGet(const AnyString& id);
	static int GetRandomHeroClass(bool crazy = false);
	static UnitData& GetRandomHeroData(bool crazy = false);
	static UnitData& GetHeroData(int clas, bool crazy = false);
	static vector<Class*> classes;
	static const int RANDOM = -1;
	static const int INVALID = -2;
};

//=================================================================================================
//inline bool ClassInfo::IsPickable(Class c)
//{
//	if(c < (Class)0 || c >= Class::MAX)
//		return false;
//	return ClassInfo::classes[(int)c].pickable;
//}
