#include "Pch.h"
#include "GameCore.h"
#include "GameDialog.h"
#include "ScriptManager.h"
#include <angelscript.h>

GameDialog::Map GameDialog::dialogs;
DialogScripts DialogScripts::global;


//=================================================================================================
int DialogScripts::AddCode(FUNC f, const string& code)
{
	vector<string>& s = scripts[f];
	for(int i = 0, count = (int)s.size(); i < count; ++i)
	{
		if(code == s[i])
			return i;
	}
	s.push_back({ code });
	return (int)s.size() - 1;
}

//=================================================================================================
DialogScripts::~DialogScripts()
{
	if(!built)
		return;
	for(int i = 0; i < F_MAX; ++i)
	{
		if(func[i])
			func[i]->Release();
	}
}

//=================================================================================================
void DialogScripts::GetFormattedCode(FUNC f, string& code)
{
	vector<string>& s = scripts[f];
	if(s.empty())
	{
		code.clear();
		return;
	}

	switch(f)
	{
	case F_SCRIPT:
		code = "void _script(int index) {\n switch(index) {\n";
		for(uint i = 0; i < s.size(); ++i)
			code += Format("  case %d: {%s;} break;\n", i, s[i].c_str());
		code += " }\n}";
		break;
	case F_IF_SCRIPT:
		code = "bool _if_script(int index) {\n switch(index) {\n";
		for(uint i = 0; i < s.size(); ++i)
			code += Format("  case %d: return (%s);\n", i, s[i].c_str());
		code += " }\n return false;\n}";
		break;
	case F_FORMAT:
		code = "string _format(int index) {\n switch(index) {\n";
		for(uint i = 0; i < s.size(); ++i)
			code += Format("  case %d: return %s;\n", i, s[i].c_str());
		code += " }\n return \"ERROR\";\n}";
		break;
	}
}

//=================================================================================================
void DialogScripts::Build()
{
	asIScriptModule* module = SM.GetEngine()->GetModule("Quests", asGM_CREATE_IF_NOT_EXISTS);
	LocalString code;
	for(int i = 0; i < F_MAX; ++i)
	{
		if(scripts[i].empty())
			continue;
		cstring name[F_MAX] = { "_script", "_if_script", "_format" };
		GetFormattedCode((DialogScripts::FUNC)i, code.get_ref());
		int r = module->CompileFunction(name[i], code.c_str(), -1, 0, &func[i]);
		if(r < 0)
			Error("Failed to compile dialogs script %s (%d): %s", name[i], r, code.c_str());
	}
	built = true;
}

//=================================================================================================
void DialogScripts::Set(asITypeInfo* type)
{
	func[F_SCRIPT] = type->GetMethodByDecl("void _script(int)");
	func[F_IF_SCRIPT] = type->GetMethodByDecl("bool _if_script(int)");
	func[F_FORMAT] = type->GetMethodByDecl("string _format(int)");
}


//=================================================================================================
GameDialog* GameDialog::TryGet(cstring id)
{
	auto it = dialogs.find(id);
	if(it == dialogs.end())
		return nullptr;
	else
		return it->second;
}

//=================================================================================================
void GameDialog::Cleanup()
{
	for(auto& it : dialogs)
		delete it.second;
}
