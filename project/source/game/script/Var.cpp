#include "Pch.h"
#include "GameCore.h"
#include "Var.h"

VarsContainer::~VarsContainer()
{
	DeleteElements(vars);
}

Var* VarsContainer::Get(const string& id)
{
	auto it = vars.lower_bound(id);
	if(it != vars.end() && !(vars.key_comp()(id, it->first)))
		return it->second;
	else
	{
		Var* var = new Var;
		var->type = Var::Type::None;
		vars.insert(it, std::map<string, Var*>::value_type(id, var));
		return var;
	}
}

void VarsContainer::Save(FileWriter& f)
{
	f << vars.size();
	for(auto& e : vars)
	{
		f << e.first;
		f.WriteCasted<byte>(e.second->type);
		f << e.second->_int;
	}
}

void VarsContainer::Load(FileReader& f)
{
	uint count;
	f >> count;
	for(uint i = 0; i < count; ++i)
	{
		const string& var_name = f.ReadString1();
		Var* v = new Var;
		f.ReadCasted<byte>(v->type);
		f >> v->_int;
		vars[var_name] = v;
	}
}

void VarsContainer::Clear()
{
	DeleteElements(vars);
}
