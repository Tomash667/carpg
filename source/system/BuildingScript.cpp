#include "Pch.h"
#include "BuildingScript.h"

#include "Building.h"
#include "BuildingGroup.h"

//-----------------------------------------------------------------------------
vector<BuildingScript*> BuildingScript::scripts;

//=================================================================================================
BuildingScript* BuildingScript::TryGet(Cstring id)
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
bool BuildingScript::HaveBuilding(const string& groupId) const
{
	auto group = BuildingGroup::TryGet(groupId);
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
	vector<IfStatus> ifStatus;

	while(code != end)
	{
		Code c = (Code)*code++;

		switch(c)
		{
		case BS_ADD_BUILDING:
			if(IsEntryGroup(code, group))
			{
				if(ifStatus.empty())
					return true;
				else
				{
					IfStatus& s = ifStatus.back();
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
					if(ifStatus.empty())
						return true;
					else
					{
						IfStatus& s = ifStatus.back();
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
			ifStatus.push_back(IFS_IF);
			++code;
			break;
		case BS_IF_RAND:
			ifStatus.push_back(IFS_IF);
			break;
		case BS_ELSE:
			{
				IfStatus& s = ifStatus.back();
				if(s == IFS_IF)
					s = IFS_ELSE;
				else
					s = IFS_ELSE_OK;
			}
			break;
		case BS_ENDIF:
			{
				IfStatus s = ifStatus.back();
				ifStatus.pop_back();
				if(ifStatus.empty())
				{
					if(s == IFS_IF_OK || s == IFS_ELSE_OK2)
						return true;
				}
				else
				{
					IfStatus& s2 = ifStatus.back();
					if(s == IFS_IF_OK || s == IFS_ELSE_OK2)
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
