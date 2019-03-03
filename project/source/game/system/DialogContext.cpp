#include "Pch.h"
#include "GameCore.h"
#include "DialogContext.h"
#include "ScriptManager.h"
#include "Quest.h"
#include "Game.h"
#include "PlayerInfo.h"
#include "GlobalGui.h"
#include "GameGui.h"
#include "Inventory.h"
#include "Journal.h"
#include "QuestManager.h"
#include "Level.h"
#include "World.h"
#include "LocationHelper.h"
#include "Arena.h"
#include "NameHelper.h"
#include "Quest_Sawmill.h"
#include "Quest_Mine.h"
#include "Quest_Bandits.h"
#include "Quest_Mages.h"
#include "Quest_Orcs.h"
#include "Quest_Goblins.h"
#include "Quest_Evil.h"
#include "Quest_Scripted.h"
#include "Team.h"
#include "AIController.h"
#include "News.h"
#include "MultiInsideLocation.h"
#include "QuestScheme.h"
#include <angelscript.h>

DialogContext* DialogContext::current;

//=================================================================================================
void DialogContext::StartDialog(Unit* talker, GameDialog* dialog)
{
	assert(!dialog_mode);

	// use up auto talk
	if(talker && (talker->auto_talk == AutoTalkMode::Yes || talker->auto_talk == AutoTalkMode::Wait) && talker->auto_talk_dialog == nullptr)
		talker->auto_talk = AutoTalkMode::No;

	dialog_mode = true;
	dialog_wait = -1;
	dialog_pos = 0;
	show_choices = false;
	dialog_text = nullptr;
	dialog_once = true;
	dialog_quest = nullptr;
	dialog_skip = -1;
	dialog_esc = -1;
	this->talker = talker;
	this->dialog = dialog ? dialog : talker->data->dialog;
	if(Net::IsLocal())
	{
		// this vars are useless for clients, don't increase ref counter
		talker->busy = Unit::Busy_Talking;
		talker->look_target = pc->unit;
	}
	update_news = true;
	update_locations = 1;
	pc->action = PlayerController::Action_Talk;
	pc->action_unit = talker;
	ClearChoices();
	can_skip = true;
	force_end = false;
	if(!dialog)
	{
		quest_dialogs = talker->dialogs;
		if(quest_dialogs.empty())
			quest_dialog_index = -1;
		else
		{
			quest_dialog_index = 0;
			prev.push_back({ this->dialog, dialog_quest, -1 });
			this->dialog = quest_dialogs[0].dialog;
			dialog_quest = (Quest*)quest_dialogs[0].quest;
		}
	}
	else
	{
		quest_dialogs.clear();
		quest_dialog_index = -1;
	}

	if(Net::IsLocal())
	{
		// talk sound
		SOUND snd = talker->GetTalkSound();
		if(snd)
		{
			Game::Get().PlayAttachedSound(*talker, snd, 2.f, 5.f);
			if(Net::IsServer())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::HELLO;
				c.unit = talker;
			}
		}
	}

	if(is_local)
		Game::Get().CloseAllPanels();
}

//=================================================================================================
void DialogContext::StartNextDialog(GameDialog* new_dialog, Quest* quest)
{
	assert(new_dialog);

	prev.push_back({ dialog, dialog_quest, dialog_pos });
	dialog = new_dialog;
	dialog_quest = quest;
	dialog_pos = -1;
}

//=================================================================================================
void DialogContext::Update(float dt)
{
	Game& game = Game::Get();
	current = this;

	// wyœwietlono opcje dialogowe, wybierz jedn¹ z nich (w mp czekaj na wybór)
	if(show_choices)
	{
		bool ok = false;
		if(!is_local)
		{
			if(choice_selected != -1)
				ok = true;
		}
		else
			ok = game.gui->game_gui->UpdateChoice(*this, choices.size());

		if(ok)
		{
			DialogChoice& choice = choices[choice_selected];
			cstring msg = choice.talk_msg ? choice.talk_msg : choice.msg;
			game.gui->game_gui->AddSpeechBubble(pc->unit, msg);

			if(Net::IsOnline())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::TALK;
				c.unit = pc->unit;
				c.str = StringPool.Get();
				*c.str = msg;
				c.id = 0;
				c.count = 0;
				N.net_strs.push_back(c.str);
			}

			show_choices = false;
			if(choice.type == DialogChoice::Normal)
			{
				if(choice.quest_dialog_index != -1)
				{
					dialog_quest = quest_dialogs[choice.quest_dialog_index].quest;
					dialog = quest_dialogs[choice.quest_dialog_index].dialog;
				}
				dialog_pos = choice.pos;
				ClearChoices();
				choice_selected = -1;
				dialog_esc = -1;
			}
			else
			{
				bool ok = LearnPerk(choice.pos);
				ClearChoices();
				choice_selected = -1;
				dialog_esc = -1;
				if(ok)
					return;
			}
		}
		else
			return;
	}

	if(dialog_wait > 0.f)
	{
		if(is_local)
		{
			bool skip = false;
			if(can_skip)
			{
				if(GKey.KeyPressedReleaseAllowed(GK_SELECT_DIALOG)
					|| GKey.KeyPressedReleaseAllowed(GK_SKIP_DIALOG)
					|| (GKey.AllowKeyboard() && Key.PressedRelease(VK_ESCAPE)))
					skip = true;
				else
				{
					game.pc_data.wasted_key = GKey.KeyDoReturn(GK_ATTACK_USE, &KeyStates::PressedRelease);
					if(game.pc_data.wasted_key != VK_NONE)
						skip = true;
				}
			}

			if(skip)
				dialog_wait = -1.f;
			else
				dialog_wait -= dt;
		}
		else
		{
			if(choice_selected == 1)
			{
				dialog_wait = -1.f;
				choice_selected = -1;
			}
			else
				dialog_wait -= dt;
		}

		if(dialog_wait > 0.f)
			return;
	}

	can_skip = true;
	if(dialog_skip != -1)
	{
		dialog_pos = dialog_skip;
		dialog_skip = -1;
	}

	if(force_end)
	{
		EndDialog();
		return;
	}

	ScriptContext& ctx = SM.GetContext();
	ctx.pc = pc;
	ctx.target = talker;
	UpdateLoop();
	ctx.pc = nullptr;
	ctx.target = nullptr;
}

void DialogContext::UpdateLoop()
{
	bool cmp_result = false;
	while(true)
	{
		DialogEntry& de = *(dialog->code.data() + dialog_pos);

		switch(de.type)
		{
		case DTF_CHOICE:
			{
				if(de.op == OP_ESCAPE)
					dialog_esc = (int)choices.size();
				talk_msg = nullptr;
				cstring text = GetText(de.value);
				if(text == dialog_s_text.c_str())
				{
					string* str = StringPool.Get();
					*str = text;
					choices.push_back(DialogChoice(dialog_pos + 2, str->c_str(), quest_dialog_index, str));
				}
				else
					choices.push_back(DialogChoice(dialog_pos + 2, text, quest_dialog_index));
				if(talk_msg)
					choices.back().talk_msg = talk_msg;
			}
			break;
		case DTF_END:
			if(prev.empty())
			{
				EndDialog();
				return;
			}
			else if(quest_dialog_index == -1 || quest_dialog_index + 1 == (int)quest_dialogs.size())
			{
				Entry& p = prev.back();
				dialog = p.dialog;
				dialog_pos = p.pos;
				dialog_quest = p.quest;
				prev.pop_back();
				quest_dialog_index = -1;
			}
			else
			{
				++quest_dialog_index;
				dialog = quest_dialogs[quest_dialog_index].dialog;
				dialog_quest = (Quest*)quest_dialogs[quest_dialog_index].quest;
				dialog_pos = -1;
			}
			break;
		case DTF_END2:
			EndDialog();
			return;
		case DTF_SHOW_CHOICES:
			show_choices = true;
			if(is_local)
			{
				choice_selected = 0;
				GameGui* gui = Game::Get().gui->game_gui;
				gui->dialog_cursor_pos = Int2(-1, -1);
				gui->UpdateScrollbar(choices.size());
			}
			else
			{
				choice_selected = -1;
				NetChangePlayer& c = Add1(pc->player_info->changes);
				c.type = NetChangePlayer::SHOW_DIALOG_CHOICES;
			}
			return;
		case DTF_RESTART:
			quest_dialogs = talker->dialogs;
			if(!prev.empty())
			{
				prev.clear();
				if(quest_dialogs.empty())
				{
					dialog = talker->data->dialog;
					dialog_quest = nullptr;
					quest_dialog_index = -1;
				}
				else
				{
					dialog = quest_dialogs[0].dialog;
					dialog_quest = (Quest*)quest_dialogs[0].quest;
					quest_dialog_index = 0;
				}
			}
			dialog_pos = -1;
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
			pc->action = PlayerController::Action_Trade;
			pc->action_unit = talker;
			pc->chest_trade = &talker->stock->items;

			if(is_local)
				Game::Get().gui->inventory->StartTrade(I_TRADE, *pc->chest_trade, talker);
			else
			{
				NetChangePlayer& c = Add1(pc->player_info->changes);
				c.type = NetChangePlayer::START_TRADE;
				c.id = talker->netid;
			}
			return;
		case DTF_TALK:
			{
				cstring msg = GetText(de.value);
				DialogTalk(msg);
				++dialog_pos;
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
			assert(dialog_quest);
			dialog_quest->SetProgress(de.value);
			if(dialog_wait > 0.f)
			{
				++dialog_pos;
				return;
			}
			break;
		case DTF_IF_QUEST_TIMEOUT:
			assert(dialog_quest);
			cmp_result = (dialog_quest->IsActive() && dialog_quest->IsTimedout());
			if(de.op == OP_NOT_EQUAL)
				cmp_result = !cmp_result;
			break;
		case DTF_IF_RAND:
			cmp_result = (Rand() % de.value == 0);
			if(de.op == OP_NOT_EQUAL)
				cmp_result = !cmp_result;
			break;
		case DTF_IF_ONCE:
			cmp_result = dialog_once;
			if(de.op == OP_NOT_EQUAL)
				cmp_result = !cmp_result;
			if(cmp_result)
				dialog_once = false;
			break;
		case DTF_DO_ONCE:
			dialog_once = false;
			break;
		case DTF_CHECK_QUEST_TIMEOUT:
			{
				Quest* quest = QM.FindQuest(L.location_index, (QuestType)de.value);
				if(quest && quest->IsActive() && quest->IsTimedout())
				{
					dialog_once = false;
					StartNextDialog(quest->GetDialog(QUEST_DIALOG_FAIL), quest);
				}
			}
			break;
		case DTF_IF_HAVE_QUEST_ITEM:
			{
				cstring msg = dialog->strs[de.value].c_str();
				cmp_result = pc->unit->FindQuestItem(msg, nullptr, nullptr, false);
				if(de.op == OP_NOT_EQUAL)
					cmp_result = !cmp_result;
			}
			break;
		case DTF_IF_HAVE_QUEST_ITEM_CURRENT:
			cmp_result = (dialog_quest && pc->unit->HaveQuestItem(dialog_quest->refid));
			if(de.op == OP_NOT_EQUAL)
				cmp_result = !cmp_result;
			break;
		case DTF_IF_HAVE_QUEST_ITEM_NOT_ACTIVE:
			{
				cstring msg = dialog->strs[de.value].c_str();
				cmp_result = pc->unit->FindQuestItem(msg, nullptr, nullptr, true);
				if(de.op == OP_NOT_EQUAL)
					cmp_result = !cmp_result;
			}
			break;
		case DTF_DO_QUEST_ITEM:
			{
				cstring msg = dialog->strs[de.value].c_str();
				Quest* quest;
				if(pc->unit->FindQuestItem(msg, &quest, nullptr))
					StartNextDialog(quest->GetDialog(QUEST_DIALOG_NEXT), quest);
			}
			break;
		case DTF_IF_QUEST_PROGRESS:
			assert(dialog_quest);
			cmp_result = DoIfOp(dialog_quest->prog, de.value, de.op);
			break;
		case DTF_IF_NEED_TALK:
			{
				cstring msg = dialog->strs[de.value].c_str();
				bool negate = (de.op == OP_NOT_EQUAL);
				for(Quest* quest : QM.quests)
				{
					cmp_result = (quest->IsActive() && quest->IfNeedTalk(msg));
					if(negate)
						cmp_result = !cmp_result;
					if(cmp_result)
						break;
				}
			}
			break;
		case DTF_DO_QUEST:
			{
				cstring msg = dialog->strs[de.value].c_str();
				for(Quest* quest : QM.quests)
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
				cmp_result = ExecuteSpecialIf(msg);
				if(de.op == OP_NOT_EQUAL)
					cmp_result = !cmp_result;
			}
			break;
		case DTF_IF_CHOICES:
			cmp_result = DoIfOp(choices.size(), de.value, de.op);
			break;
		case DTF_DO_QUEST2:
			{
				cstring msg = dialog->strs[de.value].c_str();
				bool found = false;
				for(Quest* quest : QM.quests)
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
					for(Quest* quest : QM.unaccepted_quests)
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
				cmp_result = pc->unit->HaveItem(item);
				if(de.op == OP_NOT_EQUAL)
					cmp_result = !cmp_result;
			}
			break;
		case DTF_IF_QUEST_EVENT:
			{
				assert(dialog_quest);
				cmp_result = dialog_quest->IfQuestEvent();
				if(de.op == OP_NOT_EQUAL)
					cmp_result = !cmp_result;
			}
			break;
		case DTF_END_OF_DIALOG:
			assert(0);
			throw Format("Broken dialog '%s'.", dialog->id.c_str());
		case DTF_IF_QUEST_SPECIAL:
			{
				assert(dialog_quest);
				cstring msg = dialog->strs[de.value].c_str();
				cmp_result = dialog_quest->SpecialIf(*this, msg);
				if(de.op == OP_NOT_EQUAL)
					cmp_result = !cmp_result;
			}
			break;
		case DTF_QUEST_SPECIAL:
			{
				assert(dialog_quest);
				cstring msg = dialog->strs[de.value].c_str();
				dialog_quest->Special(*this, msg);
			}
			break;
		case DTF_SCRIPT:
			{
				ScriptContext& ctx = SM.GetContext();
				ctx.pc = pc;
				ctx.target = talker;
				DialogScripts* scripts;
				void* instance;
				if(dialog_quest && dialog_quest->quest_id == Q_SCRIPTED)
				{
					Quest_Scripted* quest = (Quest_Scripted*)dialog_quest;
					scripts = &quest->GetScheme()->scripts;
					instance = quest->GetInstance();
					ctx.quest = quest;
				}
				else
				{
					scripts = &DialogScripts::global;
					instance = nullptr;
				}
				int index = de.value;
				SM.RunScript(scripts->Get(DialogScripts::F_SCRIPT), instance, [index](asIScriptContext* ctx, int stage)
				{
					if(stage == 0)
					{
						CHECKED(ctx->SetArgDWord(0, index));
					}
				});
				ctx.quest = nullptr;
				ctx.target = nullptr;
				ctx.pc = nullptr;
			}
			break;
		case DTF_IF_SCRIPT:
			{
				ScriptContext& ctx = SM.GetContext();
				DialogScripts* scripts;
				void* instance;
				ctx.pc = pc;
				ctx.target = talker;
				if(dialog_quest && dialog_quest->quest_id == Q_SCRIPTED)
				{
					Quest_Scripted* quest = (Quest_Scripted*)dialog_quest;
					scripts = &quest->GetScheme()->scripts;
					instance = quest->GetInstance();
					ctx.quest = quest;
				}
				else
				{
					scripts = &DialogScripts::global;
					instance = nullptr;
				}
				int index = de.value;
				SM.RunScript(scripts->Get(DialogScripts::F_IF_SCRIPT), instance, [index, &cmp_result](asIScriptContext* ctx, int stage)
				{
					if(stage == 0)
					{
						CHECKED(ctx->SetArgDWord(0, index));
					}
					else if(stage == 1)
					{
						cmp_result = (ctx->GetReturnByte() != 0);
					}
				});
				ctx.quest = nullptr;
				ctx.pc = nullptr;
				ctx.target = nullptr;
				if(de.op == OP_NOT_EQUAL)
					cmp_result = !cmp_result;
			}
			break;
		case DTF_JMP:
			dialog_pos = de.value - 1;
			break;
		case DTF_CJMP:
			if(!cmp_result)
				dialog_pos = de.value - 1;
			break;
		default:
			assert(0 && "Unknown dialog type!");
			break;
		}

		++dialog_pos;
	}
}

//=================================================================================================
void DialogContext::EndDialog()
{
	ClearChoices();
	prev.clear();
	dialog_mode = false;

	if(talker->busy == Unit::Busy_Trading)
	{
		if(!is_local)
		{
			NetChangePlayer& c = Add1(pc->player_info->changes);
			c.type = NetChangePlayer::END_DIALOG;
		}
		return;
	}
	talker->busy = Unit::Busy_No;
	talker->look_target = nullptr;
	talker = nullptr;
	pc->action = PlayerController::Action_None;

	if(!is_local)
	{
		NetChangePlayer& c = Add1(pc->player_info->changes);
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
cstring DialogContext::GetText(int index)
{
	GameDialog::Text& text = GetTextInner(index);
	const string& str = dialog->strs[text.index];

	if(!text.formatted)
		return str.c_str();

	static string str_part;
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
				ScriptContext& ctx = SM.GetContext();
				DialogScripts* scripts;
				void* instance;
				ctx.pc = pc;
				ctx.target = talker;
				if(dialog_quest && dialog_quest->quest_id == Q_SCRIPTED)
				{
					Quest_Scripted* quest = (Quest_Scripted*)dialog_quest;
					scripts = &quest->GetScheme()->scripts;
					instance = quest->GetInstance();
					ctx.quest = quest;
				}
				else
				{
					scripts = &DialogScripts::global;
					instance = nullptr;
				}
				SM.RunScript(scripts->Get(DialogScripts::F_FORMAT), instance, [&](asIScriptContext* ctx, int stage)
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
				ctx.quest = nullptr;
				ctx.pc = nullptr;
				ctx.target = nullptr;
				i = pos;
			}
			else
			{
				while(str[i] != '$')
				{
					str_part.push_back(str[i]);
					++i;
				}
				if(dialog_quest)
					dialog_s_text += dialog_quest->FormatString(str_part);
				else
					dialog_s_text += FormatString(str_part);
			}
		}
		else
			dialog_s_text.push_back(str[i]);
	}

	return dialog_s_text.c_str();
}

//=================================================================================================
GameDialog::Text& DialogContext::GetTextInner(int index)
{
	GameDialog::Text& text = dialog->texts[index];
	if(text.next == -1)
		return text;
	else
	{
		int count = 1;
		GameDialog::Text* t = &dialog->texts[index];
		while(t->next != -1)
		{
			++count;
			t = &dialog->texts[t->next];
		}
		int id = Rand() % count;
		t = &dialog->texts[index];
		for(int i = 0; i <= id; ++i)
		{
			if(i == id)
				return *t;
			t = &dialog->texts[t->next];
		}
	}
	return text;
}

//=================================================================================================
bool DialogContext::ExecuteSpecial(cstring msg)
{
	bool result;
	if(QM.HandleSpecial(*this, msg, result))
		return result;

	Game& game = Game::Get();

	if(strcmp(msg, "burmistrz_quest") == 0)
	{
		bool have_quest = true;
		if(L.city_ctx->quest_mayor == CityQuestState::Failed)
		{
			DialogTalk(RandomString(game.txMayorQFailed));
			++dialog_pos;
			return true;
		}
		else if(W.GetWorldtime() - L.city_ctx->quest_mayor_time > 30 || L.city_ctx->quest_mayor_time == -1)
		{
			if(L.city_ctx->quest_mayor == CityQuestState::InProgress)
			{
				Quest* quest = QM.FindUnacceptedQuest(L.location_index, QuestType::Mayor);
				DeleteElement(QM.unaccepted_quests, quest);
			}

			// jest nowe zadanie (mo¿e), czas starego min¹³
			L.city_ctx->quest_mayor_time = W.GetWorldtime();
			L.city_ctx->quest_mayor = CityQuestState::InProgress;

			Quest* quest = QM.GetMayorQuest();
			if(quest)
			{
				// add new quest
				quest->refid = QM.quest_counter++;
				quest->Start();
				QM.unaccepted_quests.push_back(quest);
				StartNextDialog(quest->GetDialog(QUEST_DIALOG_START), quest);
			}
			else
				have_quest = false;
		}
		else if(L.city_ctx->quest_mayor == CityQuestState::InProgress)
		{
			// ju¿ ma przydzielone zadanie ?
			Quest* quest = QM.FindUnacceptedQuest(L.location_index, QuestType::Mayor);
			if(quest)
			{
				// quest nie zosta³ zaakceptowany
				StartNextDialog(quest->GetDialog(QUEST_DIALOG_START), quest);
			}
			else
			{
				quest = QM.FindQuest(L.location_index, QuestType::Mayor);
				if(quest)
				{
					DialogTalk(RandomString(game.txQuestAlreadyGiven));
					++dialog_pos;
					return true;
				}
				else
					have_quest = false;
			}
		}
		else
			have_quest = false;
		if(!have_quest)
		{
			DialogTalk(RandomString(game.txMayorNoQ));
			++dialog_pos;
			return true;
		}
	}
	else if(strcmp(msg, "dowodca_quest") == 0)
	{
		bool have_quest = true;
		if(L.city_ctx->quest_captain == CityQuestState::Failed)
		{
			DialogTalk(RandomString(game.txCaptainQFailed));
			++dialog_pos;
			return true;
		}
		else if(W.GetWorldtime() - L.city_ctx->quest_captain_time > 30 || L.city_ctx->quest_captain_time == -1)
		{
			if(L.city_ctx->quest_captain == CityQuestState::InProgress)
			{
				Quest* quest = QM.FindUnacceptedQuest(L.location_index, QuestType::Captain);
				DeleteElement(QM.unaccepted_quests, quest);
			}

			// jest nowe zadanie (mo¿e), czas starego min¹³
			L.city_ctx->quest_captain_time = W.GetWorldtime();
			L.city_ctx->quest_captain = CityQuestState::InProgress;

			Quest* quest = QM.GetCaptainQuest();
			if(quest)
			{
				// add new quest
				quest->refid = QM.quest_counter++;
				quest->Start();
				QM.unaccepted_quests.push_back(quest);
				StartNextDialog(quest->GetDialog(QUEST_DIALOG_START), quest);
			}
			else
				have_quest = false;
		}
		else if(L.city_ctx->quest_captain == CityQuestState::InProgress)
		{
			// ju¿ ma przydzielone zadanie
			Quest* quest = QM.FindUnacceptedQuest(L.location_index, QuestType::Captain);
			if(quest)
			{
				// quest nie zosta³ zaakceptowany
				StartNextDialog(quest->GetDialog(QUEST_DIALOG_START), quest);
			}
			else
			{
				quest = QM.FindQuest(L.location_index, QuestType::Captain);
				if(quest)
				{
					DialogTalk(RandomString(game.txQuestAlreadyGiven));
					++dialog_pos;
					return true;
				}
				else
					have_quest = false;
			}
		}
		else
			have_quest = false;
		if(!have_quest)
		{
			DialogTalk(RandomString(game.txCaptainNoQ));
			++dialog_pos;
			return true;
		}
	}
	else if(strcmp(msg, "przedmiot_quest") == 0)
	{
		if(talker->quest_refid == -1)
		{
			Quest* quest = QM.GetAdventurerQuest();
			quest->refid = QM.quest_counter++;
			talker->quest_refid = quest->refid;
			quest->Start();
			QM.unaccepted_quests.push_back(quest);
			StartNextDialog(quest->GetDialog(QUEST_DIALOG_START), quest);
		}
		else
		{
			Quest* quest = QM.FindUnacceptedQuest(talker->quest_refid);
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
			dialog_s_text = Format(game.txNeedMoreGold, cost - pc->unit->gold);
			DialogTalk(dialog_s_text.c_str());
			dialog_pos = 0;
			return true;
		}

		// give gold and freeze
		pc->unit->ModGold(-cost);
		pc->unit->frozen = FROZEN::YES;
		if(is_local)
		{
			game.fallback_type = FALLBACK::REST;
			game.fallback_t = -1.f;
			game.fallback_1 = days;
		}
		else
		{
			NetChangePlayer& c = Add1(pc->player_info->changes);
			c.type = NetChangePlayer::REST;
			c.id = days;
		}
	}
	else if(strcmp(msg, "gossip") == 0 || strcmp(msg, "gossip_drunk") == 0)
	{
		bool drunkman = (strcmp(msg, "gossip_drunk") == 0);
		if(!drunkman && (Rand() % 3 == 0 || (Key.Down(VK_SHIFT) && game.devmode)))
		{
			int what = Rand() % 3;
			if(QM.quest_rumor_counter != 0 && Rand() % 2 == 0)
				what = 2;
			if(game.devmode)
			{
				if(Key.Down('1'))
					what = 0;
				else if(Key.Down('2'))
					what = 1;
				else if(Key.Down('3'))
					what = 2;
			}
			const vector<Location*>& locations = W.GetLocations();
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
						Location& cloc = *L.location;
						cstring distance;
						float dist = Vec2::Distance(loc.pos, cloc.pos);
						if(dist <= 300)
							distance = game.txNear;
						else if(dist <= 500)
							distance = "";
						else if(dist <= 700)
							distance = game.txFar;
						else
							distance = game.txVeryFar;

						dialog_s_text = Format(RandomString(game.txLocationDiscovered), distance, GetLocationDirName(cloc.pos, loc.pos), loc.name.c_str());
						DialogTalk(dialog_s_text.c_str());
						++dialog_pos;

						return true;
					}
					else
					{
						DialogTalk(RandomString(game.txAllDiscovered));
						++dialog_pos;
						return true;
					}
				}
				break;
			case 1:
				// info about unknown camp
				{
					Location* new_camp = nullptr;
					static vector<Location*> camps;
					for(vector<Location*>::const_iterator it = locations.begin(), end = locations.end(); it != end; ++it)
					{
						if(*it && (*it)->type == L_CAMP && (*it)->state != LS_HIDDEN)
						{
							camps.push_back(*it);
							if((*it)->state == LS_UNKNOWN && !new_camp)
								new_camp = *it;
						}
					}

					if(!new_camp && !camps.empty())
						new_camp = camps[Rand() % camps.size()];

					camps.clear();

					if(new_camp)
					{
						Location& loc = *new_camp;
						Location& cloc = *L.location;
						cstring distance;
						float dist = Vec2::Distance(loc.pos, cloc.pos);
						if(dist <= 300)
							distance = game.txNear;
						else if(dist <= 500)
							distance = "";
						else if(dist <= 700)
							distance = game.txFar;
						else
							distance = game.txVeryFar;

						cstring enemies;
						switch(loc.spawn)
						{
						case SG_ORCS:
							enemies = game.txSGOOrcs;
							break;
						case SG_BANDITS:
							enemies = game.txSGOBandits;
							break;
						case SG_GOBLINS:
							enemies = game.txSGOGoblins;
							break;
						default:
							assert(0);
							enemies = game.txSGOEnemies;
							break;
						}

						loc.SetKnown();

						dialog_s_text = Format(RandomString(game.txCampDiscovered), distance, GetLocationDirName(cloc.pos, loc.pos), enemies);
						DialogTalk(dialog_s_text.c_str());
						++dialog_pos;
						return true;
					}
					else
					{
						DialogTalk(RandomString(game.txAllCampDiscovered));
						++dialog_pos;
						return true;
					}
				}
				break;
			case 2:
				// info about quest
				if(QM.quest_rumor_counter == 0)
				{
					DialogTalk(RandomString(game.txNoQRumors));
					++dialog_pos;
					return true;
				}
				else
				{
					what = Rand() % R_MAX;
					while(QM.quest_rumor[what])
						what = (what + 1) % R_MAX;
					--QM.quest_rumor_counter;
					QM.quest_rumor[what] = true;

					switch(what)
					{
					case R_SAWMILL:
						dialog_s_text = Format(game.txRumorQ[0], locations[QM.quest_sawmill->start_loc]->name.c_str());
						break;
					case R_MINE:
						dialog_s_text = Format(game.txRumorQ[1], locations[QM.quest_mine->start_loc]->name.c_str());
						break;
					case R_CONTEST:
						dialog_s_text = game.txRumorQ[2];
						break;
					case R_BANDITS:
						dialog_s_text = Format(game.txRumorQ[3], locations[QM.quest_bandits->start_loc]->name.c_str());
						break;
					case R_MAGES:
						dialog_s_text = Format(game.txRumorQ[4], locations[QM.quest_mages->start_loc]->name.c_str());
						break;
					case R_MAGES2:
						dialog_s_text = game.txRumorQ[5];
						break;
					case R_ORCS:
						dialog_s_text = Format(game.txRumorQ[6], locations[QM.quest_orcs->start_loc]->name.c_str());
						break;
					case R_GOBLINS:
						dialog_s_text = Format(game.txRumorQ[7], locations[QM.quest_goblins->start_loc]->name.c_str());
						break;
					case R_EVIL:
						dialog_s_text = Format(game.txRumorQ[8], locations[QM.quest_evil->start_loc]->name.c_str());
						break;
					default:
						assert(0);
						return true;
					}

					game.gui->journal->AddRumor(dialog_s_text.c_str());
					DialogTalk(dialog_s_text.c_str());
					++dialog_pos;
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
			int count = countof(game.txRumor);
			if(drunkman)
				count += countof(game.txRumorD);
			cstring rumor;
			do
			{
				uint what = Rand() % count;
				if(what < countof(game.txRumor))
					rumor = game.txRumor[what];
				else
					rumor = game.txRumorD[what - countof(game.txRumor)];
			}
			while(last_rumor == rumor);
			last_rumor = rumor;

			static string str, str_part;
			str.clear();
			for(uint i = 0, len = strlen(rumor); i < len; ++i)
			{
				if(rumor[i] == '$')
				{
					str_part.clear();
					++i;
					while(rumor[i] != '$')
					{
						str_part.push_back(rumor[i]);
						++i;
					}
					str += FormatString(str_part);
				}
				else
					str.push_back(rumor[i]);
			}

			DialogTalk(str.c_str());
			++dialog_pos;
			return true;
		}
	}
	else if(strncmp(msg, "train/", 6) == 0)
	{
		const int gold_cost = 200;
		cstring s = msg + 6;
		bool is_skill;
		int what;
		int* train;

		Attribute* attrib = Attribute::Find(s);
		if(attrib)
		{
			is_skill = false;
			what = (int)attrib->attrib_id;
			train = &pc->attrib[what].train;
		}
		else
		{
			Skill* skill = Skill::Find(s);
			if(skill)
			{
				is_skill = true;
				what = (int)skill->skill_id;
				train = &pc->skill[what].train;
			}
			else
			{
				assert(0);
				return false;
			}
		}

		// check learning points
		int cost = pc->GetTrainCost(*train);
		if(pc->learning_points < cost)
		{
			DialogTalk(game.txNeedLearningPoints);
			force_end = true;
			return true;
		}

		// does player have enough gold?
		if(pc->unit->gold < gold_cost)
		{
			dialog_s_text = Format(game.txNeedMoreGold, gold_cost - pc->unit->gold);
			DialogTalk(dialog_s_text.c_str());
			force_end = true;
			return true;
		}

		// give gold and freeze
		*train += 1;
		pc->learning_points -= cost;
		pc->unit->ModGold(-gold_cost);
		pc->unit->frozen = FROZEN::YES;
		if(is_local)
		{
			game.fallback_type = FALLBACK::TRAIN;
			game.fallback_t = -1.f;
			game.fallback_1 = (is_skill ? 1 : 0);
			game.fallback_2 = what;
		}
		else
		{
			NetChangePlayer& c = Add1(pc->player_info->changes);
			c.type = NetChangePlayer::TRAIN;
			c.id = (is_skill ? 1 : 0);
			c.count = what;

			pc->player_info->update_flags |= PlayerInfo::UF_LEARNING_POINTS;
		}
	}
	else if(strcmp(msg, "near_loc") == 0)
	{
		const vector<Location*>& locations = W.GetLocations();
		if(update_locations == 1)
		{
			active_locations.clear();
			const Vec2& world_pos = W.GetWorldPos();

			int index = 0;
			for(Location* loc : locations)
			{
				if(loc && loc->type != L_CITY && Vec2::Distance(loc->pos, world_pos) <= 150.f && loc->state != LS_HIDDEN)
					active_locations.push_back(std::pair<int, bool>(index, loc->state == LS_UNKNOWN));
				++index;
			}

			if(!active_locations.empty())
			{
				std::random_shuffle(active_locations.begin(), active_locations.end(), MyRand);
				std::sort(active_locations.begin(), active_locations.end(),
					[](const std::pair<int, bool>& l1, const std::pair<int, bool>& l2) -> bool { return l1.second < l2.second; });
				update_locations = 0;
			}
			else
				update_locations = -1;
		}

		if(update_locations == -1)
		{
			DialogTalk(game.txNoNearLoc);
			++dialog_pos;
			return true;
		}
		else if(active_locations.empty())
		{
			DialogTalk(game.txAllNearLoc);
			++dialog_pos;
			return true;
		}

		int id = active_locations.back().first;
		active_locations.pop_back();
		Location& loc = *locations[id];
		loc.SetKnown();
		dialog_s_text = Format(game.txNearLoc, GetLocationDirName(W.GetWorldPos(), loc.pos), loc.name.c_str());
		if(loc.spawn == SG_NONE)
		{
			if(loc.type != L_CAVE && loc.type != L_FOREST && loc.type != L_MOONWELL)
				dialog_s_text += RandomString(game.txNearLocEmpty);
		}
		else if(loc.state == LS_CLEARED)
			dialog_s_text += Format(game.txNearLocCleared, g_spawn_groups[loc.spawn].name);
		else
		{
			SpawnGroup& sg = g_spawn_groups[loc.spawn];
			cstring jacy;
			if(loc.st < 5)
			{
				if(sg.k == K_I)
					jacy = game.txELvlVeryWeak[0];
				else
					jacy = game.txELvlVeryWeak[1];
			}
			else if(loc.st < 8)
			{
				if(sg.k == K_I)
					jacy = game.txELvlWeak[0];
				else
					jacy = game.txELvlWeak[1];
			}
			else if(loc.st < 11)
			{
				if(sg.k == K_I)
					jacy = game.txELvlAverage[0];
				else
					jacy = game.txELvlAverage[1];
			}
			else if(loc.st < 14)
			{
				if(sg.k == K_I)
					jacy = game.txELvlQuiteStrong[0];
				else
					jacy = game.txELvlQuiteStrong[1];
			}
			else
			{
				if(sg.k == K_I)
					jacy = game.txELvlStrong[0];
				else
					jacy = game.txELvlStrong[1];
			}
			dialog_s_text += Format(RandomString(game.txNearLocEnemy), jacy, g_spawn_groups[loc.spawn].name);
		}

		DialogTalk(dialog_s_text.c_str());
		++dialog_pos;
		return true;
	}
	else if(strcmp(msg, "tell_name") == 0)
		talker->RevealName(false);
	else if(strcmp(msg, "hero_about") == 0)
	{
		Class clas = talker->GetClass();
		if(clas < Class::MAX)
			DialogTalk(ClassInfo::classes[(int)clas].about.c_str());
		++dialog_pos;
		return true;
	}
	else if(strcmp(msg, "recruit") == 0)
	{
		int cost = talker->hero->JoinCost();
		pc->unit->ModGold(-cost);
		talker->gold += cost;
		Team.AddTeamMember(talker, false);
		talker->temporary = false;
		if(Team.free_recruits > 0)
			--Team.free_recruits;
		talker->hero->SetupMelee();
		if(Net::IsOnline() && !is_local)
			pc->player_info->UpdateGold();
	}
	else if(strcmp(msg, "recruit_free") == 0)
	{
		Team.AddTeamMember(talker, false);
		--Team.free_recruits;
		talker->temporary = false;
		talker->hero->SetupMelee();
	}
	else if(strcmp(msg, "follow_me") == 0)
	{
		assert(talker->IsFollower());
		talker->hero->mode = HeroData::Follow;
		talker->ai->city_wander = false;
	}
	else if(strcmp(msg, "go_free") == 0)
	{
		assert(talker->IsFollower());
		talker->hero->mode = HeroData::Wander;
		talker->ai->city_wander = false;
		talker->ai->loc_timer = Random(5.f, 10.f);
	}
	else if(strcmp(msg, "wait") == 0)
	{
		assert(talker->IsFollower());
		talker->hero->mode = HeroData::Wait;
		talker->ai->city_wander = false;
	}
	else if(strcmp(msg, "give_item") == 0)
	{
		Unit* t = talker;
		t->busy = Unit::Busy_Trading;
		EndDialog();
		pc->action = PlayerController::Action_GiveItems;
		pc->action_unit = t;
		pc->chest_trade = &t->items;
		if(is_local)
			game.gui->inventory->StartTrade(I_GIVE, *t);
		else
		{
			NetChangePlayer& c = Add1(pc->player_info->changes);
			c.type = NetChangePlayer::START_GIVE;
		}
		return true;
	}
	else if(strcmp(msg, "share_items") == 0)
	{
		Unit* t = talker;
		t->busy = Unit::Busy_Trading;
		EndDialog();
		pc->action = PlayerController::Action_ShareItems;
		pc->action_unit = t;
		pc->chest_trade = &t->items;
		if(is_local)
			game.gui->inventory->StartTrade(I_SHARE, *t);
		else
		{
			NetChangePlayer& c = Add1(pc->player_info->changes);
			c.type = NetChangePlayer::START_SHARE;
		}
		return true;
	}
	else if(strcmp(msg, "kick_npc") == 0)
	{
		Team.RemoveTeamMember(talker);
		if(L.city_ctx)
			talker->hero->mode = HeroData::Wander;
		else
			talker->hero->mode = HeroData::Leave;
		talker->hero->credit = 0;
		talker->ai->city_wander = true;
		talker->ai->loc_timer = Random(5.f, 10.f);
		Team.CheckCredit(false);
		talker->temporary = true;
	}
	else if(strcmp(msg, "give_item_credit") == 0)
		Team.TeamShareGiveItemCredit(*this);
	else if(strcmp(msg, "sell_item") == 0)
		Team.TeamShareSellItem(*this);
	else if(strcmp(msg, "share_decline") == 0)
		Team.TeamShareDecline(*this);
	else if(strcmp(msg, "force_attack") == 0)
	{
		talker->dont_attack = false;
		talker->attack_team = true;
		talker->ai->change_ai_mode = true;
	}
	else if(strcmp(msg, "ginger_hair") == 0)
	{
		pc->unit->human_data->hair_color = g_hair_colors[8];
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
			Vec4 kolor = pc->unit->human_data->hair_color;
			do
			{
				pc->unit->human_data->hair_color = g_hair_colors[Rand() % n_hair_colors];
			}
			while(kolor.Equal(pc->unit->human_data->hair_color));
		}
		else
		{
			Vec4 kolor = pc->unit->human_data->hair_color;
			do
			{
				pc->unit->human_data->hair_color = Vec4(Random(0.f, 1.f), Random(0.f, 1.f), Random(0.f, 1.f), 1.f);
			}
			while(kolor.Equal(pc->unit->human_data->hair_color));
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
		Team.AddTeamMember(talker, true);
		talker->dont_attack = true;
	}
	else if(strcmp(msg, "captive_escape") == 0)
	{
		if(talker->hero->team_member)
			Team.RemoveTeamMember(talker);
		talker->hero->mode = HeroData::Leave;
		talker->dont_attack = false;
	}
	else if(strcmp(msg, "news") == 0)
	{
		if(update_news)
		{
			update_news = false;
			active_news = W.GetNews();
			if(active_news.empty())
			{
				DialogTalk(RandomString(game.txNoNews));
				++dialog_pos;
				return true;
			}
		}

		if(active_news.empty())
		{
			DialogTalk(RandomString(game.txAllNews));
			++dialog_pos;
			return true;
		}

		int id = Rand() % active_news.size();
		News* news = active_news[id];
		active_news.erase(active_news.begin() + id);

		DialogTalk(news->text.c_str());
		++dialog_pos;
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
		LocalVector<PerkInfo*> to_pick;
		PerkContext ctx(pc, false);
		for(PerkInfo& info : PerkInfo::perks)
		{
			if(IS_SET(info.flags, PerkInfo::History))
				continue;
			if(pc->HavePerk(info.perk_id))
				continue;
			TakenPerk tp(info.perk_id);
			if(tp.CanTake(ctx))
				to_pick->push_back(&info);
		}
		std::sort(to_pick->begin(), to_pick->end(), [](const PerkInfo* info1, const PerkInfo* info2)
		{
			return info1->name < info2->name;
		});
		for(PerkInfo* info : to_pick)
		{
			string* str = StringPool.Get();
			*str = Format("%s (%s %d %s)", info->name.c_str(), info->desc.c_str(), info->cost,
				info->cost == 1 ? game.txLearningPoint : game.txLearningPoints);
			DialogChoice choice((int)info->perk_id, str->c_str(), quest_dialog_index, str);
			choice.type = DialogChoice::Perk;
			choice.talk_msg = info->name.c_str();
			choices.push_back(choice);
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
	if(QM.HandleSpecialIf(*this, msg, result))
		return result;

	if(strcmp(msg, "have_team") == 0)
		return Team.HaveTeamMember();
	else if(strcmp(msg, "have_pc_team") == 0)
		return Team.HaveOtherPlayer();
	else if(strcmp(msg, "have_npc_team") == 0)
		return Team.HaveActiveNpc();
	else if(strcmp(msg, "is_drunk") == 0)
		return IS_SET(talker->data->flags, F_AI_DRUNKMAN) && talker->in_building != -1;
	else if(strcmp(msg, "is_not_known") == 0)
	{
		assert(talker->IsHero());
		return !talker->hero->know_name;
	}
	else if(strcmp(msg, "is_inside_dungeon") == 0)
		return L.local_ctx.type == LevelContext::Inside;
	else if(strcmp(msg, "is_team_full") == 0)
		return Team.GetActiveTeamSize() >= Team.GetMaxSize();
	else if(strcmp(msg, "can_join") == 0)
		return pc->unit->gold >= talker->hero->JoinCost();
	else if(strcmp(msg, "is_inside_town") == 0)
		return L.city_ctx != nullptr;
	else if(strcmp(msg, "is_free") == 0)
	{
		assert(talker->IsHero());
		return talker->hero->mode == HeroData::Wander;
	}
	else if(strcmp(msg, "is_not_free") == 0)
	{
		assert(talker->IsHero());
		return talker->hero->mode != HeroData::Wander;
	}
	else if(strcmp(msg, "is_not_follow") == 0)
	{
		assert(talker->IsHero());
		return talker->hero->mode != HeroData::Follow;
	}
	else if(strcmp(msg, "is_not_waiting") == 0)
	{
		assert(talker->IsHero());
		return talker->hero->mode != HeroData::Wait;
	}
	else if(strcmp(msg, "is_near_arena") == 0)
		return L.city_ctx && IS_SET(L.city_ctx->flags, City::HaveArena) && Vec3::Distance2d(talker->pos, L.city_ctx->arena_pos) < 5.f;
	else if(strcmp(msg, "is_lost_pvp") == 0)
	{
		assert(talker->IsHero());
		return talker->hero->lost_pvp;
	}
	else if(strcmp(msg, "is_healthy") == 0)
		return talker->GetHpp() >= 0.75f;
	else if(strcmp(msg, "is_bandit") == 0)
		return Team.is_bandit;
	else if(strcmp(msg, "is_ginger") == 0)
		return pc->unit->human_data->hair_color.Equal(g_hair_colors[8]);
	else if(strcmp(msg, "is_bald") == 0)
		return pc->unit->human_data->hair == -1;
	else if(strcmp(msg, "is_camp") == 0)
		return Game::Get().target_loc_is_camp;
	else if(strcmp(msg, "dont_have_quest") == 0)
		return talker->quest_refid == -1;
	else if(strcmp(msg, "have_unaccepted_quest") == 0)
		return QM.FindUnacceptedQuest(talker->quest_refid);
	else if(strcmp(msg, "have_completed_quest") == 0)
	{
		Quest* q = QM.FindQuest(talker->quest_refid);
		if(q && !q->IsActive())
			return true;
	}
	else if(strcmp(msg, "is_free_recruit") == 0)
		return talker->level <= 8 && Team.free_recruits > 0;
	else if(strcmp(msg, "have_unique_quest") == 0)
	{
		if(((QM.quest_orcs2->orcs_state == Quest_Orcs2::State::Accepted || QM.quest_orcs2->orcs_state == Quest_Orcs2::State::OrcJoined)
			&& QM.quest_orcs->start_loc == L.location_index)
			|| (QM.quest_mages2->mages_state >= Quest_Mages2::State::TalkedWithCaptain
			&& QM.quest_mages2->mages_state < Quest_Mages2::State::Completed
			&& QM.quest_mages2->start_loc == L.location_index))
			return true;
	}
	else if(strcmp(msg, "is_not_mage") == 0)
		return talker->GetClass() != Class::MAGE;
	else if(strcmp(msg, "prefer_melee") == 0)
		return talker->hero->melee;
	else
	{
		Warn("DTF_IF_SPECIAL: %s", msg);
		assert(0);
	}

	return false;
}

//=================================================================================================
cstring DialogContext::FormatString(const string& str_part)
{
	cstring result;
	if(QM.HandleFormatString(str_part, result))
		return result;

	if(str_part == "rcitynhere")
		return W.GetRandomSettlement(L.location_index)->name.c_str();
	else if(str_part == "name")
	{
		assert(talker->IsHero());
		return talker->hero->name.c_str();
	}
	else if(str_part == "join_cost")
	{
		assert(talker->IsHero());
		return Format("%d", talker->hero->JoinCost());
	}
	else if(str_part == "item")
	{
		assert(team_share_id != -1);
		return team_share_item->name.c_str();
	}
	else if(str_part == "item_value")
	{
		assert(team_share_id != -1);
		return Format("%d", team_share_item->value / 2);
	}
	else if(str_part == "player_name")
		return pc->name.c_str();
	else if(str_part == "rhero")
	{
		static string str;
		NameHelper::GenerateHeroName(ClassInfo::GetRandom(), Rand() % 4 == 0, str);
		return str.c_str();
	}
	else if(strncmp(str_part.c_str(), "player/", 7) == 0)
	{
		int id = int(str_part[7] - '1');
		return Game::Get().arena->near_players_str[id].c_str();
	}
	else if(strncmp(str_part.c_str(), "train_lab/", 10) == 0)
	{
		cstring s = str_part.c_str() + 10;
		cstring name;
		int cost;

		Attribute* attrib = Attribute::Find(s);
		if(attrib)
		{
			name = attrib->name.c_str();
			cost = pc->GetTrainCost(pc->attrib[(int)attrib->attrib_id].train);
		}
		else
		{
			Skill* skill = Skill::Find(s);
			if(skill)
			{
				name = skill->name.c_str();
				cost = pc->GetTrainCost(pc->skill[(int)skill->skill_id].train);
			}
			else
			{
				assert(0);
				return "INVALID_TRAIN_STAT";
			}
		}

		talk_msg = name;

		Game& game = Game::Get();
		return Format("%s (%d %s)", name, cost, cost == 1 ? game.txLearningPoint : game.txLearningPoints);
	}
	else if(str_part == "date")
		return W.GetDate();
	else
	{
		assert(0);
		return "INVALID_FORMAT_STRING";
	}
}

//=================================================================================================
void DialogContext::DialogTalk(cstring msg)
{
	assert(msg);

	Game& game = Game::Get();

	dialog_text = msg;
	dialog_wait = 1.f + float(strlen(dialog_text)) / 20;

	int ani;
	if(!talker->usable && talker->data->type == UNIT_TYPE::HUMAN && talker->action == A_NONE && Rand() % 3 != 0)
	{
		ani = Rand() % 2 + 1;
		talker->mesh_inst->Play(ani == 1 ? "i_co" : "pokazuje", PLAY_ONCE | PLAY_PRIO2, 0);
		talker->mesh_inst->groups[0].speed = 1.f;
		talker->animation = ANI_PLAY;
		talker->action = A_ANIMATION;
	}
	else
		ani = 0;

	game.gui->game_gui->AddSpeechBubble(talker, dialog_text);

	pc->Train(TrainWhat::Talk, 0.f, 0);

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::TALK;
		c.unit = talker;
		c.str = StringPool.Get();
		*c.str = msg;
		c.id = ani;
		c.count = game.skip_id_counter++;
		skip_id = c.count;
		N.net_strs.push_back(c.str);
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
		return value1 < (value2 & 0xFFFF) || value1 > (int)((value2 & 0xFFFF0000) >> 16);
	}
}

//=================================================================================================
bool DialogContext::LearnPerk(int perk)
{
	const int cost = 200;
	PerkInfo& info = PerkInfo::perks[perk];
	Game& game = Game::Get();

	// check learning points
	if(pc->learning_points < info.cost)
	{
		DialogTalk(game.txNeedLearningPoints);
		force_end = true;
		return false;
	}

	// does player have enough gold?
	if(pc->unit->gold < cost)
	{
		dialog_s_text = Format(game.txNeedMoreGold, cost - pc->unit->gold);
		DialogTalk(dialog_s_text.c_str());
		force_end = true;
		return false;
	}

	// give gold and freeze
	pc->learning_points -= info.cost;
	pc->unit->ModGold(-cost);
	pc->unit->frozen = FROZEN::YES;
	pc->AddPerk((Perk)perk, -1);
	if(is_local)
	{
		game.fallback_type = FALLBACK::TRAIN;
		game.fallback_t = -1.f;
		game.fallback_1 = 3;
	}
	else
	{
		NetChangePlayer& c = Add1(pc->player_info->changes);
		c.type = NetChangePlayer::TRAIN;
		c.id = 3;
		c.count = 0;

		pc->player_info->update_flags |= PlayerInfo::UF_LEARNING_POINTS;
	}

	force_end = true;
	return true;
}
