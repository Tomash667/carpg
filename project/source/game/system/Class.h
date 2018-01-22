#pragma once

//-----------------------------------------------------------------------------
struct Action;
struct UnitData;

//-----------------------------------------------------------------------------
struct Class
{
	int index;
	string id, unit_data_id;
	// localized data
	string name, desc, about;
	vector<string> names, nicknames;
	TEX icon;
	UnitData* unit_data;
	Action* action;
	bool pickable;

	Class() : icon(nullptr), unit_data(nullptr), action(nullptr), pickable(false)
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
