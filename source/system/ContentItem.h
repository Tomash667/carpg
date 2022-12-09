#pragma once

//-----------------------------------------------------------------------------
#include "ScriptException.h"

//-----------------------------------------------------------------------------
template<typename T>
struct ContentItem
{
	int hash;
	string id;

	ContentItem() : hash(0) {}

	static std::unordered_map<int, T*> items;
	static T* TryGet(int hash)
	{
		auto it = items.find(hash);
		if(it != items.end())
			return it->second;
		return nullptr;
	}
	static T* Get(int hash)
	{
		T* item = T::TryGet(hash);
		if(item)
			return item;
		throw Format("Missing %s hash %d.", T::typeName, hash);
	}
	static T* TryGet(Cstring id)
	{
		return T::TryGet(Hash(id));
	}
	static T* Get(Cstring id)
	{
		T* item = T::TryGet(Hash(id));
		if(item)
			return item;
		throw Format("Missing %s '%s'.", T::typeName, id);
	}
	static T* GetS(const string& id)
	{
		T* item = T::TryGet(id);
		if(item)
			return item;
		throw ScriptException("Invalid %s '%s'.", T::typeName, id.c_str());
	}
};
