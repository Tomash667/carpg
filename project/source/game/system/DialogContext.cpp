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
#include "QuestManager.h"
#include "Level.h"
#include "World.h"
#include "LocationHelper.h"
#include "Arena.h"
#include "NameHelper.h"

DialogContext* DialogContext::current;

//=================================================================================================
void DialogContext::StartNextDialog(GameDialog* dialog, int& if_level, Quest* quest)
{
	assert(dialog);

	prev.push_back({ dialog, dialog_quest, dialog_pos, dialog_level });
	dialog = dialog;
	dialog_quest = quest;
	dialog_pos = -1;
	dialog_level = 0;
	if_level = 0;
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
			cstring msg = choices[choice_selected].msg;
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
				game.net_talk.push_back(c.str);
			}

			show_choices = false;
			dialog_pos = choices[choice_selected].pos;
			dialog_level = choices[choice_selected].lvl;
			choices.clear();
			choice_selected = -1;
			dialog_esc = -1;
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

	int if_level = dialog_level;

	while(true)
	{
		DialogEntry& de = *(dialog->code.data() + dialog_pos);

		switch(de.type)
		{
		case DTF_CHOICE:
			if(if_level == dialog_level)
			{
				cstring text = GetText(de.value);
				choices.push_back(DialogChoice(dialog_pos + 1, text, dialog_level + 1));
			}
			++if_level;
			break;
		case DTF_END_CHOICE:
		case DTF_END_IF:
			if(if_level == dialog_level)
				--dialog_level;
			--if_level;
			break;
		case DTF_END:
			if(if_level == dialog_level)
			{
				if(prev.empty())
				{
					EndDialog();
					return;
				}
				else
				{
					Entry& p = prev.back();
					dialog = p.dialog;
					dialog_pos = p.pos;
					dialog_level = p.level;
					dialog_quest = p.quest;
					prev.pop_back();
					if_level = dialog_level;
				}
			}
			break;
		case DTF_END2:
			if(if_level == dialog_level)
			{
				EndDialog();
				return;
			}
			break;
		case DTF_SHOW_CHOICES:
			if(if_level == dialog_level)
			{
				show_choices = true;
				if(is_local)
				{
					choice_selected = 0;
					game.gui->game_gui->dialog_cursor_pos = Int2(-1, -1);
					game.gui->game_gui->UpdateScrollbar(choices.size());
				}
				else
				{
					choice_selected = -1;
					NetChangePlayer& c = Add1(pc->player_info->changes);
					c.type = NetChangePlayer::SHOW_DIALOG_CHOICES;
				}
				return;
			}
			break;
		case DTF_RESTART:
			if(if_level == dialog_level)
				dialog_pos = -1;
			break;
		case DTF_TRADE:
			if(if_level == dialog_level)
			{
				if(!talker->data->trader)
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
					game.gui->inventory->StartTrade(I_TRADE, *pc->chest_trade, talker);
				else
				{
					NetChangePlayer& c = Add1(pc->player_info->changes);
					c.type = NetChangePlayer::START_TRADE;
					c.id = talker->netid;
				}

				pc->Train(TrainWhat::Trade, 0.f, 0);

				return;
			}
			break;
		case DTF_TALK:
			if(dialog_level == if_level)
			{
				cstring msg = GetText(de.value);
				game.DialogTalk(*this, msg);
				++dialog_pos;
				return;
			}
			break;
		case DTF_SPECIAL:
			if(dialog_level == if_level)
			{
				cstring msg = dialog->strs[de.value].c_str();
				if(game.ExecuteGameDialogSpecial(*this, msg, if_level))
					return;
			}
			break;
		case DTF_SET_QUEST_PROGRESS:
			if(if_level == dialog_level)
			{
				assert(dialog_quest);
				dialog_quest->SetProgress(de.value);
				if(dialog_wait > 0.f)
				{
					++dialog_pos;
					return;
				}
			}
			break;
		case DTF_IF_QUEST_TIMEOUT:
			if(if_level == dialog_level)
			{
				assert(dialog_quest);
				bool ok = (dialog_quest->IsActive() && dialog_quest->IsTimedout());
				if(negate_if)
				{
					negate_if = false;
					ok = !ok;
				}
				if(ok)
					++dialog_level;
			}
			++if_level;
			break;
		case DTF_IF_RAND:
			if(if_level == dialog_level)
			{
				bool ok = (Rand() % de.value == 0);
				if(negate_if)
				{
					negate_if = false;
					ok = !ok;
				}
				if(ok)
					++dialog_level;
			}
			++if_level;
			break;
		case DTF_IF_ONCE:
			if(if_level == dialog_level)
			{
				bool ok = dialog_once;
				if(negate_if)
				{
					negate_if = false;
					ok = !ok;
				}
				if(ok)
				{
					dialog_once = false;
					++dialog_level;
				}
			}
			++if_level;
			break;
		case DTF_DO_ONCE:
			if(if_level == dialog_level)
				dialog_once = false;
			break;
		case DTF_ELSE:
			if(if_level == dialog_level)
				--dialog_level;
			else if(if_level == dialog_level + 1)
				++dialog_level;
			break;
		case DTF_CHECK_QUEST_TIMEOUT:
			if(if_level == dialog_level)
			{
				Quest* quest = QM.FindQuest(L.location_index, (QuestType)de.value);
				if(quest && quest->IsActive() && quest->IsTimedout())
				{
					dialog_once = false;
					StartNextDialog(quest->GetDialog(QUEST_DIALOG_FAIL), if_level, quest);
				}
			}
			break;
		case DTF_IF_HAVE_QUEST_ITEM:
			if(if_level == dialog_level)
			{
				cstring msg = dialog->strs[de.value].c_str();
				bool ok = pc->unit->FindQuestItem(msg, nullptr, nullptr, not_active);
				if(negate_if)
				{
					negate_if = false;
					ok = !ok;
				}
				if(ok)
					++dialog_level;
				not_active = false;
			}
			++if_level;
			break;
		case DTF_DO_QUEST_ITEM:
			if(if_level == dialog_level)
			{
				cstring msg = dialog->strs[de.value].c_str();

				Quest* quest;
				if(pc->unit->FindQuestItem(msg, &quest, nullptr))
					StartNextDialog(quest->GetDialog(QUEST_DIALOG_NEXT), if_level, quest);
			}
			break;
		case DTF_IF_QUEST_PROGRESS:
			if(if_level == dialog_level)
			{
				assert(dialog_quest);
				bool ok = (dialog_quest->prog == de.value);
				if(negate_if)
				{
					negate_if = false;
					ok = !ok;
				}
				if(ok)
					++dialog_level;
			}
			++if_level;
			break;
		case DTF_IF_QUEST_PROGRESS_RANGE:
			if(if_level == dialog_level)
			{
				assert(dialog_quest);
				int x = de.value & 0xFFFF;
				int y = (de.value & 0xFFFF0000) >> 16;
				assert(y > x);
				bool ok = InRange(dialog_quest->prog, x, y);
				if(negate_if)
				{
					negate_if = false;
					ok = !ok;
				}
				if(ok)
					++dialog_level;
			}
			++if_level;
			break;
		case DTF_IF_NEED_TALK:
			if(if_level == dialog_level)
			{
				cstring msg = dialog->strs[de.value].c_str();
				bool negate = false;
				if(negate_if)
				{
					negate_if = false;
					negate = true;
				}

				for(Quest* quest : QM.quests)
				{
					bool ok = (quest->IsActive() && quest->IfNeedTalk(msg));
					if(negate)
						ok = !ok;
					if(ok)
					{
						++dialog_level;
						break;
					}
				}
			}
			++if_level;
			break;
		case DTF_DO_QUEST:
			if(if_level == dialog_level)
			{
				cstring msg = dialog->strs[de.value].c_str();

				for(Quest* quest : QM.quests)
				{
					if(quest->IsActive() && quest->IfNeedTalk(msg))
					{
						StartNextDialog(quest->GetDialog(QUEST_DIALOG_NEXT), if_level, quest);
						break;
					}
				}
			}
			break;
		case DTF_ESCAPE_CHOICE:
			if(if_level == dialog_level)
				dialog_esc = (int)choices.size() - 1;
			break;
		case DTF_IF_SPECIAL:
			if(if_level == dialog_level)
			{
				cstring msg = dialog->strs[de.value].c_str();
				bool ok = game.ExecuteGameDialogSpecialIf(*this, msg);
				if(negate_if)
				{
					negate_if = false;
					ok = !ok;
				}
				if(ok)
					++dialog_level;
			}
			++if_level;
			break;
		case DTF_IF_CHOICES:
			if(if_level == dialog_level)
			{
				bool ok = (choices.size() == de.value);
				if(negate_if)
				{
					negate_if = false;
					ok = !ok;
				}
				if(ok)
					++dialog_level;
			}
			++if_level;
			break;
		case DTF_DO_QUEST2:
			if(if_level == dialog_level)
			{
				cstring msg = dialog->strs[de.value].c_str();

				for(Quest* quest : QM.quests)
				{
					if(quest->IfNeedTalk(msg))
					{
						StartNextDialog(quest->GetDialog(QUEST_DIALOG_NEXT), if_level, quest);
						break;
					}
				}

				if(dialog_pos != -1)
				{
					for(Quest* quest : QM.unaccepted_quests)
					{
						if(quest->IfNeedTalk(msg))
						{
							StartNextDialog(quest->GetDialog(QUEST_DIALOG_NEXT), if_level, quest);
							break;
						}
					}
				}
			}
			break;
		case DTF_IF_HAVE_ITEM:
			if(if_level == dialog_level)
			{
				const Item* item = (const Item*)de.value;
				bool ok = pc->unit->HaveItem(item);
				if(negate_if)
				{
					negate_if = false;
					ok = !ok;
				}
				if(ok)
					++dialog_level;
			}
			++if_level;
			break;
		case DTF_IF_QUEST_EVENT:
			if(if_level == dialog_level)
			{
				assert(dialog_quest);
				bool ok = dialog_quest->IfQuestEvent();
				if(negate_if)
				{
					negate_if = false;
					ok = !ok;
				}
				if(ok)
					++dialog_level;
			}
			++if_level;
			break;
		case DTF_END_OF_DIALOG:
			assert(0);
			throw Format("Broken dialog '%s'.", dialog->id.c_str());
		case DTF_NOT_ACTIVE:
			not_active = true;
			break;
		case DTF_IF_QUEST_SPECIAL:
			if(if_level == dialog_level)
			{
				assert(dialog_quest);
				cstring msg = dialog->strs[de.value].c_str();
				bool ok = dialog_quest->SpecialIf(*this, msg);
				if(negate_if)
				{
					negate_if = false;
					ok = !ok;
				}
				if(ok)
					++dialog_level;
			}
			++if_level;
			break;
		case DTF_QUEST_SPECIAL:
			if(if_level == dialog_level)
			{
				assert(dialog_quest);
				cstring msg = dialog->strs[de.value].c_str();
				dialog_quest->Special(*this, msg);
			}
			break;
		case DTF_NOT:
			if(if_level == dialog_level)
				negate_if = true;
			break;
		case DTF_SCRIPT:
			if(if_level == dialog_level)
			{
				cstring msg = dialog->strs[de.value].c_str();
				SM.SetContext(pc, talker);
				SM.RunScript(msg);
			}
			break;
		case DTF_IF_SCRIPT:
			if(if_level == dialog_level)
			{
				cstring msg = dialog->strs[de.value].c_str();
				SM.SetContext(pc, talker);
				bool ok = SM.RunIfScript(msg);
				if(negate_if)
				{
					negate_if = false;
					ok = !ok;
				}
				if(ok)
					++dialog_level;
			}
			++if_level;
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
	choices.clear();
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
cstring DialogContext::GetText(int index)
{
	GameDialog::Text& text = GetTextInner(index);
	cstring str = dialog->strs[text.index].c_str();

	if(!text.formatted)
		return str;

	static string str_part;
	dialog_s_text.clear();

	for(uint i = 0, len = strlen(str); i < len; ++i)
	{
		if(str[i] == '$')
		{
			str_part.clear();
			++i;
			if(str[i] == '(')
			{
				++i;
				while(str[i] != ')')
				{
					str_part.push_back(str[i]);
					++i;
				}
				SM.RunStringScript(str_part.c_str(), str_part);
				dialog_s_text += str_part;
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
cstring DialogContext::FormatString(const string& str_part)
{
	cstring result;
	if(QM.HandleFormatString(str_part, result))
		return result;

	if(str_part == "burmistrzem")
		return LocationHelper::IsCity(L.location) ? "burmistrzem" : "so³tysem";
	else if(str_part == "mayor")
		return LocationHelper::IsCity(L.location) ? "mayor" : "soltys";
	else if(str_part == "rcitynhere")
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
	else
	{
		assert(0);
		return "";
	}
}
