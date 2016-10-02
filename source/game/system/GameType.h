#pragma once

//-----------------------------------------------------------------------------
#include "Crc.h"
#include "GameTypeId.h"
#include "GameTypeHandler.h"

//-----------------------------------------------------------------------------
class GameType
{
	friend class GameTypeManager;
	friend struct GameTypeProxy;

public:
	class CustomFieldHandler
	{
	public:
		virtual ~CustomFieldHandler() {}
		virtual void LoadText(Tokenizer& t, GameTypeItem item) = 0;
		virtual void UpdateCrc(CRC32& crc, GameTypeItem item) = 0;
	};

	class Field
	{
		friend class GameTypeManager;
		friend class GameType;
		friend struct GameTypeProxy;

		enum Type
		{
			STRING, // offset, handler (for ID)
			MESH, // offset, data_offset
			FLAGS, // offset, keyword_group
			REFERENCE, // offset, gametype_id
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
				GameTypeId gametype_id;
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
		friend class GameTypeManager;
		friend class GameType;
		friend struct GameTypeProxy;

		string name;
		uint offset;
		bool required;
	};

	GameType(GameTypeId gametype_id, cstring id) : gametype_id(gametype_id), id(id), handler(nullptr), loaded(0)
	{

	}
	~GameType()
	{
		DeleteElements(fields);
		DeleteElements(localized_fields);
		delete handler;
	}

	Field& AddId(uint offset, CustomFieldHandler* handler = nullptr);
	Field& AddString(cstring name, uint offset);
	Field& AddMesh(cstring name, uint id_offset, uint data_offset);
	Field& AddFlags(cstring name, uint offset, uint keyword_group);
	Field& AddReference(cstring name, GameTypeId gametype_id, uint offset);
	Field& AddCustomField(cstring name, CustomFieldHandler* handler);
	void AddLocalizedString(cstring name, uint offset, bool required = true);


	// Return calculated CRC for all objects of this type. Calculated in GameTypeManager.CalculateCrc.
	inline uint GetCrc() const
	{
		return crc;
	}
	// Return gametype handler.
	inline GameTypeHandler* GetHandler() const
	{
		return handler;
	}

	inline uint GetIdOffset() const { return fields[0]->offset; }
	inline const string& GetId() const { return id; }

	// Set gametype handler.
	inline void SetHandler(GameTypeHandler* _handler)
	{
		assert(_handler);
		handler = _handler;
	}
	// Set gametype handler using SimpleGameTypeHandler.
	template<typename T>
	inline void SetHandler(vector<T>& container)
	{
		handler = new SimpleGameTypeHandler()
	}

private:
	void CalculateCrc();

	GameTypeId gametype_id;
	string id, group_name;
	GameTypeHandler* handler;
	vector<Field*> fields;
	vector<LocalizedField*> localized_fields;
	uint required_fields, required_localized_fields, group, localized_group, crc, other_crc, loaded;
	int alias;
};
