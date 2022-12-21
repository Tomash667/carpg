#include "Pch.h"
#include "DialogContext.h"

#include "Ability.h"
#include "AIController.h"
#include "Arena.h"
#include "Game.h"
#include "GameGui.h"
#include "GameResources.h"
#include "Inventory.h"
#include "Journal.h"
#include "Level.h"
#include "LevelGui.h"
#include "LocationHelper.h"
#include "PlayerInfo.h"
#include "Quest2.h"
#include "QuestManager.h"
#include "QuestScheme.h"
#include "Quest_Bandits.h"
#include "Quest_Contest.h"
#include "Quest_Evil.h"
#include "Quest_Goblins.h"
#include "Quest_Mages.h"
#include "Quest_Mine.h"
#include "Quest_Orcs.h"
#include "Quest_Sawmill.h"
#include "MultiInsideLocation.h"
#include "NameHelper.h"
#include "News.h"
#include "ScriptManager.h"
#include "Team.h"
#include "World.h"

#include <angelscript.h>

DialogContext* DialogContext::current;
int DialogContext::var;

//=================================================================================================
void DialogContext::StartDialog(Unit* talker, GameDialog* dialog, Quest* quest)
{
	assert(!dialogMode);

	// use up auto talk
	if(talker && !dialog && talker->GetOrder() == ORDER_AUTO_TALK)
	{
		dialog = talker->order->autoTalkDialog;
		quest = talker->order->autoTalkQuest;
		talker->OrderNext();
	}

	mode = NONE;
	dialogMode = true;
	dialogPos = 0;
	dialogText = nullptr;
	once = true;
	dialogQuest = quest;
	dialogEsc = -1;
	this->talker = talker;
	this->dialog = dialog ? dialog : talker->data->dialog;
	if(Net::IsLocal())
	{
		// this vars are useless for clients, don't increase ref counter
		talker->busy = Unit::Busy_Talking;
		talker->lookTarget = pc->unit;
	}
	if(this->dialog && this->dialog->quest)
		dialogQuest = questMgr->FindAnyQuest(this->dialog->quest);
	updateNews = true;
	updateLocations = 1;
	pc->action = PlayerAction::Talk;
	pc->actionUnit = talker;
	ClearChoices();
	forceEnd = false;
	if(!dialog)
	{
		questDialogs = talker->dialogs;
		std::sort(questDialogs.begin(), questDialogs.end(),
			[](const QuestDialog& dialog1, const QuestDialog& dialog2) { return dialog1.priority > dialog2.priority; });
		if(questDialogs.empty())
			questDialogIndex = QUEST_INDEX_NONE;
		else
		{
			questDialogIndex = 0;
			prev.push_back({ this->dialog, dialogQuest, -1 });
			this->dialog = questDialogs[0].dialog;
			dialogQuest = (Quest*)questDialogs[0].quest;
		}
	}
	else
	{
		questDialogs.clear();
		questDialogIndex = QUEST_INDEX_NONE;
	}
	assert(this->dialog);

	if(Net::IsLocal())
	{
		// talk sound
		Sound* sound = talker->GetSound(SOUND_TALK);
		if(sound)
		{
			game->PlayAttachedSound(*talker, sound, Unit::TALK_SOUND_DIST);
			if(Net::IsServer())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::UNIT_SOUND;
				c.unit = talker;
				c.id = SOUND_TALK;
			}
		}
	}
	else
	{
		if(!predialog.empty())
		{
			mode = DialogContext::WAIT_TALK;
			dialogString = predialog;
			dialogText = dialogString.c_str();
			timer = 1.f;
			predialog.clear();
		}
		else if(talker->bubble && talker->bubble != predialogBubble)
		{
			mode = DialogContext::WAIT_TALK;
			dialogString = talker->bubble->text;
			dialogText = dialogString.c_str();
			timer = 1.f;
			skipId = talker->bubble->skipId;
		}
	}

	if(isLocal)
		gameGui->CloseAllPanels();
}

//=================================================================================================
void DialogContext::StartNextDialog(GameDialog* newDialog, Quest* quest, bool returns)
{
	assert(newDialog);

	if(returns)
		prev.push_back({ dialog, dialogQuest, dialogPos });
	dialog = newDialog;
	dialogQuest = quest;
	dialogPos = -1;
}

//=================================================================================================
cstring DialogContext::GetIdleText(Unit& talker)
{
	assert(talker.data->idleDialog);

	mode = IDLE;
	dialogPos = 0;
	dialogText = nullptr;
	once = true;
	dialogQuest = nullptr;
	dialogEsc = -1;
	this->talker = &talker;
	dialog = talker.data->idleDialog;
	updateNews = true;
	updateLocations = 1;
	ClearChoices();
	forceEnd = false;
	questDialogs.clear();
	questDialogIndex = QUEST_INDEX_NONE;

	ScriptContext& ctx = scriptMgr->GetContext();
	ctx.pc = nullptr;
	ctx.target = this->talker;
	current = this;
	UpdateLoop();
	current = nullptr;
	ctx.target = nullptr;

	return dialogText;
}

//=================================================================================================
void DialogContext::Update(float dt)
{
	switch(mode)
	{
	case WAIT_CHOICES:
		// wyœwietlono opcje dialogowe, wybierz jedn¹ z nich (w mp czekaj na wybór)
		{
			bool ok = false;
			if(!isLocal)
			{
				if(choiceSelected != -1)
					ok = true;
			}
			else
				ok = gameGui->levelGui->UpdateChoice();

			if(ok)
			{
				mode = NONE;
				DialogChoice& choice = choices[choiceSelected];
				cstring msg = choice.talkMsg ? choice.talkMsg : choice.msg;
				gameGui->levelGui->AddSpeechBubble(pc->unit, msg);

				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::TALK;
					c.unit = pc->unit;
					c.str = StringPool.Get();
					*c.str = msg;
					c.id = 0;
					c.count = 0;
					net->netStrs.push_back(c.str);
				}

				bool useReturn = false;
				switch(choice.type)
				{
				case DialogChoice::Normal:
					if(choice.questDialogIndex != QUEST_INDEX_NONE)
					{
						prev.push_back({ this->dialog, dialogQuest, -1 });
						questDialogIndex = choice.questDialogIndex;
						dialogQuest = questDialogs[questDialogIndex].quest;
						dialog = questDialogs[questDialogIndex].dialog;
					}
					dialogPos = choice.pos;
					break;
				case DialogChoice::Perk:
					useReturn = LearnPerk((Perk*)choice.pos);
					break;
				case DialogChoice::Hero:
					useReturn = RecruitHero((Class*)choice.pos);
					break;
				}

				ClearChoices();
				choiceSelected = -1;
				dialogEsc = -1;
				if(useReturn)
					return;
			}
			else
				return;
		}
		break;

	case WAIT_TALK:
	case WAIT_TIMER:
		if(isLocal)
		{
			bool skip = false;
			if(mode == WAIT_TALK)
			{
				if(GKey.KeyPressedReleaseAllowed(GK_SELECT_DIALOG)
					|| GKey.KeyPressedReleaseAllowed(GK_SKIP_DIALOG)
					|| (GKey.AllowKeyboard() && input->PressedRelease(Key::Escape)))
					skip = true;
				else
				{
					game->pc->data.wastedKey = GKey.KeyDoReturn(GK_ATTACK_USE, &Input::PressedRelease);
					if(game->pc->data.wastedKey != Key::None)
						skip = true;
				}
			}

			if(skip)
				timer = -1.f;
			else
				timer -= dt;
		}
		else
		{
			if(choiceSelected == 1)
			{
				timer = -1.f;
				choiceSelected = -1;
			}
			else
				timer -= dt;
		}

		if(timer > 0.f)
			return;
		mode = NONE;
		break;
	}

	if(forceEnd)
	{
		EndDialog();
		return;
	}

	ScriptContext& ctx = scriptMgr->GetContext();
	ctx.pc = pc;
	ctx.target = talker;
	current = this;
	UpdateLoop();
	current = nullptr;
	ctx.pc = nullptr;
	ctx.target = nullptr;
}

//=================================================================================================
void DialogContext::UpdateClient()
{
	switch(mode)
	{
	case WAIT_CHOICES:
		{
			const int prevChoice = choiceSelected;
			if(gameGui->levelGui->UpdateChoice())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::CHOICE;
				c.id = choiceSelected;

				mode = WAIT_MP_RESPONSE;
				choiceSelected = prevChoice;
			}
		}
		break;

	case WAIT_TALK:
		if(skipId != -1 && (
			GKey.KeyPressedReleaseAllowed(GK_SKIP_DIALOG) || GKey.KeyPressedReleaseAllowed(GK_SELECT_DIALOG) || GKey.KeyPressedReleaseAllowed(GK_ATTACK_USE)
			|| (GKey.AllowKeyboard() && input->PressedRelease(Key::Escape))))
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::SKIP_DIALOG;
			c.id = skipId;
			skipId = -1;
		}
		break;
	}
}

//=================================================================================================
void DialogContext::UpdateLoop()
{
	bool cmpResult = false;
	while(true)
	{
		DialogEntry& de = *(dialog->code.data() + dialogPos);

		switch(de.type)
		{
		case DTF_CHOICE:
			{
				if(de.op == OP_ESCAPE)
					dialogEsc = (int)choices.size();
				talkMsg = nullptr;
				cstring text = GetText(de.value);
				if(text == dialogString.c_str())
				{
					string* str = StringPool.Get();
					*str = text;
					choices.push_back(DialogChoice(dialogPos + 2, str->c_str(), questDialogIndex, str));
				}
				else
					choices.push_back(DialogChoice(dialogPos + 2, text, questDialogIndex));
				if(talkMsg)
					choices.back().talkMsg = talkMsg;
			}
			break;
		case DTF_END:
			if(prev.empty())
			{
				EndDialog();
				return;
			}
			else if(questDialogIndex == QUEST_INDEX_NONE || questDialogIndex + 1 == (int)questDialogs.size())
			{
				// finished all quest dialogs, use normal dialog
				Entry& p = prev.back();
				dialog = p.dialog;
				dialogPos = p.pos;
				dialogQuest = p.quest;
				prev.pop_back();
				questDialogIndex = QUEST_INDEX_NONE;
			}
			else
			{
				// handle remove quest dialog when was inside this dialog
				if(questDialogIndex == QUEST_INDEX_RESTART)
					questDialogIndex = QUEST_INDEX_NONE;

				// use next quest dialog
				++questDialogIndex;
				dialog = questDialogs[questDialogIndex].dialog;
				dialogQuest = (Quest*)questDialogs[questDialogIndex].quest;
				dialogPos = -1;
			}
			break;
		case DTF_END2:
			EndDialog();
			return;
		case DTF_SHOW_CHOICES:
			mode = WAIT_CHOICES;
			if(isLocal)
			{
				choiceSelected = 0;
				LevelGui* gui = gameGui->levelGui;
				gui->dialogCursorPos = Int2(-1, -1);
				gui->UpdateScrollbar(choices.size());
			}
			else
			{
				choiceSelected = -1;
				NetChangePlayer& c = Add1(pc->playerInfo->changes);
				c.type = NetChangePlayer::SHOW_DIALOG_CHOICES;
			}
			return;
		case DTF_RESTART:
			questDialogs = talker->dialogs;
			std::sort(questDialogs.begin(), questDialogs.end(),
				[](const QuestDialog& dialog1, const QuestDialog& dialog2) { return dialog1.priority > dialog2.priority; });
			if(!prev.empty())
			{
				Entry e = prev[0];
				prev.clear();
				if(questDialogs.empty())
				{
					dialog = e.dialog;
					dialogQuest = e.quest;
					questDialogIndex = QUEST_INDEX_NONE;
				}
				else
				{
					prev.push_back({ e.dialog, e.quest, -1 });
					dialog = questDialogs[0].dialog;
					dialogQuest = (Quest*)questDialogs[0].quest;
					questDialogIndex = 0;
				}
			}
			else
			{
				if(questDialogs.empty())
					questDialogIndex = QUEST_INDEX_NONE;
				else
				{
					questDialogIndex = 0;
					prev.push_back({ this->dialog, dialogQuest, -1 });
					dialog = questDialogs[0].dialog;
					dialogQuest = (Quest*)questDialogs[0].quest;
				}
			}
			dialogPos = -1;
			break;
		case DTF_TRADE:
			if(!talker || !talker->data->trader)
			{
				assert(0);
				Error("DTF_TRADE, unit '%s' is not trader.", talker->data->id.c_str());
				EndDialog();
				return;
			}

			talker->busy = Unit::Busy_Trading;
			EndDialog();
			pc->action = PlayerAction::Trade;
			pc->actionUnit = talker;
			pc->chestTrade = &talker->stock->items;

			if(isLocal)
				gameGui->inventory->StartTrade(I_TRADE, *pc->chestTrade, talker);
			else
			{
				NetChangePlayer& c = Add1(pc->playerInfo->changes);
				c.type = NetChangePlayer::START_TRADE;
				c.id = talker->id;
			}
			return;
		case DTF_TALK:
		case DTF_MULTI_TALK:
			{
				cstring msg = GetText(de.value, de.type == DTF_MULTI_TALK);
				Talk(msg);
				++dialogPos;
			}
			return;
		case DTF_SPECIAL:
			{
				cstring msg = dialog->strs[de.value].c_str();
				if(ExecuteSpecial(msg))
					return;
			}
			break;
		case DTF_SET_QUEST_PROGRESS:
			assert(dialogQuest);
			dialogQuest->SetProgress(de.value);
			if((mode == WAIT_TALK || mode == WAIT_TIMER) && timer > 0.f)
			{
				++dialogPos;
				return;
			}
			break;
		case DTF_IF_QUEST_TIMEOUT:
			assert(dialogQuest);
			cmpResult = (dialogQuest->IsActive() && dialogQuest->IsTimedout());
			if(de.op == OP_NOT_EQUAL)
				cmpResult = !cmpResult;
			break;
		case DTF_IF_RAND:
			cmpResult = (Rand() % de.value == 0);
			if(de.op == OP_NOT_EQUAL)
				cmpResult = !cmpResult;
			break;
		case DTF_IF_RAND_P:
			cmpResult = (Rand() % 100 < de.value);
			if(de.op == OP_NOT_EQUAL)
				cmpResult = !cmpResult;
			break;
		case DTF_IF_ONCE:
			cmpResult = once;
			if(de.op == OP_NOT_EQUAL)
				cmpResult = !cmpResult;
			if(cmpResult)
				once = false;
			break;
		case DTF_DO_ONCE:
			once = false;
			break;
		case DTF_CHECK_QUEST_TIMEOUT:
			{
				Quest* quest = questMgr->FindQuest(gameLevel->location, (QuestCategory)de.value);
				if(quest && quest->IsActive() && quest->IsTimedout())
				{
					once = false;
					StartNextDialog(quest->GetDialog(QUEST_DIALOG_FAIL), quest);
				}
			}
			break;
		case DTF_IF_HAVE_QUEST_ITEM:
			{
				cstring msg = dialog->strs[de.value].c_str();
				int questId = -1;
				if(msg[0] == '$' && msg[1] == '$')
					questId = talker->questId;
				cmpResult = pc->unit->FindQuestItem(msg, nullptr, nullptr, false, questId);
				if(de.op == OP_NOT_EQUAL)
					cmpResult = !cmpResult;
			}
			break;
		case DTF_IF_HAVE_QUEST_ITEM_CURRENT:
			cmpResult = (dialogQuest && pc->unit->HaveQuestItem(dialogQuest->id));
			if(de.op == OP_NOT_EQUAL)
				cmpResult = !cmpResult;
			break;
		case DTF_IF_HAVE_QUEST_ITEM_NOT_ACTIVE:
			{
				cstring msg = dialog->strs[de.value].c_str();
				cmpResult = pc->unit->FindQuestItem(msg, nullptr, nullptr, true);
				if(de.op == OP_NOT_EQUAL)
					cmpResult = !cmpResult;
			}
			break;
		case DTF_DO_QUEST_ITEM:
			{
				cstring msg = dialog->strs[de.value].c_str();
				int questId = -1;
				if(msg[0] == '$' && msg[1] == '$')
					questId = talker->questId;
				Quest* quest;
				if(pc->unit->FindQuestItem(msg, &quest, nullptr, false, questId))
					StartNextDialog(quest->GetDialog(QUEST_DIALOG_NEXT), quest);
			}
			break;
		case DTF_IF_QUEST_PROGRESS:
			assert(dialogQuest);
			cmpResult = DoIfOp(dialogQuest->prog, de.value, de.op);
			break;
		case DTF_IF_NEED_TALK:
			{
				cstring msg = dialog->strs[de.value].c_str();
				bool negate = (de.op == OP_NOT_EQUAL);
				for(Quest* quest : questMgr->quests)
				{
					cmpResult = (quest->IsActive() && quest->IfNeedTalk(msg));
					if(negate)
						cmpResult = !cmpResult;
					if(cmpResult)
						break;
				}
			}
			break;
		case DTF_DO_QUEST:
			{
				cstring msg = dialog->strs[de.value].c_str();
				for(Quest* quest : questMgr->quests)
				{
					if(quest->IsActive() && quest->IfNeedTalk(msg))
					{
						StartNextDialog(quest->GetDialog(QUEST_DIALOG_NEXT), quest);
						break;
					}
				}
			}
			break;
		case DTF_IF_SPECIAL:
			{
				cstring msg = dialog->strs[de.value].c_str();
				cmpResult = ExecuteSpecialIf(msg);
				if(de.op == OP_NOT_EQUAL)
					cmpResult = !cmpResult;
			}
			break;
		case DTF_IF_CHOICES:
			cmpResult = DoIfOp(choices.size(), de.value, de.op);
			break;
		case DTF_DO_QUEST2:
			{
				cstring msg = dialog->strs[de.value].c_str();
				bool found = false;
				for(Quest* quest : questMgr->quests)
				{
					if(quest->IfNeedTalk(msg))
					{
						StartNextDialog(quest->GetDialog(QUEST_DIALOG_NEXT), quest);
						found = true;
						break;
					}
				}
				if(!found)
				{
					for(Quest* quest : questMgr->unacceptedQuests)
					{
						if(quest->IfNeedTalk(msg))
						{
							StartNextDialog(quest->GetDialog(QUEST_DIALOG_NEXT), quest);
							break;
						}
					}
				}
			}
			break;
		case DTF_IF_HAVE_ITEM:
			{
				const Item* item = (const Item*)de.value;
				cmpResult = pc->unit->HaveItem(item);
				if(de.op == OP_NOT_EQUAL)
					cmpResult = !cmpResult;
			}
			break;
		case DTF_IF_QUEST_EVENT:
			{
				assert(dialogQuest);
				cmpResult = dialogQuest->IfQuestEvent();
				if(de.op == OP_NOT_EQUAL)
					cmpResult = !cmpResult;
			}
			break;
		case DTF_END_OF_DIALOG:
			assert(0);
			throw Format("Broken dialog '%s'.", dialog->id.c_str());
		case DTF_IF_QUEST_SPECIAL:
			{
				assert(dialogQuest);
				cstring msg = dialog->strs[de.value].c_str();
				cmpResult = dialogQuest->SpecialIf(*this, msg);
				if(de.op == OP_NOT_EQUAL)
					cmpResult = !cmpResult;
			}
			break;
		case DTF_QUEST_SPECIAL:
			{
				assert(dialogQuest);
				cstring msg = dialog->strs[de.value].c_str();
				dialogQuest->Special(*this, msg);
			}
			break;
		case DTF_SCRIPT:
			{
				ScriptContext& ctx = scriptMgr->GetContext();
				ctx.pc = pc;
				ctx.target = talker;

				DialogScripts* scripts;
				Quest2* quest;
				void* instance;
				if(dialogQuest && dialogQuest->isNew)
				{
					quest = static_cast<Quest2*>(dialogQuest);
					scripts = &quest->GetScheme()->scripts;
					instance = quest->GetInstance();
				}
				else
				{
					quest = nullptr;
					scripts = &DialogScripts::global;
					instance = nullptr;
				}

				int index = de.value;
				scriptMgr->RunScript(scripts->Get(DialogScripts::F_SCRIPT), instance, quest, [index](asIScriptContext* ctx, int stage)
				{
					if(stage == 0)
					{
						CHECKED(ctx->SetArgDWord(0, index));
					}
				});

				ctx.target = nullptr;
				ctx.pc = nullptr;
			}
			break;
		case DTF_IF_SCRIPT:
			{
				ScriptContext& ctx = scriptMgr->GetContext();
				ctx.pc = pc;
				ctx.target = talker;

				DialogScripts* scripts;
				Quest2* quest;
				void* instance;
				if(dialogQuest && dialogQuest->isNew)
				{
					quest = static_cast<Quest2*>(dialogQuest);
					scripts = &quest->GetScheme()->scripts;
					instance = quest->GetInstance();
				}
				else
				{
					quest = nullptr;
					scripts = &DialogScripts::global;
					instance = nullptr;
				}

				int index = de.value;
				scriptMgr->RunScript(scripts->Get(DialogScripts::F_IF_SCRIPT), instance, quest, [index, &cmpResult](asIScriptContext* ctx, int stage)
				{
					if(stage == 0)
					{
						CHECKED(ctx->SetArgDWord(0, index));
					}
					else if(stage == 1)
					{
						cmpResult = (ctx->GetReturnByte() != 0);
					}
				});

				ctx.pc = nullptr;
				ctx.target = nullptr;
				if(de.op == OP_NOT_EQUAL)
					cmpResult = !cmpResult;
			}
			break;
		case DTF_JMP:
			dialogPos = de.value - 1;
			break;
		case DTF_CJMP:
			if(!cmpResult)
				dialogPos = de.value - 1;
			break;
		case DTF_IF_VAR:
			cmpResult = DoIfOp(var, de.value, de.op);
			break;
		case DTF_NEXT:
			{
				cstring id = dialog->strs[de.value].c_str();
				GameDialog* dialog = GameDialog::TryGet(id);
				StartNextDialog(dialog, nullptr, false);
			}
			break;
		case DTF_RAND_VAR:
			var = Rand() % de.value;
			break;
		default:
			assert(0 && "Unknown dialog type!");
			break;
		}

		++dialogPos;
	}
}

//=================================================================================================
void DialogContext::EndDialog()
{
	ClearChoices();
	prev.clear();
	dialogMode = false;

	if(talker->busy == Unit::Busy_Trading)
	{
		if(!isLocal)
		{
			NetChangePlayer& c = Add1(pc->playerInfo->changes);
			c.type = NetChangePlayer::END_DIALOG;
		}
		return;
	}
	talker->busy = Unit::Busy_No;
	talker->lookTarget = nullptr;
	talker = nullptr;
	pc->action = PlayerAction::None;

	if(!isLocal)
	{
		NetChangePlayer& c = Add1(pc->playerInfo->changes);
		c.type = NetChangePlayer::END_DIALOG;
	}
}

//=================================================================================================
void DialogContext::ClearChoices()
{
	for(DialogChoice& choice : choices)
	{
		if(choice.pooled)
			StringPool.Free(choice.pooled);
	}
	choices.clear();
}

//=================================================================================================
cstring DialogContext::GetText(int index, bool multi)
{
	GameDialog::Text& text = multi ? dialog->GetMultiText(index) : dialog->GetText(index);
	const string& str = dialog->strs[text.index];

	if(!text.formatted)
		return str.c_str();

	static string strPart;
	dialogString.clear();

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
				ScriptContext& ctx = scriptMgr->GetContext();
				ctx.pc = pc;
				ctx.target = talker;

				Quest2* quest;
				DialogScripts* scripts;
				void* instance;
				if(dialogQuest && dialogQuest->isNew)
				{
					quest = static_cast<Quest2*>(dialogQuest);
					scripts = &quest->GetScheme()->scripts;
					instance = quest->GetInstance();
				}
				else
				{
					quest = nullptr;
					scripts = &DialogScripts::global;
					instance = nullptr;
				}

				scriptMgr->RunScript(scripts->Get(DialogScripts::F_FORMAT), instance, quest, [&](asIScriptContext* ctx, int stage)
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

				ctx.pc = nullptr;
				ctx.target = nullptr;
				i = pos;
			}
			else
			{
				while(str[i] != '$')
				{
					strPart.push_back(str[i]);
					++i;
				}

				if(dialogQuest)
					dialogString += dialogQuest->FormatString(strPart);
				else
					dialogString += FormatString(strPart);
			}
		}
		else
			dialogString.push_back(str[i]);
	}

	return dialogString.c_str();
}

//=================================================================================================
bool DialogContext::ExecuteSpecial(cstring msg)
{
	bool result;
	if(questMgr->HandleSpecial(*this, msg, result))
		return result;

	if(strcmp(msg, "mayor_quest") == 0)
	{
		bool haveQuest = true;
		if(world->GetWorldtime() - gameLevel->cityCtx->questMayorTime > 30
			|| gameLevel->cityCtx->questMayorTime == -1
			|| gameLevel->cityCtx->questMayor == CityQuestState::Failed)
		{
			if(gameLevel->cityCtx->questMayor == CityQuestState::InProgress)
			{
				Quest* quest = questMgr->FindUnacceptedQuest(gameLevel->location, QuestCategory::Mayor);
				if(quest)
					DeleteElement(questMgr->unacceptedQuests, quest);
			}

			// jest nowe zadanie (mo¿e), czas starego min¹³
			gameLevel->cityCtx->questMayorTime = world->GetWorldtime();
			gameLevel->cityCtx->questMayor = CityQuestState::InProgress;

			Quest* quest = questMgr->GetMayorQuest();
			if(quest)
				StartNextDialog(quest->GetDialog(QUEST_DIALOG_START), quest);
			else
				haveQuest = false;
		}
		else if(gameLevel->cityCtx->questMayor == CityQuestState::InProgress)
		{
			// ju¿ ma przydzielone zadanie ?
			Quest* quest = questMgr->FindUnacceptedQuest(gameLevel->location, QuestCategory::Mayor);
			if(quest)
			{
				// quest nie zosta³ zaakceptowany
				StartNextDialog(quest->GetDialog(QUEST_DIALOG_START), quest);
			}
			else
			{
				quest = questMgr->FindQuest(gameLevel->location, QuestCategory::Mayor);
				if(quest)
				{
					Talk(RandomString(game->txQuestAlreadyGiven));
					++dialogPos;
					return true;
				}
				else
					haveQuest = false;
			}
		}
		else
			haveQuest = false;

		if(!haveQuest)
		{
			Talk(RandomString(game->txMayorNoQ));
			++dialogPos;
			return true;
		}
	}
	else if(strcmp(msg, "captain_quest") == 0)
	{
		bool haveQuest = true;
		if(world->GetWorldtime() - gameLevel->cityCtx->questCaptainTime > 30
			|| gameLevel->cityCtx->questCaptainTime == -1
			|| gameLevel->cityCtx->questCaptain == CityQuestState::Failed)
		{
			if(gameLevel->cityCtx->questCaptain == CityQuestState::InProgress)
			{
				Quest* quest = questMgr->FindUnacceptedQuest(gameLevel->location, QuestCategory::Captain);
				if(quest)
					DeleteElement(questMgr->unacceptedQuests, quest);
			}

			// jest nowe zadanie (mo¿e), czas starego min¹³
			gameLevel->cityCtx->questCaptainTime = world->GetWorldtime();
			gameLevel->cityCtx->questCaptain = CityQuestState::InProgress;

			Quest* quest = questMgr->GetCaptainQuest();
			if(quest)
				StartNextDialog(quest->GetDialog(QUEST_DIALOG_START), quest);
			else
				haveQuest = false;
		}
		else if(gameLevel->cityCtx->questCaptain == CityQuestState::InProgress)
		{
			// ju¿ ma przydzielone zadanie
			Quest* quest = questMgr->FindUnacceptedQuest(gameLevel->location, QuestCategory::Captain);
			if(quest)
			{
				// quest nie zosta³ zaakceptowany
				StartNextDialog(quest->GetDialog(QUEST_DIALOG_START), quest);
			}
			else
			{
				quest = questMgr->FindQuest(gameLevel->location, QuestCategory::Captain);
				if(quest)
				{
					Talk(RandomString(game->txQuestAlreadyGiven));
					++dialogPos;
					return true;
				}
				else
					haveQuest = false;
			}
		}
		else
			haveQuest = false;

		if(!haveQuest)
		{
			Talk(RandomString(game->txCaptainNoQ));
			++dialogPos;
			return true;
		}
	}
	else if(strcmp(msg, "item_quest") == 0)
	{
		if(talker->questId == -1)
		{
			Quest* quest = questMgr->GetAdventurerQuest();
			talker->questId = quest->id;
			StartNextDialog(quest->GetDialog(QUEST_DIALOG_START), quest);
		}
		else
		{
			Quest* quest = questMgr->FindUnacceptedQuest(talker->questId);
			StartNextDialog(quest->GetDialog(QUEST_DIALOG_START), quest);
		}
	}
	else if(strcmp(msg, "rest1") == 0 || strcmp(msg, "rest5") == 0 || strcmp(msg, "rest10") == 0 || strcmp(msg, "rest30") == 0)
	{
		// rest in inn
		int days, cost;
		if(strcmp(msg, "rest1") == 0)
		{
			days = 1;
			cost = 5;
		}
		else if(strcmp(msg, "rest5") == 0)
		{
			days = 5;
			cost = 20;
		}
		else if(strcmp(msg, "rest10") == 0)
		{
			days = 10;
			cost = 35;
		}
		else
		{
			days = 30;
			cost = 100;
		}

		// does player have enough gold?
		if(pc->unit->gold < cost)
		{
			// restart dialog
			dialogString = Format(game->txNeedMoreGold, cost - pc->unit->gold);
			Talk(dialogString.c_str());
			dialogPos = 0;
			return true;
		}

		// give gold and freeze
		pc->unit->ModGold(-cost);
		pc->unit->frozen = FROZEN::YES;
		if(isLocal)
		{
			game->fallbackType = FALLBACK::REST;
			game->fallbackTimer = -1.f;
			game->fallbackValue = days;
		}
		else
		{
			NetChangePlayer& c = Add1(pc->playerInfo->changes);
			c.type = NetChangePlayer::REST;
			c.id = days;
		}
	}
	else if(strcmp(msg, "gossip") == 0 || strcmp(msg, "gossip_drunk") == 0)
	{
		bool drunkman = (strcmp(msg, "gossip_drunk") == 0);
		if(!drunkman && (Rand() % 3 == 0 || (input->Down(Key::Shift) && game->devmode)))
		{
			int what = Rand() % 3;
			if(questMgr->HaveQuestRumors() && Rand() % 2 == 0)
				what = 2;
			if(game->devmode && input->Down(Key::Shift))
			{
				if(input->Down(Key::N1))
					what = 0;
				else if(input->Down(Key::N2))
					what = 1;
				else if(input->Down(Key::N3))
					what = 2;
			}
			const vector<Location*>& locations = world->GetLocations();
			switch(what)
			{
			case 0:
				// info about unknown location
				{
					int id = Rand() % locations.size();
					int id2 = id;
					bool ok = false;
					do
					{
						if(locations[id2] && locations[id2]->state == LS_UNKNOWN)
						{
							ok = true;
							break;
						}
						else
						{
							++id2;
							if(id2 == locations.size())
								id2 = 0;
						}
					}
					while(id != id2);
					if(ok)
					{
						Location& loc = *locations[id2];
						loc.SetKnown();
						Location& cloc = *gameLevel->location;
						cstring distance;
						float dist = Vec2::Distance(loc.pos, cloc.pos);
						if(dist <= 300)
							distance = game->txNear;
						else if(dist <= 500)
							distance = "";
						else if(dist <= 700)
							distance = game->txFar;
						else
							distance = game->txVeryFar;

						dialogString = Format(RandomString(game->txLocationDiscovered), distance, GetLocationDirName(cloc.pos, loc.pos), loc.name.c_str());
						Talk(dialogString.c_str());
						++dialogPos;

						return true;
					}
					else
					{
						Talk(RandomString(game->txAllDiscovered));
						++dialogPos;
						return true;
					}
				}
				break;
			case 1:
				// info about unknown camp
				{
					Location* newCamp = nullptr;
					static vector<Location*> camps;
					for(vector<Location*>::const_iterator it = locations.begin(), end = locations.end(); it != end; ++it)
					{
						if(*it && (*it)->type == L_CAMP && (*it)->state != LS_HIDDEN)
						{
							camps.push_back(*it);
							if((*it)->state == LS_UNKNOWN && !newCamp)
								newCamp = *it;
						}
					}

					if(!newCamp && !camps.empty())
						newCamp = camps[Rand() % camps.size()];

					camps.clear();

					if(newCamp)
					{
						Location& loc = *newCamp;
						Location& cloc = *gameLevel->location;
						cstring distance;
						float dist = Vec2::Distance(loc.pos, cloc.pos);
						if(dist <= 300)
							distance = game->txNear;
						else if(dist <= 500)
							distance = "";
						else if(dist <= 700)
							distance = game->txFar;
						else
							distance = game->txVeryFar;

						loc.SetKnown();

						dialogString = Format(RandomString(game->txCampDiscovered), distance, GetLocationDirName(cloc.pos, loc.pos), loc.group->name2.c_str());
						Talk(dialogString.c_str());
						++dialogPos;
						return true;
					}
					else
					{
						Talk(RandomString(game->txAllCampDiscovered));
						++dialogPos;
						return true;
					}
				}
				break;
			case 2:
				// info about quest
				if(!questMgr->HaveQuestRumors())
				{
					Talk(RandomString(game->txNoQRumors));
					++dialogPos;
					return true;
				}
				else
				{
					dialogString = questMgr->GetRandomQuestRumor();
					gameGui->journal->AddRumor(dialogString.c_str());
					Talk(dialogString.c_str());
					++dialogPos;
					return true;
				}
				break;
			default:
				assert(0);
				break;
			}
		}
		else
		{
			int count = countof(game->txRumor);
			if(drunkman)
				count += countof(game->txRumorD);
			cstring rumor;
			do
			{
				uint what = Rand() % count;
				if(what < countof(game->txRumor))
					rumor = game->txRumor[what];
				else
					rumor = game->txRumorD[what - countof(game->txRumor)];
			}
			while(lastRumor == rumor);
			lastRumor = rumor;

			static string str, strPart;
			str.clear();
			for(uint i = 0, len = strlen(rumor); i < len; ++i)
			{
				if(rumor[i] == '$')
				{
					strPart.clear();
					++i;
					while(rumor[i] != '$')
					{
						strPart.push_back(rumor[i]);
						++i;
					}
					str += FormatString(strPart);
				}
				else
					str.push_back(rumor[i]);
			}

			Talk(str.c_str());
			++dialogPos;
			return true;
		}
	}
	else if(strncmp(msg, "train/", 6) == 0)
	{
		const int goldCost = 200;
		cstring s = msg + 6;
		int type, what, cost;
		int* train;

		if(Ability* ability = Ability::Get(s))
		{
			// check required skill
			const SkillId skill = ability->GetSkill();
			if(skill != SkillId::NONE && pc->unit->stats->skill[(int)skill] < ability->skill)
			{
				Talk(game->txCantLearnAbility);
				forceEnd = true;
				return true;
			}

			type = 4;
			what = ability->hash;
			train = nullptr;
			cost = ability->learningPoints;
		}
		else if(Attribute* attrib = Attribute::Find(s))
		{
			type = 0;
			what = (int)attrib->attribId;
			train = &pc->attrib[what].train;
		}
		else if(const Skill* skill = Skill::Find(s))
		{
			if(skill->locked && pc->unit->GetBase(skill->skillId) <= 0)
			{
				Talk(game->txCantLearnSkill);
				forceEnd = true;
				return true;
			}

			type = 1;
			what = (int)skill->skillId;
			train = &pc->skill[what].train;
		}
		else
		{
			assert(0);
			return false;
		}

		// check learning points
		if(train)
			cost = pc->GetTrainCost(*train);
		if(pc->learningPoints < cost)
		{
			Talk(game->txNeedLearningPoints);
			forceEnd = true;
			return true;
		}

		// does player have enough gold?
		if(pc->unit->gold < goldCost)
		{
			dialogString = Format(game->txNeedMoreGold, goldCost - pc->unit->gold);
			Talk(dialogString.c_str());
			forceEnd = true;
			return true;
		}

		// give gold and freeze
		if(train)
			*train += 1;
		pc->learningPoints -= cost;
		pc->unit->ModGold(-goldCost);
		pc->unit->frozen = FROZEN::YES;
		if(isLocal)
		{
			game->fallbackType = FALLBACK::TRAIN;
			game->fallbackTimer = -1.f;
			game->fallbackValue = type;
			game->fallbackValue2 = what;
		}
		else
		{
			NetChangePlayer& c = Add1(pc->playerInfo->changes);
			c.type = NetChangePlayer::TRAIN;
			c.id = type;
			c.count = what;

			pc->playerInfo->updateFlags |= PlayerInfo::UF_LEARNING_POINTS;
		}
	}
	else if(strcmp(msg, "near_loc") == 0)
	{
		const vector<Location*>& locations = world->GetLocations();
		if(updateLocations == 1)
		{
			activeLocations.clear();
			const Vec2& worldPos = world->GetWorldPos();

			int index = 0;
			for(Location* loc : locations)
			{
				if(loc && loc->type != L_CITY && Vec2::Distance(loc->pos, worldPos) <= 150.f && loc->state != LS_HIDDEN)
					activeLocations.push_back(pair<int, bool>(index, loc->state == LS_UNKNOWN));
				++index;
			}

			if(!activeLocations.empty())
			{
				Shuffle(activeLocations.begin(), activeLocations.end());
				std::sort(activeLocations.begin(), activeLocations.end(),
					[](const pair<int, bool>& l1, const pair<int, bool>& l2) -> bool { return l1.second < l2.second; });
				updateLocations = 0;
			}
			else
				updateLocations = -1;
		}

		if(updateLocations == -1)
		{
			Talk(game->txNoNearLoc);
			++dialogPos;
			return true;
		}
		else if(activeLocations.empty())
		{
			Talk(game->txAllNearLoc);
			++dialogPos;
			return true;
		}

		int id = activeLocations.back().first;
		activeLocations.pop_back();
		Location& loc = *locations[id];
		loc.SetKnown();
		dialogString = Format(game->txNearLoc, GetLocationDirName(world->GetWorldPos(), loc.pos), loc.name.c_str());
		if(loc.group->IsEmpty())
			dialogString += RandomString(game->txNearLocEmpty);
		else if(loc.state == LS_CLEARED)
			dialogString += Format(game->txNearLocCleared, loc.group->name3.c_str());
		else
		{
			int gender = loc.group->gender ? 1 : 0;
			cstring desc;
			if(loc.st < 5)
				desc = game->txELvlVeryWeak[gender];
			else if(loc.st < 8)
				desc = game->txELvlWeak[gender];
			else if(loc.st < 11)
				desc = game->txELvlAverage[gender];
			else if(loc.st < 14)
				desc = game->txELvlQuiteStrong[gender];
			else
				desc = game->txELvlStrong[gender];
			dialogString += Format(RandomString(game->txNearLocEnemy), desc, loc.group->name.c_str());
		}

		Talk(dialogString.c_str());
		++dialogPos;
		return true;
	}
	else if(strcmp(msg, "hero_about") == 0)
	{
		Class* clas = talker->GetClass();
		if(clas)
			Talk(clas->about.c_str());
		++dialogPos;
		return true;
	}
	else if(strcmp(msg, "recruit") == 0)
	{
		int cost = talker->hero->JoinCost();
		pc->unit->ModGold(-cost);
		talker->gold += cost;
		team->AddMember(talker, HeroType::Normal);
		talker->temporary = false;
		if(team->freeRecruits > 0)
			--team->freeRecruits;
		talker->hero->SetupMelee();
		if(Net::IsOnline() && !isLocal)
			pc->playerInfo->UpdateGold();
	}
	else if(strcmp(msg, "recruit_free") == 0)
	{
		team->AddMember(talker, HeroType::Normal);
		--team->freeRecruits;
		talker->temporary = false;
		talker->hero->SetupMelee();
	}
	else if(strcmp(msg, "give_item") == 0)
	{
		Unit* t = talker;
		t->busy = Unit::Busy_Trading;
		EndDialog();
		pc->action = PlayerAction::GiveItems;
		pc->actionUnit = t;
		pc->chestTrade = &t->items;
		if(isLocal)
			gameGui->inventory->StartTrade(I_GIVE, *t);
		else
		{
			NetChangePlayer& c = Add1(pc->playerInfo->changes);
			c.type = NetChangePlayer::START_GIVE;
		}
		return true;
	}
	else if(strcmp(msg, "share_items") == 0)
	{
		Unit* t = talker;
		t->busy = Unit::Busy_Trading;
		EndDialog();
		pc->action = PlayerAction::ShareItems;
		pc->actionUnit = t;
		pc->chestTrade = &t->items;
		if(isLocal)
			gameGui->inventory->StartTrade(I_SHARE, *t);
		else
		{
			NetChangePlayer& c = Add1(pc->playerInfo->changes);
			c.type = NetChangePlayer::START_SHARE;
		}
		return true;
	}
	else if(strcmp(msg, "kick_npc") == 0)
	{
		team->RemoveMember(talker);
		if(gameLevel->IsSafeSettlement())
			talker->OrderWander();
		else
			talker->OrderLeave();
		talker->hero->credit = 0;
		talker->ai->cityWander = true;
		talker->ai->locTimer = Random(5.f, 10.f);
		team->CheckCredit(false);
		talker->temporary = true;
	}
	else if(strcmp(msg, "give_item_credit") == 0)
		team->TeamShareGiveItemCredit(*this);
	else if(strcmp(msg, "sell_item") == 0)
		team->TeamShareSellItem(*this);
	else if(strcmp(msg, "share_decline") == 0)
		team->TeamShareDecline(*this);
	else if(strcmp(msg, "force_attack") == 0)
	{
		talker->dontAttack = false;
		talker->attackTeam = true;
		talker->ai->changeAiMode = true;
	}
	else if(strcmp(msg, "ginger_hair") == 0)
	{
		pc->unit->humanData->hairColor = gHairColors[8];
		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::HAIR_COLOR;
			c.unit = pc->unit;
		}
	}
	else if(strcmp(msg, "random_hair") == 0)
	{
		if(Rand() % 2 == 0)
		{
			Vec4 color = pc->unit->humanData->hairColor;
			do
			{
				pc->unit->humanData->hairColor = gHairColors[Rand() % nHairColors];
			}
			while(color.Equal(pc->unit->humanData->hairColor));
		}
		else
		{
			Vec4 color = pc->unit->humanData->hairColor;
			do
			{
				pc->unit->humanData->hairColor = Vec4(Random(0.f, 1.f), Random(0.f, 1.f), Random(0.f, 1.f), 1.f);
			}
			while(color.Equal(pc->unit->humanData->hairColor));
		}
		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::HAIR_COLOR;
			c.unit = pc->unit;
		}
	}
	else if(strcmp(msg, "captive_join") == 0)
	{
		team->AddMember(talker, HeroType::Visitor);
		talker->dontAttack = true;
	}
	else if(strcmp(msg, "captive_escape") == 0)
	{
		if(talker->hero->teamMember)
			team->RemoveMember(talker);
		talker->OrderLeave();
		talker->dontAttack = false;
	}
	else if(strcmp(msg, "news") == 0)
	{
		if(updateNews)
		{
			updateNews = false;
			activeNews = world->GetNews();
			if(activeNews.empty())
			{
				Talk(RandomString(game->txNoNews));
				++dialogPos;
				return true;
			}
		}

		if(activeNews.empty())
		{
			Talk(RandomString(game->txAllNews));
			++dialogPos;
			return true;
		}

		int id = Rand() % activeNews.size();
		News* news = activeNews[id];
		activeNews.erase(activeNews.begin() + id);

		Talk(news->text.c_str());
		++dialogPos;
		return true;
	}
	else if(strcmp(msg, "go_melee") == 0)
	{
		assert(talker->IsHero());
		talker->hero->melee = true;
	}
	else if(strcmp(msg, "go_ranged") == 0)
	{
		assert(talker->IsHero());
		talker->hero->melee = false;
	}
	else if(strcmp(msg, "show_perks") == 0)
	{
		LocalVector<Perk*> toPick;
		PerkContext ctx(pc, false);
		for(Perk* perk : Perk::perks)
		{
			if(IsSet(perk->flags, Perk::History))
				continue;
			if(pc->HavePerk(perk))
				continue;
			TakenPerk tp(perk);
			if(tp.CanTake(ctx))
				toPick->push_back(perk);
		}
		std::sort(toPick->begin(), toPick->end(), [](const Perk* info1, const Perk* info2)
		{
			return info1->name < info2->name;
		});
		for(Perk* perk : toPick)
		{
			string* str = StringPool.Get();
			*str = Format("%s (%s %d %s)", perk->name.c_str(), perk->desc.c_str(), perk->cost,
				perk->cost == 1 ? game->txLearningPoint : game->txLearningPoints);
			DialogChoice choice((int)perk, str->c_str(), questDialogIndex, str);
			choice.type = DialogChoice::Perk;
			choice.talkMsg = perk->name.c_str();
			choices.push_back(choice);
		}
	}
	else if(strcmp(msg, "select_hero") == 0)
	{
		LocalVector<Class*> available;
		for(Class* clas : Class::classes)
		{
			if(clas->hero)
				available->push_back(clas);
		}
		std::sort(available->begin(), available->end(), [](const Class* c1, const Class* c2)
		{
			return c1->name < c2->name;
		});
		for(Class* clas : available)
		{
			DialogChoice choice((int)clas, clas->name.c_str(), questDialogIndex);
			choice.type = DialogChoice::Hero;
			choice.talkMsg = clas->name.c_str();
			choices.push_back(choice);
		}
	}
	else if(strcmp(msg, "coughs") == 0)
	{
		talker->PlaySound(gameRes->sCoughs, Unit::COUGHS_SOUND_DIST);
		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::UNIT_MISC_SOUND;
			c.unit = talker;
		}
	}
	else
	{
		Warn("DTF_SPECIAL: %s", msg);
		assert(0);
	}

	return false;
}

//=================================================================================================
bool DialogContext::ExecuteSpecialIf(cstring msg)
{
	bool result;
	if(questMgr->HandleSpecialIf(*this, msg, result))
		return result;

	if(strcmp(msg, "is_drunk") == 0)
		return IsSet(talker->data->flags, F_AI_DRUNKMAN) && talker->locPart->partType == LocationPart::Type::Building;
	else if(strcmp(msg, "is_inside_dungeon") == 0)
		return gameLevel->localPart->partType == LocationPart::Type::Inside;
	else if(strcmp(msg, "is_team_full") == 0)
		return team->GetActiveTeamSize() >= team->GetMaxSize();
	else if(strcmp(msg, "can_join") == 0)
		return pc->unit->gold >= talker->hero->JoinCost();
	else if(strcmp(msg, "is_near_arena") == 0)
		return gameLevel->cityCtx && IsSet(gameLevel->cityCtx->flags, City::HaveArena) && Vec3::Distance2d(talker->pos, gameLevel->cityCtx->arenaPos) < 5.f;
	else if(strcmp(msg, "is_ginger") == 0)
		return pc->unit->humanData->hairColor.Equal(gHairColors[8]);
	else if(strcmp(msg, "is_bald") == 0)
		return pc->unit->humanData->hair == -1;
	else if(strcmp(msg, "dont_have_quest") == 0)
		return talker->questId == -1;
	else if(strcmp(msg, "have_unaccepted_quest") == 0)
		return questMgr->FindUnacceptedQuest(talker->questId);
	else if(strcmp(msg, "have_completed_quest") == 0)
	{
		Quest* quest = questMgr->FindQuest(talker->questId, false);
		return quest && !quest->IsActive();
	}
	else if(strcmp(msg, "is_free_recruit") == 0)
		return talker->level <= 8 && team->freeRecruits > 0 && !talker->hero->otherTeam;
	else if(strcmp(msg, "have_unique_quest") == 0)
	{
		return (((questMgr->questOrcs2->orcsState == Quest_Orcs2::State::Accepted || questMgr->questOrcs2->orcsState == Quest_Orcs2::State::OrcJoined)
			&& questMgr->questOrcs->startLoc == gameLevel->location)
			|| (questMgr->questMages2->magesState >= Quest_Mages2::State::TalkedWithCaptain
			&& questMgr->questMages2->magesState < Quest_Mages2::State::Completed
			&& questMgr->questMages2->startLoc == gameLevel->location));
	}
	else if(strcmp(msg, "is_not_mage") == 0)
	{
		Class* clas = talker->GetClass();
		return !clas || clas->id != "mage";
	}
	else if(strcmp(msg, "prefer_melee") == 0)
		return talker->hero->melee;
	else if(strcmp(msg, "is_inside_inn") == 0)
		return talker->locPart->partType == LocationPart::Type::Building && gameLevel->cityCtx->FindInn() == talker->locPart;
	else if(strcmp(msg, "is_before_contest") == 0)
		return questMgr->questContest->state >= Quest_Contest::CONTEST_TODAY;
	else if(strcmp(msg, "is_drunkmage") == 0)
		return IsSet(talker->data->flags3, F3_DRUNK_MAGE) && questMgr->questMages2->magesState < Quest_Mages2::State::MageCured;
	else if(strcmp(msg, "is_guard") == 0)
		return IsSet(talker->data->flags2, F2_GUARD);
	else if(strcmp(msg, "mayor_quest_failed") == 0)
		return gameLevel->cityCtx->questMayor == CityQuestState::Failed;
	else if(strcmp(msg, "captain_quest_failed") == 0)
		return gameLevel->cityCtx->questCaptain == CityQuestState::Failed;
	else
	{
		Warn("DTF_IF_SPECIAL: %s", msg);
		assert(0);
	}

	return false;
}

//=================================================================================================
cstring DialogContext::FormatString(const string& strPart)
{
	cstring result;
	if(questMgr->HandleFormatString(strPart, result))
		return result;

	if(strPart == "rcitynhere")
		return world->GetRandomSettlement(gameLevel->location)->name.c_str();
	else if(strPart == "name")
	{
		assert(talker->IsHero());
		return talker->hero->name.c_str();
	}
	else if(strPart == "join_cost")
	{
		assert(talker->IsHero());
		return Format("%d", talker->hero->JoinCost());
	}
	else if(strPart == "item")
	{
		assert(teamShareId != -1);
		return teamShareItem->name.c_str();
	}
	else if(strPart == "item_value")
	{
		assert(teamShareId != -1);
		return Format("%d", teamShareItem->value / 2);
	}
	else if(strPart == "player_name")
		return pc->name.c_str();
	else if(strPart == "rhero")
	{
		static string str;
		bool crazy = Rand() % 4 == 0;
		Class* clas = crazy ? Class::GetRandomCrazy() : Class::GetRandomHero();
		NameHelper::GenerateHeroName(clas, crazy, str);
		return str.c_str();
	}
	else if(strncmp(strPart.c_str(), "player/", 7) == 0)
	{
		int id = int(strPart[7] - '1');
		return game->arena->nearPlayersStr[id].c_str();
	}
	else if(strncmp(strPart.c_str(), "train_lab/", 10) == 0)
	{
		cstring s = strPart.c_str() + 10;
		if(Ability* ability = Ability::Get(s))
		{
			talkMsg = ability->name.c_str();
			const SkillId skill = ability->GetSkill();
			cstring type = IsSet(ability->flags, Ability::Mage) ? game->txSpell : game->txAbility;
			cstring points = ability->learningPoints == 1 ? game->txLearningPoint : game->txLearningPoints;
			if(skill != SkillId::NONE)
			{
				return Format("%s: %s (%d %s, %d %s)", type, ability->name.c_str(), ability->skill,
					Skill::Get(skill).name.c_str(), ability->learningPoints, points);
			}
			else
				return Format("%s: %s (%d %s)", type, ability->name.c_str(), ability->learningPoints, points);
		}
		else
		{
			cstring name;
			int cost;
			if(Attribute* attrib = Attribute::Find(s))
			{
				name = attrib->name.c_str();
				cost = pc->GetTrainCost(pc->attrib[(int)attrib->attribId].train);
			}
			else if(const Skill* skill = Skill::Find(s))
			{
				name = skill->name.c_str();
				cost = pc->GetTrainCost(pc->skill[(int)skill->skillId].train);
			}
			else
			{
				assert(0);
				return "INVALID_TRAIN_STAT";
			}

			talkMsg = name;
			return Format("%s (%d %s)", name, cost, cost == 1 ? game->txLearningPoint : game->txLearningPoints);
		}
	}
	else if(strPart == "date")
		return world->GetDate();
	else
	{
		assert(0);
		return "INVALID_FORMAT_STRING";
	}
}

//=================================================================================================
void DialogContext::Talk(cstring msg)
{
	assert(msg);

	dialogText = msg;
	if(mode == IDLE)
		return;

	mode = WAIT_TALK;
	timer = 1.f + float(strlen(dialogText)) / 20;

	int ani;
	if(!talker->usable && talker->data->type == UNIT_TYPE::HUMAN && talker->action == A_NONE && Rand() % 3 != 0)
	{
		ani = Rand() % 2 + 1;
		talker->meshInst->Play(ani == 1 ? "i_co" : "pokazuje", PLAY_ONCE | PLAY_PRIO2, 0);
		talker->animation = ANI_PLAY;
		talker->action = A_ANIMATION;
	}
	else
		ani = 0;

	gameGui->levelGui->AddSpeechBubble(talker, dialogText);

	pc->Train(TrainWhat::Talk, 0.f, 0);

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::TALK;
		c.unit = talker;
		c.str = StringPool.Get();
		*c.str = msg;
		c.id = ani;
		c.count = game->skipIdCounter++;
		skipId = c.count;
		net->netStrs.push_back(c.str);
	}
}

//=================================================================================================
void DialogContext::ClientTalk(Unit* unit, const string& text, int skipId, int animation)
{
	gameGui->levelGui->AddSpeechBubble(unit, text.c_str());
	unit->bubble->skipId = skipId;

	if(animation != 0)
	{
		unit->meshInst->Play(animation == 1 ? "i_co" : "pokazuje", PLAY_ONCE | PLAY_PRIO2, 0);
		unit->animation = ANI_PLAY;
		unit->action = A_ANIMATION;
	}

	if(dialogMode && talker == unit)
	{
		if(mode == DialogContext::WAIT_MP_RESPONSE)
			ClearChoices();
		dialogString = text;
		dialogText = dialogString.c_str();
		mode = DialogContext::WAIT_TALK;
		this->skipId = skipId;
	}
	else if(pc->action == PlayerAction::Talk && pc->actionUnit == unit)
	{
		predialog = text;
		this->skipId = skipId;
	}
}

//=================================================================================================
bool DialogContext::DoIfOp(int value1, int value2, DialogOp op)
{
	switch(op)
	{
	case OP_EQUAL:
	default:
		return value1 == value2;
	case OP_NOT_EQUAL:
		return value1 != value2;
	case OP_GREATER:
		return value1 > value2;
	case OP_GREATER_EQUAL:
		return value1 >= value2;
	case OP_LESS:
		return value1 < value2;
	case OP_LESS_EQUAL:
		return value1 <= value2;
	case OP_BETWEEN:
		return value1 >= (value2 & 0xFFFF) && value1 <= (int)((value2 & 0xFFFF0000) >> 16);
	case OP_NOT_BETWEEN:
		return value1 < (value2 & 0xFFFF) || value1 >(int)((value2 & 0xFFFF0000) >> 16);
	}
}

//=================================================================================================
bool DialogContext::LearnPerk(Perk* perk)
{
	const int cost = 200;

	// check learning points
	if(pc->learningPoints < perk->cost)
	{
		Talk(game->txNeedLearningPoints);
		forceEnd = true;
		return false;
	}

	// does player have enough gold?
	if(pc->unit->gold < cost)
	{
		dialogString = Format(game->txNeedMoreGold, cost - pc->unit->gold);
		Talk(dialogString.c_str());
		forceEnd = true;
		return false;
	}

	// give gold and freeze
	pc->learningPoints -= perk->cost;
	pc->unit->ModGold(-cost);
	pc->unit->frozen = FROZEN::YES;
	if(isLocal)
	{
		game->fallbackType = FALLBACK::TRAIN;
		game->fallbackTimer = -1.f;
		game->fallbackValue = 3;
		game->fallbackValue2 = perk->hash;
	}
	else
	{
		NetChangePlayer& c = Add1(pc->playerInfo->changes);
		c.type = NetChangePlayer::TRAIN;
		c.id = 3;
		c.count = perk->hash;

		pc->playerInfo->updateFlags |= PlayerInfo::UF_LEARNING_POINTS;
	}

	forceEnd = true;
	return true;
}

//=================================================================================================
bool DialogContext::RecruitHero(Class* clas)
{
	if(team->GetActiveTeamSize() >= 4u)
	{
		Talk(game->txTeamTooBig);
		forceEnd = true;
		return false;
	}

	Unit* u = gameLevel->SpawnUnitNearLocation(*talker->locPart, Vec3(131.f, 0, 121.f), *clas->hero, nullptr);
	u->rot = 0.f;
	u->SetKnownName(true);
	u->hero->loner = false;
	team->AddMember(u, HeroType::Normal);
	dialogString = Format(game->txHeroJoined, u->GetName());
	Talk(dialogString.c_str());
	forceEnd = true;
	return true;
}

//=================================================================================================
void DialogContext::RemoveQuestDialog(Quest2* quest)
{
	assert(quest);

	if(questDialogIndex == QUEST_INDEX_NONE)
	{
		RemoveElements(questDialogs, [=](QuestDialog& dialog) { return dialog.quest == quest; });
		return;
	}

	// find next quest dialog
	for(QuestDialog& dialog : questDialogs)
	{
		if(dialog.quest == quest)
			dialog.dialog = nullptr;
	}
	QuestDialog next = {};
	for(int i = questDialogIndex + 1; i < (int)questDialogs.size(); ++i)
	{
		if(questDialogs[i].dialog)
		{
			next = questDialogs[i];
			break;
		}
	}
	RemoveElements(questDialogs, [](QuestDialog& dialog) { return !dialog.dialog; });
	if(next.dialog)
	{
		for(int i = 0; i < (int)questDialogs.size(); ++i)
		{
			if(questDialogs[i].dialog == next.dialog && questDialogs[i].quest == next.quest)
			{
				questDialogIndex = i - 1;
				if(questDialogIndex == QUEST_INDEX_NONE)
					questDialogIndex = QUEST_INDEX_RESTART;
				break;
			}
		}
	}
}
