#include "Pch.h"
#include "Quest2.h"

#include "City.h"
#include "DialogContext.h"
#include "QuestManager.h"
#include "QuestScheme.h"
#include "ScriptManager.h"
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
		event.location->RemoveEventHandler(this, EVENT_ENTER, false);
		break;
	case EVENT_PICKUP:
		event.unit->RemoveEventHandler(this, EVENT_PICKUP, false);
		break;
	case EVENT_UPDATE:
		event.unit->RemoveEventHandler(this, EVENT_UPDATE, false);
		break;
	case EVENT_TIMEOUT:
	case EVENT_ENCOUNTER:
		break;
	case EVENT_DIE:
		event.unit->RemoveEventHandler(this, EVENT_DIE, false);
		break;
	case EVENT_CLEARED:
		event.location->RemoveEventHandler(this, EVENT_CLEARED, false);
		break;
	case EVENT_GENERATE:
		event.location->RemoveEventHandler(this, EVENT_GENERATE, false);
		break;
	case EVENT_USE:
		questMgr->RemoveItemEventHandler(this, event.item);
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
	static string strPart;
	static string dialogString;
	dialogString.clear();

	ScriptContext& ctx = scriptMgr->GetContext();
	ctx.quest = this;

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
					strPart.push_back(str[i]);
					++i;
				}
				dialogString += FormatString(strPart);
			}
		}
		else
			dialogString.push_back(str[i]);
	}

	ctx.quest = nullptr;
	return dialogString.c_str();
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
	LoadQuest2(f, nullptr);
	return LoadResult::Ok;
}

//=================================================================================================
void Quest2::LoadQuest2(GameReader& f, cstring schemeId)
{
	Quest::Load(f);

	QuestScheme* scheme;
	if(schemeId)
		scheme = questMgr->FindQuestInfo(schemeId)->scheme;
	else
		scheme = questMgr->FindQuestInfo(type)->scheme;

	SetScheme(scheme);
	if(IsActive())
	{
		f >> timeoutDays;
		LoadDetails(f);
	}
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

	asIScriptFunction* factory = scheme->scriptType->GetFactoryByIndex(0);
	asIScriptObject* instance;
	scriptMgr->RunScript(factory, nullptr, this, [&](asIScriptContext* ctx, int stage)
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
		static_cast<City*>(startLoc)->questMayor = (state == State::Completed ? CityQuestState::None : CityQuestState::Failed);
		break;
	case QuestCategory::Captain:
		static_cast<City*>(startLoc)->questCaptain = (state == State::Completed ? CityQuestState::None : CityQuestState::Failed);
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
