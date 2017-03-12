#pragma once

struct GameTypeProxy
{
	inline explicit GameTypeProxy(GameType& gametype, bool create = true, bool own = true) : gametype(gametype), own(own)
	{
		item = (create ? gametype.handler->Create() : nullptr);
	}
	inline ~GameTypeProxy()
	{
		if(item && own)
			gametype.handler->Destroy(item);
	}

	template<typename T>
	inline T& Get(uint offset)
	{
		return offset_cast<T>(item, offset);
	}

	inline string& GetId()
	{
		return Get<string>(gametype.fields[0]->offset);
	}

	inline cstring GetName()
	{
		if(item)
		{
			string& id = GetId();
			if(id.empty())
				return gametype.id.c_str();
			else
				return Format("%s %s", gametype.id.c_str(), id.c_str());
		}
		else
			return gametype.id.c_str();
	}

	GameTypeItem item;
	GameType& gametype;
	bool own;
};
