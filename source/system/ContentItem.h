#pragma once

//-----------------------------------------------------------------------------
#include "ScriptException.h"

//-----------------------------------------------------------------------------
template<typename T>
struct ContentItem
{
	ContentItem() : hash(0) {}

	uint hash;
	string id;

	inline static vector<T*> items;
	inline static std::unordered_map<uint, T*> hashes;

	static T* TryGet(uint hash)
	{
		auto it = hashes.find(hash);
		if(it != hashes.end())
			return it->second;
		return nullptr;
	}

	static T* Get(uint hash)
	{
		T* item = T::TryGet(hash);
		if(item)
			return item;
		throw Format("Missing %s hash %u.", T::typeName, hash);
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
