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

ScriptException::ScriptException(cstring msg)
{
	SM->SetException(msg);
}

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
	RegisterCommon();
	RegisterGame();
}

static void FormatStrGeneric(asIScriptGeneric* gen)
{
	int count = gen->GetArgCount();

	string result, part;
	string& fmt = **(string**)gen->GetAddressOfArg(0);

	for(uint i = 0, len = fmt.length(); i < len; ++i)
	{
		char c = fmt[i];
		if(c != '{')
		{
			result += c;
			continue;
		}
		if(++i == len)
			throw ScriptException("Broken format string, { at end of string.");
		c = fmt[i];
		if(c == '{')
		{
			result += 'c';
			continue;
		}
		uint pos = fmt.find_first_of('}', i);
		if(pos == string::npos)
			throw ScriptException("Broken format string, missing closing }.");
		part = fmt.substr(i, pos - i);
		int index;
		if(!TextHelper::ToInt(part.c_str(), index))
			throw ScriptException("Broken format string, invalid index '%s'.", part.c_str());
		++index;
		if(index < 1 || index >= count)
			throw ScriptException("Broken format string, invalid index %d.", index - 1);
		auto type_id = gen->GetArgTypeId(index);
		void* adr = gen->GetAddressOfArg(index);
		cstring s = nullptr;
		switch(type_id)
		{
		case asTYPEID_BOOL:
			{
				bool value = **(bool**)adr;
				s = (value ? "true" : "false");
			}
			break;
		case asTYPEID_INT8:
			{
				char value = **(char**)adr;
				s = Format("%d", (int)value);
			}
			break;
		case asTYPEID_INT16:
			{
				short value = **(short**)adr;
				s = Format("%d", (int)value);
			}
			break;
		case asTYPEID_INT32:
			{
				int value = **(int**)adr;
				s = Format("%d", value);
			}
			break;
		case asTYPEID_INT64:
			{
				int64 value = **(int64**)adr;
				s = Format("%I64d", value);
			}
			break;
		case asTYPEID_UINT8:
			{
				byte value = **(byte**)adr;
				s = Format("%u", (uint)value);
			}
			break;
		case asTYPEID_UINT16:
			{
				word value = **(word**)adr;
				s = Format("%u", (uint)value);
			}
			break;
		case asTYPEID_UINT32:
			{
				uint value = **(uint**)adr;
				s = Format("%u", value);
			}
			break;
		case asTYPEID_UINT64:
			{
				uint64 value = **(uint64**)adr;
				s = Format("%I64u", value);
			}
			break;
		case asTYPEID_FLOAT:
			{
				float value = **(float**)adr;
				s = Format("%g", value);
			}
			break;
		case asTYPEID_DOUBLE:
			{
				double value = **(double**)adr;
				s = Format("%g", value);
			}
			break;
		/*default:
			{
				part = ToString(gen, adr, type_id);
				s = part.c_str();
			}
			break;*/
		}

		result += s;
		i = pos;
	}

	new(gen->GetAddressOfReturnLocation()) string(result);
}

static void ScriptInfo(const string& str)
{
	SM->Log(Logger::L_INFO, str.c_str());
}
static void ScriptWarn(const string& str)
{
	SM->Log(Logger::L_WARN, str.c_str());
}
static void ScriptError(const string& str)
{
	SM->Log(Logger::L_ERROR, str.c_str());
}

void ScriptManager::RegisterCommon()
{
	AddFunction("void Info(const string& in)", asFUNCTION(ScriptInfo));
	AddFunction("void Warn(const string& in)", asFUNCTION(ScriptWarn));
	AddFunction("void Error(const string& in)", asFUNCTION(ScriptError));

	string func_sign = "string Format(const string& in)";
	for(int i = 1; i <= 9; ++i)
	{
		func_sign.pop_back();
		func_sign += ", ?& in)";
		CHECKED(engine->RegisterGlobalFunction(func_sign.c_str(), asFUNCTION(FormatStrGeneric), asCALL_GENERIC));
	}

	/*sm.AddStruct<Int2>("Int2")
		.Constructor<int, int>("void f(int, int)")
		.Constructor<const Int2&>("void f(const Int2& in)")
		.Member("int x", offsetof(Int2, x))
		.Member("int y", offsetof(Int2, y));

	sm.AddStruct<Vec2>("Vec2")
		.Constructor<float, float>("void f(float, float)")
		.Constructor<const Vec2&>("void f(const Vec2& in)")
		.Member("float x", offsetof(Vec2, x))
		.Member("float y", offsetof(Vec2, y));

	sm.AddStruct<Vec3>("Vec3")
		.Constructor<float, float, float>("void f(float, float, float)")
		.Constructor<const Vec3&>("void f(const Vec3& in)")
		.Member("float x", offsetof(Vec3, x))
		.Member("float y", offsetof(Vec3, y))
		.Member("float z", offsetof(Vec3, z));

	sm.AddStruct<Vec4>("Vec4")
		.Constructor<float, float, float, float>("void f(float, float, float, float)")
		.Constructor<const Vec4&>("void f(const Vec4& in)")
		.Member("float x", offsetof(Vec4, x))
		.Member("float y", offsetof(Vec4, y))
		.Member("float z", offsetof(Vec4, z))
		.Member("float w", offsetof(Vec4, w));*/
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

void ScriptManager::RegisterGame()
{
	AddType("Var")
		.Method("bool GetBool() const", asMETHOD(VarsContainer::Var, GetBool))
		.Method("int GetInt() const", asMETHOD(VarsContainer::Var, GetInt))
		.Method("float GetFloat() const", asMETHOD(VarsContainer::Var, GetFloat))
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
		.Method("void AddPerkPoint(uint points = 1)", asMETHOD(PlayerController, AddPerkPoint))
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
		last_exception = nullptr;
		r = tmp_context->Execute();
	}

	bool finished = (r == asEXECUTION_FINISHED);
	if(!finished)
	{
		if(r == asEXECUTION_EXCEPTION)
		{
			if(!last_exception)
				Error("ScriptManager: Failed to run script, exception thrown \"%s\" at %s(%d): %.100s", tmp_context->GetExceptionString(),
					tmp_context->GetExceptionFunction()->GetName(), tmp_context->GetExceptionLineNumber(), code);
			else
				Error("ScriptManager: Failed to run script, script exception thrown \"%s\" at %s(%d): %.100s", last_exception,
					tmp_context->GetExceptionFunction()->GetName(), tmp_context->GetExceptionLineNumber(), code);
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

void ScriptManager::AddFunction(cstring decl, const asSFuncPtr& funcPointer)
{
	assert(decl);
	CHECKED(engine->RegisterGlobalFunction(decl, funcPointer, asCALL_CDECL));
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
