#include "Pch.h"
#include "GameCore.h"
#include "ScriptManager.h"
#include <angelscript.h>
#include <scriptarray/scriptarray.h>
#include <scriptstdstring/scriptstdstring.h>
#include "TypeBuilder.h"
#include "PlayerController.h"
#include "SaveState.h"
#include "Game.h"
#include "ItemHelper.h"
#include "Level.h"
#include "City.h"
#include "InsideLocation.h"
#include "BaseLocation.h"
#include "Team.h"
#include "Quest_Scripted.h"
#include "LocationHelper.h"
#include "Encounter.h"
#include "UnitGroup.h"

ScriptManager* global::script_mgr;
static std::map<int, asIScriptFunction*> tostring_map;
static string tmp_str_result;

ScriptException::ScriptException(cstring msg)
{
	script_mgr->SetException(msg);
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
	script_mgr->Log(level, Format("(%d:%d) %s", msg->row, msg->col, msg->message));
}

ScriptManager::ScriptManager() : engine(nullptr), module(nullptr)
{
}

ScriptManager::~ScriptManager()
{
	if(engine)
		engine->ShutDownAndRelease();
	DeleteElements(unit_vars);
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

asIScriptFunction* FindToString(asIScriptEngine* engine, int type_id)
{
	// find mapped type
	auto it = tostring_map.find(type_id);
	if(it != tostring_map.end())
		return it->second;

	// get type
	asITypeInfo* type = engine->GetTypeInfoById(type_id);
	assert(type);

	// special case - cast to string
	cstring name = type->GetName();
	if(strcmp(name, "string") == 0)
	{
		tostring_map[type_id] = nullptr;
		return nullptr;
	}

	// find function
	asIScriptFunction* func = type->GetMethodByDecl("string ToString() const");
	if(!func)
		func = type->GetMethodByDecl("string ToString()");
	if(!func)
		throw ScriptException("Missing ToString method for object '%s'.", name);

	// add mapping
	tostring_map[type_id] = func;
	return func;
}

string& ToString(asIScriptGeneric* gen, void* adr, int type_id)
{
	asIScriptEngine* engine = gen->GetEngine();
	asIScriptFunction* func = FindToString(engine, type_id);

	if(!func)
	{
		// cast to string
		string& value = **(string**)adr;
		return value;
	}

	// call function
	asIScriptObject* obj = *(asIScriptObject**)adr;
	asIScriptContext* ctx = engine->RequestContext();
	int r = ctx->Prepare(func);
	if(r >= 0)
	{
		r = ctx->SetObject(obj);
		if(r >= 0)
			r = ctx->Execute();
	}
	if(r < 0)
	{
		asITypeInfo* type = engine->GetTypeInfoById(type_id);
		cstring name = type->GetName();
		throw ScriptException("Failed to call ToString on object '%s' (%d).", name, r);
	}

	void* ret_adr = ctx->GetReturnAddress();
	tmp_str_result = *(string*)ret_adr;
	engine->ReturnContext(ctx);
	return tmp_str_result;
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
		default:
			s = ToString(gen, adr, type_id).c_str();
			break;
		}

		result += s;
		i = pos;
	}

	new(gen->GetAddressOfReturnLocation()) string(result);
}

static void ScriptInfo(const string& str)
{
	script_mgr->Log(Logger::L_INFO, str.c_str());
}
static void ScriptDevInfo(const string& str)
{
	if(game->devmode)
		Info(str.c_str());
}
static void ScriptWarn(const string& str)
{
	script_mgr->Log(Logger::L_WARN, str.c_str());
}
static void ScriptError(const string& str)
{
	script_mgr->Log(Logger::L_ERROR, str.c_str());
}

string Vec2_ToString(const Vec2& v)
{
	return Format("%g; %g", v.x, v.y);
}

void ScriptManager::RegisterCommon()
{
	ScriptBuilder sb(engine);

	AddFunction("void Info(const string& in)", asFUNCTION(ScriptInfo));
	AddFunction("void DevInfo(const string& in)", asFUNCTION(ScriptDevInfo));
	AddFunction("void Warn(const string& in)", asFUNCTION(ScriptWarn));
	AddFunction("void Error(const string& in)", asFUNCTION(ScriptError));

	string func_sign = "string Format(const string& in)";
	for(int i = 1; i <= 9; ++i)
	{
		CHECKED(engine->RegisterGlobalFunction(func_sign.c_str(), asFUNCTION(FormatStrGeneric), asCALL_GENERIC));
		func_sign.pop_back();
		func_sign += ", ?& in)";
	}

	AddFunction("int Random(int, int)", asFUNCTIONPR(Random, (int, int), int));
	AddFunction("int Rand()", asFUNCTIONPR(Rand, (), int));

	sb.AddStruct<Int2>("Int2", asOBJ_APP_CLASS_ALLINTS)
		.Constructor<>("void f()")
		.Constructor<int, int>("void f(int, int)")
		.Constructor<const Int2&>("void f(const Int2& in)")
		.Member("int x", offsetof(Int2, x))
		.Member("int y", offsetof(Int2, y))
		.Method("bool opEquals(const Int2& in) const", asMETHOD(Int2, operator ==))
		.Method("Int2& opAssign(const Int2& in)", asMETHODPR(Int2, operator =, (const Int2&), Int2&))
		.Method("Int2& opAddAssign(const Int2& in)", asMETHOD(Int2, operator +=))
		.Method("Int2& opSubAssign(const Int2& in)", asMETHOD(Int2, operator -=))
		.Method("Int2& opMulAssign(int)", asMETHODPR(Int2, operator *=, (int), Int2&))
		.Method("Int2& opMulAssign(float)", asMETHODPR(Int2, operator *=, (float), Int2&))
		.Method("Int2& opDivAssign(int)", asMETHODPR(Int2, operator /=, (int), Int2&))
		.Method("Int2& opDivAssign(float)", asMETHODPR(Int2, operator /=, (float), Int2&))
		.Method("Int2 opAdd(const Int2& in) const", asMETHODPR(Int2, operator +, (const Int2&) const, Int2))
		.Method("Int2 opSub(const Int2& in) const", asMETHODPR(Int2, operator -, (const Int2&) const, Int2))
		.Method("Int2 opMul(int) const", asMETHODPR(Int2, operator *, (int) const, Int2))
		.Method("Int2 opDiv(int) const", asMETHODPR(Int2, operator /, (int) const, Int2));

	sb.AddStruct<Vec2>("Vec2", asOBJ_APP_CLASS_ALLFLOATS)
		.Constructor<>("void f()")
		.Constructor<float, float>("void f(float, float)")
		.Constructor<const Vec2&>("void f(const Vec2& in)")
		.Member("float x", offsetof(Vec2, x))
		.Member("float y", offsetof(Vec2, y))
		.Method("bool opEquals(const Vec2& in) const", asMETHOD(Vec2, operator ==))
		.Method("Vec2& opAssign(const Vec2& in)", asMETHODPR(Vec2, operator =, (const Vec2&), Vec2&))
		.Method("Vec2& opAddAssign(const Vec2& in)", asMETHOD(Vec2, operator +=))
		.Method("Vec2& opSubAssign(const Vec2& in)", asMETHOD(Vec2, operator -=))
		.Method("Vec2& opMulAssign(float)", asMETHOD(Vec2, operator *=))
		.Method("Vec2& opDivAssign(float)", asMETHOD(Vec2, operator /=))
		.Method("Vec2 opAdd(const Vec2& in) const", asMETHODPR(Vec2, operator +, (const Vec2&) const, Vec2))
		.Method("Vec2 opSub(const Vec2& in) const", asMETHODPR(Vec2, operator -, (const Vec2&) const, Vec2))
		.Method("Vec2 opMul(float) const", asMETHODPR(Vec2, operator *, (float) const, Vec2))
		.Method("Vec2 opDiv(float) const", asMETHODPR(Vec2, operator /, (float) const, Vec2))
		.Method("string ToString() const", asFUNCTION(Vec2_ToString))
		.WithNamespace()
		.AddFunction("float Distance(const Vec2& in, const Vec2& in)", asFUNCTION(Vec2::Distance));

	sb.AddStruct<Vec3>("Vec3", asOBJ_APP_CLASS_ALLFLOATS)
		.Constructor<>("void f()")
		.Constructor<float, float, float>("void f(float, float, float)")
		.Constructor<const Vec3&>("void f(const Vec3& in)")
		.Member("float x", offsetof(Vec3, x))
		.Member("float y", offsetof(Vec3, y))
		.Member("float z", offsetof(Vec3, z))
		.Method("bool opEquals(const Vec3& in) const", asMETHOD(Vec3, operator ==))
		.Method("Vec3& opAssign(const Vec3& in)", asMETHODPR(Vec3, operator =, (const Vec3&), Vec3&))
		.Method("Vec3& opAddAssign(const Vec3& in)", asMETHOD(Vec3, operator +=))
		.Method("Vec3& opSubAssign(const Vec3& in)", asMETHOD(Vec3, operator -=))
		.Method("Vec3& opMulAssign(float)", asMETHOD(Vec3, operator *=))
		.Method("Vec3& opDivAssign(float)", asMETHOD(Vec3, operator /=))
		.Method("Vec3 opAdd(const Vec3& in) const", asMETHODPR(Vec3, operator +, (const Vec3&) const, Vec3))
		.Method("Vec3 opSub(const Vec3& in) const", asMETHODPR(Vec3, operator -, (const Vec3&) const, Vec3))
		.Method("Vec3 opMul(float) const", asMETHODPR(Vec3, operator *, (float) const, Vec3))
		.Method("Vec3 opDiv(float) const", asMETHODPR(Vec3, operator /, (float) const, Vec3))
		.WithNamespace()
		.AddFunction("float Distance(const Vec3& in, const Vec3& in)", asFUNCTION(Vec3::Distance));

	sb.AddStruct<Vec4>("Vec4", asOBJ_APP_CLASS_ALLFLOATS)
		.Constructor<>("void f()")
		.Constructor<float, float, float, float>("void f(float, float, float, float)")
		.Constructor<const Vec4&>("void f(const Vec4& in)")
		.Member("float x", offsetof(Vec4, x))
		.Member("float y", offsetof(Vec4, y))
		.Member("float z", offsetof(Vec4, z))
		.Member("float w", offsetof(Vec4, w))
		.Method("bool opEquals(const Vec4& in) const", asMETHOD(Vec4, operator ==))
		.Method("Vec4& opAssign(const Vec4& in)", asMETHODPR(Vec4, operator =, (const Vec4&), Vec4&))
		.Method("Vec4& opAddAssign(const Vec4& in)", asMETHOD(Vec4, operator +=))
		.Method("Vec4& opSubAssign(const Vec4& in)", asMETHOD(Vec4, operator -=))
		.Method("Vec4& opMulAssign(float)", asMETHOD(Vec4, operator *=))
		.Method("Vec4& opDivAssign(float)", asMETHOD(Vec4, operator /=))
		.Method("Vec4 opAdd(const Vec4& in) const", asMETHODPR(Vec4, operator +, (const Vec4&) const, Vec4))
		.Method("Vec4 opSub(const Vec4& in) const", asMETHODPR(Vec4, operator -, (const Vec4&) const, Vec4))
		.Method("Vec4 opMul(float) const", asMETHODPR(Vec4, operator *, (float) const, Vec4))
		.Method("Vec4 opDiv(float) const", asMETHODPR(Vec4, operator /, (float) const, Vec4));

	AddFunction("void Sleep(float)", asMETHOD(ScriptManager, ScriptSleep), this);
}

#include "PlayerInfo.h"
#include "Level.h"
#include "World.h"

VarsContainer globals;
VarsContainer* p_globals = &globals;

VarsContainer* Unit_GetVars(Unit* unit)
{
	return script_mgr->GetVars(unit);
}

string World_GetDirName(const Vec2& pos1, const Vec2& pos2)
{
	return GetLocationDirName(pos1, pos2);
}

Location* World_GetRandomCity()
{
	return world->GetLocation(world->GetRandomCityIndex());
}

Location* World_GetRandomSettlementWithBuilding(const string& building_id)
{
	Building* b = Building::TryGet(building_id);
	if(!b)
		throw ScriptException("Missing building '%s'.", building_id.c_str());
	return world->GetRandomSettlement([b](City* city)
	{
		return city->FindBuilding(b) != nullptr;
	});
}

Location* World_GetRandomSettlement(asIScriptFunction* func)
{
	asIScriptEngine* engine = func->GetEngine();
	asIScriptContext* ctx = engine->RequestContext();
	Location* target = world->GetRandomSettlementWeighted([=](Location* loc)
	{
		CHECKED(ctx->Prepare(func));
		CHECKED(ctx->SetArgObject(0, loc));
		CHECKED(ctx->Execute());
		return ctx->GetReturnFloat();
	});
	engine->ReturnContext(ctx);
	func->Release();
	return target;
}

Location* World_GetClosestLocation2(LOCATION type, const Vec2& pos, CScriptArray* array, int flags)
{
	int index = world->GetClosestLocation(type, pos, (int*)array->GetBuffer(), array->GetSize(), flags);
	return world->GetLocation(index);
}

void StockScript_AddItem(const Item* item, uint count)
{
	vector<ItemSlot>* stock = script_mgr->GetContext().stock;
	if(!stock)
		throw ScriptException("This method must be called from StockScript.");
	InsertItemBare(*stock, item, count);
}

void StockScript_AddRandomItem(ITEM_TYPE type, int price_limit, int flags, uint count)
{
	vector<ItemSlot>* stock = script_mgr->GetContext().stock;
	if(!stock)
		throw ScriptException("This method must be called from StockScript.");
	ItemHelper::AddRandomItem(*stock, type, price_limit, flags, count);
}

void ScriptManager::RegisterGame()
{
	ScriptBuilder sb(engine);

	AddType("Unit");
	AddType("Player");
	AddType("Hero");
	AddType("LevelArea");

	AddEnum("ITEM_TYPE", {
		{ "IT_WEAPON", IT_WEAPON },
		{ "IT_BOW", IT_BOW },
		{ "IT_SHIELD", IT_SHIELD },
		{ "IT_ARMOR", IT_ARMOR },
		{ "IT_OTHER", IT_OTHER },
		{ "IT_CONSUMABLE", IT_CONSUMABLE },
		{ "IT_BOOK", IT_BOOK }
		});

	AddEnum("ITEM_FLAGS", {
		{ "ITEM_NOT_SHOP", ITEM_NOT_SHOP },
		{ "ITEM_NOT_MERCHANT", ITEM_NOT_MERCHANT },
		{ "ITEM_NOT_BLACKSMITH", ITEM_NOT_BLACKSMITH },
		{ "ITEM_NOT_ALCHEMIST", ITEM_NOT_ALCHEMIST }
		});

	AddEnum("EventType", {
		{ "EVENT_ENTER", EVENT_ENTER },
		{ "EVENT_PICKUP", EVENT_PICKUP },
		{ "EVENT_UPDATE", EVENT_UPDATE },
		{ "EVENT_TIMEOUT", EVENT_TIMEOUT },
		{ "EVENT_ENCOUNTER", EVENT_ENCOUNTER },
		{ "EVENT_DIE", EVENT_DIE }
		});

	AddEnum("LOCATION", {
		{ "L_CITY", L_CITY },
		{ "L_CAVE", L_CAVE },
		{ "L_CAMP", L_CAMP },
		{ "L_DUNGEON", L_DUNGEON },
		{ "L_OUTSIDE", L_OUTSIDE },
		{ "L_ENCOUNTER", L_ENCOUNTER }
		});

	AddEnum("LOCATION_TARGET", {
		{ "FOREST", FOREST },
		{ "MOONWELL", MOONWELL },
		{ "ACADEMY", ACADEMY },
		{ "VILLAGE", VILLAGE },
		{ "CITY", CITY },
		{ "CAPITAL", CAPITAL }
		});

	AddEnum("UNIT_ORDER", {
		{ "ORDER_NONE", ORDER_NONE },
		{ "ORDER_WANDER", ORDER_WANDER },
		{ "ORDER_WAIT", ORDER_WAIT },
		{ "ORDER_FOLLOW", ORDER_FOLLOW },
		{ "ORDER_LEAVE", ORDER_LEAVE },
		{ "ORDER_MOVE", ORDER_MOVE },
		{ "ORDER_LOOK_AT", ORDER_LOOK_AT },
		{ "ORDER_ESCAPE_TO", ORDER_ESCAPE_TO },
		{ "ORDER_ESCAPE_TO_UNIT", ORDER_ESCAPE_TO_UNIT },
		{ "ORDER_GOTO_INN", ORDER_GOTO_INN },
		{ "ORDER_GUARD", ORDER_GUARD },
		{ "ORDER_AUTO_TALK", ORDER_AUTO_TALK }
		});

	AddEnum("MOVE_TYPE", {
		{ "MOVE_RUN", MOVE_RUN },
		{ "MOVE_WALK", MOVE_WALK },
		{ "MOVE_RUN_WHEN_NEAR_TEAM", MOVE_RUN_WHEN_NEAR_TEAM }
		});

	AddEnum("LOCATION_IMAGE", {
		{ "LI_CITY", LI_CITY },
		{ "LI_VILLAGE", LI_VILLAGE },
		{ "LI_CAVE", LI_CAVE },
		{ "LI_CAMP", LI_CAMP },
		{ "LI_DUNGEON", LI_DUNGEON },
		{ "LI_CRYPT", LI_CRYPT },
		{ "LI_FOREST", LI_FOREST },
		{ "LI_MOONWELL", LI_MOONWELL },
		{ "LI_TOWER", LI_TOWER },
		{ "LI_LABYRINTH", LI_LABYRINTH },
		{ "LI_MINE", LI_MINE },
		{ "LI_SAWMILL", LI_SAWMILL },
		{ "LI_DUNGEON2", LI_DUNGEON2 },
		{ "LI_ACADEMY", LI_ACADEMY },
		{ "LI_CAPITAL", LI_CAPITAL }
		});

	AddType("Var")
		.Method("bool IsNone() const", asMETHOD(Var, IsNone))
		.Method("bool IsBool() const", asMETHODPR(Var, IsBool, () const, bool))
		.Method("bool IsBool(bool) const", asMETHODPR(Var, IsBool, (bool) const, bool))
		.Method("bool IsInt() const", asMETHODPR(Var, IsInt, () const, bool))
		.Method("bool IsInt(int) const", asMETHODPR(Var, IsInt, (int) const, bool))
		.Method("bool IsFloat() const", asMETHODPR(Var, IsFloat, () const, bool))
		.Method("bool IsFloat(float) const", asMETHODPR(Var, IsFloat, (float) const, bool))
		.Method("bool IsVar(Var@) const", asMETHOD(Var, IsVar))
		.Method("bool opEquals(bool) const", asMETHODPR(Var, IsBool, (bool) const, bool))
		.Method("bool opEquals(int) const", asMETHODPR(Var, IsInt, (int) const, bool))
		.Method("bool opEquals(float) const", asMETHODPR(Var, IsFloat, (float) const, bool))
		.Method("bool opEquals(Var@) const", asMETHOD(Var, IsVar))
		.Method("Var@ SetBool(bool)", asMETHOD(Var, SetBool))
		.Method("Var@ SetInt(int)", asMETHOD(Var, SetInt))
		.Method("Var@ SetFloat(float)", asMETHOD(Var, SetFloat))
		.Method("Var@ opAssign(bool)", asMETHOD(Var, SetBool))
		.Method("Var@ opAssign(int)", asMETHOD(Var, SetInt))
		.Method("Var@ opAssign(float)", asMETHOD(Var, SetFloat))
		.Method("Var@ opAssign(Var@)", asMETHOD(Var, SetVar))
		.Method("bool GetBool() const", asMETHOD(Var, GetBool))
		.Method("int GetInt() const", asMETHOD(Var, GetInt))
		.Method("float GetFloat() const", asMETHOD(Var, GetFloat));

	AddType("VarsContainer")
		.Method("Var@ opIndex(const string& in)", asMETHOD(VarsContainer, Get))
		.WithInstance("VarsContainer@ globals", &p_globals);

	AddType("Dialog")
		.WithNamespace()
		.AddFunction("Dialog@ Get(const string& in)", asFUNCTION(GameDialog::GetS));

	AddType("Quest")
		.Method("void AddEntry(const string& in)", asMETHOD(Quest_Scripted, AddEntry))
		.Method("void SetStarted(const string& in)", asMETHOD(Quest_Scripted, SetStarted))
		.Method("void SetFailed()", asMETHOD(Quest_Scripted, SetFailed))
		.Method("void SetCompleted()", asMETHOD(Quest_Scripted, SetCompleted))
		.Method("void SetTimeout(int)", asMETHOD(Quest_Scripted, SetTimeout))
		.Method("void SetProgress(int)", asMETHOD(Quest_Scripted, SetProgress))
		.Method("int get_progress()", asMETHOD(Quest_Scripted, GetProgress))
		.Method("string GetString(int)", asMETHOD(Quest_Scripted, GetString))
		.Method("Dialog@ GetDialog(const string& in)", asMETHODPR(Quest_Scripted, GetDialog, (const string&), GameDialog*))
		.Method("void AddRumor(const string& in)", asMETHOD(Quest_Scripted, AddRumor))
		.Method("void RemoveRumor()", asMETHOD(Quest_Scripted, RemoveRumor))
		.WithInstance("Quest@ quest", &ctx.quest);

	AddType("Item")
		.Member("const int value", offsetof(Item, value))
		.Member("const string name", offsetof(Item, name))
		.Method("Item@ QuestCopy(Quest@, const string& in)", asMETHOD(Item, QuestCopy))
		.WithNamespace()
		.AddFunction("Item@ Get(const string& in)", asFUNCTION(Item::GetS))
		.AddFunction("Item@ GetRandom(int)", asFUNCTION(ItemHelper::GetRandomItem));

	AddType("ItemList")
		.Method("Item@ GetItem()", asMETHODPR(ItemList, Get, () const, const Item*))
		.Method("Item@ GetItem(int)", asMETHOD(ItemList, GetByIndex))
		.Method("int Size()", asMETHOD(ItemList, GetSize))
		.WithNamespace()
		.AddFunction("ItemList@ Get(const string& in)", asFUNCTION(ItemList::GetS));

	AddType("GroundItem")
		.Member("const Vec3 pos", offsetof(GroundItem, pos))
		.Member("const Item@ base", offsetof(GroundItem, item));

	AddType("UnitData")
		.WithNamespace()
		.AddFunction("UnitData@ Get(const string& in)", asFUNCTION(UnitData::GetS));

	AddType("UnitOrderBuilder")
		.Method("UnitOrderBuilder@ WithTimer(float)", asMETHOD(UnitOrderEntry, WithTimer))
		.Method("UnitOrderBuilder@ ThenWander()", asMETHOD(UnitOrderEntry, ThenWander))
		.Method("UnitOrderBuilder@ ThenWait()", asMETHOD(UnitOrderEntry, ThenWait))
		.Method("UnitOrderBuilder@ ThenFollow(Unit@)", asMETHOD(UnitOrderEntry, ThenFollow))
		.Method("UnitOrderBuilder@ ThenLeave()", asMETHOD(UnitOrderEntry, ThenLeave))
		.Method("UnitOrderBuilder@ ThenMove(const Vec3& in, MOVE_TYPE)", asMETHOD(UnitOrderEntry, ThenMove))
		.Method("UnitOrderBuilder@ ThenLookAt(const Vec3& in)", asMETHOD(UnitOrderEntry, ThenLookAt))
		.Method("UnitOrderBuilder@ ThenEscapeTo(const Vec3& in)", asMETHOD(UnitOrderEntry, ThenEscapeTo))
		.Method("UnitOrderBuilder@ ThenEscapeToUnit(Unit@)", asMETHOD(UnitOrderEntry, ThenEscapeToUnit))
		.Method("UnitOrderBuilder@ ThenGoToInn()", asMETHOD(UnitOrderEntry, ThenGoToInn))
		.Method("UnitOrderBuilder@ ThenGuard(Unit@)", asMETHOD(UnitOrderEntry, ThenGuard))
		.Method("UnitOrderBuilder@ ThenAutoTalk(bool=true, Dialog@=null, Quest@=null)", asMETHOD(UnitOrderEntry, ThenAutoTalk));

	ForType("Unit")
		.Member("const Vec3 pos", offsetof(Unit, pos))
		.Member("const Player@ player", offsetof(Unit, player))
		.Member("const Hero@ hero", offsetof(Unit, hero))
		.Member("LevelArea@ area", offsetof(Unit, area))
		.Method("int get_gold() const", asMETHOD(Unit, GetGold))
		.Method("void set_gold(int)", asMETHOD(Unit, SetGold))
		.Method("VarsContainer@ get_vars()", asFUNCTION(Unit_GetVars)) // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		.Method("const string& get_name()", asMETHOD(Unit, GetNameS))
		.Method("void set_name(const string& in)", asMETHOD(Unit, SetName))
		.Method("bool get_dont_attack() const", asMETHOD(Unit, GetDontAttack))
		.Method("void set_dont_attack(bool)", asMETHOD(Unit, SetDontAttack))
		.Method("bool get_known_name() const", asMETHOD(Unit, GetKnownName))
		.Method("void set_known_name(bool)", asMETHOD(Unit, SetKnownName))
		.Method("bool IsTeamMember()", asMETHOD(Unit, IsTeamMember))
		.Method("bool IsFollowing(Unit@)", asMETHOD(Unit, IsFollowing))
		.Method("float GetHpp()", asMETHOD(Unit, GetHpp))
		.Method("void AddItem(Item@, uint = 1)", asMETHOD(Unit, AddItemS))
		.Method("void AddTeamItem(Item@, uint = 1)", asMETHOD(Unit, AddTeamItemS))
		.Method("uint RemoveItem(const string& in, uint = 1)", asMETHOD(Unit, RemoveItemS))
		.Method("uint RemoveItem(Item@, uint = 1)", asMETHODPR(Unit, RemoveItem, (const Item*, uint), uint))
		.Method("void RemoveQuestItem(Quest@)", asMETHOD(Unit, RemoveQuestItemS))
		.Method("void ConsumeItem(Item@)", asMETHOD(Unit, ConsumeItemS))
		.Method("void AddDialog(Quest@, const string& in)", asMETHOD(Unit, AddDialogS))
		.Method("void RemoveDialog(Quest@)", asMETHOD(Unit, RemoveDialog))
		.Method("void AddEventHandler(Quest@, EventType)", asMETHOD(Unit, AddEventHandler))
		.Method("void RemoveEventHandler(Quest@)", asMETHOD(Unit, RemoveEventHandlerS))
		.Method("UNIT_ORDER get_order() const", asMETHOD(Unit, GetOrder))
		.Method("void OrderClear()", asMETHOD(Unit, OrderClear))
		.Method("void OrderNext()", asMETHOD(Unit, OrderNext))
		.Method("void OrderAttack()", asMETHOD(Unit, OrderAttack))
		.Method("UnitOrderBuilder@ OrderWander()", asMETHOD(Unit, OrderWander))
		.Method("UnitOrderBuilder@ OrderWait()", asMETHOD(Unit, OrderWait))
		.Method("UnitOrderBuilder@ OrderFollow(Unit@)", asMETHOD(Unit, OrderFollow))
		.Method("UnitOrderBuilder@ OrderLeave()", asMETHOD(Unit, OrderLeave))
		.Method("UnitOrderBuilder@ OrderMove(const Vec3& in, MOVE_TYPE)", asMETHOD(Unit, OrderMove))
		.Method("UnitOrderBuilder@ OrderLookAt(const Vec3& in)", asMETHOD(Unit, OrderLookAt))
		.Method("UnitOrderBuilder@ OrderEscapeTo(const Vec3& in)", asMETHOD(Unit, OrderEscapeTo))
		.Method("UnitOrderBuilder@ OrderEscapeToUnit(Unit@)", asMETHOD(Unit, OrderEscapeToUnit))
		.Method("UnitOrderBuilder@ OrderGoToInn()", asMETHOD(Unit, OrderGoToInn))
		.Method("UnitOrderBuilder@ OrderGuard(Unit@)", asMETHOD(Unit, OrderGuard))
		.Method("UnitOrderBuilder@ OrderAutoTalk(bool=false, Dialog@=null, Quest@=null)", asMETHOD(Unit, OrderAutoTalk))
		.Method("void Talk(const string& in, int = -1)", asMETHOD(Unit, TalkS))
		.Method("void RotateTo(const Vec3& in)", asMETHODPR(Unit, RotateTo, (const Vec3&), void))
		.WithInstance("Unit@ target", &ctx.target)
		.WithNamespace()
		.AddFunction("Unit@ Id(int)", asFUNCTION(Unit::GetById));

	ForType("Player")
		.Member("Unit@ unit", offsetof(PlayerController, unit))
		.Member("const string name", offsetof(PlayerController, name))
		.Method("bool HavePerk(const string& in)", asMETHOD(PlayerController, HavePerkS))
		.Method("bool IsLeader()", asMETHOD(PlayerController, IsLeader))
		.WithInstance("Player@ pc", &ctx.pc);

	ForType("Hero")
		.Member("bool lost_pvp", offsetof(HeroData, lost_pvp));

	AddType("UnitGroup")
		.WithNamespace()
		.AddFunction("UnitGroup@ Get(const string& in)", asFUNCTION(UnitGroup::GetS))
		.AddObject("UnitGroup@ empty", &UnitGroup::empty);

	WithNamespace("Team", team)
		.AddFunction("Unit@ get_leader()", asMETHOD(Team, GetLeader))
		.AddFunction("uint get_size()", asMETHOD(Team, GetActiveTeamSize))
		.AddFunction("uint get_max_size()", asMETHOD(Team, GetMaxSize))
		.AddFunction("bool get_bandit()", asMETHOD(Team, IsBandit))
		.AddFunction("void set_bandit(bool)", asMETHOD(Team, SetBandit))
		.AddFunction("bool HaveMember()", asMETHOD(Team, HaveOtherActiveTeamMember))
		.AddFunction("bool HavePcMember()", asMETHOD(Team, HaveOtherPlayer))
		.AddFunction("bool HaveNpcMember()", asMETHOD(Team, HaveActiveNpc))
		.AddFunction("bool HaveItem(Item@)", asMETHOD(Team, HaveItem))
		.AddFunction("void AddGold(uint)", asMETHOD(Team, AddGoldS))
		.AddFunction("void AddExp(int)", asMETHOD(Team, AddExpS))
		.AddFunction("void AddReward(uint, uint = 0)", asMETHOD(Team, AddReward))
		.AddFunction("uint RemoveItem(Item@, uint = 1)", asMETHOD(Team, RemoveItem))
		.AddFunction("void AddMember(Unit@, int = 0)", asMETHOD(Team, AddTeamMember))
		.AddFunction("void Warp(const Vec3& in, const Vec3& in)", asMETHOD(Team, Warp));

	sb.AddStruct<TmpUnitGroup::Spawn>("Spawn");

	AddType("SpawnGroup", true)
		.Factory(asFUNCTION(TmpUnitGroup::GetInstanceS))
		.ReferenceCounting(asMETHOD(TmpUnitGroup, AddRefS), asMETHOD(TmpUnitGroup, ReleaseS))
		.Method("uint get_count()", asMETHOD(TmpUnitGroup, GetCount))
		.Method("void Fill(UnitGroup@, int, int)", asMETHOD(TmpUnitGroup, FillS))
		.Method("Spawn Get(uint)", asMETHOD(TmpUnitGroup, GetS));

	AddType("Location")
		.Member("const Vec2 pos", offsetof(Location, pos))
		.Member("const string name", offsetof(Location, name))
		.Member("const LOCATION type", offsetof(Location, type))
		.Member("int st", offsetof(Location, st))
		.Member("bool reset", offsetof(Location, reset))
		.Member("Quest@ active_quest", offsetof(Location, active_quest))
		.Member("UnitGroup@ group", offsetof(Location, group))
		.Method("const string& get_name() const", asMETHOD(Location, GetName))
		.Method("void set_name(const string& in)", asMETHOD(Location, SetNameS))
		.Method("LOCATION_IMAGE get_image() const", asMETHOD(Location, GetImage))
		.Method("void set_image(LOCATION_IMAGE)", asMETHOD(Location, SetImage))
		.Method("void AddEventHandler(Quest@, EventType)", asMETHOD(Location, AddEventHandler))
		.Method("void RemoveEventHandler(Quest@)", asMETHOD(Location, RemoveEventHandlerS))
		.Method("void SetKnown()", asMETHOD(Location, SetKnown))
		.Method("bool IsCity()", asFUNCTIONPR(LocationHelper::IsCity, (Location*), bool))
		.Method("bool IsVillage()", asFUNCTIONPR(LocationHelper::IsVillage, (Location*), bool));

	AddType("Encounter")
		.Member("Vec2 pos", offsetof(Encounter, pos))
		.Member("bool dont_attack", offsetof(Encounter, dont_attack))
		.Member("Quest@ quest", offsetof(Encounter, quest))
		.Member("Dialog@ dialog", offsetof(Encounter, dialog))
		.Member("int st", offsetof(Encounter, st))
		.Member("UnitGroup@ group", offsetof(Encounter, group))
		.Method("const string& get_text()", asMETHOD(Encounter, GetTextS))
		.Method("void set_text(const string& in)", asMETHOD(Encounter, SetTextS));

	CHECKED(engine->RegisterFuncdef("float GetLocationCallback(Location@)"));

	WithNamespace("World", world)
		.AddFunction("Vec2 get_size()", asMETHOD(World, GetSize))
		.AddFunction("Vec2 get_pos()", asMETHOD(World, GetPos))
		.AddFunction("uint GetSettlements()", asMETHOD(World, GetSettlements))
		.AddFunction("Location@ GetLocation(uint)", asMETHOD(World, GetLocation))
		.AddFunction("string GetDirName(const Vec2& in, const Vec2& in)", asFUNCTION(World_GetDirName)) // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		.AddFunction("float GetTravelDays(float)", asMETHOD(World, GetTravelDays))
		.AddFunction("Location@ GetRandomCity()", asFUNCTION(World_GetRandomCity)) // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		.AddFunction("Location@ GetRandomSettlementWithBuilding(const string& in)", asFUNCTION(World_GetRandomSettlementWithBuilding)) // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		.AddFunction("Location@ GetRandomSettlement(Location@)", asMETHODPR(World, GetRandomSettlement, (Location*), Location*))
		.AddFunction("Location@ GetRandomSettlement(GetLocationCallback@)", asFUNCTION(World_GetRandomSettlement))
		.AddFunction("Location@ GetClosestLocation(LOCATION, const Vec2& in, int = -1)", asMETHOD(World, GetClosestLocationS))
		.AddFunction("Location@ CreateLocation(LOCATION, const Vec2& in, LOCATION_TARGET = 0)", asMETHOD(World, CreateLocationS))
		.AddFunction("Encounter@ AddEncounter(Quest@)", asMETHOD(World, AddEncounterS))
		.AddFunction("void RemoveEncounter(Quest@)", asMETHODPR(World, RemoveEncounter, (Quest*), void))
		.AddFunction("void SetStartLocation(Location@)", asMETHOD(World, SetStartLocation));

	WithNamespace("Level", game_level)
		.AddFunction("Location@ get_location()", asMETHOD(Level, GetLocation))
		.AddFunction("bool IsSettlement()", asMETHOD(Level, IsSettlement))
		.AddFunction("bool IsCity()", asMETHOD(Level, IsCity))
		.AddFunction("bool IsVillage()", asMETHOD(Level, IsVillage))
		.AddFunction("bool IsTutorial()", asMETHOD(Level, IsTutorial))
		.AddFunction("bool IsSafe()", asMETHOD(Level, IsSafe))
		.AddFunction("Unit@ FindUnit(UnitData@)", asMETHODPR(Level, FindUnit, (UnitData*), Unit*))
		.AddFunction("Unit@ GetNearestEnemy(Unit@)", asMETHOD(Level, GetNearestEnemy))
		.AddFunction("GroundItem@ FindItem(Item@)", asMETHOD(Level, FindItem))
		.AddFunction("GroundItem@ FindNearestItem(Item@, const Vec3& in)", asMETHOD(Level, FindNearestItem))
		.AddFunction("void SpawnItemRandomly(Item@, uint = 1)", asMETHOD(Level, SpawnItemRandomly))
		.AddFunction("Unit@ SpawnUnitNearLocation(UnitData@, const Vec3& in, float = 2)", asMETHOD(Level, SpawnUnitNearLocationS))
		.AddFunction("Unit@ SpawnUnit(LevelArea@, Spawn)", asMETHOD(Level, SpawnUnit))
		.AddFunction("Unit@ GetMayor()", asMETHOD(Level, GetMayor));

	WithNamespace("StockScript")
		.AddFunction("void AddItem(Item@, uint = 1)", asFUNCTION(StockScript_AddItem)) // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		.AddFunction("void AddRandomItem(ITEM_TYPE, int, int, uint = 1)", asFUNCTION(StockScript_AddRandomItem)); // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

	AddType("Event")
		.Member("EventType event", offsetof(ScriptEvent, type))
		.Member("Location@ location", offsetof(ScriptEvent, location))
		.Member("Unit@ unit", offsetof(ScriptEvent, unit))
		.Member("GroundItem@ item", offsetof(ScriptEvent, item));

	WithNamespace("Cutscene", game)
		.AddFunction("void Start(bool = true)", asMETHOD(Game, CutsceneStart))
		.AddFunction("void End()", asMETHOD(Game, CutsceneEnd))
		.AddFunction("void Image(const string& in, float)", asMETHOD(Game, CutsceneImage))
		.AddFunction("void Text(const string& in, float)", asMETHOD(Game, CutsceneText))
		.AddFunction("bool ShouldSkip()", asMETHOD(Game, CutsceneShouldSkip));

	AddVarType(Var::Type::Bool, "bool", false);
	AddVarType(Var::Type::Int, "int", false);
	AddVarType(Var::Type::Float, "float", false);
	AddVarType(Var::Type::Int2, "Int2", false);
	AddVarType(Var::Type::Vec2, "Vec2", false);
	AddVarType(Var::Type::Vec3, "Vec3", false);
	AddVarType(Var::Type::Vec4, "Vec4", false);
	AddVarType(Var::Type::Item, "Item", true);
	AddVarType(Var::Type::Location, "Location", true);
	AddVarType(Var::Type::GroundItem, "GroundItem", true);
}

void ScriptManager::RunScript(cstring code)
{
	assert(code);

	// compile
	asIScriptModule* tmp_module = engine->GetModule("RunScriptModule", asGM_ALWAYS_CREATE);
	cstring packed_code = Format("void f() { %s; }", code);
	asIScriptFunction* func;
	int r = tmp_module->CompileFunction("RunScript", packed_code, -1, 0, &func);
	if(r < 0)
	{
		Log(Logger::L_ERROR, Format("Failed to parse script (%d).", r), code);
		return;
	}

	// run
	asIScriptContext* tmp_context = engine->RequestContext();
	r = tmp_context->Prepare(func);
	func->Release();
	if(r < 0)
	{
		Log(Logger::L_ERROR, Format("Failed to prepare script (%d).", r));
		engine->ReturnContext(tmp_context);
	}
	else
		ExecuteScript(tmp_context);
}

asIScriptFunction* ScriptManager::PrepareScript(cstring name, cstring code)
{
	assert(code);

	// compile
	asIScriptModule* tmp_module = engine->GetModule("RunScriptModule", asGM_ALWAYS_CREATE);
	if(!name)
		name = "f";
	cstring packed_code = Format("void %s() { %s; }", name, code);
	asIScriptFunction* func;
	int r = tmp_module->CompileFunction("RunScript", packed_code, -1, 0, &func);
	if(r < 0)
	{
		Log(Logger::L_ERROR, Format("Failed to compile script (%d).", r), code);
		return nullptr;
	}

	return func;
}

void ScriptManager::RunScript(asIScriptFunction* func, void* instance, delegate<void(asIScriptContext*, int)> clbk)
{
	// run
	asIScriptContext* tmp_context = engine->RequestContext();
	int r = tmp_context->Prepare(func);
	if(r >= 0)
	{
		if(instance)
			r = tmp_context->SetObject(instance);
		if(r >= 0)
		{
			if(clbk)
				clbk(tmp_context, 0);
		}
	}

	if(r < 0)
	{
		Log(Logger::L_ERROR, Format("Failed to prepare script (%d).", r));
		engine->ReturnContext(tmp_context);
		return;
	}

	last_exception = nullptr;
	r = tmp_context->Execute();

	if(r == asEXECUTION_SUSPENDED)
		return;

	if(r == asEXECUTION_FINISHED)
	{
		if(clbk)
			clbk(tmp_context, 1);
	}
	else
	{
		if(r == asEXECUTION_EXCEPTION)
		{
			cstring msg = last_exception ? last_exception : tmp_context->GetExceptionString();
			Log(Logger::L_ERROR, Format("Script exception thrown \"%s\" in %s(%d).", msg, tmp_context->GetExceptionFunction()->GetName(),
				tmp_context->GetExceptionFunction()));
		}
		else
			Log(Logger::L_ERROR, Format("Script execution failed (%d).", r));
	}

	engine->ReturnContext(tmp_context);
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

void ScriptManager::Log(Logger::Level level, cstring msg, cstring code)
{
	if(code)
		Logger::GetInstance()->Log(level, Format("ScriptManager: %s Code:\n%s", msg, code));
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
		Logger::GetInstance()->Log(level, msg);
}

void ScriptManager::AddFunction(cstring decl, const asSFuncPtr& funcPointer)
{
	assert(decl);
	CHECKED(engine->RegisterGlobalFunction(decl, funcPointer, asCALL_CDECL));
}

void ScriptManager::AddFunction(cstring decl, const asSFuncPtr& funcPointer, void* obj)
{
	assert(decl);
	CHECKED(engine->RegisterGlobalFunction(decl, funcPointer, asCALL_THISCALL_ASGLOBAL, obj));
}

void ScriptManager::AddEnum(cstring name, std::initializer_list<pair<cstring, int>> const& values)
{
	assert(name);
	CHECKED(engine->RegisterEnum(name));
	for(const pair<cstring, int>& value : values)
		CHECKED(engine->RegisterEnumValue(name, value.first, value.second));
}

TypeBuilder ScriptManager::AddType(cstring name, bool refcount)
{
	assert(name);
	int flags = asOBJ_REF | (refcount ? 0 : asOBJ_NOCOUNT);
	CHECKED(engine->RegisterObjectType(name, 0, flags));
	return ForType(name);
}

TypeBuilder ScriptManager::ForType(cstring name)
{
	assert(name);
	TypeBuilder builder(name, engine);
	return builder;
}

NamespaceBuilder ScriptManager::WithNamespace(cstring name, void* auxiliary)
{
	assert(name);
	return NamespaceBuilder(engine, name, auxiliary);
}

VarsContainer* ScriptManager::GetVars(Unit* unit)
{
	assert(unit);
	auto it = unit_vars.lower_bound(unit);
	if(it == unit_vars.end() || it->first != unit)
	{
		VarsContainer* vars = new VarsContainer;
		unit_vars.insert(it, std::unordered_map<Unit*, VarsContainer*>::value_type(unit, vars));
		return vars;
	}
	else
		return it->second;
}

Var& ScriptManager::GetVar(cstring name)
{
	return *globals.Get(name);
}

void ScriptManager::Reset()
{
	globals.Clear();
	DeleteElements(unit_vars);
	ctx.Clear();
}

void ScriptManager::Save(FileWriter& f)
{
	// global vars
	globals.Save(f);

	// unit vars
	uint count = 0;
	uint pos = f.BeginPatch(count);
	for(auto& e : unit_vars)
	{
		if(e.second->IsEmpty())
			continue;
		++count;
		f << e.first->id;
		e.second->Save(f);
	}
	if(count > 0)
		f.Patch(pos, count);
}

void ScriptManager::Load(FileReader& f)
{
	if(LOAD_VERSION < V_0_7)
		return;

	// global vars
	if(LOAD_VERSION >= V_0_8)
		globals.Load(f);

	// unit vars
	uint count;
	f >> count;
	for(uint i = 0; i < count; ++i)
	{
		int id = f.Read<int>();
		Unit* unit = Unit::GetById(id);
		VarsContainer* vars = new VarsContainer;
		vars->Load(f);
		unit_vars[unit] = vars;
	}
}

ScriptManager::RegisterResult ScriptManager::RegisterGlobalVar(const string& type, bool is_ref, const string& name)
{
	auto it = var_type_map.find(type);
	if(it == var_type_map.end())
		return InvalidType;
	ScriptTypeInfo info = script_type_infos[it->second];
	if(info.require_ref != is_ref)
		return InvalidType;
	Var* var = globals.TryGet(name);
	if(var)
		return AlreadyExists;
	var = globals.Add(info.type, name, true);
	var->_int = 0;
	cstring decl = Format("%s%s %s", type.c_str(), is_ref ? "@" : "", name.c_str());
	CHECKED(engine->RegisterGlobalProperty(decl, &var->ptr));
	return Ok;
}

void ScriptManager::AddVarType(Var::Type type, cstring name, bool is_ref)
{
	int type_id = engine->GetTypeIdByDecl(name);
	var_type_map[name] = type_id;
	script_type_infos[type_id] = ScriptTypeInfo(type, is_ref);
}

Var::Type ScriptManager::GetVarType(int type_id)
{
	if(IsSet(type_id, asTYPEID_OBJHANDLE))
		ClearBit(type_id, asTYPEID_OBJHANDLE);
	auto it = script_type_infos.find(type_id);
	assert(it != script_type_infos.end());
	return it->second.type;
}

bool ScriptManager::CheckVarType(int type_id, bool is_ref)
{
	if(IsSet(type_id, asTYPEID_OBJHANDLE))
	{
		is_ref = true;
		ClearBit(type_id, asTYPEID_OBJHANDLE);
	}

	auto it = script_type_infos.find(type_id);
	if(it == script_type_infos.end() || it->second.require_ref != is_ref)
		return false;
	return true;
}

void ScriptManager::ScriptSleep(float time)
{
	if(time <= 0)
		return;

	asIScriptContext* ctx = asGetActiveContext();
	ctx->Suspend();

	for(SuspendedScript& ss : suspended_scripts)
	{
		if(ss.ctx == ctx)
		{
			ss.time = time;
			return;
		}
	}

	SuspendedScript& ss = Add1(suspended_scripts);
	ss.ctx = ctx;
	ss.sctx = this->ctx;
	ss.time = time;
}

void ScriptManager::UpdateScripts(float dt)
{
	LoopAndRemove(suspended_scripts, [&](SuspendedScript& ss)
	{
		if(ss.time < 0)
			return false;

		ss.time -= dt;
		if(ss.time > 0.f)
			return false;

		this->ctx = ss.sctx;
		return ExecuteScript(ss.ctx);
	});
}

bool ScriptManager::ExecuteScript(asIScriptContext* ctx)
{
	assert(ctx);

	last_exception = nullptr;

	int r = ctx->Execute();
	if(r == asEXECUTION_SUSPENDED)
		return false;
	if(r != asEXECUTION_FINISHED)
	{
		if(r == asEXECUTION_EXCEPTION)
		{
			cstring msg = last_exception ? last_exception : ctx->GetExceptionString();
			Log(Logger::L_ERROR, Format("Script exception thrown \"%s\" in %s(%d).", msg, ctx->GetExceptionFunction()->GetName(),
				ctx->GetExceptionFunction()));
		}
		else
			Log(Logger::L_ERROR, Format("Script execution failed (%d).", r));
	}

	engine->ReturnContext(ctx);
	return true;
}

void ScriptManager::StopAllScripts()
{
	for(SuspendedScript& ss : suspended_scripts)
		engine->ReturnContext(ss.ctx);
	suspended_scripts.clear();
}

asIScriptContext* ScriptManager::SuspendScript()
{
	asIScriptContext* ctx = asGetActiveContext();
	if(!ctx)
		return nullptr;

	ctx->Suspend();

	SuspendedScript& ss = Add1(suspended_scripts);
	ss.ctx = ctx;
	ss.sctx = this->ctx;
	ss.time = -1;

	return ctx;
}

void ScriptManager::ResumeScript(asIScriptContext* ctx)
{
	for(auto it = suspended_scripts.begin(), end = suspended_scripts.end(); it != end; ++it)
	{
		if(it->ctx == ctx)
		{
			this->ctx = it->sctx;
			suspended_scripts.erase(it);
			ExecuteScript(ctx);
			return;
		}
	}
}
