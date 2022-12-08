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
Quest_Scripted::Quest_Scripted() : instance(nullptr), cellDepth(0), inUpgrade(false)
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

	if(scheme->startupUseVars)
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
	if(!scheme->fStartup)
		return;
	BeforeCall();
	if(scheme->startupUseVars)
	{
		assert(vars);
		scriptMgr->RunScript(scheme->fStartup, instance, [vars](asIScriptContext* ctx, int stage)
		{
			if(stage == 0)
				CHECKED(ctx->SetArgAddress(0, vars));
		});
	}
	else
	{
		assert(!vars);
		scriptMgr->RunScript(scheme->fStartup, instance);
	}
	AfterCall();
}

//=================================================================================================
void Quest_Scripted::Save(GameWriter& f)
{
	Quest::Save(f);

	f << scheme->id;
	f << timeoutDays;

	if(!instance)
		return;

	uint props = scheme->scriptType->GetPropertyCount();
	f << props;
	for(uint i = 0; i < props; ++i)
	{
		int type_id;
		cstring name;
		scheme->scriptType->GetProperty(i, &name, &type_id);
		Var::Type varType = scriptMgr->GetVarType(type_id);
		f << Hash(name);
		void* ptr = instance->GetAddressOfProperty(i);
		switch(varType)
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
						f << item->questId;
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
	f >> timeoutDays;
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
		const uint scheme_props = scheme->scriptType->GetPropertyCount();
		for(uint i = 0; i < props; ++i)
		{
			uint name_hash;
			f >> name_hash;
			for(uint j = 0; j < scheme_props; ++j)
			{
				int type_id;
				cstring name;
				scheme->scriptType->GetProperty(j, &name, &type_id);
				if(name_hash == Hash(name))
				{
					Var::Type varType = scriptMgr->GetVarType(type_id);
					void* ptr = instance->GetAddressOfProperty(j);
					LoadVar(f, varType, ptr);
					break;
				}
			}
		}
	}
	else
	{
		const uint props = scheme->scriptType->GetPropertyCount();
		if(scheme->id == "main")
		{
			for(uint i = 0; i < props; ++i)
			{
				int type_id;
				cstring name;
				scheme->scriptType->GetProperty(i, &name, &type_id);
				if(strcmp(name, "village") == 0 || strcmp(name, "counter") == 0)
				{
					Var::Type varType = scriptMgr->GetVarType(type_id);
					void* ptr = instance->GetAddressOfProperty(i);
					LoadVar(f, varType, ptr);
				}
			}
		}
		else
		{
			for(uint i = 0; i < props; ++i)
			{
				int type_id;
				scheme->scriptType->GetProperty(i, nullptr, &type_id);
				Var::Type varType = scriptMgr->GetVarType(type_id);
				void* ptr = instance->GetAddressOfProperty(i);
				LoadVar(f, varType, ptr);
			}
		}
	}

	return LoadResult::Ok;
}

//=================================================================================================
void Quest_Scripted::LoadVar(GameReader& f, Var::Type varType, void* ptr)
{
	switch(varType)
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
				int questId = f.Read<int>();
				questMgr->AddQuestItemRequest((const Item**)ptr, item_id.c_str(), questId, nullptr);
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
	if(cellDepth == 0)
	{
		journalState = JournalState::None;
		journalChanges = 0;
		scriptMgr->GetContext().quest = this;
	}
	++cellDepth;
}

//=================================================================================================
void Quest_Scripted::AfterCall()
{
	--cellDepth;
	if(cellDepth != 0)
		return;
	if(journalChanges || journalState == JournalState::Changed)
	{
		gameGui->journal->NeedUpdate(Journal::Quests, questIndex);
		gameGui->messages->AddGameMsg3(GMS_JOURNAL_UPDATED);
		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.id = id;
			c.type = NetChange::UPDATE_QUEST;
			c.count = journalChanges;
		}
	}
	scriptMgr->GetContext().quest = nullptr;
}

//=================================================================================================
void Quest_Scripted::SetProgress(int prog2)
{
	if(prog == prog2)
		return;
	if(inUpgrade)
	{
		prog = prog2;
		return;
	}
	int prev = prog;
	prog = prog2;
	BeforeCall();
	if(scheme->setProgressUsePrev)
	{
		scriptMgr->RunScript(scheme->fProgress, instance, [prev](asIScriptContext* ctx, int stage)
		{
			if(stage == 0)
				CHECKED(ctx->SetArgDWord(0, prev));
		});
	}
	else
		scriptMgr->RunScript(scheme->fProgress, instance);
	AfterCall();
}

//=================================================================================================
void Quest_Scripted::AddEntry(const string& str)
{
	if(journalState != JournalState::Added)
		++journalChanges;
	msgs.push_back(str);
}

//=================================================================================================
void Quest_Scripted::SetStarted(const string& name)
{
	assert(state == Quest::Hidden);
	assert(journalState == JournalState::None);
	OnStart(name.c_str());
	journalState = JournalState::Added;
}

//=================================================================================================
void Quest_Scripted::SetCompleted()
{
	assert(journalState == JournalState::None);
	journalState = JournalState::Changed;
	state = Quest::Completed;
	if(category == QuestCategory::Mayor)
		static_cast<City*>(startLoc)->questMayor = CityQuestState::None;
	else if(category == QuestCategory::Captain)
		static_cast<City*>(startLoc)->questCaptain = CityQuestState::None;
	if(category == QuestCategory::Unique)
		questMgr->EndUniqueQuest();
	Cleanup();
}

//=================================================================================================
void Quest_Scripted::SetFailed()
{
	assert(journalState == JournalState::None);
	journalState = JournalState::Changed;
	state = Quest::Failed;
	if(category == QuestCategory::Mayor)
		static_cast<City*>(startLoc)->questMayor = CityQuestState::Failed;
	else if(category == QuestCategory::Captain)
		static_cast<City*>(startLoc)->questCaptain = CityQuestState::Failed;
	Cleanup();
}

//=================================================================================================
void Quest_Scripted::FireEvent(ScriptEvent& event)
{
	if(!scheme->fEvent)
		return;
	BeforeCall();
	scriptMgr->RunScript(scheme->fEvent, instance, [&event](asIScriptContext* ctx, int stage)
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
	startTime = quest->startTime;
	startLoc = quest->startLoc;
	msgs = quest->msgs;

	// convert
	ConversionData data;
	data.vars = new Vars;
	quest->GetConversionData(data);
	scheme = QuestScheme::TryGet(data.id);
	isNew = true;
	if(!scheme || !scheme->fUpgrade)
		throw Format("Missing upgrade quest '%s'.", data.id);

	type = Q_SCRIPTED;
	category = scheme->category;
	instance = CreateInstance(false);

	// call method
	inUpgrade = true;
	BeforeCall();
	scriptMgr->RunScript(scheme->fUpgrade, instance, [&data](asIScriptContext* ctx, int stage)
	{
		if(stage == 0)
			CHECKED(ctx->SetArgAddress(0, data.vars));
	});
	AfterCall();
	inUpgrade = false;
}
