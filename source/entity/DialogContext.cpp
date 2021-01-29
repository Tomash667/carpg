#include "Pch.h"
#include "DialogContext.h"

#include "Ability.h"
#include "AIController.h"
#include "Arena.h"
#include "Game.h"
#include "GameGui.h"
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
	assert(!dialog_mode);

	// use up auto talk
	if(talker && !dialog && talker->GetOrder() == ORDER_AUTO_TALK)
	{
		dialog = talker->order->auto_talk_dialog;
		quest = talker->order->auto_talk_quest;
		talker->OrderNext();
	}

	mode = NONE;
	dialog_mode = true;
	dialog_pos = 0;
	dialog_text = nullptr;
	once = true;
	dialog_quest = quest;
	dialog_esc = -1;
	this->talker = talker;
	this->dialog = dialog ? dialog : talker->data->dialog;
	if(Net::IsLocal())
	{
		// this vars are useless for clients, don't increase ref counter
		talker->busy = Unit::Busy_Talking;
		talker->look_target = pc->unit;
	}
	if(this->dialog && this->dialog->quest)
		dialog_quest = quest_mgr->FindAnyQuest(this->dialog->quest);
	update_news = true;
	update_locations = 1;
	pc->action = PlayerAction::Talk;
	pc->action_unit = talker;
	ClearChoices();
	force_end = false;
	if(!dialog)
	{
		quest_dialogs = talker->dialogs;
		std::sort(quest_dialogs.begin(), quest_dialogs.end(),
			[](const QuestDialog& dialog1, const QuestDialog& dialog2) { return dialog1.priority > dialog2.priority; });
		if(quest_dialogs.empty())
			quest_dialog_index = QUEST_INDEX_NONE;
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
		quest_dialog_index = QUEST_INDEX_NONE;
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
			dialog_s_text = predialog;
			dialog_text = dialog_s_text.c_str();
			timer = 1.f;
			predialog.clear();
		}
		else if(talker->bubble)
		{
			dialog_s_text = talker->bubble->text;
			dialog_text = dialog_s_text.c_str();
			timer = 1.f;
			skip_id = talker->bubble->skip_id;
		}
	}

	if(is_local)
		game_gui->CloseAllPanels();
}

//=================================================================================================
void DialogContext::StartNextDialog(GameDialog* new_dialog, Quest* quest, bool returns)
{
	assert(new_dialog);

	if(returns)
		prev.push_back({ dialog, dialog_quest, dialog_pos });
	dialog = new_dialog;
	dialog_quest = quest;
	dialog_pos = -1;
}

//=================================================================================================
cstring DialogContext::GetIdleText(Unit& talker)
{
	assert(talker.data->idleDialog);

	mode = IDLE;
	dialog_pos = 0;
	dialog_text = nullptr;
	once = true;
	dialog_quest = nullptr;
	dialog_esc = -1;
	this->talker = &talker;
	dialog = talker.data->idleDialog;
	update_news = true;
	update_locations = 1;
	ClearChoices();
	force_end = false;
	quest_dialogs.clear();
	quest_dialog_index = QUEST_INDEX_NONE;

	ScriptContext& ctx = script_mgr->GetContext();
	ctx.pc = nullptr;
	ctx.target = this->talker;
	current = this;
	UpdateLoop();
	current = nullptr;
	ctx.target = nullptr;

	return dialog_text;
}

//=================================================================================================
void DialogContext::Update(float dt)
{
	switch(mode)
	{
	case WAIT_CHOICES:
		// wy�wietlono opcje dialogowe, wybierz jedn� z nich (w mp czekaj na wyb�r)
		{
			bool ok = false;
			if(!is_local)
			{
				if(choice_selected != -1)
					ok = true;
			}
			else
				ok = game_gui->level_gui->UpdateChoice();

			if(ok)
			{
				mode = NONE;
				DialogChoice& choice = choices[choice_selected];
				cstring msg = choice.talk_msg ? choice.talk_msg : choice.msg;
				game_gui->level_gui->AddSpeechBubble(pc->unit, msg);

				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::TALK;
					c.unit = pc->unit;
					c.str = StringPool.Get();
					*c.str = msg;
					c.id = 0;
					c.count = 0;
					net->net_strs.push_back(c.str);
				}

				bool use_return = false;
				switch(choice.type)
				{
				case DialogChoice::Normal:
					if(choice.quest_dialog_index != QUEST_INDEX_NONE)
					{
						prev.push_back({ this->dialog, dialog_quest, -1 });
						quest_dialog_index = choice.quest_dialog_index;
						dialog_quest = quest_dialogs[quest_dialog_index].quest;
						dialog = quest_dialogs[quest_dialog_index].dialog;
					}
					dialog_pos = choice.pos;
					break;
				case DialogChoice::Perk:
					use_return = LearnPerk((Perk*)choice.pos);
					break;
				case DialogChoice::Hero:
					use_return = RecruitHero((Class*)choice.pos);
					break;
				}

				ClearChoices();
				choice_selected = -1;
				dialog_esc = -1;
				if(use_return)
					return;
			}
			else
				return;
		}
		break;

	case WAIT_TALK:
	case WAIT_TIMER:
		if(is_local)
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
					game->pc->data.wasted_key = GKey.KeyDoReturn(GK_ATTACK_USE, &Input::PressedRelease);
					if(game->pc->data.wasted_key != Key::None)
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
			if(choice_selected == 1)
			{
				timer = -1.f;
				choice_selected = -1;
			}
			else
				timer -= dt;
		}

		if(timer > 0.f)
			return;
		mode = NONE;
		break;
	}

	if(force_end)
	{
		EndDialog();
		return;
	}

	ScriptContext& ctx = script_mgr->GetContext();
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
		if(game_gui->level_gui->UpdateChoice())
		{
			mode = NONE;
			dialog_text = "";
			ClearChoices();

			NetChange& c = Add1(Net::changes);
			c.type = NetChange::CHOICE;
			c.id = choice_selected;
		}
		break;

	case WAIT_TALK:
		if(skip_id != -1 && (
			GKey.KeyPressedReleaseAllowed(GK_SKIP_DIALOG) || GKey.KeyPressedReleaseAllowed(GK_SELECT_DIALOG) || GKey.KeyPressedReleaseAllowed(GK_ATTACK_USE)
			|| (GKey.AllowKeyboard() && input->PressedRelease(Key::Escape))))
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::SKIP_DIALOG;
			c.id = skip_id;
			skip_id = -1;
		}
		break;
	}
}

//=================================================================================================
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
			else if(quest_dialog_index == QUEST_INDEX_NONE || quest_dialog_index + 1 == (int)quest_dialogs.size())
			{
				// finished all quest dialogs, use normal dialog
				Entry& p = prev.back();
				dialog = p.dialog;
				dialog_pos = p.pos;
				dialog_quest = p.quest;
				prev.pop_back();
				quest_dialog_index = QUEST_INDEX_NONE;
			}
			else
			{
				// handle remove quest dialog when was inside this dialog
				if(quest_dialog_index == QUEST_INDEX_RESTART)
					quest_dialog_index = QUEST_INDEX_NONE;

				// use next quest dialog
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
			mode = WAIT_CHOICES;
			if(is_local)
			{
				choice_selected = 0;
				LevelGui* gui = game_gui->level_gui;
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
			std::sort(quest_dialogs.begin(), quest_dialogs.end(),
				[](const QuestDialog& dialog1, const QuestDialog& dialog2) { return dialog1.priority > dialog2.priority; });
			if(!prev.empty())
			{
				Entry e = prev[0];
				prev.clear();
				if(quest_dialogs.empty())
				{
					dialog = e.dialog;
					dialog_quest = e.quest;
					quest_dialog_index = QUEST_INDEX_NONE;
				}
				else
				{
					prev.push_back({ e.dialog, e.quest, -1 });
					dialog = quest_dialogs[0].dialog;
					dialog_quest = (Quest*)quest_dialogs[0].quest;
					quest_dialog_index = 0;
				}
			}
			else
			{
				if(quest_dialogs.empty())
					quest_dialog_index = QUEST_INDEX_NONE;
				else
				{
					quest_dialog_index = 0;
					prev.push_back({ this->dialog, dialog_quest, -1 });
					dialog = quest_dialogs[0].dialog;
					dialog_quest = (Quest*)quest_dialogs[0].quest;
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
			pc->action = PlayerAction::Trade;
			pc->action_unit = talker;
			pc->chest_trade = &talker->stock->items;

			if(is_local)
				game_gui->inventory->StartTrade(I_TRADE, *pc->chest_trade, talker);
			else
			{
				NetChangePlayer& c = Add1(pc->player_info->changes);
				c.type = NetChangePlayer::START_TRADE;
				c.id = talker->id;
			}
			return;
		case DTF_TALK:
		case DTF_MULTI_TALK:
			{
				cstring msg = GetText(de.value, de.type == DTF_MULTI_TALK);
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
			if((mode == WAIT_TALK || mode == WAIT_TIMER) && timer > 0.f)
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
		case DTF_IF_RAND_P:
			cmp_result = (Rand() % 100 < de.value);
			if(de.op == OP_NOT_EQUAL)
				cmp_result = !cmp_result;
			break;
		case DTF_IF_ONCE:
			cmp_result = once;
			if(de.op == OP_NOT_EQUAL)
				cmp_result = !cmp_result;
			if(cmp_result)
				once = false;
			break;
		case DTF_DO_ONCE:
			once = false;
			break;
		case DTF_CHECK_QUEST_TIMEOUT:
			{
				Quest* quest = quest_mgr->FindQuest(game_level->location, (QuestCategory)de.value);
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
				int quest_id = -1;
				if(msg[0] == '$' && msg[1] == '$')
					quest_id = talker->quest_id;
				cmp_result = pc->unit->FindQuestItem(msg, nullptr, nullptr, false, quest_id);
				if(de.op == OP_NOT_EQUAL)
					cmp_result = !cmp_result;
			}
			break;
		case DTF_IF_HAVE_QUEST_ITEM_CURRENT:
			cmp_result = (dialog_quest && pc->unit->HaveQuestItem(dialog_quest->id));
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
				int quest_id = -1;
				if(msg[0] == '$' && msg[1] == '$')
					quest_id = talker->quest_id;
				Quest* quest;
				if(pc->unit->FindQuestItem(msg, &quest, nullptr, false, quest_id))
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
				for(Quest* quest : quest_mgr->quests)
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
				for(Quest* quest : quest_mgr->quests)
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
				for(Quest* quest : quest_mgr->quests)
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
					for(Quest* quest : quest_mgr->unaccepted_quests)
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
				ScriptContext& ctx = script_mgr->GetContext();
				ctx.pc = pc;
				ctx.target = talker;
				DialogScripts* scripts;
				void* instance;
				if(dialog_quest && dialog_quest->isNew)
				{
					Quest2* quest = (Quest2*)dialog_quest;
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
				script_mgr->RunScript(scripts->Get(DialogScripts::F_SCRIPT), instance, [index](asIScriptContext* ctx, int stage)
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
				ScriptContext& ctx = script_mgr->GetContext();
				DialogScripts* scripts;
				void* instance;
				ctx.pc = pc;
				ctx.target = talker;
				if(dialog_quest && dialog_quest->isNew)
				{
					Quest2* quest = (Quest2*)dialog_quest;
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
				script_mgr->RunScript(scripts->Get(DialogScripts::F_IF_SCRIPT), instance, [index, &cmp_result](asIScriptContext* ctx, int stage)
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
		case DTF_IF_VAR:
			cmp_result = DoIfOp(var, de.value, de.op);
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
	pc->action = PlayerAction::None;

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
cstring DialogContext::GetText(int index, bool multi)
{
	GameDialog::Text& text = multi ? dialog->GetMultiText(index) : dialog->GetText(index);
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
				ScriptContext& ctx = script_mgr->GetContext();
				DialogScripts* scripts;
				void* instance;
				ctx.pc = pc;
				ctx.target = talker;
				if(dialog_quest && dialog_quest->isNew)
				{
					Quest2* quest = (Quest2*)dialog_quest;
					scripts = &quest->GetScheme()->scripts;
					instance = quest->GetInstance();
					ctx.quest = quest;
				}
				else
				{
					scripts = &DialogScripts::global;
					instance = nullptr;
				}
				script_mgr->RunScript(scripts->Get(DialogScripts::F_FORMAT), instance, [&](asIScriptContext* ctx, int stage)
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
bool DialogContext::ExecuteSpecial(cstring msg)
{
	bool result;
	if(quest_mgr->HandleSpecial(*this, msg, result))
		return result;

	if(strcmp(msg, "mayor_quest") == 0)
	{
		bool have_quest = true;
		if(world->GetWorldtime() - game_level->city_ctx->quest_mayor_time > 30
			|| game_level->city_ctx->quest_mayor_time == -1
			|| game_level->city_ctx->quest_mayor == CityQuestState::Failed)
		{
			if(game_level->city_ctx->quest_mayor == CityQuestState::InProgress)
			{
				Quest* quest = quest_mgr->FindUnacceptedQuest(game_level->location, QuestCategory::Mayor);
				if(quest)
				DeleteElement(quest_mgr->unaccepted_quests, quest);
			}

			// jest nowe zadanie (mo�e), czas starego min��
			game_level->city_ctx->quest_mayor_time = world->GetWorldtime();
			game_level->city_ctx->quest_mayor = CityQuestState::InProgress;

			Quest* quest = quest_mgr->GetMayorQuest();
			if(quest)
				StartNextDialog(quest->GetDialog(QUEST_DIALOG_START), quest);
			else
				have_quest = false;
		}
		else if(game_level->city_ctx->quest_mayor == CityQuestState::InProgress)
		{
			// ju� ma przydzielone zadanie ?
			Quest* quest = quest_mgr->FindUnacceptedQuest(game_level->location, QuestCategory::Mayor);
			if(quest)
			{
				// quest nie zosta� zaakceptowany
				StartNextDialog(quest->GetDialog(QUEST_DIALOG_START), quest);
			}
			else
			{
				quest = quest_mgr->FindQuest(game_level->location, QuestCategory::Mayor);
				if(quest)
				{
					DialogTalk(RandomString(game->txQuestAlreadyGiven));
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
			DialogTalk(RandomString(game->txMayorNoQ));
			++dialog_pos;
			return true;
		}
	}
	else if(strcmp(msg, "captain_quest") == 0)
	{
		bool have_quest = true;
		if(world->GetWorldtime() - game_level->city_ctx->quest_captain_time > 30
			|| game_level->city_ctx->quest_captain_time == -1
			|| game_level->city_ctx->quest_captain == CityQuestState::Failed)
		{
			if(game_level->city_ctx->quest_captain == CityQuestState::InProgress)
			{
				Quest* quest = quest_mgr->FindUnacceptedQuest(game_level->location, QuestCategory::Captain);
				if(quest)
				DeleteElement(quest_mgr->unaccepted_quests, quest);
			}

			// jest nowe zadanie (mo�e), czas starego min��
			game_level->city_ctx->quest_captain_time = world->GetWorldtime();
			game_level->city_ctx->quest_captain = CityQuestState::InProgress;

			Quest* quest = quest_mgr->GetCaptainQuest();
			if(quest)
				StartNextDialog(quest->GetDialog(QUEST_DIALOG_START), quest);
			else
				have_quest = false;
		}
		else if(game_level->city_ctx->quest_captain == CityQuestState::InProgress)
		{
			// ju� ma przydzielone zadanie
			Quest* quest = quest_mgr->FindUnacceptedQuest(game_level->location, QuestCategory::Captain);
			if(quest)
			{
				// quest nie zosta� zaakceptowany
				StartNextDialog(quest->GetDialog(QUEST_DIALOG_START), quest);
			}
			else
			{
				quest = quest_mgr->FindQuest(game_level->location, QuestCategory::Captain);
				if(quest)
				{
					DialogTalk(RandomString(game->txQuestAlreadyGiven));
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
			DialogTalk(RandomString(game->txCaptainNoQ));
			++dialog_pos;
			return true;
		}
	}
	else if(strcmp(msg, "item_quest") == 0)
	{
		if(talker->quest_id == -1)
		{
			Quest* quest = quest_mgr->GetAdventurerQuest();
			talker->quest_id = quest->id;
			StartNextDialog(quest->GetDialog(QUEST_DIALOG_START), quest);
		}
		else
		{
			Quest* quest = quest_mgr->FindUnacceptedQuest(talker->quest_id);
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
			dialog_s_text = Format(game->txNeedMoreGold, cost - pc->unit->gold);
			DialogTalk(dialog_s_text.c_str());
			dialog_pos = 0;
			return true;
		}

		// give gold and freeze
		pc->unit->ModGold(-cost);
		pc->unit->frozen = FROZEN::YES;
		if(is_local)
		{
			game->fallback_type = FALLBACK::REST;
			game->fallback_t = -1.f;
			game->fallback_1 = days;
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
		if(!drunkman && (Rand() % 3 == 0 || (input->Down(Key::Shift) && game->devmode)))
		{
			int what = Rand() % 3;
			if(quest_mgr->HaveQuestRumors() && Rand() % 2 == 0)
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
						Location& cloc = *game_level->location;
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

						dialog_s_text = Format(RandomString(game->txLocationDiscovered), distance, GetLocationDirName(cloc.pos, loc.pos), loc.name.c_str());
						DialogTalk(dialog_s_text.c_str());
						++dialog_pos;

						return true;
					}
					else
					{
						DialogTalk(RandomString(game->txAllDiscovered));
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
						Location& cloc = *game_level->location;
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

						dialog_s_text = Format(RandomString(game->txCampDiscovered), distance, GetLocationDirName(cloc.pos, loc.pos), loc.group->name2.c_str());
						DialogTalk(dialog_s_text.c_str());
						++dialog_pos;
						return true;
					}
					else
					{
						DialogTalk(RandomString(game->txAllCampDiscovered));
						++dialog_pos;
						return true;
					}
				}
				break;
			case 2:
				// info about quest
				if(!quest_mgr->HaveQuestRumors())
				{
					DialogTalk(RandomString(game->txNoQRumors));
					++dialog_pos;
					return true;
				}
				else
				{
					dialog_s_text = quest_mgr->GetRandomQuestRumor();
					game_gui->journal->AddRumor(dialog_s_text.c_str());
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
		int type, what, cost;
		int* train;

		if(Ability* ability = Ability::Get(s))
		{
			// check required skill
			if(pc->unit->stats->skill[(int)SkillId::MYSTIC_MAGIC] < ability->skill)
			{
				DialogTalk(game->txCantLearnAbility);
				force_end = true;
				return true;
			}

			type = 4;
			what = ability->hash;
			train = nullptr;
			cost = ability->learning_points;
		}
		else if(Attribute* attrib = Attribute::Find(s))
		{
			type = 0;
			what = (int)attrib->attrib_id;
			train = &pc->attrib[what].train;
		}
		else if(Skill* skill = Skill::Find(s))
		{
			if(skill->locked && pc->unit->GetBase(skill->skill_id) <= 0)
			{
				DialogTalk(game->txCantLearnSkill);
				force_end = true;
				return true;
			}

			type = 1;
			what = (int)skill->skill_id;
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
		if(pc->learning_points < cost)
		{
			DialogTalk(game->txNeedLearningPoints);
			force_end = true;
			return true;
		}

		// does player have enough gold?
		if(pc->unit->gold < gold_cost)
		{
			dialog_s_text = Format(game->txNeedMoreGold, gold_cost - pc->unit->gold);
			DialogTalk(dialog_s_text.c_str());
			force_end = true;
			return true;
		}

		// give gold and freeze
		if(train)
			*train += 1;
		pc->learning_points -= cost;
		pc->unit->ModGold(-gold_cost);
		pc->unit->frozen = FROZEN::YES;
		if(is_local)
		{
			game->fallback_type = FALLBACK::TRAIN;
			game->fallback_t = -1.f;
			game->fallback_1 = type;
			game->fallback_2 = what;
		}
		else
		{
			NetChangePlayer& c = Add1(pc->player_info->changes);
			c.type = NetChangePlayer::TRAIN;
			c.id = type;
			c.count = what;

			pc->player_info->update_flags |= PlayerInfo::UF_LEARNING_POINTS;
		}
	}
	else if(strcmp(msg, "near_loc") == 0)
	{
		const vector<Location*>& locations = world->GetLocations();
		if(update_locations == 1)
		{
			active_locations.clear();
			const Vec2& world_pos = world->GetWorldPos();

			int index = 0;
			for(Location* loc : locations)
			{
				if(loc && loc->type != L_CITY && Vec2::Distance(loc->pos, world_pos) <= 150.f && loc->state != LS_HIDDEN)
					active_locations.push_back(pair<int, bool>(index, loc->state == LS_UNKNOWN));
				++index;
			}

			if(!active_locations.empty())
			{
				Shuffle(active_locations.begin(), active_locations.end());
				std::sort(active_locations.begin(), active_locations.end(),
					[](const pair<int, bool>& l1, const pair<int, bool>& l2) -> bool { return l1.second < l2.second; });
				update_locations = 0;
			}
			else
				update_locations = -1;
		}

		if(update_locations == -1)
		{
			DialogTalk(game->txNoNearLoc);
			++dialog_pos;
			return true;
		}
		else if(active_locations.empty())
		{
			DialogTalk(game->txAllNearLoc);
			++dialog_pos;
			return true;
		}

		int id = active_locations.back().first;
		active_locations.pop_back();
		Location& loc = *locations[id];
		loc.SetKnown();
		dialog_s_text = Format(game->txNearLoc, GetLocationDirName(world->GetWorldPos(), loc.pos), loc.name.c_str());
		if(loc.group->IsEmpty())
			dialog_s_text += RandomString(game->txNearLocEmpty);
		else if(loc.state == LS_CLEARED)
			dialog_s_text += Format(game->txNearLocCleared, loc.group->name3.c_str());
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
			dialog_s_text += Format(RandomString(game->txNearLocEnemy), desc, loc.group->name.c_str());
		}

		DialogTalk(dialog_s_text.c_str());
		++dialog_pos;
		return true;
	}
	else if(strcmp(msg, "hero_about") == 0)
	{
		Class* clas = talker->GetClass();
		if(clas)
			DialogTalk(clas->about.c_str());
		++dialog_pos;
		return true;
	}
	else if(strcmp(msg, "recruit") == 0)
	{
		int cost = talker->hero->JoinCost();
		pc->unit->ModGold(-cost);
		talker->gold += cost;
		team->AddMember(talker, HeroType::Normal);
		talker->temporary = false;
		if(team->free_recruits > 0)
			--team->free_recruits;
		talker->hero->SetupMelee();
		if(Net::IsOnline() && !is_local)
			pc->player_info->UpdateGold();
	}
	else if(strcmp(msg, "recruit_free") == 0)
	{
		team->AddMember(talker, HeroType::Normal);
		--team->free_recruits;
		talker->temporary = false;
		talker->hero->SetupMelee();
	}
	else if(strcmp(msg, "give_item") == 0)
	{
		Unit* t = talker;
		t->busy = Unit::Busy_Trading;
		EndDialog();
		pc->action = PlayerAction::GiveItems;
		pc->action_unit = t;
		pc->chest_trade = &t->items;
		if(is_local)
			game_gui->inventory->StartTrade(I_GIVE, *t);
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
		pc->action = PlayerAction::ShareItems;
		pc->action_unit = t;
		pc->chest_trade = &t->items;
		if(is_local)
			game_gui->inventory->StartTrade(I_SHARE, *t);
		else
		{
			NetChangePlayer& c = Add1(pc->player_info->changes);
			c.type = NetChangePlayer::START_SHARE;
		}
		return true;
	}
	else if(strcmp(msg, "kick_npc") == 0)
	{
		team->RemoveMember(talker);
		if(game_level->city_ctx)
			talker->OrderWander();
		else
			talker->OrderLeave();
		talker->hero->credit = 0;
		talker->ai->city_wander = true;
		talker->ai->loc_timer = Random(5.f, 10.f);
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
			Vec4 color = pc->unit->human_data->hair_color;
			do
			{
				pc->unit->human_data->hair_color = g_hair_colors[Rand() % n_hair_colors];
			}
			while(color.Equal(pc->unit->human_data->hair_color));
		}
		else
		{
			Vec4 color = pc->unit->human_data->hair_color;
			do
			{
				pc->unit->human_data->hair_color = Vec4(Random(0.f, 1.f), Random(0.f, 1.f), Random(0.f, 1.f), 1.f);
			}
			while(color.Equal(pc->unit->human_data->hair_color));
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
		talker->dont_attack = true;
	}
	else if(strcmp(msg, "captive_escape") == 0)
	{
		if(talker->hero->team_member)
			team->RemoveMember(talker);
		talker->OrderLeave();
		talker->dont_attack = false;
	}
	else if(strcmp(msg, "news") == 0)
	{
		if(update_news)
		{
			update_news = false;
			active_news = world->GetNews();
			if(active_news.empty())
			{
				DialogTalk(RandomString(game->txNoNews));
				++dialog_pos;
				return true;
			}
		}

		if(active_news.empty())
		{
			DialogTalk(RandomString(game->txAllNews));
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
		LocalVector<Perk*> to_pick;
		PerkContext ctx(pc, false);
		for(Perk* perk : Perk::perks)
		{
			if(IsSet(perk->flags, Perk::History))
				continue;
			if(pc->HavePerk(perk))
				continue;
			TakenPerk tp(perk);
			if(tp.CanTake(ctx))
				to_pick->push_back(perk);
		}
		std::sort(to_pick->begin(), to_pick->end(), [](const Perk* info1, const Perk* info2)
		{
			return info1->name < info2->name;
		});
		for(Perk* perk : to_pick)
		{
			string* str = StringPool.Get();
			*str = Format("%s (%s %d %s)", perk->name.c_str(), perk->desc.c_str(), perk->cost,
				perk->cost == 1 ? game->txLearningPoint : game->txLearningPoints);
			DialogChoice choice((int)perk, str->c_str(), quest_dialog_index, str);
			choice.type = DialogChoice::Perk;
			choice.talk_msg = perk->name.c_str();
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
			DialogChoice choice((int)clas, clas->name.c_str(), quest_dialog_index);
			choice.type = DialogChoice::Hero;
			choice.talk_msg = clas->name.c_str();
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
	if(quest_mgr->HandleSpecialIf(*this, msg, result))
		return result;

	if(strcmp(msg, "is_drunk") == 0)
		return IsSet(talker->data->flags, F_AI_DRUNKMAN) && talker->area->area_type == LevelArea::Type::Building;
	else if(strcmp(msg, "is_inside_dungeon") == 0)
		return game_level->local_area->area_type == LevelArea::Type::Inside;
	else if(strcmp(msg, "is_team_full") == 0)
		return team->GetActiveTeamSize() >= team->GetMaxSize();
	else if(strcmp(msg, "can_join") == 0)
		return pc->unit->gold >= talker->hero->JoinCost();
	else if(strcmp(msg, "is_near_arena") == 0)
		return game_level->city_ctx && IsSet(game_level->city_ctx->flags, City::HaveArena) && Vec3::Distance2d(talker->pos, game_level->city_ctx->arena_pos) < 5.f;
	else if(strcmp(msg, "is_ginger") == 0)
		return pc->unit->human_data->hair_color.Equal(g_hair_colors[8]);
	else if(strcmp(msg, "is_bald") == 0)
		return pc->unit->human_data->hair == -1;
	else if(strcmp(msg, "dont_have_quest") == 0)
		return talker->quest_id == -1;
	else if(strcmp(msg, "have_unaccepted_quest") == 0)
		return quest_mgr->FindUnacceptedQuest(talker->quest_id);
	else if(strcmp(msg, "have_completed_quest") == 0)
	{
		Quest* quest = quest_mgr->FindQuest(talker->quest_id, false);
		return quest && !quest->IsActive();
	}
	else if(strcmp(msg, "is_free_recruit") == 0)
		return talker->level <= 8 && team->free_recruits > 0 && !talker->hero->otherTeam;
	else if(strcmp(msg, "have_unique_quest") == 0)
	{
		return (((quest_mgr->quest_orcs2->orcs_state == Quest_Orcs2::State::Accepted || quest_mgr->quest_orcs2->orcs_state == Quest_Orcs2::State::OrcJoined)
			&& quest_mgr->quest_orcs->startLoc == game_level->location)
			|| (quest_mgr->quest_mages2->mages_state >= Quest_Mages2::State::TalkedWithCaptain
			&& quest_mgr->quest_mages2->mages_state < Quest_Mages2::State::Completed
			&& quest_mgr->quest_mages2->startLoc == game_level->location));
	}
	else if(strcmp(msg, "is_not_mage") == 0)
	{
		Class* clas = talker->GetClass();
		return !clas || clas->id != "mage";
	}
	else if(strcmp(msg, "prefer_melee") == 0)
		return talker->hero->melee;
	else if(strcmp(msg, "is_inside_inn") == 0)
		return talker->area->area_type == LevelArea::Type::Building && game_level->city_ctx->FindInn() == talker->area;
	else if(strcmp(msg, "is_before_contest") == 0)
		return quest_mgr->quest_contest->state >= Quest_Contest::CONTEST_TODAY;
	else if(strcmp(msg, "is_drunkmage") == 0)
		return IsSet(talker->data->flags3, F3_DRUNK_MAGE) && quest_mgr->quest_mages2->mages_state < Quest_Mages2::State::MageCured;
	else if(strcmp(msg, "is_guard") == 0)
		return IsSet(talker->data->flags2, F2_GUARD);
	else if(strcmp(msg, "mayor_quest_failed") == 0)
		return game_level->city_ctx->quest_mayor == CityQuestState::Failed;
	else if(strcmp(msg, "captain_quest_failed") == 0)
		return game_level->city_ctx->quest_captain == CityQuestState::Failed;
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
	if(quest_mgr->HandleFormatString(str_part, result))
		return result;

	if(str_part == "rcitynhere")
		return world->GetRandomSettlement(game_level->location)->name.c_str();
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
		bool crazy = Rand() % 4 == 0;
		Class* clas = crazy ? Class::GetRandomCrazy() : Class::GetRandomHero();
		NameHelper::GenerateHeroName(clas, crazy, str);
		return str.c_str();
	}
	else if(strncmp(str_part.c_str(), "player/", 7) == 0)
	{
		int id = int(str_part[7] - '1');
		return game->arena->near_players_str[id].c_str();
	}
	else if(strncmp(str_part.c_str(), "train_lab/", 10) == 0)
	{
		cstring s = str_part.c_str() + 10;
		if(Ability* ability = Ability::Get(s))
		{
			talk_msg = ability->name.c_str();
			return Format("%s: %s (%d %s, %d %s)", game->txSpell, ability->name.c_str(), ability->skill, Skill::Find("mystic_magic")->name.c_str(),
				ability->learning_points, ability->learning_points == 1 ? game->txLearningPoint : game->txLearningPoints);
		}
		else
		{
			cstring name;
			int cost;
			if(Attribute* attrib = Attribute::Find(s))
			{
				name = attrib->name.c_str();
				cost = pc->GetTrainCost(pc->attrib[(int)attrib->attrib_id].train);
			}
			else if(Skill* skill = Skill::Find(s))
			{
				name = skill->name.c_str();
				cost = pc->GetTrainCost(pc->skill[(int)skill->skill_id].train);
			}
			else
			{
				assert(0);
				return "INVALID_TRAIN_STAT";
			}

			talk_msg = name;
			return Format("%s (%d %s)", name, cost, cost == 1 ? game->txLearningPoint : game->txLearningPoints);
		}
	}
	else if(str_part == "date")
		return world->GetDate();
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

	dialog_text = msg;
	if(mode == IDLE)
		return;

	mode = WAIT_TALK;
	timer = 1.f + float(strlen(dialog_text)) / 20;

	int ani;
	if(!talker->usable && talker->data->type == UNIT_TYPE::HUMAN && talker->action == A_NONE && Rand() % 3 != 0)
	{
		ani = Rand() % 2 + 1;
		talker->mesh_inst->Play(ani == 1 ? "i_co" : "pokazuje", PLAY_ONCE | PLAY_PRIO2, 0);
		talker->animation = ANI_PLAY;
		talker->action = A_ANIMATION;
	}
	else
		ani = 0;

	game_gui->level_gui->AddSpeechBubble(talker, dialog_text);

	pc->Train(TrainWhat::Talk, 0.f, 0);

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::TALK;
		c.unit = talker;
		c.str = StringPool.Get();
		*c.str = msg;
		c.id = ani;
		c.count = game->skip_id_counter++;
		skip_id = c.count;
		net->net_strs.push_back(c.str);
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
	if(pc->learning_points < perk->cost)
	{
		DialogTalk(game->txNeedLearningPoints);
		force_end = true;
		return false;
	}

	// does player have enough gold?
	if(pc->unit->gold < cost)
	{
		dialog_s_text = Format(game->txNeedMoreGold, cost - pc->unit->gold);
		DialogTalk(dialog_s_text.c_str());
		force_end = true;
		return false;
	}

	// give gold and freeze
	pc->learning_points -= perk->cost;
	pc->unit->ModGold(-cost);
	pc->unit->frozen = FROZEN::YES;
	if(is_local)
	{
		game->fallback_type = FALLBACK::TRAIN;
		game->fallback_t = -1.f;
		game->fallback_1 = 3;
		game->fallback_2 = perk->hash;
	}
	else
	{
		NetChangePlayer& c = Add1(pc->player_info->changes);
		c.type = NetChangePlayer::TRAIN;
		c.id = 3;
		c.count = perk->hash;

		pc->player_info->update_flags |= PlayerInfo::UF_LEARNING_POINTS;
	}

	force_end = true;
	return true;
}

//=================================================================================================
bool DialogContext::RecruitHero(Class* clas)
{
	if(team->GetActiveTeamSize() >= 4u)
	{
		DialogTalk(game->txTeamTooBig);
		force_end = true;
		return false;
	}

	Unit* u = game_level->SpawnUnitNearLocation(*talker->area, Vec3(131.f, 0, 121.f), *clas->hero, nullptr);
	u->rot = 0.f;
	u->SetKnownName(true);
	u->hero->loner = false;
	team->AddMember(u, HeroType::Normal);
	dialog_s_text = Format(game->txHeroJoined, u->GetName());
	DialogTalk(dialog_s_text.c_str());
	force_end = true;
	return true;
}

//=================================================================================================
void DialogContext::RemoveQuestDialog(Quest2* quest)
{
	assert(quest);

	if(quest_dialog_index == QUEST_INDEX_NONE)
	{
		RemoveElements(quest_dialogs, [=](QuestDialog& dialog) { return dialog.quest == quest; });
		return;
	}

	// find next quest dialog
	for(QuestDialog& dialog : quest_dialogs)
	{
		if(dialog.quest == quest)
			dialog.dialog = nullptr;
	}
	QuestDialog next = {};
	for(int i = quest_dialog_index + 1; i < (int)quest_dialogs.size(); ++i)
	{
		if(quest_dialogs[i].dialog)
		{
			next = quest_dialogs[i];
			break;
		}
	}
	RemoveElements(quest_dialogs, [](QuestDialog& dialog) { return !dialog.dialog; });
	if(next.dialog)
	{
		for(int i = 0; i < (int)quest_dialogs.size(); ++i)
		{
			if(quest_dialogs[i].dialog == next.dialog && quest_dialogs[i].quest == next.quest)
			{
				quest_dialog_index = i - 1;
				if(quest_dialog_index == QUEST_INDEX_NONE)
					quest_dialog_index = QUEST_INDEX_RESTART;
				break;
			}
		}
	}
}
