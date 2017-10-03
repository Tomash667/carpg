#include "Pch.h"
#include "Core.h"
#include "BuildingScript.h"
#include "BuildingGroup.h"
#include "Building.h"

//-----------------------------------------------------------------------------
vector<BuildingScript*> BuildingScript::scripts;

//=================================================================================================
BuildingScript* BuildingScript::TryGet(const AnyString& id)
{
	for(auto script : scripts)
	{
		if(script->id == id)
			return script;
	}

	return nullptr;
}

//=================================================================================================
// Checks if building script have building from selected building group
bool BuildingScript::HaveBuilding(const string& group_id) const
{
	auto group = BuildingGroup::TryGet(group_id);
	if(!group)
		return false;
	for(Variant* v : variants)
	{
		if(!HaveBuilding(group, v))
			return false;
	}
	return true;
}

//=================================================================================================
// Checks if building script have building from selected building group
bool BuildingScript::HaveBuilding(BuildingGroup* group, Variant* variant) const
{
	assert(group && variant);

	enum IfStatus
	{
		IFS_IF, // if { ? }
		IFS_IF_OK, // if { OK }
		IFS_ELSE, // if { X } else { ? }
		IFS_ELSE_OK, // if { OK } else { ? }
		IFS_ELSE_OK2 // if { OK } else { OK }
	};

	const int* code = variant->code.data();
	const int* end = code + variant->code.size();
	vector<IfStatus> if_status;

	while(code != end)
	{
		Code c = (Code)*code++;

		switch(c)
		{
		case BS_ADD_BUILDING:
			if(IsEntryGroup(code, group))
			{
				if(if_status.empty())
					return true;
				else
				{
					IfStatus& s = if_status.back();
					if(s == IFS_IF)
						s = IFS_IF_OK;
					else if(s == IFS_ELSE_OK)
						s = IFS_ELSE_OK2;
				}
			}
			break;
		case BS_RANDOM:
			{
				int count = *code++;
				int ok = 0;
				for(int i = 0; i < count; ++i)
				{
					if(IsEntryGroup(code, group))
						++ok;
				}
				if(count == ok)
				{
					if(if_status.empty())
						return true;
					else
					{
						IfStatus& s = if_status.back();
						if(s == IFS_IF)
							s = IFS_IF_OK;
						else if(s == IFS_ELSE_OK)
							s = IFS_ELSE_OK2;
					}
				}
			}
			break;
		case BS_SHUFFLE_START:
		case BS_SHUFFLE_END:
		case BS_CALL:
		case BS_ADD:
		case BS_SUB:
		case BS_MUL:
		case BS_DIV:
		case BS_NEG:
			break;
		case BS_REQUIRED_OFF:
			// buildings that aren't required don't count for HaveBuilding
			return false;
		case BS_PUSH_INT:
		case BS_PUSH_VAR:
		case BS_SET_VAR:
		case BS_INC:
		case BS_DEC:
			++code;
			break;
		case BS_IF:
			if_status.push_back(IFS_IF);
			++code;
			break;
		case BS_IF_RAND:
			if_status.push_back(IFS_IF);
			break;
		case BS_ELSE:
			{
				IfStatus& s = if_status.back();
				if(s == IFS_IF)
					s = IFS_ELSE;
				else
					s = IFS_ELSE_OK;
			}
			break;
		case BS_ENDIF:
			{
				IfStatus s = if_status.back();
				if_status.pop_back();
				if(if_status.empty())
				{
					if(s == IFS_IF_OK || IFS_ELSE_OK2)
						return true;
				}
				else
				{
					IfStatus& s2 = if_status.back();
					if(s == IFS_IF_OK || IFS_ELSE_OK2)
					{
						if(s2 == IFS_IF)
							s2 = IFS_IF_OK;
						else if(s2 == IFS_ELSE_OK)
							s2 = IFS_ELSE_OK2;
					}
				}
			}
			break;
		default:
			assert(0);
			return false;
		}
	}

	return false;
}

//=================================================================================================
// Check if building or group belongs to building group in code
bool BuildingScript::IsEntryGroup(const int*& code, BuildingGroup* group) const
{
	Code type = (Code)*code++;
	if(type == BS_BUILDING)
	{
		Building* b = (Building*)*code++;
		return b->group == group;
	}
	else // type == BS_GROUP
	{
		BuildingGroup* g = (BuildingGroup*)*code++;
		return g == group;
	}
}
