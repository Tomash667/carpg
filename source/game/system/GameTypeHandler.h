#pragma once

//-----------------------------------------------------------------------------
typedef void* GameTypeItem;

//-----------------------------------------------------------------------------
// GameType handler create, contain and destroy object instances. Also have callbacks used by manager.
class GameTypeHandler
{
public:
	struct Enumerator
	{
		struct Iterator
		{
			Iterator(GameTypeHandler* handler, GameTypeItem item) : handler(handler), item(item) {}

			bool operator != (const Iterator& it) const
			{
				assert(handler == it.handler);
				return item != it.item;
			}

			Iterator& operator ++ ()
			{
				assert(item);
				item = handler->GetNextItem();
				return *this;
			}

			GameTypeItem operator * ()
			{
				return item;
			}

		private:
			GameTypeHandler* handler;
			GameTypeItem item;
		};

		Enumerator(GameTypeHandler* handler) : handler(handler) {}

		Iterator begin() { return Iterator(handler, handler->GetFirstItem()); }
		Iterator end() { return Iterator(handler, nullptr); }

	private:
		GameTypeHandler* handler;
	};

	virtual ~GameTypeHandler() {}
	virtual void AfterLoad() {}
	virtual GameTypeItem Find(const string& id, bool hint) = 0;
	virtual GameTypeItem Create() = 0;
	virtual void Insert(GameTypeItem item) = 0;
	virtual void Destroy(GameTypeItem item) = 0;
	virtual GameTypeItem GetFirstItem() = 0;
	virtual GameTypeItem GetNextItem() = 0;
	virtual void Callback(GameTypeItem item, GameTypeItem ref_item, int type) {}
	virtual uint Count() = 0;

	Enumerator ForEach()
	{
		return Enumerator(this);
	}
};

//-----------------------------------------------------------------------------
// GameType handler using vector
template<typename T>
class SimpleGameTypeHandler : public GameTypeHandler
{
	typedef typename vector<T*>::iterator iterator;
public:
	SimpleGameTypeHandler(vector<T*>& container, uint id_offset) : container(container), id_offset(id_offset) {}
	~SimpleGameTypeHandler()
	{
		DeleteElements(container);
	}
	GameTypeItem Find(const string& id, bool hint) override
	{
		for(T* item : container)
		{
			string& item_id = offset_cast<string>(item, id_offset);
			if(item_id == id)
				return item;
		}
		return nullptr;
	}
	GameTypeItem Create() override
	{
		return new T;
	}
	void Insert(GameTypeItem item) override
	{
		container.push_back((T*)item);
	}
	void Destroy(GameTypeItem item) override
	{
		T* t = (T*)item;
		delete t;
	}
	GameTypeItem GetFirstItem() override
	{
		it = container.begin();
		end = container.end();
		if(it == end)
			return nullptr;
		else
			return *it++;
	}
	GameTypeItem GetNextItem() override
	{
		if(it == end)
			return nullptr;
		else
			return *it++;
	}
	uint Count() override
	{
		return container.size();
	}

private:
	vector<T*>& container;
	uint id_offset;
	iterator it, end;
};
