#pragma once

#include "Type.h"

struct TypeProxy
{
	explicit TypeProxy(Type& type, bool create = true, bool own = true) : type(type), own(own)
	{
		item = (create ? type.Create() : nullptr);
	}
	TypeProxy(Type& type, TypeItem item) : type(type), item(item), own(false)
	{
	}
	~TypeProxy()
	{
		if(item && own)
			type.Destroy(item);
	}

	template<typename T>
	T& Get(uint offset)
	{
		return offset_cast<T>(item, offset);
	}

	string& GetId()
	{
		return Get<string>(type.fields[0]->offset);
	}

	cstring GetName()
	{
		if(item)
		{
			string& id = GetId();
			if(id.empty())
				return type.token.c_str();
			else
				return Format("%s %s", type.token.c_str(), id.c_str());
		}
		else
			return type.token.c_str();
	}

	Type& type;
	TypeItem item;
	bool own;
};
