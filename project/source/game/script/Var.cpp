#include "Pch.h"
#include "GameCore.h"
#include "Var.h"
#include "Item.h"
#include "Location.h"
#include "World.h"
#include "Encounter.h"
#include "GroundItem.h"
#include "SaveState.h"
#pragma warning(error: 4062)

string VarsContainer::tmp_str;

//=================================================================================================
VarsContainer::~VarsContainer()
{
	DeleteElements(vars);
}

//=================================================================================================
Var* VarsContainer::Add(Var::Type type, const string& name, bool registered)
{
	Var* var = new Var;
	var->type = type;
	var->registered = registered;
	vars[name] = var;
	return var;
}

//=================================================================================================
Var* VarsContainer::Get(const string& name)
{
	auto it = vars.lower_bound(name);
	if(it != vars.end() && !(vars.key_comp()(name, it->first)))
		return it->second;
	else
	{
		Var* var = new Var;
		var->type = Var::Type::None;
		vars.insert(it, std::map<string, Var*>::value_type(name, var));
		return var;
	}
}

//=================================================================================================
Var* VarsContainer::TryGet(const string& name)
{
	auto it = vars.lower_bound(name);
	if(it != vars.end() && !(vars.key_comp()(name, it->first)))
		return it->second;
	return nullptr;
}

//=================================================================================================
void VarsContainer::Save(FileWriter& f)
{
	f << vars.size();
	for(auto& e : vars)
	{
		f << e.first;
		f.WriteCasted<byte>(e.second->type);
		switch(e.second->type)
		{
		case Var::Type::None:
			break;
		case Var::Type::Bool:
			f << e.second->_bool;
			break;
		case Var::Type::Int:
			f << e.second->_int;
			break;
		case Var::Type::Float:
			f << e.second->_float;
			break;
		case Var::Type::Int2:
			f << e.second->int2;
			break;
		case Var::Type::Vec2:
			f << e.second->vec2;
			break;
		case Var::Type::Vec3:
			f << e.second->vec3;
			break;
		case Var::Type::Vec4:
			f << e.second->vec4;
			break;
		case Var::Type::Item:
			if(e.second->item)
				f << e.second->item->id;
			else
				f.Write0();
			break;
		case Var::Type::Location:
			f << (e.second->location ? e.second->location->index : -1);
			break;
		case Var::Type::Encounter:
			f << (e.second->encounter ? e.second->encounter->index : -1);
			break;
		case Var::Type::GroundItem:
			f << (e.second->ground_item ? e.second->ground_item->refid : -1);
			break;
		}
	}
}

//=================================================================================================
void VarsContainer::Load(FileReader& f)
{
	uint count;
	f >> count;
	for(uint i = 0; i < count; ++i)
	{
		tmp_str = f.ReadString1();
		auto it = vars.find(tmp_str);
		Var* v;
		if(it == vars.end())
		{
			v = new Var;
			f.ReadCasted<byte>(v->type);
			vars[tmp_str] = v;
		}
		else
		{
			v = it->second;
			byte type = f.Read<byte>();
			if(LOAD_VERSION < V_DEV && type >= (byte)Var::Type::Int2)
				type += 4; // added more simple types
			if(v->registered)
				assert(v->type == (Var::Type)type);
			else
				v->type = (Var::Type)type;
		}
		switch(v->type)
		{
		case Var::Type::None:
			break;
		case Var::Type::Bool:
			f >> v->_bool;
			break;
		case Var::Type::Int:
			f >> v->_int;
			break;
		case Var::Type::Float:
			f >> v->_float;
			break;
		case Var::Type::Int2:
			f >> v->int2;
			break;
		case Var::Type::Vec2:
			f >> v->vec2;
			break;
		case Var::Type::Vec3:
			f >> v->vec3;
			break;
		case Var::Type::Vec4:
			f >> v->vec4;
			break;
		case Var::Type::Item:
			{
				const string& id = f.ReadString1();
				if(id.empty())
					v->item = nullptr;
				else
					v->item = Item::Get(id);
			}
			break;
		case Var::Type::Location:
			{
				int index;
				f >> index;
				if(index == -1)
					v->location = nullptr;
				else
					v->location = W.GetLocation(index);
			}
			break;
		case Var::Type::Encounter:
			{
				int index;
				f >> index;
				if(index == -1)
					v->encounter = nullptr;
				else
					v->encounter = W.GetEncounter(index);
			}
			break;
		case Var::Type::GroundItem:
			{
				int refid;
				f >> refid;
				v->ground_item = GroundItem::GetByRefid(refid);
			}
			break;
		}
	}
}

//=================================================================================================
void VarsContainer::Clear()
{
	LoopAndRemove(vars, [](Var* var)
	{
		if(var->registered)
		{
			var->_int = 0;
			return false;
		}
		delete var;
		return true;
	});
}
