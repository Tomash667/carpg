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
#include "ScriptExtensions.h"
#include "ScriptManager.h"
#include "World.h"

#include <angelscript.h>
#include <scriptarray\scriptarray.h>
#include <scriptdictionary\scriptdictionary.h>
#pragma warning(error: 4062)

//=================================================================================================
Quest_Scripted::Quest_Scripted() : instance(nullptr), journalState(JournalState::None), journalChanges(0), inUpgrade(false)
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
	CreateInstance();

	// call Startup
	if(!scheme->fStartup)
		return;

	if(scheme->startupUseVars)
	{
		assert(vars);
		scriptMgr->RunScript(scheme->fStartup, instance, this, [vars](asIScriptContext* ctx, int stage)
		{
			if(stage == 0)
				CHECKED(ctx->SetArgAddress(0, vars));
		});
	}
	else
	{
		assert(!vars);
		scriptMgr->RunScript(scheme->fStartup, instance, this);
	}
}

//=================================================================================================
void Quest_Scripted::CreateInstance()
{
	asIScriptFunction* factory = scheme->scriptType->GetFactoryByIndex(0);
	scriptMgr->RunScript(factory, nullptr, this, [&](asIScriptContext* ctx, int stage)
	{
		if(stage == 1)
		{
			void* ptr = ctx->GetAddressOfReturnValue();
			instance = *(asIScriptObject**)ptr;
			instance->AddRef();
		}
	});
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
		int typeId;
		cstring name;
		scheme->scriptType->GetProperty(i, &name, &typeId);
		Var::Type varType = scriptMgr->GetVarType(typeId);
		f << Hash(name);
		SaveVar(f, varType, instance->GetAddressOfProperty(i));
	}
}

//=================================================================================================
void Quest_Scripted::SaveVar(GameWriter& f, Var::Type varType, void* ptr)
{
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
	case Var::Type::Array:
		{
			CScriptArray* arr = static_cast<CScriptArray*>(ptr);
			const uint size = arr->GetSize();
			const int elementSize = CScriptArray_GetElementSize(*arr);
			const Var::Type varType = scriptMgr->GetVarType(arr->GetElementTypeId());
			byte* buf = static_cast<byte*>(arr->GetBuffer());
			f << size;
			for(uint i = 0; i < size; ++i)
			{
				SaveVar(f, varType, buf);
				buf += elementSize;
			}
		}
		break;
	}
}

//=================================================================================================
Quest::LoadResult Quest_Scripted::Load(GameReader& f)
{
	Quest::Load(f);

	const string& schemeId = f.ReadString1();
	scheme = QuestScheme::TryGet(schemeId);
	if(!scheme)
		throw Format("Missing quest scheme '%s'.", schemeId.c_str());
	f >> timeoutDays;
	isNew = true;

	// fix for not initializing category for 'side_cleric' quest
	if(LOAD_VERSION <= V_0_17)
		category = scheme->category;

	if(prog == -1)
		return LoadResult::Ok;

	CreateInstance();

	// properties
	if(LOAD_VERSION >= V_0_14)
	{
		uint props;
		f >> props;
		for(uint i = 0; i < props; ++i)
		{
			uint nameHash;
			f >> nameHash;
			int propId = scheme->GetPropertyId(nameHash);
			if(propId != -1)
			{
				int typeId;
				scheme->scriptType->GetProperty(propId, nullptr, &typeId);
				Var::Type varType = scriptMgr->GetVarType(typeId);
				void* ptr = instance->GetAddressOfProperty(propId);
				LoadVar(f, varType, ptr);
			}
			else
				Error("Missing quest %s property %u.", scheme->id.c_str(), nameHash);
		}
	}
	else
	{
		const uint props = scheme->scriptType->GetPropertyCount();
		if(scheme->id == "main")
		{
			for(uint i = 0; i < props; ++i)
			{
				int typeId;
				cstring name;
				scheme->scriptType->GetProperty(i, &name, &typeId);
				if(strcmp(name, "village") == 0 || strcmp(name, "counter") == 0)
				{
					Var::Type varType = scriptMgr->GetVarType(typeId);
					void* ptr = instance->GetAddressOfProperty(i);
					LoadVar(f, varType, ptr);
				}
			}
		}
		else
		{
			for(uint i = 0; i < props; ++i)
			{
				int typeId;
				scheme->scriptType->GetProperty(i, nullptr, &typeId);
				Var::Type varType = scriptMgr->GetVarType(typeId);
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
			const string& itemId = f.ReadString1();
			if(itemId.empty())
				*(Item**)ptr = nullptr;
			else if(itemId[0] != '$')
				*(Item**)ptr = Item::Get(itemId);
			else
			{
				int questId = f.Read<int>();
				questMgr->AddQuestItemRequest((const Item**)ptr, itemId.c_str(), questId, nullptr);
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
	case Var::Type::Array:
		{
			CScriptArray* arr = static_cast<CScriptArray*>(ptr);
			const uint size = f.Read<uint>();
			const int elementSize = CScriptArray_GetElementSize(*arr);
			const Var::Type varType = scriptMgr->GetVarType(arr->GetElementTypeId());
			arr->Resize(size);
			byte* buf = static_cast<byte*>(arr->GetBuffer());
			for(uint i = 0; i < size; ++i)
			{
				LoadVar(f, varType, buf);
				buf += elementSize;
			}
		}
		break;
	}
}

//=================================================================================================
GameDialog* Quest_Scripted::GetDialog(const string& dialogId)
{
	GameDialog* dialog = scheme->GetDialog(dialogId);
	if(!dialog)
		throw new ScriptException("Missing quest dialog '%s'.", dialogId.c_str());
	return dialog;
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
	if(scheme->setProgressUsePrev)
	{
		scriptMgr->RunScript(scheme->fProgress, instance, this, [prev](asIScriptContext* ctx, int stage)
		{
			if(stage == 0)
				CHECKED(ctx->SetArgDWord(0, prev));
		});
	}
	else
		scriptMgr->RunScript(scheme->fProgress, instance, this);
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

	scriptMgr->RunScript(scheme->fEvent, instance, this, [&event](asIScriptContext* ctx, int stage)
	{
		if(stage == 0)
			CHECKED(ctx->SetArgObject(0, &event));
	});
}

//=================================================================================================
string Quest_Scripted::GetString(int index)
{
	GameDialog* dialog = scheme->dialogs[0];
	if(index < 0 || index >= (int)dialog->texts.size())
		throw ScriptException("Invalid text index.");
	GameDialog::Text& text = dialog->GetText(index);
	const string& str = dialog->strs[text.index];

	if(!text.formatted)
		return str;

	LocalString strPart, dialogString;
	for(uint i = 0, len = str.length(); i < len; ++i)
	{
		if(str[i] == '$')
		{
			strPart.clear();
			++i;
			if(str[i] == '(')
			{
				uint pos = FindClosingPos(str, i);
				int index = atoi(str.substr(i + 1, pos - i - 1).c_str());
				scriptMgr->RunScript(scheme->scripts.Get(DialogScripts::F_FORMAT), instance, this, [&](asIScriptContext* ctx, int stage)
				{
					if(stage == 0)
					{
						CHECKED(ctx->SetArgDWord(0, index));
					}
					else if(stage == 1)
					{
						string* result = (string*)ctx->GetAddressOfReturnValue();
						dialogString += *result;
					}
				});
				i = pos;
			}
			else
			{
				while(str[i] != '$')
				{
					strPart += str[i];
					++i;
				}
				dialogString += FormatString(strPart);
			}
		}
		else
			dialogString += str[i];
	}

	return dialogString.c_str();
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
	CreateInstance();

	// call method
	inUpgrade = true;
	scriptMgr->RunScript(scheme->fUpgrade, instance, this, [&data](asIScriptContext* ctx, int stage)
	{
		if(stage == 0)
			CHECKED(ctx->SetArgAddress(0, data.vars));
	});
	inUpgrade = false;
}

//=================================================================================================
bool Quest_Scripted::PostRun()
{
	bool showMessage = false;
	if(journalChanges || journalState == JournalState::Changed)
	{
		gameGui->journal->NeedUpdate(Journal::Quests, questIndex);
		showMessage = true;

		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.id = id;
			c.type = NetChange::UPDATE_QUEST;
			c.count = journalChanges;
		}
	}
	else if(journalState == JournalState::Added)
		showMessage = true;

	journalState = JournalState::None;
	journalChanges = 0;
	return showMessage;
}
