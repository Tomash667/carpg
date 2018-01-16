#include "Pch.h"
#include "Core.h"
#include "ScriptManager.h"
#include <angelscript.h>
#include <scriptarray/scriptarray.h>
#include <scriptstdstring/scriptstdstring.h>
#include "TypeBuilder.h"
#include "PlayerController.h"

#ifdef _DEBUG
#	define CHECKED(x) { int _r = (x); assert(_r >= 0); }
#else
#	define CHECKED(x) x
#endif

static ScriptManager* SM;

void MessageCallback(const asSMessageInfo* msg, void* param)
{
	Logger::Level level;
	switch(msg->type)
	{
	case asMSGTYPE_ERROR:
		level = Logger::L_ERROR;
		break;
	case asMSGTYPE_WARNING:
		level = Logger::L_WARN;
		break;
	case asMSGTYPE_INFORMATION:
	default:
		level = Logger::L_INFO;
		break;
	}
	SM->Log(level, Format("(%d:%d) %s", msg->row, msg->col, msg->message));
}

ScriptManager::ScriptManager() : engine(nullptr), module(nullptr)
{
	SM = this;
}

ScriptManager::~ScriptManager()
{
	if(engine)
		engine->ShutDownAndRelease();
}

void ScriptManager::Init()
{
	Info("Initializing ScriptManager...");

	engine = asCreateScriptEngine();
	module = engine->GetModule("Core", asGM_CREATE_IF_NOT_EXISTS);

	CHECKED(engine->SetMessageCallback(asFUNCTION(MessageCallback), nullptr, asCALL_CDECL));

	RegisterScriptArray(engine, true);
	RegisterStdString(engine);
	RegisterStdStringUtils(engine);
	Register();
}

#include "PlayerInfo.h"

uint PlayerController_GetPerkPoints(PlayerController* pc)
{
	return pc->perk_points;
}

void PlayerController_SetPerkPoints(PlayerController* pc, uint perk_points)
{
	pc->perk_points = perk_points;
	if(pc->player_info)
		pc->player_info->update_flags |= PlayerInfo::UF_PERK_POINTS;
}

VarsContainer::Var* PlayerController_GetVar(PlayerController* pc, const string& name)
{
	return pc->vars->GetVar(name);
}

void ScriptManager::Register()
{
	AddType("Var")
		.Method("bool IsNone() const", asMETHOD(VarsContainer::Var, IsNone))
		.Method("bool IsBool() const", asMETHODPR(VarsContainer::Var, IsBool, () const, bool))
		.Method("bool IsBool(bool) const", asMETHODPR(VarsContainer::Var, IsBool, (bool) const, bool))
		.Method("bool IsInt() const", asMETHODPR(VarsContainer::Var, IsInt, () const, bool))
		.Method("bool IsInt(int) const", asMETHODPR(VarsContainer::Var, IsInt, (int) const, bool))
		.Method("bool IsFloat() const", asMETHODPR(VarsContainer::Var, IsFloat, () const, bool))
		.Method("bool IsFloat(float) const", asMETHODPR(VarsContainer::Var, IsFloat, (float) const, bool))
		.Method("void SetNone()", asMETHOD(VarsContainer::Var, SetNone))
		.Method("void SetBool(bool)", asMETHOD(VarsContainer::Var, SetBool))
		.Method("void SetInt(int)", asMETHOD(VarsContainer::Var, SetInt))
		.Method("void SetFloat(float)", asMETHOD(VarsContainer::Var, SetFloat));

	AddType("Player")
		.Method("void AddPerkPoint(uint points = 0)", asMETHOD(PlayerController, AddPerkPoint))
		.Method("uint get_perk_points()", asFUNCTION(PlayerController_GetPerkPoints))
		.Method("void set_perk_points(uint)", asFUNCTION(PlayerController_SetPerkPoints))
		.Method("Var@ GetVar(string& in)", asFUNCTION(PlayerController_GetVar))
		.WithInstance("Player@ pc", &pc);
}

void ScriptManager::SetContext(PlayerController* pc)
{
	this->pc = pc;
}

bool ScriptManager::RunScript(cstring code)
{
	assert(code);

	// compile
	auto tmp_module = engine->GetModule("RunScriptModule", asGM_ALWAYS_CREATE);
	cstring packed_code = Format("void f() { %s; }", code);
	asIScriptFunction* func;
	int r = tmp_module->CompileFunction("RunScript", packed_code, -1, 0, &func);
	if(r < 0)
	{
		Error("ScriptManager: Failed to parse script (%d): %.100s", r, code);
		return false;
	}

	// run
	auto tmp_context = engine->RequestContext();
	r = tmp_context->Prepare(func);
	if(r >= 0)
	{
		//last_exception = nullptr;
		r = tmp_context->Execute();
	}

	bool finished = (r == asEXECUTION_FINISHED);
	if(!finished)
	{
		if(r == asEXECUTION_EXCEPTION)
		{
			//if(!last_exception)
				Error("ScriptManager: Failed to run script, exception thrown \"%s\" at %s(%d): %.100s", tmp_context->GetExceptionString(),
					tmp_context->GetExceptionFunction()->GetName(), tmp_context->GetExceptionLineNumber(), code);
			//else
			//	Error("ScriptManager: Failed to run script, script exception thrown \"%s\" at %s(%d): %.100s", last_exception,
			//		tmp_context->GetExceptionFunction()->GetName(), tmp_context->GetExceptionLineNumber(), code);
		}
		else
			Error("ScriptManager: Failed to run script (%d): %.100s", r, code);
	}

	func->Release();
	engine->ReturnContext(tmp_context);

	return finished;
}

string& ScriptManager::OpenOutput()
{
	gather_output = true;
	return output;
}

void ScriptManager::CloseOutput()
{
	gather_output = false;
	output.clear();
}

void ScriptManager::Log(Logger::Level level, cstring msg)
{
	if(gather_output)
	{
		if(!output.empty())
			output += '\n';
		switch(level)
		{
		case Logger::L_INFO:
			output += "INFO:";
			break;
		case Logger::L_WARN:
			output += "WARN:";
			break;
		case Logger::L_ERROR:
			output += "ERROR:";
			break;
		}
		output += msg;
	}
	else
		Logger::global->Log(level, msg);
}

TypeBuilder ScriptManager::AddType(cstring name)
{
	assert(name);
	CHECKED(engine->RegisterObjectType(name, 0, asOBJ_REF | asOBJ_NOCOUNT));
	return ForType(name);
}

TypeBuilder ScriptManager::ForType(cstring name)
{
	assert(name);
	TypeBuilder builder(name, engine);
	return builder;
}
