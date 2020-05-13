#pragma once

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
		T* item = TryGet(hash);
		if(item)
			return item;
		throw Format("Missing %s hash %d.", T::type_name, hash);
	}
	static T* TryGet(Cstring id)
	{
		return TryGet(Hash(id));
	}
	static T* Get(Cstring id)
	{
		T* item = TryGet(Hash(id));
		if(item)
			return item;
		return Format("Missing %s '%s'.", T::type_name, id);
	}
	static T* GetS(const string& id)
	{
		Recipe* recipe = TryGet(id);
		if(recipe)
			return recipe;
		throw ScriptException("Invalid %s '%s'.", T::type_name, id.c_str());
	}
};
