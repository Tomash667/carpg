#pragma once

//-----------------------------------------------------------------------------
#include "Crc.h"

//-----------------------------------------------------------------------------
class Datatype;

//-----------------------------------------------------------------------------
typedef void* DatatypeItem;

//-----------------------------------------------------------------------------
enum DatatypeId
{
	DT_Unit,
	DT_BuildingGroup,
	DT_Building,
	DT_BuildingScript,
	DT_Max
};

//-----------------------------------------------------------------------------
class CustomFieldHandler
{
public:
	virtual ~CustomFieldHandler() {}
	virtual void LoadText(Tokenizer& t, DatatypeItem item) = 0;
	virtual void UpdateCrc(CRC32& crc, DatatypeItem item) = 0;
};

//-----------------------------------------------------------------------------
// Datatype handler create, contain and destroy object instances. Also have callbacks used by manager.
class DatatypeHandler
{
public:
	virtual ~DatatypeHandler() {}
	virtual void AfterLoad() {}
	virtual DatatypeItem Find(const string& id, bool hint) = 0;
	virtual DatatypeItem Create() = 0;
	virtual void Insert(DatatypeItem item) = 0;
	virtual void Destroy(DatatypeItem item) = 0;
	virtual DatatypeItem GetFirstItem() = 0;
	virtual DatatypeItem GetNextItem() = 0;
	virtual void Callback(DatatypeItem item, DatatypeItem ref_item, int type) {}
};

//-----------------------------------------------------------------------------
class Datatype
{
	friend class DatatypeManager;
	friend struct DatatypeProxy;

public:
	class Field
	{
		friend class DatatypeManager;
		friend class Datatype;
		friend struct DatatypeProxy;

		enum Type
		{
			STRING, // offset, handler (for ID)
			MESH, // offset, data_offset
			FLAGS, // offset, keyword_group
			REFERENCE, // offset, datatype_id
			CUSTOM, // handler
		};

		string name;
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
				DatatypeId datatype_id;
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

		inline Field& NotRequired()
		{
			required = false;
			return *this;
		}

		inline Field& Callback(int _type)
		{
			assert(type == REFERENCE);
			callback = _type;
			return *this;
		}

		inline Field& Alias()
		{
			assert(type == STRING);
			alias = true;
			return *this;
		}
	};

	class LocalizedField
	{
		friend class DatatypeManager;
		friend class Datatype;
		friend struct DatatypeProxy;

		string name;
		uint offset;
		bool required;
	};

	Datatype(DatatypeId datatype_id, cstring id) : datatype_id(datatype_id), id(id), handler(nullptr), loaded(0)
	{

	}
	~Datatype()
	{
		DeleteElements(fields);
		DeleteElements(localized_fields);
		delete handler;
	}

	Field& AddId(uint offset, CustomFieldHandler* handler = nullptr);
	Field& AddString(cstring name, uint offset);
	Field& AddMesh(cstring name, uint id_offset, uint data_offset);
	Field& AddFlags(cstring name, uint offset, uint keyword_group);
	Field& AddReference(cstring name, DatatypeId datatype_id, uint offset);
	Field& AddCustomField(cstring name, CustomFieldHandler* handler);
	void AddLocalizedString(cstring name, uint offset, bool required = true);


	// Return calculated CRC for all objects of this type. Calculated in DatatypeManager.CalculateCrc.
	inline uint GetCrc() const
	{
		return crc;
	}
	// Return datatype handler.
	inline DatatypeHandler* GetHandler() const
	{
		return handler;
	}

	inline uint GetIdOffset() const { return fields[0]->offset; }

	// Set datatype handler.
	inline void SetHandler(DatatypeHandler* _handler)
	{
		assert(_handler);
		handler = _handler;
	}
	// Set datatype handler using SimpleDatatypeHandler.
	template<typename T>
	inline void SetHandler(vector<T>& container)
	{
		handler = new SimpleDatatypeHandler()
	}

private:
	void CalculateCrc();

	DatatypeId datatype_id;
	string id, group_name;
	DatatypeHandler* handler;
	vector<Field*> fields;
	vector<LocalizedField*> localized_fields;
	uint required_fields, required_localized_fields, group, localized_group, crc, other_crc, loaded;
	int alias;
};
