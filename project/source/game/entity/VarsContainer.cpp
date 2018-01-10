#include "Pch.h"
#include "Core.h"
#include "VarsContainer.h"

void VarsContainer::OnFree()
{
	for(auto var : vars)
	{
		//if(var->type == String)
		//	StringPool.Free(var->_string);
		var->Free();
	}
	vars.clear();
}

void VarsContainer::Write(FileWriter& f)
{
	f << vars.size();
	for(auto var : vars)
	{
		f.WriteCasted<byte>(var->type);
		f << var->name;
		switch(var->type)
		{
		case None:
			break;
		case Bool:
			f << var->_bool;
			break;
		case Int:
			f << var->_int;
			break;
		case Float:
			f << var->_float;
			break;
		//case String:
		//	f << *var->_string;
		//	break;
		}
	}
}

void VarsContainer::Read(FileReader& f)
{
	uint count;
	f >> count;
	for(uint i = 0; i < count; ++i)
	{
		Var* var = Var::Get();
		f.ReadCasted<byte>(var->type);
		f >> var->name;
		switch(var->type)
		{
		case None:
			break;
		case Bool:
			f >> var->_bool;
			break;
		case Int:
			f >> var->_int;
			break;
		case Float:
			f >> var->_float;
			break;
		//case String:
		//	var->_string = StringPool.Get();
		//	f >> *var->_string;
		//	break;
		}
	}
}

VarsContainer::Var* VarsContainer::GetVar(AnyString name)
{
	Var* var = Var::Get();
	var->name = name;
	auto it = vars.lower_bound(var);
	if(it == vars.end() || (*it)->name != var->name)
	{
		vars.insert(it, var);
		return var;
	}
	else
	{
		var->Free();
		return *it;
	}
}
