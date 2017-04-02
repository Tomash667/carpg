#include "Pch.h"
#include "Base.h"
#include "TypeVectorContainer.h"

TypeVectorContainer::Enumerator::Enumerator(Vector& v)
{
	it = v.begin();
	end = v.end();
	if(it != end)
		current = *it;
	else
		current = nullptr;
}

bool TypeVectorContainer::Enumerator::Next()
{
	if(current == nullptr)
		return false;
	++it;
	if(it == end)
	{
		current = nullptr;
		return false;
	}
	else
	{
		current = *it;
		return true;
	}
}

TypeVectorContainer::TypeVectorContainer(Type& type, Vector& items) : type(type), items(items)
{
	id_offset = type.GetIdOffset();
}

TypeVectorContainer::~TypeVectorContainer()
{
	for(auto item : items)
		type.Destroy(item);
}

void TypeVectorContainer::Add(TypeItem* item)
{
	assert(item);
	items.push_back(item);
}

Ptr<Type::Container::Enumerator> TypeVectorContainer::GetEnumerator()
{
	return Ptr<Type::Container::Enumerator>(new Enumerator(items));
}

uint TypeVectorContainer::Count()
{
	return items.size();
}

TypeItem* TypeVectorContainer::Find(const string& id)
{
	for(auto item : items)
	{
		string item_id = offset_cast<string>(item, id_offset);
		if(item_id == id)
			return item;
	}

	return nullptr;
}

void TypeVectorContainer::Merge(vector<TypeEntity*>& new_items, vector<TypeEntity*>& removed_items)
{
	// cleanup removed items
	for(auto e : removed_items)
	{
		type.Destroy(e->item);
		type.Destroy(e->old);
		delete e;
	}
	removed_items.clear();

	// add new items
	items.clear();
	items.reserve(new_items.size());
	for(auto e : new_items)
	{
		if(e->state == TypeEntity::CHANGED)
		{
			type.Copy(e->item, e->old);
			e->state = TypeEntity::UNCHANGED;
			items.push_back(e->old);
		}
		else if(e->state == TypeEntity::NEW_ATTACHED)
		{
			e->old = e->item;
			e->item = type.Duplicate(e->old);
			e->state = TypeEntity::UNCHANGED;
		}
		items.push_back(e->old);
	}
}
