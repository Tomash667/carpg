#include "Pch.h"
#include "ScriptManager.h"

#include "Ability.h"
#include "BaseLocation.h"
#include "City.h"
#include "DungeonMapGenerator.h"
#include "Encounter.h"
#include "Game.h"
#include "InsideLocation.h"
#include "ItemHelper.h"
#include "LocationHelper.h"
#include "Level.h"
#include "PlayerController.h"
#include "PlayerInfo.h"
#include "QuestManager.h"
#include "Quest_Scripted.h"
#include "RoomType.h"
#include "Team.h"
#include "TypeBuilder.h"
#include "UnitGroup.h"
#include "World.h"

#include <angelscript.h>
#include <scriptarray/scriptarray.h>
#include <scriptdictionary/scriptdictionary.h>
#include <scriptstdstring/scriptstdstring.h>

ScriptManager* script_mgr;
static std::map<int, asIScriptFunction*> tostring_map;
static string tmp_str_result;
Vars globals;
Vars* p_globals = &globals;

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
	for(pair<QuestScheme*, asIScriptObject*>& p : sharedInstances)
		p.second->Release();
	if(engine)
		engine->ShutDownAndRelease();
	DeleteElements(unit_vars);
}

void ScriptManager::Init()
{
	Info("Initializing ScriptManager...");

	engine = asCreateScriptEngine();
	engine->SetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES, 1);
	module = engine->GetModule("Core", asGM_CREATE_IF_NOT_EXISTS);

	CHECKED(engine->SetMessageCallback(asFUNCTION(MessageCallback), nullptr, asCALL_CDECL));

	RegisterScriptArray(engine, true);
	RegisterStdString(engine);
	RegisterStdStringUtils(engine);
	RegisterScriptDictionary(engine);
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

static cstring ArgToString(asIScriptGeneric* gen, int index)
{
	auto type_id = gen->GetArgTypeId(index);
	void* adr = gen->GetAddressOfArg(index);
	switch(type_id)
	{
	case asTYPEID_BOOL:
		{
			bool value = **(bool**)adr;
			return (value ? "true" : "false");
		}
	case asTYPEID_INT8:
		{
			char value = **(char**)adr;
			return Format("%d", (int)value);
		}
	case asTYPEID_INT16:
		{
			short value = **(short**)adr;
			return Format("%d", (int)value);
		}
	case asTYPEID_INT32:
		{
			int value = **(int**)adr;
			return Format("%d", value);
		}
	case asTYPEID_INT64:
		{
			int64 value = **(int64**)adr;
			return Format("%I64d", value);
		}
	case asTYPEID_UINT8:
		{
			byte value = **(byte**)adr;
			return Format("%u", (uint)value);
		}
	case asTYPEID_UINT16:
		{
			word value = **(word**)adr;
			return Format("%u", (uint)value);
		}
	case asTYPEID_UINT32:
		{
			uint value = **(uint**)adr;
			return Format("%u", value);
		}
	case asTYPEID_UINT64:
		{
			uint64 value = **(uint64**)adr;
			return Format("%I64u", value);
		}
	case asTYPEID_FLOAT:
		{
			float value = **(float**)adr;
			return Format("%g", value);
		}
		break;
	case asTYPEID_DOUBLE:
		{
			double value = **(double**)adr;
			return Format("%g", value);
		}
	default:
		return ToString(gen, adr, type_id).c_str();
	}
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
			result += '{';
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
		cstring s = ArgToString(gen, index);
		result += s;
		i = pos;
	}

	new(gen->GetAddressOfReturnLocation()) string(result);
}

static void ToStrGeneric(asIScriptGeneric* gen)
{
	cstring s = ArgToString(gen, 0);
	new(gen->GetAddressOfReturnLocation()) string(s);
}

static string Upper1(const string& str)
{
	string s(str);
	s[0] = toupper(s[0]);
	return s;
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

string String_Upper(string& str)
{
	string s = str;
	s[0] = toupper(s[0]);
	return s;
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
	AddFunction("string Upper1(const string& in)", asFUNCTION(Upper1));

	CHECKED(engine->RegisterGlobalFunction("string Str(?& in)", asFUNCTION(ToStrGeneric), asCALL_GENERIC));

	sb.AddStruct<Int2>("Int2", asOBJ_POD | asOBJ_APP_CLASS_ALLINTS)
		.Constructor()
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

	sb.AddStruct<Vec2>("Vec2", asOBJ_POD | asOBJ_APP_CLASS_ALLFLOATS)
		.Constructor()
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

	sb.AddStruct<Vec3>("Vec3", asOBJ_POD | asOBJ_APP_CLASS_ALLFLOATS)
		.Constructor()
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

	sb.AddStruct<Vec4>("Vec4", asOBJ_POD | asOBJ_APP_CLASS_ALLFLOATS)
		.Constructor()
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

	sb.ForType("string")
		.Method("string Upper() const", asFUNCTION(String_Upper));

	sb.AddStruct<SpawnPoint>("SpawnPoint", asOBJ_POD | asOBJ_APP_CLASS_ALLFLOATS)
		.Member("Vec3 pos", offsetof(SpawnPoint, pos))
		.Member("float rot", offsetof(SpawnPoint, rot));

	sb.AddStruct<Date>("Date", asOBJ_POD | asOBJ_APP_CLASS_ALLINTS)
		.Constructor()
		.Constructor<int, int, int>("void f(int, int, int)")
		.Member("int year", offsetof(Date, year))
		.Member("int month", offsetof(Date, month))
		.Member("int day", offsetof(Date, day));

	AddFunction("void Sleep(float)", asMETHOD(ScriptManager, ScriptSleep), this);
}

Vars* Unit_GetVars(Unit* unit)
{
	return script_mgr->GetVars(unit);
}

string World_GetDirName(const Vec2& pos1, const Vec2& pos2)
{
	return GetLocationDirName(pos1, pos2);
}

string World_GetDirName2(Location* loc1, Location* loc2)
{
	return GetLocationDirName(loc1->pos, loc2->pos);
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
	AddType("Location");
	AddType("LocationPart");

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
		{ "ITEM_NOT_BLACKSMITH", ITEM_NOT_BLACKSMITH }
		});

	AddEnum("EVENT", {
		{ "EVENT_ANY", EVENT_ANY },
		{ "EVENT_ENTER", EVENT_ENTER },
		{ "EVENT_PICKUP", EVENT_PICKUP },
		{ "EVENT_UPDATE", EVENT_UPDATE },
		{ "EVENT_TIMEOUT", EVENT_TIMEOUT },
		{ "EVENT_ENCOUNTER", EVENT_ENCOUNTER },
		{ "EVENT_DIE", EVENT_DIE },
		{ "EVENT_CLEARED", EVENT_CLEARED },
		{ "EVENT_GENERATE", EVENT_GENERATE }
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
		{ "CAPITAL", CAPITAL },
		{ "HUMAN_FORT", HUMAN_FORT },
		{ "DWARF_FORT", DWARF_FORT },
		{ "MAGE_TOWER", MAGE_TOWER },
		{ "BANDITS_HIDEOUT", BANDITS_HIDEOUT },
		{ "HERO_CRYPT", HERO_CRYPT },
		{ "MONSTER_CRYPT", MONSTER_CRYPT },
		{ "OLD_TEMPLE", OLD_TEMPLE },
		{ "VAULT", VAULT },
		{ "NECROMANCER_BASE", NECROMANCER_BASE },
		{ "LABYRINTH", LABYRINTH },
		{ "ANCIENT_ARMORY", ANCIENT_ARMORY },
		{ "TUTORIAL_FORT", TUTORIAL_FORT },
		{ "THRONE_FORT", THRONE_FORT },
		{ "THRONE_VAULT", THRONE_VAULT },
		{ "HUNTERS_CAMP", HUNTERS_CAMP },
		{ "HILLS", HILLS }
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
		{ "LI_CAPITAL", LI_CAPITAL },
		{ "LI_HUNTERS_CAMP", LI_HUNTERS_CAMP },
		{ "LI_HILLS", LI_HILLS }
		});

	AddEnum("QUEST_STATE", {
		{ "Q_HIDDEN", Quest::Hidden },
		{ "Q_STARTED", Quest::Started },
		{ "Q_COMPLETED", Quest::Completed },
		{ "Q_FAILED", Quest::Failed }
		});

	AddEnum<RoomTarget>("ROOM_TARGET", {
		{ "ROOM_NONE", RoomTarget::None },
		{ "ROOM_CORRIDOR", RoomTarget::Corridor },
		{ "ROOM_ENTRY_PREV", RoomTarget::EntryPrev },
		{ "ROOM_ENTRY_NEXT", RoomTarget::EntryNext },
		{ "ROOM_TREASURY", RoomTarget::Treasury },
		{ "ROOM_PORTAL", RoomTarget::Portal },
		{ "ROOM_PRISON", RoomTarget::Prison },
		{ "ROOM_THRONE", RoomTarget::Throne }
		});

	AddEnum("ENTRY_LOCATION", {
		{ "ENTRY_NONE", MapSettings::ENTRY_NONE },
		{ "ENTRY_RANDOM", MapSettings::ENTRY_RANDOM },
		{ "ENTRY_FAR_FROM_ROOM", MapSettings::ENTRY_FAR_FROM_ROOM },
		{ "ENTRY_BORDER", MapSettings::ENTRY_BORDER },
		{ "ENTRY_FAR_FROM_PREV", MapSettings::ENTRY_FAR_FROM_PREV }
		});

	AddEnum("GetLocationFlag", {
		{ "F_ALLOW_ACTIVE", F_ALLOW_ACTIVE },
		{ "F_EXCLUDED", F_EXCLUDED }
		});

	AddType("Var")
		.Method("bool opEquals(bool) const", asMETHODPR(Var, IsBool, (bool) const, bool))
		.Method("bool opEquals(int) const", asMETHODPR(Var, IsInt, (int) const, bool))
		.Method("bool opEquals(float) const", asMETHODPR(Var, IsFloat, (float) const, bool))
		.Method("bool opEquals(const ?& in)", asMETHOD(Var, IsGeneric))
		.Method("Var& opAssign(bool)", asMETHOD(Var, SetBool))
		.Method("Var& opAssign(int)", asMETHOD(Var, SetInt))
		.Method("Var& opAssign(float)", asMETHOD(Var, SetFloat))
		.Method("Var& opAssign(const ?& in)", asMETHOD(Var, SetGeneric))
		.Method("bool opImplConv()", asMETHOD(Var, GetBool))
		.Method("int opImplConv()", asMETHOD(Var, GetInt))
		.Method("float opImplConv()", asMETHOD(Var, GetFloat))
		.Method("void opImplConv(?& out)", asMETHOD(Var, GetGeneric))
		.Method("void opImplCast(?& out)", asMETHOD(Var, GetGeneric));

	AddType("Vars", true)
		.Factory(asFUNCTION(Vars::Create))
		.ReferenceCounting(asMETHOD(Vars, AddRef), asMETHOD(Vars, Release))
		.Method("Var@ opIndex(const string& in)", asMETHOD(Vars, Get))
		.Method("bool IsSet(const string& in)", asMETHOD(Vars, IsSet))
		.WithInstance("Vars@ globals", &p_globals);

	AddType("Dialog")
		.WithNamespace()
		.AddProperty("int var", &DialogContext::var)
		.AddFunction("Dialog@ Get(const string& in)", asFUNCTION(GameDialog::GetS));

	AddType("Building");

	AddType("BuildingGroup")
		.WithNamespace()
		.AddFunction("BuildingGroup@ Get(const string& in)", asFUNCTION(BuildingGroup::GetS));

	AddType("CityBuilding")
		.Method("Vec3 get_unit_pos() property", asMETHOD(CityBuilding, GetUnitPos))
		.Method("float get_unit_rot() property", asMETHOD(CityBuilding, GetUnitRot));

	AddType("Quest")
		.Member("const QUEST_STATE state", offsetof(Quest_Scripted, state))
		.Member("Location@ startLoc", offsetof(Quest_Scripted, startLoc))
		.Method("int get_timeout() property", asMETHOD(Quest_Scripted, GetTimeout))
		.Method("int get_progress() property", asMETHOD(Quest_Scripted, GetProgress))
		.Method("void AddEntry(const string& in)", asMETHOD(Quest_Scripted, AddEntry))
		.Method("void SetStarted(const string& in)", asMETHOD(Quest_Scripted, SetStarted))
		.Method("void SetCompleted()", asMETHOD(Quest_Scripted, SetCompleted))
		.Method("void SetFailed()", asMETHOD(Quest_Scripted, SetFailed))
		.Method("void SetTimeout(int)", asMETHOD(Quest_Scripted, SetTimeout))
		.Method("void SetProgress(int)", asMETHOD(Quest_Scripted, SetProgress))
		.Method("string GetString(int)", asMETHOD(Quest_Scripted, GetString))
		.Method("Dialog@ GetDialog(const string& in)", asMETHODPR(Quest_Scripted, GetDialog, (const string&), GameDialog*))
		.Method("Var@ GetValue(int offset)", asMETHOD(Quest2, GetValue))
		.Method("void AddRumor(const string& in)", asMETHOD(Quest_Scripted, AddRumor))
		.Method("void RemoveRumor()", asMETHOD(Quest_Scripted, RemoveRumor))
		.Method("void Start(Vars@)", asMETHODPR(Quest_Scripted, Start, (Vars*), void))
		.WithInstance("Quest@ quest", &ctx.quest)
		.WithNamespace(quest_mgr)
		.AddFunction("Quest@ Find(const string& in)", asMETHOD(QuestManager, FindQuestS))
		.AddFunction("int CalculateReward(int, const Int2& in, const Int2& in)", asFUNCTION(ItemHelper::CalculateReward));

	AddType("Item")
		.Member("const int value", offsetof(Item, value))
		.Method("const string& get_name() const property", asMETHOD(Item, GetName))
		.Method("void set_name(const string& in) property", asMETHOD(Item, RenameS))
		.Method("Item@ QuestCopy(Quest@)", asMETHODPR(Item, QuestCopy, (Quest*), Item*))
		.Method("Item@ QuestCopy(Quest@, const string& in)", asMETHODPR(Item, QuestCopy, (Quest*, const string&), Item*))
		.WithNamespace()
		.AddFunction("Item@ Get(const string& in)", asFUNCTION(Item::GetS))
		.AddFunction("Item@ GetRandom(int)", asFUNCTION(ItemHelper::GetRandomItem));

	AddType("ItemList")
		.Method("Item@ GetItem()", asMETHODPR(ItemList, Get, () const, const Item*))
		.Method("Item@ GetItem(int)", asMETHOD(ItemList, GetByIndex))
		.Method("int Size()", asMETHOD(ItemList, GetSize))
		.WithNamespace()
		.AddFunction("ItemList@ Get(const string& in)", asFUNCTION(ItemList::GetS))
		.AddFunction("Item@ GetRandomItem(const string& in)", asFUNCTION(ItemList::GetItemS));

	AddType("GroundItem")
		.Member("const Vec3 pos", offsetof(GroundItem, pos))
		.Member("const Item@ base", offsetof(GroundItem, item));

	AddType("Ability")
		.WithNamespace()
		.AddFunction("Ability@ Get(const string& in)", asFUNCTION(Ability::GetS));

	AddType("Recipe")
		.WithNamespace()
		.AddFunction("Recipe@ Get(const string& in)", asFUNCTION(Recipe::GetS));

	AddType("UnitData")
		.Member("const string id", offsetof(UnitData, id))
		.WithNamespace()
		.AddFunction("UnitData@ Get(const string& in)", asFUNCTION(UnitData::GetS));

	AddType("UnitOrderBuilder")
		.Method("UnitOrderBuilder@ WithTimer(float)", asMETHOD(UnitOrderEntry, WithTimer))
		.Method("UnitOrderBuilder@ WithMoveType(MOVE_TYPE)", asMETHOD(UnitOrderEntry, WithMoveType))
		.Method("UnitOrderBuilder@ WithRange(float)", asMETHOD(UnitOrderEntry, WithRange))
		.Method("UnitOrderBuilder@ ThenWander()", asMETHOD(UnitOrderEntry, ThenWander))
		.Method("UnitOrderBuilder@ ThenWait()", asMETHOD(UnitOrderEntry, ThenWait))
		.Method("UnitOrderBuilder@ ThenFollow(Unit@)", asMETHOD(UnitOrderEntry, ThenFollow))
		.Method("UnitOrderBuilder@ ThenLeave()", asMETHOD(UnitOrderEntry, ThenLeave))
		.Method("UnitOrderBuilder@ ThenMove(const Vec3& in)", asMETHOD(UnitOrderEntry, ThenMove))
		.Method("UnitOrderBuilder@ ThenLookAt(const Vec3& in)", asMETHOD(UnitOrderEntry, ThenLookAt))
		.Method("UnitOrderBuilder@ ThenEscapeTo(const Vec3& in)", asMETHOD(UnitOrderEntry, ThenEscapeTo))
		.Method("UnitOrderBuilder@ ThenEscapeToUnit(Unit@)", asMETHOD(UnitOrderEntry, ThenEscapeToUnit))
		.Method("UnitOrderBuilder@ ThenGoToInn()", asMETHOD(UnitOrderEntry, ThenGoToInn))
		.Method("UnitOrderBuilder@ ThenGuard(Unit@)", asMETHOD(UnitOrderEntry, ThenGuard))
		.Method("UnitOrderBuilder@ ThenAutoTalk(bool=false, Dialog@=null, Quest@=null)", asMETHOD(UnitOrderEntry, ThenAutoTalk));

	ForType("Unit")
		.Member("const UnitData@ data", offsetof(Unit, data))
		.Member("const Vec3 pos", offsetof(Unit, pos))
		.Member("const Player@ player", offsetof(Unit, player))
		.Member("const Hero@ hero", offsetof(Unit, hero))
		.Member("LocationPart@ locPart", offsetof(Unit, locPart))
		.Member("bool temporary", offsetof(Unit, temporary))
		.Method("int get_gold() const property", asMETHOD(Unit, GetGold))
		.Method("void set_gold(int) property", asMETHOD(Unit, SetGold))
		.Method("Vars@ get_vars() property", asFUNCTION(Unit_GetVars)) // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		.Method("const string& get_name() property", asMETHOD(Unit, GetNameS))
		.Method("const string& get_real_name() property", asMETHOD(Unit, GetRealNameS))
		.Method("void set_name(const string& in) property", asMETHOD(Unit, SetName))
		.Method("bool get_dont_attack() const property", asMETHOD(Unit, GetDontAttack))
		.Method("void set_dont_attack(bool) property", asMETHOD(Unit, SetDontAttack))
		.Method("bool get_known_name() const property", asMETHOD(Unit, GetKnownName))
		.Method("void set_known_name(bool) property", asMETHOD(Unit, SetKnownName))
		.Method("const string& get_clas() const property", asMETHOD(Unit, GetClassId))
		.Method("bool IsTeamMember()", asMETHOD(Unit, IsTeamMember))
		.Method("bool IsFollowing(Unit@)", asMETHOD(Unit, IsFollowing))
		.Method("bool IsEnemy(Unit@, bool = false)", asMETHOD(Unit, IsEnemy))
		.Method("float GetHpp()", asMETHOD(Unit, GetHpp))
		.Method("void AddItem(Item@, uint = 1)", asMETHOD(Unit, AddItemS))
		.Method("void AddTeamItem(Item@, uint = 1)", asMETHOD(Unit, AddTeamItemS))
		.Method("uint RemoveItem(const string& in, uint = 1)", asMETHOD(Unit, RemoveItemS))
		.Method("uint RemoveItem(Item@, uint = 1)", asMETHODPR(Unit, RemoveItem, (const Item*, uint), uint))
		.Method("void RemoveQuestItem(Quest@)", asMETHOD(Unit, RemoveQuestItemS))
		.Method("void ConsumeItem(Item@)", asMETHOD(Unit, ConsumeItemS))
		.Method("void AddDialog(Quest@, const string& in, int priority = 0)", asMETHOD(Unit, AddDialogS))
		.Method("void RemoveDialog(Quest@)", asMETHOD(Unit, RemoveDialogS))
		.Method("void AddEventHandler(Quest@, EVENT)", asMETHOD(Unit, AddEventHandler))
		.Method("void RemoveEventHandler(Quest@, EVENT = EVENT_ANY)", asMETHOD(Unit, RemoveEventHandlerS))
		.Method("UNIT_ORDER get_order() const property", asMETHOD(Unit, GetOrder))
		.Method("void OrderClear()", asMETHOD(Unit, OrderClear))
		.Method("void OrderNext()", asMETHOD(Unit, OrderNext))
		.Method("void OrderAttack()", asMETHOD(Unit, OrderAttack))
		.Method("UnitOrderBuilder@ OrderWander()", asMETHOD(Unit, OrderWander))
		.Method("UnitOrderBuilder@ OrderWait()", asMETHOD(Unit, OrderWait))
		.Method("UnitOrderBuilder@ OrderFollow(Unit@)", asMETHOD(Unit, OrderFollow))
		.Method("UnitOrderBuilder@ OrderLeave()", asMETHOD(Unit, OrderLeave))
		.Method("UnitOrderBuilder@ OrderMove(const Vec3& in)", asMETHOD(Unit, OrderMove))
		.Method("UnitOrderBuilder@ OrderLookAt(const Vec3& in)", asMETHOD(Unit, OrderLookAt))
		.Method("UnitOrderBuilder@ OrderEscapeTo(const Vec3& in)", asMETHOD(Unit, OrderEscapeTo))
		.Method("UnitOrderBuilder@ OrderEscapeToUnit(Unit@)", asMETHOD(Unit, OrderEscapeToUnit))
		.Method("UnitOrderBuilder@ OrderGoToInn()", asMETHOD(Unit, OrderGoToInn))
		.Method("UnitOrderBuilder@ OrderGuard(Unit@)", asMETHOD(Unit, OrderGuard))
		.Method("UnitOrderBuilder@ OrderAutoTalk(bool=false, Dialog@=null, Quest@=null)", asMETHOD(Unit, OrderAutoTalk))
		.Method("void Talk(const string& in, int = -1)", asMETHOD(Unit, TalkS))
		.Method("void RotateTo(const Vec3& in)", asMETHODPR(Unit, RotateTo, (const Vec3&), void))
		.Method("void RotateTo(float)", asMETHODPR(Unit, RotateTo, (float), void))
		.Method("void ChangeBase(UnitData@, bool=false)", asMETHOD(Unit, ChangeBase))
		.Method("void MoveToLocation(LocationPart@, const Vec3& in)", asMETHOD(Unit, MoveToLocation))
		.Method("void MoveOffscreen()", asMETHOD(Unit, MoveOffscreen))
		.Method("void Kill()", asMETHOD(Unit, Kill))
		.WithInstance("Unit@ target", &ctx.target)
		.WithNamespace()
		.AddFunction("Unit@ Id(int)", asFUNCTION(Unit::GetById));

	ForType("Player")
		.Member("Unit@ unit", offsetof(PlayerController, unit))
		.Member("const string name", offsetof(PlayerController, name))
		.Method("bool HaveAbility(Ability@)", asMETHOD(PlayerController, HaveAbility))
		.Method("bool HavePerk(const string& in)", asMETHOD(PlayerController, HavePerkS))
		.Method("bool IsLeader()", asMETHOD(PlayerController, IsLeader))
		.Method("bool AddAbility(Ability@)", asMETHOD(PlayerController, AddAbility))
		.Method("bool RemoveAbility(Ability@)", asMETHOD(PlayerController, RemoveAbility))
		.Method("bool AddRecipe(Recipe@)", asMETHOD(PlayerController, AddRecipe))
		.WithInstance("Player@ pc", &ctx.pc);

	ForType("Hero")
		.Member("bool lost_pvp", offsetof(Hero, lost_pvp))
		.Member("const int investment", offsetof(Hero, investment))
		.Method("bool get_otherTeam() const property", asMETHOD(Hero, HaveOtherTeam))
		.Method("int get_persuasionCheck() const property", asMETHOD(Hero, GetPersuasionCheckValue))
		.Method("bool get_wantJoin() const property", asMETHOD(Hero, WantJoin));

	AddType("UnitGroup")
		.Member("const string name", offsetof(UnitGroup, name))
		.Member("const bool female", offsetof(UnitGroup, gender))
		.Method("UnitData@ GetLeader(int)", asMETHOD(UnitGroup, GetLeader))
		.WithNamespace()
		.AddProperty("UnitGroup@ empty", &UnitGroup::empty)
		.AddFunction("UnitGroup@ Get(const string& in)", asFUNCTION(UnitGroup::GetS));

	WithNamespace("Team", team)
		.AddFunction("Unit@ get_leader() property", asMETHOD(Team, GetLeader))
		.AddFunction("uint get_size() property", asMETHOD(Team, GetActiveTeamSize))
		.AddFunction("uint get_max_size() property", asMETHOD(Team, GetMaxSize))
		.AddFunction("bool get_bandit() property", asMETHOD(Team, IsBandit))
		.AddFunction("void set_bandit(bool) property", asMETHOD(Team, SetBandit))
		.AddFunction("bool HaveMember()", asMETHOD(Team, HaveOtherActiveTeamMember))
		.AddFunction("bool HavePcMember()", asMETHOD(Team, HaveOtherPlayer))
		.AddFunction("bool HaveNpcMember()", asMETHOD(Team, HaveActiveNpc))
		.AddFunction("bool HaveItem(Item@)", asMETHOD(Team, HaveItem))
		.AddFunction("void AddGold(uint)", asMETHOD(Team, AddGoldS))
		.AddFunction("void AddExp(int)", asMETHOD(Team, AddExpS))
		.AddFunction("void AddReward(uint, uint = 0)", asMETHOD(Team, AddReward))
		.AddFunction("uint RemoveItem(Item@, uint = 1)", asMETHOD(Team, RemoveItem))
		.AddFunction("void AddMember(Unit@, int = 0)", asMETHOD(Team, AddMember))
		.AddFunction("void RemoveMember(Unit@)", asMETHOD(Team, RemoveMember))
		.AddFunction("void Warp(const Vec3& in, const Vec3& in)", asMETHOD(Team, Warp))
		.AddFunction("bool PersuasionCheck(int)", asMETHOD(Team, PersuasionCheck));

	sb.AddStruct<TmpUnitGroup::Spawn>("Spawn");

	AddType("SpawnGroup", true)
		.Factory(asFUNCTION(TmpUnitGroup::GetInstanceS))
		.ReferenceCounting(asMETHOD(TmpUnitGroup, AddRefS), asMETHOD(TmpUnitGroup, ReleaseS))
		.Method("uint get_count() property", asMETHOD(TmpUnitGroup, GetCount))
		.Method("void Fill(UnitGroup@, int, int)", asMETHOD(TmpUnitGroup, FillS))
		.Method("Spawn Get(uint)", asMETHOD(TmpUnitGroup, GetS));

	AddType("BaseObject")
		.WithNamespace()
		.AddFunction("BaseObject@ Get(const string& in)", asFUNCTION(BaseObject::GetS));

	AddType("Object");

	AddType("Chest")
		.Method("bool AddItem(Item@, uint = 1)", asMETHODPR(Chest, AddItem, (const Item*, uint), bool));

	AddType("RoomType")
		.WithNamespace()
		.AddFunction("RoomType@ Get(const string& in)", asFUNCTION(RoomType::GetS));

	AddType("Room")
		.Member("ROOM_TARGET target", offsetof(Room, target))
		.Member("RoomType@ type", offsetof(Room, type))
		.Method("Vec3 get_center() const property", asMETHOD(Room, Center));

	ForType("Location")
		.Member("const Vec2 pos", offsetof(Location, pos))
		.Member("const string name", offsetof(Location, name))
		.Member("const LOCATION type", offsetof(Location, type))
		.Member("const bool outside", offsetof(Location, outside))
		.Member("int st", offsetof(Location, st))
		.Member("bool reset", offsetof(Location, reset))
		.Member("Quest@ active_quest", offsetof(Location, active_quest))
		.Member("UnitGroup@ group", offsetof(Location, group))
		.Method("const string& get_name() const property", asMETHOD(Location, GetName))
		.Method("void set_name(const string& in) property", asMETHOD(Location, SetNameS))
		.Method("LOCATION_IMAGE get_image() const property", asMETHOD(Location, GetImage))
		.Method("void set_image(LOCATION_IMAGE) property", asMETHOD(Location, SetImage))
		.Method("bool get_visited() const property", asMETHOD(Location, IsVisited))
		.Method("LocationPart@ get_locPart() const property", asFUNCTIONPR(LocationHelper::GetLocationPart, (Location*), LocationPart*))
		.Method("int get_levels() const property", asFUNCTION(LocationHelper::GetLevels))
		.Method("void AddEventHandler(Quest@, EVENT)", asMETHOD(Location, AddEventHandler))
		.Method("void RemoveEventHandler(Quest@, EVENT = EVENT_ANY)", asMETHOD(Location, RemoveEventHandlerS))
		.Method("void SetKnown()", asMETHOD(Location, SetKnown))
		.Method("bool IsCity()", asFUNCTIONPR(LocationHelper::IsCity, (Location*), bool))
		.Method("bool IsVillage()", asFUNCTIONPR(LocationHelper::IsVillage, (Location*), bool))
		.Method("Unit@ GetMayor()", asFUNCTION(LocationHelper::GetMayor))
		.Method("Unit@ GetCaptain()", asFUNCTION(LocationHelper::GetCaptain))
		.Method("LocationPart@ GetLocationPart(int)", asFUNCTIONPR(LocationHelper::GetLocationPart, (Location*, int), LocationPart*))
		.Method("LocationPart@ GetBuildingLocationPart(const string& in)", asFUNCTION(LocationHelper::GetBuildingLocationPart))
		.Method("int GetRandomLevel()", asMETHOD(Location, GetRandomLevel))
		.Method("Unit@ FindQuestUnit(Quest@)", asFUNCTION(LocationHelper::FindQuestUnit));

	ForType("LocationPart")
		.Method("bool RemoveItemFromChest(Item@)", asMETHOD(LocationPart, RemoveItemFromChest))
		.Method("bool RemoveItemFromUnit(Item@)", asMETHOD(LocationPart, RemoveItemFromUnit))
		.Method("bool RemoveGroundItem(Item@)", asMETHODPR(LocationPart, RemoveGroundItem, (const Item*), bool));

	AddType("Encounter")
		.Member("Vec2 pos", offsetof(Encounter, pos))
		.Member("bool dont_attack", offsetof(Encounter, dont_attack))
		.Member("Quest@ quest", offsetof(Encounter, quest))
		.Member("Dialog@ dialog", offsetof(Encounter, dialog))
		.Member("int st", offsetof(Encounter, st))
		.Member("int chance", offsetof(Encounter, chance))
		.Member("UnitGroup@ group", offsetof(Encounter, group))
		.Method("const string& get_text() property", asMETHOD(Encounter, GetTextS))
		.Method("void set_text(const string& in) property", asMETHOD(Encounter, SetTextS));

	CHECKED(engine->RegisterFuncdef("float GetLocationCallback(Location@)"));

	WithNamespace("World", world)
		.AddFunction("Vec2 get_bounds() property", asMETHOD(World, GetWorldBounds))
		.AddFunction("Vec2 get_size() property", asMETHOD(World, GetSize))
		.AddFunction("Vec2 get_pos() property", asMETHOD(World, GetPos))
		.AddFunction("int get_worldtime() property", asMETHOD(World, GetWorldtime))
		.AddFunction("const Date& get_date() property", asMETHOD(World, GetDateValue))
		.AddFunction("const Date& get_startDate() property", asMETHOD(World, GetStartDate))
		.AddFunction("void set_startDate(const Date& in) property", asMETHOD(World, SetStartDate))
		.AddFunction("uint GetSettlements()", asMETHOD(World, GetSettlements))
		.AddFunction("Location@ GetLocation(uint)", asMETHOD(World, GetLocation))
		.AddFunction("string GetDirName(const Vec2& in, const Vec2& in)", asFUNCTION(World_GetDirName)) // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		.AddFunction("string GetDirName(Location@, Location@)", asFUNCTION(World_GetDirName2)) // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		.AddFunction("float GetTravelDays(float)", asMETHOD(World, GetTravelDays))
		.AddFunction("Vec2 FindPlace(const Vec2& in, float, bool = false)", asMETHODPR(World, FindPlace, (const Vec2&, float, bool), Vec2))
		.AddFunction("Vec2 FindPlace(const Vec2& in, float, float)", asMETHODPR(World, FindPlace, (const Vec2&, float, float), Vec2))
		.AddFunction("bool TryFindPlace(Vec2&, float, bool = false)", asMETHOD(World, TryFindPlace))
		.AddFunction("Vec2 GetRandomPlace()", asMETHOD(World, GetRandomPlace))
		.AddFunction("Location@ GetRandomCity()", asMETHOD(World, GetRandomCity))
		.AddFunction("Location@ GetRandomSettlementWithBuilding(const string& in)", asFUNCTION(World_GetRandomSettlementWithBuilding)) // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		.AddFunction("Location@ GetRandomSettlement(Location@)", asMETHODPR(World, GetRandomSettlement, (Location*) const, Location*))
		.AddFunction("Location@ GetRandomSettlement(GetLocationCallback@)", asFUNCTION(World_GetRandomSettlement))
		.AddFunction("Location@ GetRandomSpawnLocation(const Vec2& in, UnitGroup@, float = 160)", asMETHOD(World, GetRandomSpawnLocation))
		.AddFunction("Location@ GetClosestLocation(LOCATION, const Vec2& in, LOCATION_TARGET = LOCATION_TARGET(-1), int flags = 0)", asMETHODPR(World, GetClosestLocation, (LOCATION, const Vec2&, int, int), Location*))
		.AddFunction("Location@ GetClosestLocation(LOCATION, const Vec2& in, array<LOCATION_TARGET>@)", asMETHOD(World, GetClosestLocationArrayS))
		.AddFunction("Location@ CreateLocation(LOCATION, const Vec2& in, LOCATION_TARGET = LOCATION_TARGET(-1), int = -1)", asMETHODPR(World, CreateLocation, (LOCATION, const Vec2&, int, int), Location*))
		.AddFunction("Location@ CreateCamp(const Vec2& in, UnitGroup@)", asMETHOD(World, CreateCamp))
		.AddFunction("void AbadonLocation(Location@)", asMETHOD(World, AbadonLocation))
		.AddFunction("Encounter@ AddEncounter(Quest@)", asMETHOD(World, AddEncounterS))
		.AddFunction("Encounter@ RecreateEncounter(Quest@, int)", asMETHOD(World, RecreateEncounterS))
		.AddFunction("void RemoveEncounter(Quest@)", asMETHODPR(World, RemoveEncounter, (Quest*), void))
		.AddFunction("void SetStartLocation(Location@)", asMETHOD(World, SetStartLocation))
		.AddFunction("void AddNews(const string& in)", asMETHOD(World, AddNews))
		.AddFunction("Unit@ CreateUnit(UnitData@, int = -1)", asMETHOD(World, CreateUnit));

	WithNamespace("Level", game_level)
		.AddFunction("Location@ get_location() property", asMETHOD(Level, GetLocation))
		.AddFunction("int get_dungeon_level() property", asMETHOD(Level, GetDungeonLevel))
		.AddFunction("bool IsSettlement()", asMETHOD(Level, IsSettlement))
		.AddFunction("bool IsCity()", asMETHOD(Level, IsCity))
		.AddFunction("bool IsVillage()", asMETHOD(Level, IsVillage))
		.AddFunction("bool IsTutorial()", asMETHOD(Level, IsTutorial))
		.AddFunction("bool IsSafe()", asMETHOD(Level, IsSafe))
		.AddFunction("bool IsOutside()", asMETHOD(Level, IsOutside))
		.AddFunction("Unit@ FindUnit(UnitData@)", asMETHODPR(Level, FindUnit, (UnitData*), Unit*))
		.AddFunction("Unit@ GetNearestEnemy(Unit@)", asMETHOD(Level, GetNearestEnemy))
		.AddFunction("GroundItem@ FindItem(Item@)", asMETHOD(Level, FindItem))
		.AddFunction("GroundItem@ FindNearestItem(Item@, const Vec3& in)", asMETHOD(Level, FindNearestItem))
		.AddFunction("GroundItem@ SpawnItem(Item@, const Vec3& in)", asMETHOD(Level, SpawnItem))
		.AddFunction("GroundItem@ SpawnItem(Item@, Object@)", asMETHOD(Level, SpawnItemAtObject))
		.AddFunction("GroundItem@ SpawnItemInsideAnyRoom(Item@)", asMETHOD(Level, SpawnGroundItemInsideAnyRoom))
		.AddFunction("void SpawnItemRandomly(Item@, uint = 1)", asMETHOD(Level, SpawnItemRandomly))
		.AddFunction("Vec3 FindSpawnPos(Room@, Unit@)", asMETHODPR(Level, FindSpawnPos, (Room* room, Unit* unit), Vec3))
		.AddFunction("Vec3 FindSpawnPos(LocationPart@, Unit@)", asMETHODPR(Level, FindSpawnPos, (LocationPart&, Unit* unit), Vec3))
		.AddFunction("Vec3 GetSpawnCenter()", asMETHOD(Level, GetSpawnCenter))
		.AddFunction("Unit@ SpawnUnitNearLocation(UnitData@, const Vec3& in, float = 2, int = -1)", asMETHOD(Level, SpawnUnitNearLocationS))
		.AddFunction("Unit@ SpawnUnit(LocationPart@, Spawn)", asMETHOD(Level, SpawnUnit))
		.AddFunction("Unit@ SpawnUnit(Room@, UnitData@, int = -1)", asMETHOD(Level, SpawnUnitInsideRoomS))
		.AddFunction("Unit@ GetMayor()", asMETHOD(Level, GetMayor))
		.AddFunction("CityBuilding@ GetRandomBuilding(BuildingGroup@)", asMETHOD(Level, GetRandomBuilding))
		.AddFunction("Room@ GetRoom(ROOM_TARGET)", asMETHOD(Level, GetRoom))
		.AddFunction("Room@ GetFarRoom()", asMETHOD(Level, GetFarRoom))
		.AddFunction("Object@ FindObject(Room@, BaseObject@)", asMETHOD(Level, FindObjectInRoom))
		.AddFunction("Chest@ GetRandomChest(Room@)", asMETHOD(Level, GetRandomChest))
		.AddFunction("Chest@ GetTreasureChest()", asMETHOD(Level, GetTreasureChest))
		.AddFunction("array<Room@>@ FindPath(Room@, Room@)", asMETHOD(Level, FindPath))
		.AddFunction("array<Unit@>@ GetUnits(Room@)", asMETHOD(Level, GetUnits))
		.AddFunction("bool FindPlaceNearWall(BaseObject@, SpawnPoint& out)", asMETHOD(Level, FindPlaceNearWall))
		.AddFunction("Object@ SpawnObject(BaseObject@, const Vec3& in, float)", asMETHOD(Level, SpawnObject));

	WithNamespace("StockScript")
		.AddFunction("void AddItem(Item@, uint = 1)", asFUNCTION(StockScript_AddItem)) // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		.AddFunction("void AddRandomItem(ITEM_TYPE, int, int, uint = 1)", asFUNCTION(StockScript_AddRandomItem)); // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

	AddType("MapSettings")
		.Member("ENTRY_LOCATION prev_entry_loc", offsetof(MapSettings, prevEntryLoc))
		.Member("ENTRY_LOCATION next_entry_loc", offsetof(MapSettings, nextEntryLoc));

	AddType("Event")
		.Member("EVENT event", offsetof(ScriptEvent, type))
		.Member("bool cancel", offsetof(ScriptEvent, cancel))
		.Member("Location@ location", offsetof(ScriptEvent, on_generate.location))
		.Member("MapSettings@ map_settings", offsetof(ScriptEvent, on_generate.map_settings))
		.Member("int stage", offsetof(ScriptEvent, on_generate.stage))
		.Member("Unit@ unit", offsetof(ScriptEvent, on_pickup.unit))
		.Member("GroundItem@ item", offsetof(ScriptEvent, on_pickup.item));

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
	AddVarType(Var::Type::String, "string", true);
	AddVarType(Var::Type::Unit, "Unit", true);
	AddVarType(Var::Type::UnitGroup, "UnitGroup", true);
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
				tmp_context->GetExceptionLineNumber()));
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

Vars* ScriptManager::GetVars(Unit* unit)
{
	assert(unit);
	auto it = unit_vars.find(unit->id);
	Vars* vars;
	if(it == unit_vars.end())
	{
		vars = new Vars;
		unit_vars.insert(std::unordered_map<int, Vars*>::value_type(unit->id, vars));
	}
	else
		vars = it->second;
	vars->AddRef();
	return vars;
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

void ScriptManager::Save(GameWriter& f)
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
		f << e.first;
		e.second->Save(f);
	}
	if(count > 0)
		f.Patch(pos, count);
}

void ScriptManager::Load(GameReader& f)
{
	// global vars
	globals.Load(f);

	// unit vars
	uint count;
	f >> count;
	for(uint i = 0; i < count; ++i)
	{
		int id = f.Read<int>();
		Unit* unit = Unit::GetById(id);
		Vars* vars = new Vars;
		vars->Load(f);
		if(unit)
			unit_vars[id] = vars;
		else
			delete vars;
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
				ctx->GetExceptionLineNumber()));
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

asIScriptObject* ScriptManager::GetSharedInstance(QuestScheme* scheme)
{
	assert(scheme);
	for(pair<QuestScheme*, asIScriptObject*>& p : sharedInstances)
	{
		if(p.first == scheme)
			return p.second;
	}
	return nullptr;
}
