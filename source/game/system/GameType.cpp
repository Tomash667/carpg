#include "Pch.h"
#include "Base.h"
#include "GameTypeManager.h"

//=================================================================================================
// Add id field (required, must be first)
//=================================================================================================
GameType::Field& GameType::AddId(uint offset, CustomFieldHandler* handler)
{
	assert(fields.empty());

	Field* f = new Field;
	f->type = Field::STRING;
	f->offset = offset;
	f->handler = handler;
	f->friendly_name = "Id";

	fields.push_back(f);
	return *f;
}

//=================================================================================================
// Add string field
//=================================================================================================
GameType::Field& GameType::AddString(cstring name, uint offset)
{
	assert(name);
	assert(!fields.empty());

	Field* f = new Field;
	f->name = name;
	f->type = Field::STRING;
	f->offset = offset;
	f->handler = nullptr;
	f->friendly_name = "Text";
	
	fields.push_back(f);
	return *f;
}

//=================================================================================================
// Add mesh field
//=================================================================================================
GameType::Field& GameType::AddMesh(cstring name, uint id_offset, uint data_offset)
{
	assert(name);
	assert(!fields.empty());

	Field* f = new Field;
	f->name = name;
	f->type = Field::MESH;
	f->offset = id_offset;
	f->data_offset = data_offset;
	f->friendly_name = "Mesh";

	fields.push_back(f);
	return *f;
}

//=================================================================================================
// Add flags field
//=================================================================================================
GameType::Field& GameType::AddFlags(cstring name, uint offset, uint keyword_group)
{
	assert(name);
	assert(!fields.empty());

	Field* f = new Field;
	f->name = name;
	f->type = Field::FLAGS;
	f->offset = offset;
	f->keyword_group = keyword_group;
	f->friendly_name = "Flags";

	fields.push_back(f);
	return *f;
}

//=================================================================================================
// Add reference field (reference to other gametype)
//=================================================================================================
GameType::Field& GameType::AddReference(cstring name, GameTypeId gametype_id, uint offset)
{
	assert(name);
	assert(!fields.empty());

	Field* f = new Field;
	f->name = name;
	f->type = Field::REFERENCE;
	f->offset = offset;
	f->gametype_id = gametype_id;
	f->friendly_name = "Refernce";

	fields.push_back(f);
	return *f;
}

//=================================================================================================
// Add custom field
//=================================================================================================
GameType::Field& GameType::AddCustomField(cstring name, CustomFieldHandler* handler)
{
	assert(name);
	assert(handler);
	assert(!fields.empty());

	Field* f = new Field;
	f->name = name;
	f->type = Field::CUSTOM;
	f->handler = handler;
	f->friendly_name = "Custom";

	fields.push_back(f);
	return *f;
}

//=================================================================================================
// Add localized string
//=================================================================================================
void GameType::AddLocalizedString(cstring name, uint offset, bool required)
{
	assert(name);

	LocalizedField* lf = new LocalizedField;
	lf->name = name;
	lf->offset = offset;
	lf->required = required;

	localized_fields.push_back(lf);
}

//=================================================================================================
// Calculate crc for gametype using all items
//=================================================================================================
void GameType::CalculateCrc()
{
	CRC32 _crc;

	GameTypeItem item = handler->GetFirstItem();
	while(item)
	{
		for(Field* field : fields)
		{
			switch(field->type)
			{
			case Field::STRING:
			case Field::MESH:
				_crc.Update(offset_cast<string>(item, field->offset));
				break;
			case Field::FLAGS:
				_crc.Update(offset_cast<uint>(item, field->offset));
				break;
			case Field::REFERENCE:
				{
					GameTypeItem ref = offset_cast<GameTypeItem>(item, field->offset);
					if(ref)
						_crc.Update(offset_cast<string>(ref, field->id_offset));
					else
						_crc.Update0();
				}
				break;
			case Field::CUSTOM:
				field->handler->UpdateCrc(_crc, item);
				break;
			}
		}

		item = handler->GetNextItem();
	}

	crc = _crc.Get();
}
