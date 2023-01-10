#include "Pch.h"
#include "Var.h"

#include "Encounter.h"
#include "GroundItem.h"
#include "Item.h"
#include "Location.h"
#include "ScriptManager.h"
#include "World.h"
#pragma warning(error: 4062)

string Vars::tmpStr;

//=================================================================================================
bool Var::IsGeneric(void* ptr, int type)
{
	Type isType = scriptMgr->GetVarType(type);
	return this->type == isType && this->ptr == ptr;
}

//=================================================================================================
void Var::SetNone()
{
	if(type == Type::String)
		StringPool.Free(str);
	type = Type::None;
}

//=================================================================================================
Var* Var::SetBool(bool value)
{
	if(type == Type::String)
		StringPool.Free(str);
	type = Type::Bool;
	_bool = value;
	return this;
}

//=================================================================================================
Var* Var::SetInt(int value)
{
	if(type == Type::String)
		StringPool.Free(str);
	type = Type::Int;
	_int = value;
	return this;
}

//=================================================================================================
Var* Var::SetFloat(float value)
{
	if(type == Type::String)
		StringPool.Free(str);
	type = Type::Float;
	_float = value;
	return this;
}

//=================================================================================================
Var* Var::SetString(const string& str)
{
	if(type == Type::String)
		*this->str = str;
	else
	{
		type = Type::String;
		this->str = StringPool.Get(str);
	}
	return this;
}

//=================================================================================================
Var* Var::SetGeneric(void* ptr, int type)
{
	if(this->type == Type::String)
		StringPool.Free(str);
	// TODO: check if this is known type
	this->type = scriptMgr->GetVarType(type);
	this->ptr = ptr;
	return this;
}

//=================================================================================================
Var* Var::SetVar(Var * var)
{
	if(type == var->type)
	{
		if(type == Type::String)
			*str = *var->str;
		else
			_int = var->_int;
	}
	else
	{
		if(type == Type::String)
		{
			StringPool.Free(str);
			_int = var->_int;
		}
		else if(var->type == Type::String)
			str = StringPool.Get(*var->str);
		else
			_int = var->_int;
		type = var->type;
	}
	return this;
}

//=================================================================================================
void Var::SetPtr(void* ptr, Type type)
{
	if(this->type == Type::String)
		StringPool.Free(str);
	this->type = type;
	this->ptr = ptr;
}

//=================================================================================================
void Var::GetGeneric(void* ptr, int type)
{
	// TODO: throw on invalid type
	assert(this->type == scriptMgr->GetVarType(type));
	*(void**)ptr = this->ptr;
}

//=================================================================================================
Vars::~Vars()
{
	DeleteElements(vars);
}

//=================================================================================================
Var* Vars::Add(Var::Type type, const string& name, bool registered)
{
	Var* var = new Var;
	var->type = type;
	var->registered = registered;
	vars[name] = var;
	return var;
}

//=================================================================================================
Var* Vars::Get(const string& name)
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
Var* Vars::TryGet(const string& name) const
{
	auto it = vars.lower_bound(name);
	if(it != vars.end() && !(vars.key_comp()(name, it->first)))
		return it->second;
	return nullptr;
}

//=================================================================================================
void Vars::Save(GameWriter& f)
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
			f << (e.second->groundItem ? e.second->groundItem->id : -1);
			break;
		case Var::Type::String:
		case Var::Type::Unit:
		case Var::Type::UnitGroup:
		case Var::Type::Array:
		case Var::Type::Class:
			assert(0); // TODO
			break;
		}
	}
}

//=================================================================================================
void Vars::Load(GameReader& f)
{
	uint count;
	f >> count;
	for(uint i = 0; i < count; ++i)
	{
		tmpStr = f.ReadString1();
		auto it = vars.find(tmpStr);
		Var* v;
		if(it == vars.end())
		{
			v = new Var;
			f.ReadCasted<byte>(v->type);
			vars[tmpStr] = v;
		}
		else
		{
			v = it->second;
			byte type = f.Read<byte>();
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
					v->location = world->GetLocation(index);
			}
			break;
		case Var::Type::Encounter:
			{
				int index;
				f >> index;
				if(index == -1)
					v->encounter = nullptr;
				else
					v->encounter = world->GetEncounter(index);
			}
			break;
		case Var::Type::GroundItem:
			{
				int id;
				f >> id;
				v->groundItem = GroundItem::GetById(id);
			}
			break;
		case Var::Type::String:
		case Var::Type::Unit:
		case Var::Type::UnitGroup:
		case Var::Type::Array:
		case Var::Type::Class:
			assert(0); // TODO
			break;
		}
	}
}

//=================================================================================================
void Vars::Clear()
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
