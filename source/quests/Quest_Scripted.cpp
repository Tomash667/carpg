#include "Pch.h"
#include "Quest_Scripted.h"

#include "City.h"
#include "DialogContext.h"
#include "Encounter.h"
#include "GameGui.h"
#include "GameMessages.h"
#include "GroundItem.h"
#include "Journal.h"
#include "Net.h"
#include "QuestManager.h"
#include "QuestScheme.h"
#include "ScriptManager.h"
#include "World.h"

#include <angelscript.h>
#include <scriptdictionary\scriptdictionary.h>
#pragma warning(error: 4062)

//=================================================================================================
Quest_Scripted::Quest_Scripted() : instance(nullptr), call_depth(0), in_upgrade(false)
{
	type = Q_SCRIPTED;
	prog = -1;
}

//=================================================================================================
Quest_Scripted::~Quest_Scripted()
{
	if(instance)
		instance->Release();
}

//=================================================================================================
void Quest_Scripted::Start()
{
	category = scheme->category;

	if(scheme->startup_use_vars)
		return;

	Start(nullptr);
}

//=================================================================================================
void Quest_Scripted::Start(Vars* vars)
{
	prog = 0;
	startLoc = world->GetCurrentLocation();

	instance = CreateInstance(false);

	// call Startup
	if(!scheme->f_startup)
		return;
	BeforeCall();
	if(scheme->startup_use_vars)
	{
		assert(vars);
		scriptMgr->RunScript(scheme->f_startup, instance, [vars](asIScriptContext* ctx, int stage)
		{
			if(stage == 0)
				CHECKED(ctx->SetArgAddress(0, vars));
		});
	}
	else
	{
		assert(!vars);
		scriptMgr->RunScript(scheme->f_startup, instance);
	}
	AfterCall();
}

//=================================================================================================
void Quest_Scripted::Save(GameWriter& f)
{
	Quest::Save(f);

	f << scheme->id;
	f << timeout_days;

	if(!instance)
		return;

	uint props = scheme->script_type->GetPropertyCount();
	f << props;
	for(uint i = 0; i < props; ++i)
	{
		int type_id;
		cstring name;
		scheme->script_type->GetProperty(i, &name, &type_id);
		Var::Type var_type = scriptMgr->GetVarType(type_id);
		f << Hash(name);
		void* ptr = instance->GetAddressOfProperty(i);
		switch(var_type)
		{
		case Var::Type::None:
			break;
		case Var::Type::Bool:
			f << *(bool*)ptr;
			break;
		case Var::Type::Int:
			f << *(int*)ptr;
			break;
		case Var::Type::Float:
			f << *(float*)ptr;
			break;
		case Var::Type::Int2:
			f << *(Int2*)ptr;
			break;
		case Var::Type::Vec2:
			f << *(Vec2*)ptr;
			break;
		case Var::Type::Vec3:
			f << *(Vec3*)ptr;
			break;
		case Var::Type::Vec4:
			f << *(Vec4*)ptr;
			break;
		case Var::Type::Item:
			{
				Item* item = *(Item**)ptr;
				if(item)
				{
					f << item->id;
					if(item->id[0] == '$')
						f << item->quest_id;
				}
				else
					f.Write0();
			}
			break;
		case Var::Type::Location:
			{
				Location* loc = *(Location**)ptr;
				if(loc)
					f << loc->index;
				else
					f << -1;
			}
			break;
		case Var::Type::Encounter:
			{
				Encounter* enc = *(Encounter**)ptr;
				if(enc)
					f << enc->index;
				else
					f << -1;
			}
			break;
		case Var::Type::GroundItem:
			{
				GroundItem* item = *(GroundItem**)ptr;
				if(item)
					f << item->id;
				else
					f << -1;
			}
			break;
		case Var::Type::String:
			f.WriteString4(*(string*)ptr);
			break;
		case Var::Type::Unit:
			{
				Unit* unit = *(Unit**)ptr;
				if(unit)
					f << unit->id;
				else
					f << -1;
			}
			break;
		case Var::Type::UnitGroup:
			{
				UnitGroup* group = *(UnitGroup**)ptr;
				if(group)
					f << group->id;
				else
					f.Write0();
			}
			break;
		case Var::Type::Magic:
			break;
		}
	}
}

//=================================================================================================
Quest::LoadResult Quest_Scripted::Load(GameReader& f)
{
	Quest::Load(f);

	const string& scheme_id = f.ReadString1();
	scheme = QuestScheme::TryGet(scheme_id);
	if(!scheme)
		throw Format("Missing quest scheme '%s'.", scheme_id.c_str());
	f >> timeout_days;
	isNew = true;

	// fix for not initializing category for 'side_cleric' quest
	if(LOAD_VERSION <= V_0_17)
		category = scheme->category;

	if(prog == -1)
		return LoadResult::Ok;

	instance = CreateInstance(false);

	// properties
	if(LOAD_VERSION >= V_0_14)
	{
		uint props;
		f >> props;
		const uint scheme_props = scheme->script_type->GetPropertyCount();
		for(uint i = 0; i < props; ++i)
		{
			uint name_hash;
			f >> name_hash;
			for(uint j = 0; j < scheme_props; ++j)
			{
				int type_id;
				cstring name;
				scheme->script_type->GetProperty(j, &name, &type_id);
				if(name_hash == Hash(name))
				{
					Var::Type var_type = scriptMgr->GetVarType(type_id);
					void* ptr = instance->GetAddressOfProperty(j);
					LoadVar(f, var_type, ptr);
					break;
				}
			}
		}
	}
	else
	{
		const uint props = scheme->script_type->GetPropertyCount();
		if(scheme->id == "main")
		{
			for(uint i = 0; i < props; ++i)
			{
				int type_id;
				cstring name;
				scheme->script_type->GetProperty(i, &name, &type_id);
				if(strcmp(name, "village") == 0 || strcmp(name, "counter") == 0)
				{
					Var::Type var_type = scriptMgr->GetVarType(type_id);
					void* ptr = instance->GetAddressOfProperty(i);
					LoadVar(f, var_type, ptr);
				}
			}
		}
		else
		{
			for(uint i = 0; i < props; ++i)
			{
				int type_id;
				scheme->script_type->GetProperty(i, nullptr, &type_id);
				Var::Type var_type = scriptMgr->GetVarType(type_id);
				void* ptr = instance->GetAddressOfProperty(i);
				LoadVar(f, var_type, ptr);
			}
		}
	}

	return LoadResult::Ok;
}

//=================================================================================================
void Quest_Scripted::LoadVar(GameReader& f, Var::Type var_type, void* ptr)
{
	switch(var_type)
	{
	case Var::Type::None:
		break;
	case Var::Type::Bool:
		*(bool*)ptr = f.Read<bool>();
		break;
	case Var::Type::Int:
		*(int*)ptr = f.Read<int>();
		break;
	case Var::Type::Float:
		*(float*)ptr = f.Read<float>();
		break;
	case Var::Type::Int2:
		*(Int2*)ptr = f.Read<Int2>();
		break;
	case Var::Type::Vec2:
		*(Vec2*)ptr = f.Read<Vec2>();
		break;
	case Var::Type::Vec3:
		*(Vec3*)ptr = f.Read<Vec3>();
		break;
	case Var::Type::Vec4:
		*(Vec4*)ptr = f.Read<Vec4>();
		break;
	case Var::Type::Item:
		{
			const string& item_id = f.ReadString1();
			if(item_id.empty())
				*(Item**)ptr = nullptr;
			else if(item_id[0] != '$')
				*(Item**)ptr = Item::Get(item_id);
			else
			{
				int quest_id = f.Read<int>();
				questMgr->AddQuestItemRequest((const Item**)ptr, item_id.c_str(), quest_id, nullptr);
			}
		}
		break;
	case Var::Type::Location:
		{
			int index = f.Read<int>();
			if(index != -1)
				*(Location**)ptr = world->GetLocation(index);
			else
				*(Location**)ptr = nullptr;
		}
		break;
	case Var::Type::Encounter:
		{
			int index = f.Read<int>();
			if(index != -1)
				*(Encounter**)ptr = world->GetEncounter(index);
			else
				*(Encounter**)ptr = nullptr;
		}
		break;
	case Var::Type::GroundItem:
		{
			int id = f.Read<int>();
			*(GroundItem**)ptr = GroundItem::GetById(id);
		}
		break;
	case Var::Type::String:
		f.ReadString4(*(string*)ptr);
		break;
	case Var::Type::Unit:
		{
			int id = f.Read<int>();
			*(Unit**)ptr = Unit::GetById(id);
		}
		break;
	case Var::Type::UnitGroup:
		{
			const string& id = f.ReadString1();
			if(!id.empty())
				*(UnitGroup**)ptr = UnitGroup::Get(id);
			else
				*(UnitGroup**)ptr = nullptr;
		}
		break;
	case Var::Type::Magic:
		break;
	}
}

//=================================================================================================
GameDialog* Quest_Scripted::GetDialog(const string& dialog_id)
{
	GameDialog* dialog = scheme->GetDialog(dialog_id);
	if(!dialog)
		throw new ScriptException("Missing quest dialog '%s'.", dialog_id.c_str());
	return dialog;
}

//=================================================================================================
void Quest_Scripted::BeforeCall()
{
	if(call_depth == 0)
	{
		journal_state = JournalState::None;
		journal_changes = 0;
		scriptMgr->GetContext().quest = this;
	}
	++call_depth;
}

//=================================================================================================
void Quest_Scripted::AfterCall()
{
	--call_depth;
	if(call_depth != 0)
		return;
	if(journal_changes || journal_state == JournalState::Changed)
	{
		gameGui->journal->NeedUpdate(Journal::Quests, quest_index);
		gameGui->messages->AddGameMsg3(GMS_JOURNAL_UPDATED);
		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.id = id;
			c.type = NetChange::UPDATE_QUEST;
			c.count = journal_changes;
		}
	}
	scriptMgr->GetContext().quest = nullptr;
}

//=================================================================================================
void Quest_Scripted::SetProgress(int prog2)
{
	if(prog == prog2)
		return;
	if(in_upgrade)
	{
		prog = prog2;
		return;
	}
	int prev = prog;
	prog = prog2;
	BeforeCall();
	if(scheme->set_progress_use_prev)
	{
		scriptMgr->RunScript(scheme->f_progress, instance, [prev](asIScriptContext* ctx, int stage)
		{
			if(stage == 0)
				CHECKED(ctx->SetArgDWord(0, prev));
		});
	}
	else
		scriptMgr->RunScript(scheme->f_progress, instance);
	AfterCall();
}

//=================================================================================================
void Quest_Scripted::AddEntry(const string& str)
{
	if(journal_state != JournalState::Added)
		++journal_changes;
	msgs.push_back(str);
}

//=================================================================================================
void Quest_Scripted::SetStarted(const string& name)
{
	assert(state == Quest::Hidden);
	assert(journal_state == JournalState::None);
	OnStart(name.c_str());
	journal_state = JournalState::Added;
}

//=================================================================================================
void Quest_Scripted::SetCompleted()
{
	assert(journal_state == JournalState::None);
	journal_state = JournalState::Changed;
	state = Quest::Completed;
	if(category == QuestCategory::Mayor)
		static_cast<City*>(startLoc)->quest_mayor = CityQuestState::None;
	else if(category == QuestCategory::Captain)
		static_cast<City*>(startLoc)->quest_captain = CityQuestState::None;
	if(category == QuestCategory::Unique)
		questMgr->EndUniqueQuest();
	Cleanup();
}

//=================================================================================================
void Quest_Scripted::SetFailed()
{
	assert(journal_state == JournalState::None);
	journal_state = JournalState::Changed;
	state = Quest::Failed;
	if(category == QuestCategory::Mayor)
		static_cast<City*>(startLoc)->quest_mayor = CityQuestState::Failed;
	else if(category == QuestCategory::Captain)
		static_cast<City*>(startLoc)->quest_captain = CityQuestState::Failed;
	Cleanup();
}

//=================================================================================================
void Quest_Scripted::FireEvent(ScriptEvent& event)
{
	if(!scheme->f_event)
		return;
	BeforeCall();
	scriptMgr->RunScript(scheme->f_event, instance, [&event](asIScriptContext* ctx, int stage)
	{
		if(stage == 0)
			CHECKED(ctx->SetArgObject(0, &event));
	});
	AfterCall();
}

//=================================================================================================
string Quest_Scripted::GetString(int index)
{
	assert(scriptMgr->GetContext().quest == this);
	GameDialog* dialog = scheme->dialogs[0];
	if(index < 0 || index >= (int)dialog->texts.size())
		throw ScriptException("Invalid text index.");
	GameDialog::Text& text = dialog->GetText(index);
	const string& str = dialog->strs[text.index];

	if(!text.formatted)
		return str;

	static string str_part;
	static string dialog_s_text;
	dialog_s_text.clear();

	for(uint i = 0, len = str.length(); i < len; ++i)
	{
		if(str[i] == '$')
		{
			str_part.clear();
			++i;
			if(str[i] == '(')
			{
				uint pos = FindClosingPos(str, i);
				int index = atoi(str.substr(i + 1, pos - i - 1).c_str());
				scriptMgr->RunScript(scheme->scripts.Get(DialogScripts::F_FORMAT), instance, [&](asIScriptContext* ctx, int stage)
				{
					if(stage == 0)
					{
						CHECKED(ctx->SetArgDWord(0, index));
					}
					else if(stage == 1)
					{
						string* result = (string*)ctx->GetAddressOfReturnValue();
						dialog_s_text += *result;
					}
				});
				i = pos;
			}
			else
			{
				while(str[i] != '$')
				{
					str_part.push_back(str[i]);
					++i;
				}
				dialog_s_text += FormatString(str_part);
			}
		}
		else
			dialog_s_text.push_back(str[i]);
	}

	return dialog_s_text.c_str();
}

//=================================================================================================
void Quest_Scripted::AddRumor(const string& str)
{
	questMgr->AddQuestRumor(id, str.c_str());
}

//=================================================================================================
void Quest_Scripted::RemoveRumor()
{
	questMgr->RemoveQuestRumor(id);
}

//=================================================================================================
void Quest_Scripted::Upgrade(Quest* quest)
{
	// copy vars
	state = quest->state;
	name = quest->name;
	prog = quest->prog;
	id = quest->id;
	start_time = quest->start_time;
	startLoc = quest->startLoc;
	msgs = quest->msgs;

	// convert
	ConversionData data;
	data.vars = new Vars;
	quest->GetConversionData(data);
	scheme = QuestScheme::TryGet(data.id);
	isNew = true;
	if(!scheme || !scheme->f_upgrade)
		throw Format("Missing upgrade quest '%s'.", data.id);

	type = Q_SCRIPTED;
	category = scheme->category;
	instance = CreateInstance(false);

	// call method
	in_upgrade = true;
	BeforeCall();
	scriptMgr->RunScript(scheme->f_upgrade, instance, [&data](asIScriptContext* ctx, int stage)
	{
		if(stage == 0)
			CHECKED(ctx->SetArgAddress(0, data.vars));
	});
	AfterCall();
	in_upgrade = false;
}
