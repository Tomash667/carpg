#include "Pch.h"
#include "Base.h"
#include "Crc.h"
#include "Type.h"
#include "TypeManager.h"

Type::Type(TypeId type_id, cstring token, cstring name, cstring file_group) : type_id(type_id), token(token), name(name), file_group(file_group),
	container(nullptr), loaded(0), delete_container(true)
{

}

Type::~Type()
{
	DeleteElements(fields);
	DeleteElements(localized_fields);
	if(delete_container)
		delete container;
}

int Type::AddKeywords(std::initializer_list<tokenizer::KeywordToRegister> const& keywords, cstring group_name)
{
	return TypeManager::Get().AddKeywords(keywords, group_name);
}

//=================================================================================================
// Add id field (required, must be first)
//=================================================================================================
Type::Field& Type::AddId(uint offset, bool is_custom)
{
	assert(fields.empty());

	Field* f = new Field;
	f->type = Field::STRING;
	f->offset = offset;
	f->handler = dynamic_cast<CustomFieldHandler*>(this);
	f->friendly_name = "Id";

	fields.push_back(f);
	return *f;
}

//=================================================================================================
// Add string field
//=================================================================================================
Type::Field& Type::AddString(cstring name, uint offset)
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
Type::Field& Type::AddMesh(cstring name, uint id_offset, uint data_offset)
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
Type::Field& Type::AddFlags(cstring name, uint offset, std::initializer_list<FlagDTO> const& flags)
{
	assert(name);
	assert(!fields.empty());

	Field* f = new Field;
	f->name = name;
	f->type = Field::FLAGS;
	f->offset = offset;
	f->friendly_name = "Flags";
	f->flags = new vector<Field::Flag>;
	f->extra_name = Format("%s flags", token.c_str());
	f->keyword_group = TypeManager::Get().AddKeywords(flags, f->extra_name.c_str());

	auto v = new vector<Field::Flag>;
	v->resize(flags.size());
	uint index = 0;
	for(auto& flag : flags)
	{
		Field::Flag& f = v->at(index);
		f.id = flag.id;
		if(flag.name)
			f.name = flag.name;
		f.value = flag.value;
		++index;
	}
	f->flags = v;

	fields.push_back(f);
	return *f;
}

//=================================================================================================
// Add reference field (reference to other type)
//=================================================================================================
Type::Field& Type::AddReference(cstring name, TypeId type_id, uint offset)
{
	assert(name);
	assert(!fields.empty());

	Field* f = new Field;
	f->name = name;
	f->type = Field::REFERENCE;
	f->offset = offset;
	f->type_id = type_id;
	f->friendly_name = "Refernce";

	fields.push_back(f);
	return *f;
}

//=================================================================================================
// Add custom field
//=================================================================================================
Type::Field& Type::AddCustomField(cstring name, CustomFieldHandler* handler)
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
void Type::AddLocalizedString(cstring name, uint offset, bool required)
{
	assert(name);

	LocalizedField* lf = new LocalizedField;
	lf->name = name;
	lf->offset = offset;
	lf->required = required;

	localized_fields.push_back(lf);
}

//=================================================================================================
// Calculate crc for type using all items
//=================================================================================================
void Type::CalculateCrc()
{
	CRC32 _crc;

	for(auto item : container->ForEach())
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
					TypeItem* ref = offset_cast<TypeItem*>(item, field->offset);
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
	}

	crc = _crc.Get();
}

bool Type::Compare(TypeItem* item1, TypeItem* item2)
{
	assert(item1 && item2);

	for(Field* field : fields)
	{
		switch(field->type)
		{
		case Field::STRING:
		case Field::MESH:
			if(offset_cast<string>(item1, field->offset) != offset_cast<string>(item2, field->offset))
				return false;
			break;
		case Field::FLAGS:
			if(offset_cast<int>(item1, field->offset) != offset_cast<int>(item2, field->offset))
				return false;
			break;
		case Field::REFERENCE:
			if(offset_cast<TypeItem*>(item1, field->offset) != offset_cast<TypeItem*>(item2, field->offset))
				return false;
			break;
		case Field::CUSTOM:
		default:
			//assert(0);
			break;
		}
	}

	for(LocalizedField* field : localized_fields)
	{
		//assert(field->name.empty());
		//assert(0);
	}

	return true;
}

void Type::Copy(TypeItem* from, TypeItem* to)
{
	to->toolset_path = from->toolset_path;

	for(Field* field : fields)
	{
		switch(field->type)
		{
		case Field::STRING:
		case Field::MESH:
			offset_cast<string>(to, field->offset) = offset_cast<string>(from, field->offset);
			break;
		case Field::FLAGS:
			offset_cast<int>(to, field->offset) = offset_cast<int>(from, field->offset);
			break;
		case Field::REFERENCE:
			offset_cast<TypeItem*>(to, field->offset) = offset_cast<TypeItem*>(from, field->offset);
			break;
		case Field::CUSTOM:
		default:
			//assert(0);
			break;
		}
	}

	for(LocalizedField* field : localized_fields)
	{
		//assert(field->name.empty());
		//assert(0);
	}
}

TypeItem* Type::Duplicate(TypeItem* item)
{
	assert(item);
	TypeItem* new_item = Create();

	Copy(item, new_item);

	return new_item;
}
