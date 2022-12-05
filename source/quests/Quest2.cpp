#include "Pch.h"
#include "Quest2.h"

#include "City.h"
#include "DialogContext.h"
#include "QuestManager.h"
#include "QuestScheme.h"
#include "ScriptManager.h"
#include "Var.h"
#include "World.h"

#include <angelscript.h>

//=================================================================================================
Quest2::Quest2() : scheme(nullptr), timeoutDays(-1)
{
}

//=================================================================================================
// Remove event ptr, must be called when removing real event
void Quest2::RemoveEventPtr(const EventPtr& event)
{
	LoopAndRemove(events, [&](EventPtr& e)
	{
		return event == e;
	});
}

//=================================================================================================
void Quest2::RemoveEvent(ScriptEvent& event)
{
	switch(event.type)
	{
	case EVENT_ENTER:
		event.on_enter.location->RemoveEventHandler(this, EVENT_ENTER, false);
		break;
	case EVENT_PICKUP:
		event.on_pickup.unit->RemoveEventHandler(this, EVENT_PICKUP, false);
		break;
	case EVENT_UPDATE:
		event.on_update.unit->RemoveEventHandler(this, EVENT_UPDATE, false);
		break;
	case EVENT_TIMEOUT:
	case EVENT_ENCOUNTER:
		break;
	case EVENT_DIE:
		event.on_die.unit->RemoveEventHandler(this, EVENT_DIE, false);
		break;
	case EVENT_CLEARED:
		event.on_cleared.location->RemoveEventHandler(this, EVENT_CLEARED, false);
		break;
	case EVENT_GENERATE:
		event.on_generate.location->RemoveEventHandler(this, EVENT_GENERATE, false);
		break;
	}
}

//=================================================================================================
void Quest2::RemoveDialogPtr(Unit* unit)
{
	LoopAndRemove(unitDialogs, [=](Unit* u)
	{
		return unit == u;
	});
}

//=================================================================================================
GameDialog* Quest2::GetDialog(Cstring name)
{
	return scheme->GetDialog(name.s);
}

//=================================================================================================
GameDialog* Quest2::GetDialog(int type2)
{
	if(type2 == QUEST_DIALOG_START)
		return scheme->GetDialog("start");
	else
	{
		assert(0); // not implemented
		return nullptr;
	}
}

//=================================================================================================
cstring Quest2::FormatString(const string& str)
{
	if(str == "date")
		return world->GetDate();
	else if(str == "name")
	{
		Unit* talker = DialogContext::current->talker;
		assert(talker->IsHero());
		return talker->hero->name.c_str();
	}
	else if(str == "player_name")
		return DialogContext::current->pc->name.c_str();
	else
	{
		assert(0);
		return Format("!!!%s!!!", str.c_str());
	}
}

//=================================================================================================
cstring Quest2::GetText(int index)
{
	GameDialog* dialog = scheme->dialogs[0];
	if(index < 0 || index >= (int)dialog->texts.size())
		throw ScriptException("Invalid text index.");
	GameDialog::Text& text = dialog->GetText(index);
	const string& str = dialog->strs[text.index];

	if(!text.formatted)
		return str.c_str();

	asIScriptObject* instance = CreateInstance(true);
	static string str_part;
	static string dialog_s_text;
	dialog_s_text.clear();

	ScriptContext& ctx = scriptMgr->GetContext();
	ctx.quest = this;

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

	ctx.quest = nullptr;
	return dialog_s_text.c_str();
}

//=================================================================================================
Var* Quest2::GetValue(int offset)
{
	static Var var;
	byte* base = reinterpret_cast<byte*>(this);
	base += sizeof(Quest2) + offset;
	var.SetPtr(*reinterpret_cast<void**>(base), Var::Type::Magic);
	return &var;
}

//=================================================================================================
void Quest2::Save(GameWriter& f)
{
	Quest::Save(f);
	if(IsActive())
	{
		f << timeoutDays;
		SaveDetails(f);
	}
}

//=================================================================================================
Quest::LoadResult Quest2::Load(GameReader& f)
{
	Quest::Load(f);
	SetScheme(questMgr->FindQuestInfo(type)->scheme);
	if(IsActive())
	{
		f >> timeoutDays;
		LoadDetails(f);
	}
	return LoadResult::Ok;
}

//=================================================================================================
asIScriptObject* Quest2::CreateInstance(bool shared)
{
	if(shared)
	{
		asIScriptObject* instance = scriptMgr->GetSharedInstance(scheme);
		if(instance)
			return instance;
	}

	asIScriptFunction* factory = scheme->script_type->GetFactoryByIndex(0);
	asIScriptObject* instance;
	scriptMgr->RunScript(factory, nullptr, [&](asIScriptContext* ctx, int stage)
	{
		if(stage == 1)
		{
			void* ptr = ctx->GetAddressOfReturnValue();
			instance = *(asIScriptObject**)ptr;
			instance->AddRef();
		}
	});

	if(shared)
		scriptMgr->RegisterSharedInstance(scheme, instance);
	return instance;
}

//=================================================================================================
void Quest2::Cleanup()
{
	if(timeoutDays != -1)
	{
		RemoveElementTry(questMgr->questTimeouts2, static_cast<Quest*>(this));
		timeoutDays = -1;
	}

	for(EventPtr& e : events)
	{
		switch(e.source)
		{
		case EventPtr::LOCATION:
			e.location->RemoveEventHandler(this, EVENT_ANY, true);
			break;
		case EventPtr::UNIT:
			e.unit->RemoveEventHandler(this, EVENT_ANY, true);
			break;
		}
	}
	events.clear();

	for(Unit* unit : unitDialogs)
		unit->RemoveDialog(this, true);
	unitDialogs.clear();

	world->RemoveEncounter(this);
}

//=================================================================================================
void Quest2::SetState(State newState)
{
	assert(state != newState && state == State::Started && Any(newState, State::Completed, State::Failed));
	state = newState;
	switch(category)
	{
	case QuestCategory::Mayor:
		static_cast<City*>(startLoc)->quest_mayor = (state == State::Completed ? CityQuestState::None : CityQuestState::Failed);
		break;
	case QuestCategory::Captain:
		static_cast<City*>(startLoc)->quest_captain = (state == State::Completed ? CityQuestState::None : CityQuestState::Failed);
		break;
	case QuestCategory::Unique:
		questMgr->EndUniqueQuest();
		break;
	}
	Cleanup();
}

//=================================================================================================
void Quest2::SetTimeout(int days)
{
	assert(Any(state, Quest::Hidden, Quest::Started));
	assert(timeoutDays == -1);
	assert(days > 0);
	timeoutDays = days;
	startTime = world->GetWorldtime();
	questMgr->questTimeouts2.push_back(this);
}

//=================================================================================================
int Quest2::GetTimeout() const
{
	int days = world->GetWorldtime() - startTime;
	if(days >= timeoutDays)
		return 0;
	return timeoutDays - days;
}

//=================================================================================================
bool Quest2::IsTimedout() const
{
	if(timeoutDays == -1)
		return false;
	return world->GetWorldtime() - startTime >= timeoutDays;
}

//=================================================================================================
bool Quest2::OnTimeout(TimeoutType ttype)
{
	ScriptEvent event(EVENT_TIMEOUT);
	timeoutDays = -1;
	FireEvent(event);
	return true;
}
