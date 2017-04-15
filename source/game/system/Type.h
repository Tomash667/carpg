#pragma once

#include "TypeId.h"
#include "TypeItem.h"

class CRC32;
class TextWriter;
struct TypeEntity;

//-----------------------------------------------------------------------------
class Type
{
	friend class Toolset;
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
			TypeItem* GetCurrent() { return current; }
			virtual bool Next() = 0;

		protected:
			TypeItem* current;
		};

		virtual ~Container() {}
		virtual void Add(TypeItem* item) = 0;
		virtual Ptr<Enumerator> GetEnumerator() = 0;
		virtual uint Count() = 0;
		virtual TypeItem* Find(const string& id) = 0;
		virtual void Merge(vector<TypeEntity*>& new_items, vector<TypeEntity*>& removed_items) = 0;

	private:
		struct EnumeratorProxy
		{
			struct Iterator
			{
				Iterator(Enumerator* _enumerator) : enumerator(_enumerator)
				{
					if(_enumerator && _enumerator->GetCurrent() == nullptr)
						enumerator = nullptr;
				}

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

				TypeItem* operator * ()
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
		virtual void SaveText(TextWriter& t, TypeItem* item, uint offset) = 0;
		virtual void LoadText(Tokenizer& t, TypeItem* item, uint offset) = 0;
		virtual void UpdateCrc(CRC32& crc, TypeItem* item, uint offset) = 0;
		virtual bool Compare(TypeItem* item1, TypeItem* item2, uint offset) = 0;
		virtual void Copy(TypeItem* from, TypeItem* to, uint offset) = 0;
	};

	class Field
	{
	public:
		friend class Toolset;
		friend class Type;
		friend class TypeManager;
		friend struct TypeProxy;

		enum Type
		{
			STRING, // offset, handler (for ID)
			MESH, // offset, data_offset
			FLAGS, // offset, keyword_group
			REFERENCE, // offset, type_id
			CUSTOM, // offset, handler
		};

	private:
		struct Flag
		{
			string id, name;
			int value;
		};

		string name, friendly_name, extra_name;
		Type type;
		union
		{
			uint offset;
		};
		union
		{
			uint data_offset;
			struct
			{
				uint keyword_group;
				vector<Flag>* flags;
			};
			struct
			{
				TypeId type_id;
				uint id_offset;
			};
			CustomFieldHandler* handler;
		};
		int callback;
		bool required, is_id;

		cstring GetFlag(int value)
		{
			for(auto& flag : *flags)
			{
				if(flag.value == value)
					return flag.id.c_str();
			}
			return nullptr;
		}

	public:
		Field() : required(true), callback(-1)
		{

		}

		~Field();

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

		CustomFieldHandler* GetCustomHandler() const { assert(type == CUSTOM); return handler; }
		uint GetOffset() const { return offset; }
		Type GetType() const { return type; }
	};

	class LocalizedField
	{
		friend class Toolset;
		friend class Type;
		friend class TypeManager;
		friend struct TypeProxy;

		string name;
		uint offset;
		bool required;
	};

	struct FlagDTO
	{
		FlagDTO(cstring id, int value, cstring name = nullptr) : id(id), value(value), name(name) {}

		cstring id, name;
		int value;
	};

	Type(TypeId type_id, cstring token, cstring name, cstring file_group);
	virtual ~Type();

	int AddKeywords(std::initializer_list<tokenizer::KeywordToRegister> const& keywords, cstring group_name = nullptr);
	Field& AddId(uint offset, bool is_custom = false);
	Field& AddString(cstring name, uint offset);
	Field& AddMesh(cstring name, uint id_offset, uint data_offset);
	Field& AddFlags(cstring name, uint offset, std::initializer_list<FlagDTO> const& flags);
	Field& AddReference(cstring name, TypeId type_id, uint offset);
	Field& AddCustomField(cstring name, CustomFieldHandler* handler, uint offset = 0);
	void AddLocalizedString(cstring name, uint offset, bool required = true);


	// called after loading everything
	virtual void AfterLoad() {}
	// called after every item loaded, when verifing in toolset
	virtual cstring Prepare(TypeItem* item) { return nullptr; }
	// return true if two items are equal
	virtual bool Compare(TypeItem* item1, TypeItem* item2);
	// copy item
	virtual void Copy(TypeItem* from, TypeItem* to);
	// create new item
	virtual TypeItem* Create() = 0;
	// destroy item
	virtual void Destroy(TypeItem* item) = 0;
	// duplicate item
	virtual TypeItem* Duplicate(TypeItem* item);
	// callback when other item references this item
	virtual void ReferenceCallback(TypeItem* item, TypeItem* ref_item, int type) {}
	// custom crc calculation
	virtual void UpdateCrc(CRC32& crc, TypeItem* item) {}

	void DependsOn(TypeId dependency) { depends_on.push_back(dependency); }
	Container* GetContainer() { return container; }
	uint GetCrc() { return crc; }
	uint GetIdOffset() const { return fields[0]->offset; }
	string& GetItemId(TypeItem* item) { return offset_cast<string>(item, GetIdOffset()); }
	const string& GetName() { return name; }
	const string& GetToken() { return token; }
	TypeId GetTypeId() { return type_id; }
	void HaveCustomCrc() { custom_crc = true; }

protected:
	void CalculateCrc();
	void DeleteContainer();

	TypeId type_id;
	string token, name, group_name, file_group;
	Container* container;
	vector<Field*> fields;
	vector<LocalizedField*> localized_fields;
	vector<TypeId> depends_on;
	uint required_fields, required_localized_fields, group, localized_group, crc, other_crc, loaded;
	bool processed, changes, delete_container, custom_crc;
};

//-----------------------------------------------------------------------------
template<typename T>
class TypeImpl : public Type
{
public:
	TypeImpl(TypeId type_id, cstring token, cstring name, cstring file_group) : Type(type_id, token, name, file_group)
	{

	}

	TypeItem* Create() override
	{
		T* it = new T;
		return it;
	}

	void Destroy(TypeItem* item)
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

	TypeItem* item, *old;
	State state;
	string& id;

	TypeEntity(TypeItem* item, TypeItem* old, State state, string& id) : item(item), old(old), state(state), id(id) {}
	void Remove(Type& type)
	{
		if(item)
			type.Destroy(item);
		delete this;
	}
};
