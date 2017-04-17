#include "Pch.h"
#include "Base.h"
#include "CheckBoxGroup.h"
#include "Label.h"
#include "ListBox.h"
#include "TextBox.h"
#include "Toolset.h"
#include "TreeView.h"
#include "TypeProxy.h"

const uint ID_MAX_LENGTH = 100;

bool ToolsetItem::AnyEntityChanges()
{
	if(!current)
		return false;

	if(current->state == TypeEntity::NEW)
		return true;

	for(auto& field : fields)
	{
		uint offset = field.field->GetOffset();
		switch(field.field->GetType())
		{
		case Type::Field::STRING:
		case Type::Field::MESH:
			if(field.text_box->GetText() != offset_cast<string>(current->item, offset))
				return true;
			break;
		case Type::Field::FLAGS:
			if(field.check_box_group->GetValue() != offset_cast<int>(current->item, offset))
				return true;
			break;
		case Type::Field::REFERENCE:
			{
				ListItem* item = field.list_box->GetItemCast<ListItem>();
				if(item->item != offset_cast<TypeItem*>(current->item, offset))
					return true;
			}
			break;
		}
	}

	return false;
}

void ToolsetItem::ApplyView(TypeEntity* entity)
{
	for(auto& field : fields)
	{
		uint offset = field.field->GetOffset();
		switch(field.field->GetType())
		{
		case Type::Field::STRING:
		case Type::Field::MESH:
			field.text_box->SetText(offset_cast<string>(entity->item, offset).c_str());
			break;
		case Type::Field::FLAGS:
			field.check_box_group->SetValue(offset_cast<int>(entity->item, offset));
			break;
		case Type::Field::REFERENCE:
			{
				TypeItem* ref_item = offset_cast<TypeItem*>(entity->item, offset);
				if(ref_item == nullptr)
					field.list_box->Select(0);
				else
					field.list_box->Select([ref_item](GuiElement* ge)
				{
					ListItem* item = (ListItem*)ge;
					return item->item == ref_item;
				});
			}
			break;
		}
	}
}

cstring ToolsetItem::GenerateEntityName(cstring name, bool dup)
{
	string old_name = name;
	int index = 1;

	if(!dup)
	{
		auto it = items.find(old_name);
		if(it == items.end())
			return name;
	}
	else
	{
		cstring pos = strrchr(name, '(');
		if(pos)
		{
			int new_index;
			if(sscanf(pos, "(%d)", &new_index) == 1)
			{
				index = new_index + 1;
				if(name != pos)
					old_name = string(name, pos - name - 1);
				else
					old_name = string(name, pos - name);
			}
		}
	}

	string new_name;
	while(true)
	{
		new_name = Format("%s (%d)", old_name.c_str(), index);
		auto it = items.find(new_name);
		if(it == items.end())
			return Format("%s", new_name.c_str());
		++index;
	}
}

void ToolsetItem::RemoveEntity(gui::TreeNode* node)
{
	if(node->IsDir())
	{
		for(auto child : node->GetChilds())
			RemoveEntity(child);
	}
	else
	{
		auto entity = node->GetData<TypeEntity>();
		if(entity)
		{
			items.erase(entity->id);
			if(entity->state == TypeEntity::NEW || entity->state == TypeEntity::NEW_ATTACHED)
			{
				entity->Remove(type);
				if(entity->state == TypeEntity::NEW_ATTACHED)
					--changes;
			}
			else
			{
				removed_items.push_back(entity);
				++changes;
			}
			UpdateCounter(-1);
			node->SetData(nullptr);
		}
	}
}

void ToolsetItem::RestoreEntity()
{
	if(!AnyEntityChanges())
		return;

	if(current->state == TypeEntity::NEW)
	{
		items.erase(current->id);
		current->Remove(type);
		current = nullptr;
		tree_view->GetCurrentNode()->Remove();
		UpdateCounter(-1);
		return;
	}

	ApplyView(current);
}

bool ToolsetItem::SaveEntity()
{
	if(!AnyEntityChanges())
		return true;

	if(!ValidateEntity())
		return false;

	TypeProxy proxy(type, current->item);

	const string& new_id = fields[0].text_box->GetText();
	if(new_id != current->id)
	{
		items.erase(current->id);
		current->id = new_id;
		items[new_id] = current;
		tree_view->GetCurrentNode()->SetText(new_id);
	}

	for(uint i = 1, count = fields.size(); i < count; ++i)
	{
		auto& field = fields[i];
		uint offset = field.field->GetOffset();
		switch(field.field->GetType())
		{
		case Type::Field::STRING:
		case Type::Field::MESH:
			offset_cast<string>(current->item, offset) = field.text_box->GetText();
			break;
		case Type::Field::FLAGS:
			offset_cast<int>(current->item, offset) = field.check_box_group->GetValue();
			break;
		case Type::Field::REFERENCE:
			{
				ListItem* item = field.list_box->GetItemCast<ListItem>();
				offset_cast<TypeItem*>(current->item, offset) = item->item;
			}
			break;
		}
	}

	UpdateEntityState(current);

	return true;
}

void ToolsetItem::UpdateEntityState(TypeEntity* e)
{
	switch(e->state)
	{
	case TypeEntity::UNCHANGED:
	case TypeEntity::CHANGED:
		{
			bool equal = type.Compare(e->item, e->old);
			TypeEntity::State new_state = (equal ? TypeEntity::UNCHANGED : TypeEntity::CHANGED);
			if(new_state != e->state)
			{
				e->state = new_state;
				changes += (new_state == TypeEntity::CHANGED ? +1 : -1);
			}
		}
		break;
	case TypeEntity::NEW:
		e->state = TypeEntity::NEW_ATTACHED;
		++changes;
		break;
	case TypeEntity::NEW_ATTACHED:
		break;
	}
}

void ToolsetItem::UpdateCounter(int change)
{
	counter += change;
	label_counter->SetText(Format("%s (%u)", type.GetName().c_str(), counter));
}

bool ToolsetItem::ValidateEntity()
{
	cstring err_msg = ValidateEntityId(fields[0].text_box->GetText(), current);
	if(err_msg)
	{
		GUI.SimpleDialog(Format("Validation failed\n----------------------\n%s", err_msg), &toolset);
		return false;
	}
	else
		return true;
}

cstring ToolsetItem::ValidateEntityId(const string& id, TypeEntity* e)
{
	if(id.length() < 1)
		return "Id must not be empty.";

	if(id.length() > ID_MAX_LENGTH)
		return Format("Id max length is %u.", ID_MAX_LENGTH);

	auto it = std::find_if(id.begin(), id.end(), [](char c) {return !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'); });
	if(it != id.end())
		return "Id can only contain ascii letters, numbers or underscore.";

	auto it2 = items.find(id);
	if(it2 != items.end() && it2->second != e)
		return "Id must be unique.";

	return nullptr;
}
