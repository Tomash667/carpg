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
#include "Quest_Sawmill.h"
#include "Quest_Mine.h"
#include "Quest_Bandits.h"
#include "Quest_Mages.h"
#include "Quest_Orcs.h"
#include "Quest_Goblins.h"
#include "Quest_Evil.h"

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
				DialogTalk(msg);
				++dialog_pos;
				return;
			}
			break;
		case DTF_SPECIAL:
			if(dialog_level == if_level)
			{
				cstring msg = dialog->strs[de.value].c_str();
				if(ExecuteSpecial(msg, if_level))
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
				bool ok = ExecuteSpecialIf(msg);
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
bool DialogContext::ExecuteSpecial(cstring msg, int& if_level)
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
				StartNextDialog(quest->GetDialog(QUEST_DIALOG_START), if_level, quest);
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
				StartNextDialog(quest->GetDialog(QUEST_DIALOG_START), if_level, quest);
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
				StartNextDialog(quest->GetDialog(QUEST_DIALOG_START), if_level, quest);
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
				StartNextDialog(quest->GetDialog(QUEST_DIALOG_START), if_level, quest);
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
			StartNextDialog(quest->GetDialog(QUEST_DIALOG_START), if_level, quest);
		}
		else
		{
			Quest* quest = QM.FindUnacceptedQuest(talker->quest_refid);
			StartNextDialog(quest->GetDialog(QUEST_DIALOG_START), if_level, quest);
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
			dialog_level = 0;
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
					} while(id != id2);
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
			} while(last_rumor == rumor);
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
	else if(strncmp(msg, "train_", 6) == 0)
	{
		const int cost = 200;
		cstring s = msg + 6;
		bool is_skill;
		int what;

		auto attrib = Attribute::Find(s);
		if(attrib)
		{
			is_skill = false;
			what = (int)attrib->attrib_id;
		}
		else
		{
			auto skill = Skill::Find(s);
			if(skill)
			{
				is_skill = true;
				what = (int)skill->skill_id;
			}
			else
			{
				assert(0);
				is_skill = false;
				what = (int)AttributeId::STR;
			}
		}

		// does player have enough gold?
		if(pc->unit->gold < cost)
		{
			dialog_s_text = Format(game.txNeedMoreGold, cost - pc->unit->gold);
			DialogTalk(dialog_s_text.c_str());
			force_end = true;
			return true;
		}

		// give gold and freeze
		pc->unit->ModGold(-cost);
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
			dialog_s_text += Format(txNearLocCleared, g_spawn_groups[loc.spawn].name);
		else
		{
			SpawnGroup& sg = g_spawn_groups[loc.spawn];
			cstring jacy;
			if(loc.st < 5)
			{
				if(sg.k == K_I)
					jacy = txELvlVeryWeak[0];
				else
					jacy = txELvlVeryWeak[1];
			}
			else if(loc.st < 8)
			{
				if(sg.k == K_I)
					jacy = txELvlWeak[0];
				else
					jacy = txELvlWeak[1];
			}
			else if(loc.st < 11)
			{
				if(sg.k == K_I)
					jacy = txELvlAverage[0];
				else
					jacy = txELvlAverage[1];
			}
			else if(loc.st < 14)
			{
				if(sg.k == K_I)
					jacy = txELvlQuiteStrong[0];
				else
					jacy = txELvlQuiteStrong[1];
			}
			else
			{
				if(sg.k == K_I)
					jacy = txELvlStrong[0];
				else
					jacy = txELvlStrong[1];
			}
			dialog_s_text += Format(RandomString(txNearLocEnemy), jacy, g_spawn_groups[loc.spawn].name);
		}

		DialogTalk(ctx, dialog_s_text.c_str());
		++dialog_pos;
		return true;
	}
	else if(strcmp(msg, "tell_name") == 0)
		talker->RevealName(false);
	else if(strcmp(msg, "hero_about") == 0)
	{
		Class clas = talker->GetClass();
		if(clas < Class::MAX)
			DialogTalk(ctx, ClassInfo::classes[(int)clas].about.c_str());
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
		Team.free_recruit = false;
		talker->hero->SetupMelee();
		if(Net::IsOnline() && !is_local)
			pc->player_info->UpdateGold();
	}
	else if(strcmp(msg, "recruit_free") == 0)
	{
		Team.AddTeamMember(talker, false);
		Team.free_recruit = false;
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
			gui->inventory->StartTrade(I_GIVE, *t);
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
			gui->inventory->StartTrade(I_SHARE, *t);
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
		Team.TeamShareGiveItemCredit(ctx);
	else if(strcmp(msg, "sell_item") == 0)
		Team.TeamShareSellItem(ctx);
	else if(strcmp(msg, "share_decline") == 0)
		Team.TeamShareDecline(ctx);
	else if(strcmp(msg, "attack") == 0)
	{
		// ta komenda jest zbyt ogólna, jeœli bêdzie kilka takich grup to wystarczy ¿e jedna tego u¿yje to wszyscy zaatakuj¹, nie obs³uguje te¿ budynków
		if(talker->data->group == G_CRAZIES)
		{
			if(!Team.crazies_attack)
			{
				Team.crazies_attack = true;
				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::CHANGE_FLAGS;
				}
			}
		}
		else
		{
			for(vector<Unit*>::iterator it = L.local_ctx.units->begin(), end = L.local_ctx.units->end(); it != end; ++it)
			{
				if((*it)->dont_attack && (*it)->IsEnemy(*Team.leader, true))
				{
					(*it)->dont_attack = false;
					(*it)->ai->change_ai_mode = true;
				}
			}
		}
	}
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
			} while(kolor.Equal(pc->unit->human_data->hair_color));
		}
		else
		{
			Vec4 kolor = pc->unit->human_data->hair_color;
			do
			{
				pc->unit->human_data->hair_color = Vec4(Random(0.f, 1.f), Random(0.f, 1.f), Random(0.f, 1.f), 1.f);
			} while(kolor.Equal(pc->unit->human_data->hair_color));
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
				DialogTalk(ctx, RandomString(txNoNews));
				++dialog_pos;
				return true;
			}
		}

		if(active_news.empty())
		{
			DialogTalk(ctx, RandomString(txAllNews));
			++dialog_pos;
			return true;
		}

		int id = Rand() % active_news.size();
		News* news = active_news[id];
		active_news.erase(active_news.begin() + id);

		DialogTalk(ctx, news->text.c_str());
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
	if(QM.HandleSpecialIf(ctx, msg, result))
		return result;

	if(strcmp(msg, "have_team") == 0)
		return Team.HaveTeamMember();
	else if(strcmp(msg, "have_pc_team") == 0)
		return Team.HaveOtherPlayer();
	else if(strcmp(msg, "have_npc_team") == 0)
		return Team.HaveActiveNpc();
	else if(strcmp(msg, "is_drunk") == 0)
		return IS_SET(talker->data->flags, F_AI_DRUNKMAN) && talker->in_building != -1;
	else if(strcmp(msg, "is_team_member") == 0)
		return Team.IsTeamMember(*talker);
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
	else if(strcmp(msg, "is_safe_location") == 0)
	{
		if(L.city_ctx)
			return true;
		else if(L.location->outside)
		{
			if(L.location->state == LS_CLEARED)
				return true;
		}
		else
		{
			InsideLocation* inside = (InsideLocation*)L.location;
			if(inside->IsMultilevel())
			{
				MultiInsideLocation* multi = (MultiInsideLocation*)inside;
				if(multi->IsLevelClear())
				{
					if(L.dungeon_level == 0)
					{
						if(!multi->from_portal)
							return true;
					}
					else
						return true;
				}
			}
			else if(L.location->state == LS_CLEARED)
				return true;
		}
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
		return target_loc_is_camp;
	else if(strcmp(msg, "dont_have_quest") == 0)
		return talker->quest_refid == -1;
	else if(strcmp(msg, "have_unaccepted_quest") == 0)
		return QM.FindUnacceptedQuest(talker->quest_refid);
	else if(strcmp(msg, "have_completed_quest") == 0)
	{
		Quest* q = QM.FindQuest(talker->quest_refid, false);
		if(q && !q->IsActive())
			return true;
	}
	else if(strcmp(msg, "is_free_recruit") == 0)
		return talker->level < 6 && Team.free_recruit;
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
	else if(strcmp(msg, "is_leader") == 0)
		return pc->unit == Team.leader;
	else if(strcmp(msg, "in_city") == 0)
		return L.city_ctx != nullptr;
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
	else
	{
		assert(0);
		return "";
	}
}

//=================================================================================================
void DialogContext::DialogTalk(cstring msg)
{
	assert(msg);

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

	gui->game_gui->AddSpeechBubble(talker, dialog_text);

	pc->Train(TrainWhat::Talk, 0.f, 0);

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::TALK;
		c.unit = talker;
		c.str = StringPool.Get();
		*c.str = msg;
		c.id = ani;
		c.count = skip_id_counter++;
		skip_id = c.count;
		net_talk.push_back(c.str);
	}
}
