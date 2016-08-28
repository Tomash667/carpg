#include "Pch.h"
#include "Base.h"
#include "DatatypeManager.h"

//=================================================================================================
// Add id field (required, must be first)
//=================================================================================================
Datatype::Field& Datatype::AddId(uint offset, CustomFieldHandler* handler)
{
	assert(fields.empty());

	Field* f = new Field;
	f->type = Field::STRING;
	f->offset = offset;
	f->handler = handler;

	fields.push_back(f);
	return *f;
}

//=================================================================================================
// Add string field
//=================================================================================================
Datatype::Field& Datatype::AddString(cstring name, uint offset)
{
	assert(name);
	assert(!fields.empty());

	Field* f = new Field;
	f->name = name;
	f->type = Field::STRING;
	f->offset = offset;
	f->handler = nullptr;
	
	fields.push_back(f);
	return *f;
}

//=================================================================================================
// Add mesh field
//=================================================================================================
Datatype::Field& Datatype::AddMesh(cstring name, uint id_offset, uint data_offset)
{
	assert(name);
	assert(!fields.empty());

	Field* f = new Field;
	f->name = name;
	f->type = Field::MESH;
	f->offset = id_offset;
	f->data_offset = data_offset;

	fields.push_back(f);
	return *f;
}

//=================================================================================================
// Add flags field
//=================================================================================================
Datatype::Field& Datatype::AddFlags(cstring name, uint offset, uint keyword_group)
{
	assert(name);
	assert(!fields.empty());

	Field* f = new Field;
	f->name = name;
	f->type = Field::FLAGS;
	f->offset = offset;
	f->keyword_group = keyword_group;

	fields.push_back(f);
	return *f;
}

//=================================================================================================
// Add reference field (reference to other datatype)
//=================================================================================================
Datatype::Field& Datatype::AddReference(cstring name, DatatypeId datatype_id, uint offset)
{
	assert(name);
	assert(!fields.empty());

	Field* f = new Field;
	f->name = name;
	f->type = Field::REFERENCE;
	f->offset = offset;
	f->datatype_id = datatype_id;

	fields.push_back(f);
	return *f;
}

//=================================================================================================
// Add custom field
//=================================================================================================
Datatype::Field& Datatype::AddCustomField(cstring name, CustomFieldHandler* handler)
{
	assert(name);
	assert(handler);
	assert(!fields.empty());

	Field* f = new Field;
	f->name = name;
	f->type = Field::CUSTOM;
	f->handler = handler;

	fields.push_back(f);
	return *f;
}

//=================================================================================================
// Add localized string
//=================================================================================================
void Datatype::AddLocalizedString(cstring name, uint offset, bool required)
{
	assert(name);

	LocalizedField* lf = new LocalizedField;
	lf->name = name;
	lf->offset = offset;
	lf->required = required;

	localized_fields.push_back(lf);
}

//=================================================================================================
// Calculate crc for datatype using all items
//=================================================================================================
void Datatype::CalculateCrc()
{
	CRC32 _crc;

	DatatypeItem item = handler->GetFirstItem();
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
					DatatypeItem ref = offset_cast<DatatypeItem>(item, field->offset);
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
