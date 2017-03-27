#pragma once

#include "TypeId.h"

class CRC32;

typedef struct _TypeItem {} *TypeItem;
struct TypeEntity;

//-----------------------------------------------------------------------------
class Type
{
	friend class TypeManager;
	friend struct TypeProxy;

public:
	class Container
	{
		

	public:
		class Enumerator
		{
		public:
			virtual ~Enumerator() {}
			TypeItem GetCurrent() { return current; }
			virtual bool Next() = 0;

		protected:
			TypeItem current;
		};

		virtual ~Container() {}
		virtual void Add(TypeItem item) = 0;
		virtual Ptr<Enumerator> GetEnumerator() = 0;
		virtual uint Count() = 0;
		virtual TypeItem Find(const string& id) = 0;
		virtual void Merge(vector<TypeEntity*>& new_items, vector<TypeEntity*>& removed_items) = 0;

	private:
		struct EnumeratorProxy
		{
			struct Iterator
			{
				Iterator(Enumerator* enumerator) : enumerator(enumerator) {}

				bool operator != (const Iterator& it) const
				{
					return enumerator != it.enumerator;
				}

				void operator ++ ()
				{
					assert(enumerator);
					enumerator->Next();
					if(enumerator->GetCurrent() == nullptr)
						enumerator = nullptr;
				}

				TypeItem operator * ()
				{
					return enumerator->GetCurrent();
				}

				Enumerator* enumerator;
			};

			EnumeratorProxy(Ptr<Enumerator>& enumerator) : enumerator(enumerator.Pin()) {}
			~EnumeratorProxy() { delete enumerator; }
			Iterator begin() { return Iterator(enumerator); }
			Iterator end() { return Iterator(nullptr); }

			Enumerator* enumerator;
		};

	public: 
		EnumeratorProxy ForEach()
		{
			return EnumeratorProxy(GetEnumerator());
		}
	};

	class CustomFieldHandler
	{
	public:
		virtual ~CustomFieldHandler() {}
		virtual void LoadText(Tokenizer& t, TypeItem item) = 0;
		virtual void UpdateCrc(CRC32& crc, TypeItem item) = 0;
	};

	class Field
	{
		friend class Type;
		friend class TypeManager;
		friend struct TypeProxy;

		enum Type
		{
			STRING, // offset, handler (for ID)
			MESH, // offset, data_offset
			FLAGS, // offset, keyword_group
			REFERENCE, // offset, type_id
			CUSTOM, // handler
		};

		string name, friendly_name;
		Type type;
		union
		{
			uint offset;
		};
		union
		{
			uint data_offset;
			uint keyword_group;
			struct
			{
				TypeId type_id;
				uint id_offset;
			};
			CustomFieldHandler* handler;
		};
		int callback;
		bool required, alias;

	public:
		Field() : required(true), callback(-1)
		{

		}

		~Field()
		{
			if(type == CUSTOM || type == STRING)
				delete handler;
		}

		Field& NotRequired()
		{
			required = false;
			return *this;
		}

		Field& Callback(int _type)
		{
			assert(type == REFERENCE);
			callback = _type;
			return *this;
		}

		Field& Alias()
		{
			assert(type == STRING);
			alias = true;
			return *this;
		}

		Field& FriendlyName(cstring new_friendly_name)
		{
			assert(new_friendly_name);
			friendly_name = new_friendly_name;
			return *this;
		}

		const string& GetFriendlyName() const
		{
			if(friendly_name.empty())
				return name;
			else
				return friendly_name;
		}
	};

	class LocalizedField
	{
		friend class Type;
		friend class TypeManager;
		friend struct TypeProxy;

		string name;
		uint offset;
		bool required;
	};

	Type(TypeId type_id, cstring token, cstring name, cstring file_group);
	virtual ~Type();

	int AddKeywords(std::initializer_list<tokenizer::KeywordToRegister> const & keywords, cstring group_name = nullptr);
	Field& AddId(uint offset, bool is_custom = false);
	Field& AddString(cstring name, uint offset);
	Field& AddMesh(cstring name, uint id_offset, uint data_offset);
	Field& AddFlags(cstring name, uint offset, uint keyword_group);
	Field& AddReference(cstring name, TypeId type_id, uint offset);
	Field& AddCustomField(cstring name, CustomFieldHandler* handler);
	void AddLocalizedString(cstring name, uint offset, bool required = true);


	virtual void AfterLoad() {}
	virtual bool Compare(TypeItem item1, TypeItem item2);
	virtual void Copy(TypeItem from, TypeItem to);
	virtual TypeItem Create() = 0;
	virtual void Destroy(TypeItem item) = 0;
	virtual TypeItem Duplicate(TypeItem item);
	virtual void ReferenceCallback(TypeItem item, TypeItem ref_item, int type) {}

	void DependsOn(TypeId dependency) { depends_on.push_back(dependency); }
	Container* GetContainer() { return container; }
	uint GetCrc() { return crc; }
	uint GetIdOffset() const { return fields[0]->offset; }
	string& GetItemId(TypeItem item) { return offset_cast<string>(item, GetIdOffset()); }
	const string& GetName() { return name; }
	const string& GetToken() { return token; }
	TypeId GetTypeId() { return type_id; }

	Container::Enumerator* FindByAlias(const string& alias, Container::Enumerator* enumerator);

protected:
	void CalculateCrc();

	TypeId type_id;
	string token, name, group_name, file_group;
	Container* container;
	vector<Field*> fields;
	vector<LocalizedField*> localized_fields;
	vector<TypeId> depends_on;
	uint required_fields, required_localized_fields, group, localized_group, crc, other_crc, loaded;
	int alias;
	bool processed, changes;
};

//-----------------------------------------------------------------------------
template<typename T>
class TypeImpl : public Type
{
public:
	TypeImpl(TypeId type_id, cstring token, cstring name, cstring file_group) : Type(type_id, token, name, file_group)
	{

	}

	TypeItem Create() override
	{
		T* it = new T;
		return (TypeItem)it;
	}

	void Destroy(TypeItem item)
	{
		T* it = (T*)item;
		delete it;
	}
};

//-----------------------------------------------------------------------------
struct TypeEntity
{
	enum State
	{
		UNCHANGED,
		CHANGED,
		NEW,
		NEW_ATTACHED
	};

	TypeItem item, old;
	State state;
	string& id;

	TypeEntity(TypeItem item, TypeItem old, State state, string& id) : item(item), old(old), state(state), id(id) {}
};
