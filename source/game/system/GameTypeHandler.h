#pragma once

//-----------------------------------------------------------------------------
typedef void* GameTypeItem;

//-----------------------------------------------------------------------------
// GameType handler create, contain and destroy object instances. Also have callbacks used by manager.
class GameTypeHandler
{
public:
	virtual ~GameTypeHandler() {}
	virtual void AfterLoad() {}
	virtual GameTypeItem Find(const string& id, bool hint) = 0;
	virtual GameTypeItem Create() = 0;
	virtual void Insert(GameTypeItem item) = 0;
	virtual void Destroy(GameTypeItem item) = 0;
	virtual GameTypeItem GetFirstItem() = 0;
	virtual GameTypeItem GetNextItem() = 0;
	virtual void Callback(GameTypeItem item, GameTypeItem ref_item, int type) {}
};

//-----------------------------------------------------------------------------
// GameType handler using vector
template<typename T>
class SimpleGameTypeHandler : public GameTypeHandler
{
	typedef typename vector<T*>::iterator iterator;
public:
	inline SimpleGameTypeHandler(vector<T*>& container, uint id_offset) : container(container), id_offset(id_offset) {}
	inline ~SimpleGameTypeHandler()
	{
		DeleteElements(container);
	}
	inline GameTypeItem Find(const string& id, bool hint) override
	{
		for(T* item : container)
		{
			string& item_id = offset_cast<string>(item, id_offset);
			if(item_id == id)
				return item;
		}
		return nullptr;
	}
	inline GameTypeItem Create() override
	{
		return new T;
	}
	inline void Insert(GameTypeItem item) override
	{
		container.push_back((T*)item);
	}
	inline void Destroy(GameTypeItem item) override
	{
		T* t = (T*)item;
		delete t;
	}
	inline GameTypeItem GetFirstItem() override
	{
		it = container.begin();
		end = container.end();
		if(it == end)
			return nullptr;
		else
			return *it++;
	}
	inline GameTypeItem GetNextItem() override
	{
		if(it == end)
			return nullptr;
		else
			return *it++;
	}

private:
	vector<T*>& container;
	uint id_offset;
	iterator it, end;
};
