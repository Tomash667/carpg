#include "Pch.h"
#include "Base.h"
#include "Dialog.h"
#include "Game.h"

extern string g_system_dir;
extern DialogEntry quest_bandits_master[];
extern DialogEntry quest_bandits_encounter[];
extern DialogEntry quest_bandits_captain[];
extern DialogEntry quest_bandits_guard[];
extern DialogEntry quest_bandits_agent[];
extern DialogEntry quest_bandits_boss[];
extern DialogEntry bandits_collect_toll_start[];
extern DialogEntry bandits_collect_toll_timeout[];
extern DialogEntry bandits_collect_toll_end[];
extern DialogEntry bandits_collect_toll_talk[];
extern DialogEntry camp_near_city_start[];
extern DialogEntry camp_near_city_timeout[];
extern DialogEntry camp_near_city_end[];
extern DialogEntry crazies_trainer[];
extern DialogEntry deliver_letter_start[];
extern DialogEntry deliver_letter_timeout[];
extern DialogEntry deliver_letter_give[];
extern DialogEntry deliver_letter_end[];
extern DialogEntry deliver_parcel_start[];
extern DialogEntry deliver_parcel_give[];
extern DialogEntry deliver_parcel_timeout[];
extern DialogEntry deliver_parcel_bandits[];
extern DialogEntry evil_cleric[];
extern DialogEntry evil_mage[];
extern DialogEntry evil_captain[];
extern DialogEntry evil_mayor[];
extern DialogEntry evil_boss[];
extern DialogEntry find_artifact_start[];
extern DialogEntry find_artifact_end[];
extern DialogEntry find_artifact_timeout[];
extern DialogEntry goblins_nobleman[];
extern DialogEntry goblins_encounter[];
extern DialogEntry goblins_messenger[];
extern DialogEntry goblins_mage[];
extern DialogEntry goblins_innkeeper[];
extern DialogEntry goblins_boss[];
extern DialogEntry kill_animals_start[];
extern DialogEntry kill_animals_timeout[];
extern DialogEntry kill_animals_end[];
extern DialogEntry lost_artifact_start[];
extern DialogEntry lost_artifact_end[];
extern DialogEntry lost_artifact_timeout[];
extern DialogEntry mages_scholar[];
extern DialogEntry mages_golem[];
extern DialogEntry mages2_captain[];
extern DialogEntry mages2_mage[];
extern DialogEntry mages2_boss[];
extern DialogEntry dialog_main_event[];
extern DialogEntry dialog_main[];
extern DialogEntry mine_investor[];
extern DialogEntry mine_messenger[];
extern DialogEntry mine_messenger2[];
extern DialogEntry mine_messenger3[];
extern DialogEntry mine_messenger4[];
extern DialogEntry mine_boss[];
extern DialogEntry messenger_talked[];
extern DialogEntry orcs_guard[];
extern DialogEntry orcs_captain[];
extern DialogEntry orcs2_gorush[];
extern DialogEntry orcs2_weak_orc[];
extern DialogEntry orcs2_blacksmith[];
extern DialogEntry orcs2_orc[];
extern DialogEntry rescue_captive_start[];
extern DialogEntry rescue_captive_timeout[];
extern DialogEntry rescue_captive_end[];
extern DialogEntry rescue_captive_talk[];
extern DialogEntry retrive_package_start[];
extern DialogEntry retrive_package_timeout[];
extern DialogEntry retrive_package_end[];
extern DialogEntry sawmill_talk[];
extern DialogEntry sawmill_messenger[];
extern DialogEntry spread_news_start[];
extern DialogEntry spread_news_tell[];
extern DialogEntry spread_news_timeout[];
extern DialogEntry spread_news_end[];
extern DialogEntry stolen_artifact_start[];
extern DialogEntry stolen_artifact_end[];
extern DialogEntry stolen_artifact_timeout[];
extern DialogEntry wanted_start[];
extern DialogEntry wanted_timeout[];
extern DialogEntry wanted_end[];

void ExportDialog(std::ofstream& o, std::ofstream& os, cstring id, DialogEntry* dialog)
{
	o << "//==============================================================================\ndialog " << id << "\n{\n";
	os << "//==============================================================================\ndialog " << id << " {\n";

	int tabs = 1;
	int index = 0;
	Game& game = Game::Get();

	while(true)
	{
		if(dialog->type == DT_END_OF_DIALOG)
			break;

		if(dialog->type == DT_END_CHOICE || dialog->type == DT_END_IF)
		{
			--tabs;
			for(int i = 0; i < tabs; ++i)
				o << '\t';
			o << "}\n";
			++dialog;
			continue;
		}
		else if(dialog->type == DT_ELSE)
		{
			--tabs;
			for(int i = 0; i < tabs; ++i)
				o << '\t';
			o << "}\n";
			for(int i = 0; i < tabs; ++i)
				o << '\t';
			o << "else\n";
			for(int i = 0; i < tabs; ++i)
				o << '\t';
			o << "{\n";
			++tabs;
			++dialog;
			continue;
		}
		else if(dialog->type == DT_ESCAPE_CHOICE)
		{
			++dialog;
			continue;
		}

		for(int i = 0; i < tabs; ++i)
			o << '\t';

		switch(dialog->type)
		{
		case DT_CHOICE:
			{
				// check if is escape choice
				DialogEntry* d = dialog + 1;
				int count = 0;
				while(true)
				{
					if(d->type == DT_END_CHOICE)
					{
						if(count == 0)
						{
							++d;
							if(d->type == DT_ESCAPE_CHOICE)
								o << "escape ";
							break;
						}
						else
							--count;
					}
					else if(d->type == DT_CHOICE)
						++count;
					++d;
				}
				o << "choice " << index << "\n";
				for(int i = 0; i < tabs; ++i)
					o << '\t';
				o << "{\n";
				os << "\t" << index << Format(" \"%s\"\n", Escape(game.txDialog[(int)dialog->msg]));
				++tabs;
				++index;
			}
			break;
		case DT_TRADE:
			o << "trade\n";
			break;
		case DT_TALK:
		case DT_TALK2:
			o << (dialog->type == DT_TALK ? "talk" : "talk2") << " " << index << "\n";
			os << "\t" << index << Format(" \"%s\"\n", Escape(game.txDialog[(int)dialog->msg]));
			++index;
			break;
		case DT_RESTART:
			o << "restart\n";
			break;
		case DT_END:
			o << "end\n";
			break;
		case DT_END2:
			o << "end2\n";
			break;
		case DT_SHOW_CHOICES:
			o << "show_choices\n";
			break;
		case DT_SPECIAL:
			o << "special \"" << dialog->msg << "\"\n";
			break;
		case DT_SET_QUEST_PROGRESS:
			o << "set_quest_progress " << (int)dialog->msg << "\n";
			break;
		case DT_IF_QUEST_TIMEOUT:
			o << "if quest_timeout\n";
			for(int i = 0; i < tabs; ++i)
				o << '\t';
			o << "{\n";
			++tabs;
			break;
		case DT_IF_RAND:
			o << "if rand " << (int)dialog->msg << "\n";
			for(int i = 0; i < tabs; ++i)
				o << '\t';
			o << "{\n";
			++tabs;
			break;
		case DT_CHECK_QUEST_TIMEOUT:
			o << "check_quest_timeout " << (int)dialog->msg << "\n";
			break;
		case DT_IF_HAVE_QUEST_ITEM:
			o << "if have_quest_item \"" << dialog->msg << "\"\n";
			for(int i = 0; i < tabs; ++i)
				o << '\t';
			o << "{\n";
			++tabs;
			break;
		case DT_DO_QUEST:
			o << "do_quest \"" << dialog->msg << "\"\n";
			break;
		case DT_DO_QUEST_ITEM:
			o << "do_quest_item \"" << dialog->msg << "\"\n";
			break;
		case DT_IF_QUEST_PROGRESS:
			o << "if quest_progress " << (int)dialog->msg << "\n";
			for(int i = 0; i < tabs; ++i)
				o << '\t';
			o << "{\n";
			++tabs;
			break;
		case DT_IF_NEED_TALK:
			o << "if need_talk \"" << dialog->msg << "\"\n";
			for(int i = 0; i < tabs; ++i)
				o << '\t';
			o << "{\n";
			++tabs;
			break;
		case DT_IF_SPECIAL:
			o << "if special \"" << dialog->msg << "\"\n";
			for(int i = 0; i < tabs; ++i)
				o << '\t';
			o << "{\n";
			++tabs;
			break;
		case DT_IF_ONCE:
			o << "if once\n";
			for(int i = 0; i < tabs; ++i)
				o << '\t';
			o << "{\n";
			++tabs;
			break;
		case DT_RANDOM_TEXT:
			{
				int count = (int)dialog->msg;
				os << "\t" << index << " {\n";
				bool is_talk2 = false;
				for(int i = 0; i < count; ++i)
				{
					++dialog;
					if(dialog->type == DT_TALK2)
						is_talk2 = true;
					else
						assert(dialog->type == DT_TALK);
					os << "\t\t\"" << Escape(game.txDialog[(int)dialog->msg]) << "\"\n";
				}
				os << "\t}\n";
				o << (is_talk2 ? "talk2" : "talk") << " " << index << "\n";
				++index;
			}
			break;
		case DT_IF_CHOICES:
			o << "if choices " << (int)dialog->msg << "\n";
			for(int i = 0; i < tabs; ++i)
				o << '\t';
			o << "{\n";
			++tabs;
			break;
		case DT_DO_QUEST2:
			o << "do_quest2 \"" << dialog->msg << "\"\n";
			break;
		case DT_IF_HAVE_ITEM:
			o << "if have_item " << dialog->msg << "\n";
			for(int i = 0; i < tabs; ++i)
				o << '\t';
			o << "{\n";
			++tabs;
			break;
		case DT_IF_QUEST_PROGRESS_RANGE:
			{
				int p = (int)dialog->msg;
				o << "if quest_progress_range " << (p & 0xFFFF) << " " << ((p & 0xFFFF0000)>>16) << "\n";
				for(int i = 0; i < tabs; ++i)
					o << '\t';
				o << "{\n";
				++tabs;
			}
			break;
		case DT_IF_QUEST_EVENT:
			o << "if quest_event\n";
			for(int i = 0; i < tabs; ++i)
				o << '\t';
			o << "{\n";
			++tabs;
			break;
		case DT_DO_ONCE:
			o << "do_once\n";
			break;
		}

		++dialog;
	}

	o << "}\n\n";
	os << "}\n\n";
}

void ExportDialogs()
{
	std::ofstream o("../system/dialogs.txt");
	std::ofstream os("../system/dialogs2.txt");

	ExportDialog(o, os, "blacksmith", dialog_kowal);
	ExportDialog(o, os, "merchant", dialog_kupiec);
	ExportDialog(o, os, "alchemist", dialog_alchemik);
	ExportDialog(o, os, "mayor", dialog_burmistrz);
	ExportDialog(o, os, "citizen", dialog_mieszkaniec);
	ExportDialog(o, os, "viewer", dialog_widz);
	ExportDialog(o, os, "guard", dialog_straznik);
	ExportDialog(o, os, "trainer", dialog_trener);
	ExportDialog(o, os, "guard_captain", dialog_dowodca_strazy);
	ExportDialog(o, os, "innkeeper", dialog_karczmarz);
	ExportDialog(o, os, "clerk", dialog_urzednik);
	ExportDialog(o, os, "arena_master", dialog_mistrz_areny);
	ExportDialog(o, os, "hero", dialog_hero);
	ExportDialog(o, os, "hero_get_item", dialog_hero_przedmiot);
	ExportDialog(o, os, "hero_buy_item", dialog_hero_przedmiot_kup);
	ExportDialog(o, os, "hero_pvp", dialog_hero_pvp);
	ExportDialog(o, os, "crazy", dialog_szalony);
	ExportDialog(o, os, "crazy_get_item", dialog_szalony_przedmiot);
	ExportDialog(o, os, "crazy_buy_item", dialog_szalony_przedmiot_kup);
	ExportDialog(o, os, "crazy_pvp", dialog_szalony_pvp);
	ExportDialog(o, os, "bandits", dialog_bandyci);
	ExportDialog(o, os, "bandit", dialog_bandyta);
	ExportDialog(o, os, "crazy_mage_encounter", dialog_szalony_mag);
	ExportDialog(o, os, "captive", dialog_porwany);
	ExportDialog(o, os, "guards_encounter", dialog_straznicy_nagroda);
	ExportDialog(o, os, "traveler", dialog_zadanie);
	ExportDialog(o, os, "q_sawmill", dialog_artur_drwal);
	ExportDialog(o, os, "woodcutter", dialog_drwal);
	ExportDialog(o, os, "q_mine", dialog_inwestor);
	ExportDialog(o, os, "miner", dialog_gornik);
	ExportDialog(o, os, "drunkman", dialog_pijak);
	ExportDialog(o, os, "q_bandits", dialog_q_bandyci);
	ExportDialog(o, os, "q_mages", dialog_q_magowie);
	ExportDialog(o, os, "q_mages2", dialog_q_magowie2);
	ExportDialog(o, os, "q_orcs", dialog_q_orkowie);
	ExportDialog(o, os, "q_orcs2", dialog_q_orkowie2);
	ExportDialog(o, os, "q_goblins", dialog_q_gobliny);
	ExportDialog(o, os, "q_evil", dialog_q_zlo);
	ExportDialog(o, os, "q_tutorial", dialog_tut_czlowiek);
	ExportDialog(o, os, "q_crazies", dialog_q_szaleni);
	ExportDialog(o, os, "crazies_encounter", dialog_szaleni);
	ExportDialog(o, os, "tomashu", dialog_tomashu);
	ExportDialog(o, os, "bodyguard", dialog_ochroniarz);
	ExportDialog(o, os, "mage_bodyguard", dialog_mag_obstawa);
	ExportDialog(o, os, "foodseller", dialog_sprzedawca_jedzenia);
	ExportDialog(o, os, "q_bandits_master", quest_bandits_master);
	ExportDialog(o, os, "q_bandits_encounter", quest_bandits_encounter);
	ExportDialog(o, os, "q_bandits_captain", quest_bandits_captain);
	ExportDialog(o, os, "q_bandits_guard", quest_bandits_guard);
	ExportDialog(o, os, "q_bandits_agent", quest_bandits_agent);
	ExportDialog(o, os, "q_bandits_boss", quest_bandits_boss);
	ExportDialog(o, os, "q_bandits_collect_toll_start", bandits_collect_toll_start);
	ExportDialog(o, os, "q_bandits_collect_toll_timeout", bandits_collect_toll_timeout);
	ExportDialog(o, os, "q_bandits_collect_toll_end", bandits_collect_toll_end);
	ExportDialog(o, os, "q_bandits_collect_toll_talk", bandits_collect_toll_talk);
	ExportDialog(o, os, "q_camp_near_city_start", camp_near_city_start);
	ExportDialog(o, os, "q_camp_near_city_timeout", camp_near_city_timeout);
	ExportDialog(o, os, "q_camp_near_city_end", camp_near_city_end);
	ExportDialog(o, os, "q_crazies_trainer", crazies_trainer);
	ExportDialog(o, os, "q_deliver_letter_start", deliver_letter_start);
	ExportDialog(o, os, "q_deliver_letter_timeout", deliver_letter_timeout);
	ExportDialog(o, os, "q_deliver_letter_give", deliver_letter_give);
	ExportDialog(o, os, "q_deliver_letter_end", deliver_letter_end);
	ExportDialog(o, os, "q_deliver_parcel_start", deliver_parcel_start);
	ExportDialog(o, os, "q_deliver_parcel_give", deliver_parcel_give);
	ExportDialog(o, os, "q_deliver_parcel_timeout", deliver_parcel_timeout);
	ExportDialog(o, os, "q_deliver_parcel_bandits", deliver_parcel_bandits);
	ExportDialog(o, os, "q_evil_cleric", evil_cleric);
	ExportDialog(o, os, "q_evil_mage", evil_mage);
	ExportDialog(o, os, "q_evil_captain", evil_captain);
	ExportDialog(o, os, "q_evil_mayor", evil_mayor);
	ExportDialog(o, os, "q_evil_boss", evil_boss);
	ExportDialog(o, os, "q_find_artifact_start", find_artifact_start);
	ExportDialog(o, os, "q_find_artifact_end", find_artifact_end);
	ExportDialog(o, os, "q_find_artifact_timeout", find_artifact_timeout);
	ExportDialog(o, os, "q_goblins_nobleman", goblins_nobleman);
	ExportDialog(o, os, "q_goblins_encounter", goblins_encounter);
	ExportDialog(o, os, "q_goblins_messenger", goblins_messenger);
	ExportDialog(o, os, "q_goblins_mage", goblins_mage);
	ExportDialog(o, os, "q_goblins_innkeeper", goblins_innkeeper);
	ExportDialog(o, os, "q_goblins_boss", goblins_boss);
	ExportDialog(o, os, "q_kill_animals_start", kill_animals_start);
	ExportDialog(o, os, "q_kill_animals_timeout", kill_animals_timeout);
	ExportDialog(o, os, "q_kill_animals_end", kill_animals_end);
	ExportDialog(o, os, "q_lost_artifact_start", lost_artifact_start);
	ExportDialog(o, os, "q_lost_artifact_end", lost_artifact_end);
	ExportDialog(o, os, "q_lost_artifact_timeout", lost_artifact_timeout);
	ExportDialog(o, os, "q_mages_scholar", mages_scholar);
	ExportDialog(o, os, "q_mages_golem", mages_golem);
	ExportDialog(o, os, "q_mages2_captain", mages2_captain);
	ExportDialog(o, os, "q_mages2_mage", mages2_mage);
	ExportDialog(o, os, "q_mages2_boss", mages2_boss);
	ExportDialog(o, os, "q_dialog_main_event", dialog_main_event);
	ExportDialog(o, os, "q_dialog_main", dialog_main);
	ExportDialog(o, os, "q_mine_investor", mine_investor);
	ExportDialog(o, os, "q_mine_messenger", mine_messenger);
	ExportDialog(o, os, "q_mine_messenger2", mine_messenger2);
	ExportDialog(o, os, "q_mine_messenger3", mine_messenger3);
	ExportDialog(o, os, "q_mine_messenger4", mine_messenger4);
	ExportDialog(o, os, "q_mine_boss", mine_boss);
	ExportDialog(o, os, "messenger_talked", messenger_talked);
	ExportDialog(o, os, "q_orcs_guard", orcs_guard);
	ExportDialog(o, os, "q_orcs_captain", orcs_captain);
	ExportDialog(o, os, "q_orcs2_gorush", orcs2_gorush);
	ExportDialog(o, os, "q_orcs2_weak_orc", orcs2_weak_orc);
	ExportDialog(o, os, "q_orcs2_blacksmith", orcs2_blacksmith);
	ExportDialog(o, os, "q_orcs2_orc", orcs2_orc);
	ExportDialog(o, os, "q_rescue_captive_start", rescue_captive_start);
	ExportDialog(o, os, "q_rescue_captive_timeout", rescue_captive_timeout);
	ExportDialog(o, os, "q_rescue_captive_end", rescue_captive_end);
	ExportDialog(o, os, "q_rescue_captive_talk", rescue_captive_talk);
	ExportDialog(o, os, "q_retrive_package_start", retrive_package_start);
	ExportDialog(o, os, "q_retrive_package_timeout", retrive_package_timeout);
	ExportDialog(o, os, "q_retrive_package_end", retrive_package_end);
	ExportDialog(o, os, "q_sawmill_talk", sawmill_talk);
	ExportDialog(o, os, "q_sawmill_messenger", sawmill_messenger);
	ExportDialog(o, os, "q_spread_news_start", spread_news_start);
	ExportDialog(o, os, "q_spread_news_tell", spread_news_tell);
	ExportDialog(o, os, "q_spread_news_timeout", spread_news_timeout);
	ExportDialog(o, os, "q_spread_news_end", spread_news_end);
	ExportDialog(o, os, "q_stolen_artifact_start", stolen_artifact_start);
	ExportDialog(o, os, "q_stolen_artifact_end", stolen_artifact_end);
	ExportDialog(o, os, "q_stolen_artifact_timeout", stolen_artifact_timeout);
	ExportDialog(o, os, "q_wanted_start", wanted_start);
	ExportDialog(o, os, "q_wanted_timeout", wanted_timeout);
	ExportDialog(o, os, "q_wanted_end", wanted_end);

	o.flush();
	os.flush();
}

enum Group
{
	G_TOP,
	G_KEYWORD
};

enum TopKeyword
{
	T_DIALOG
};

enum Keyword
{
	K_CHOICE,
	K_TRADE,
	K_TALK,
	K_TALK2,
	K_RESTART,
	K_END,
	K_END2,
	K_SHOW_CHOICES,
	K_SPECIAL,
	K_SET_QUEST_PROGRESS,
	K_IF,
	K_QUEST_TIMEOUT,
	K_RAND,
	K_ELSE,
	K_CHECK_QUEST_TIMEOUT,
	K_HAVE_QUEST_ITEM,
	K_DO_QUEST,
	K_DO_QUEST_ITEM,
	K_QUEST_PROGRESS,
	K_NEED_TALK,
	K_ESCAPE,
	K_ONCE,
	K_CHOICES,
	K_DO_QUEST2,
	K_HAVE_ITEM,
	K_QUEST_PROGRESS_RANGE,
	K_QUEST_EVENT,
	K_DO_ONCE
};

enum IfState
{
	IFS_IF,
	IFS_INLINE_IF,
	IFS_ELSE,
	IFS_INLINE_ELSE,
	IFS_CHOICE,
	IFS_INLINE_CHOICE
};

struct Dialog2
{
	string id;
	vector<DialogEntry> code;
	vector<string> strs;
};

bool LoadDialog(Tokenizer& t)
{
	Dialog2* dialog = new Dialog2;
	vector<IfState> if_state;
	bool is_inline = false;

	try
	{
		dialog->id = t.MustGetItemKeyword();
		t.Next();

		t.AssertSymbol('{');
		t.Next();

		while(true)
		{
			if(t.IsSymbol('}'))
			{

			}
			else if(t.IsKeywordGroup(G_KEYWORD))
			{
				Keyword k = (Keyword)t.GetKeywordId(G_KEYWORD);
				t.Next();

				switch(k)
				{
				case K_CHOICE:
					{
						int index = t.MustGetInt();
						if(index < 0)
							t.Throw("Invalid text index %d.", index);
						t.Next();
						dialog->code.push_back({ DT_CHOICE, (cstring)index });
						if(t.IsSymbol('{'))
							if_state.push_back(IFS_CHOICE);
						else
						{
							if_state.push_back(IFS_INLINE_CHOICE);
							is_inline = true;
						}
					}
					break;
				case K_TRADE:
					dialog->code.push_back({ DT_TRADE, NULL });
					break;
				case K_TALK:
				case K_TALK2:
					{
						int index = t.MustGetInt();
						if(index < 0)
							t.Throw("Invalid text index %d.", index);
						t.Next();
						dialog->code.push_back({ k == K_TALK ? DT_TALK : DT_TALK2, (cstring)index });
					}
					break;
				case K_RESTART:
					dialog->code.push_back({ DT_RESTART, NULL });
					break;
				case K_END:
					dialog->code.push_back({ DT_END, NULL });
					break;
				case K_END2:
					dialog->code.push_back({ DT_END2, NULL });
					break;
				case K_SHOW_CHOICES:
					dialog->code.push_back({ DT_SHOW_CHOICES, NULL });
					break;
				case K_SPECIAL:
					{
						int index = dialog->strs.size();
						dialog->strs.push_back(t.MustGetString());
						t.Next();
						dialog->code.push_back({ DT_SPECIAL, (cstring)index });
					}
					break;
				case K_SET_QUEST_PROGRESS:
					{
						int p = t.MustGetInt();
						if(p < 0)
							t.Throw("Invalid quest progress %d.", p);
						t.Next();
						dialog->code.push_back({ DT_SET_QUEST_PROGRESS, (cstring)p });
					}
					break;
				case K_IF:
					{
						k = (Keyword)t.MustGetKeywordId(G_KEYWORD);
						t.Next();

						switch(k)
						{
						case K_QUEST_TIMEOUT:
							dialog->code.push_back({ DT_IF_QUEST_TIMEOUT, NULL });
							break;
						case K_RAND:
							{
								int chance = t.MustGetInt();
								if(chance <= 0 || chance >= 100)
									t.Throw("Invalid chance %d.", chance);
								t.Next();
								dialog->code.push_back({ DT_IF_RAND, (cstring)chance });
							}
							break;
						case K_HAVE_QUEST_ITEM:
							{
								int index = dialog->strs.size();
								dialog->strs.push_back(t.MustGetString());
								t.Next();
								dialog->code.push_back({ DT_IF_HAVE_QUEST_ITEM, (cstring)index });
							}
							break;
						case K_QUEST_PROGRESS:
							{
								int p = t.MustGetInt();
								if(p < 0)
									t.Throw("Invalid quest progress %d.", p);
								t.Next();
								dialog->code.push_back({ DT_IF_QUEST_PROGRESS, (cstring)p });
							}
							break;
						case K_NEED_TALK:
							{
								int index = dialog->strs.size();
								dialog->strs.push_back(t.MustGetString());
								t.Next();
								dialog->code.push_back({ DT_IF_NEED_TALK, (cstring)index });
							}
							break;
						case K_SPECIAL:
							{
								int index = dialog->strs.size();
								dialog->strs.push_back(t.MustGetString());
								t.Next();
								dialog->code.push_back({ DT_IF_SPECIAL, (cstring)index });
							}
							break;
						case K_ONCE:
							dialog->code.push_back({ DT_IF_ONCE, NULL });
							break;
						case K_HAVE_ITEM:
							{
								const string& id = t.MustGetItemKeyword();
								ItemListResult result;
								const Item* item = FindItem(id.c_str(), false, &result);
								if(item && result.lis == NULL)
								{
									t.Next();
									dialog->code.push_back({ DT_IF_HAVE_ITEM, (cstring)item });
								}
								else
									t.Throw("Invalid item '%s'.", id.c_str());
							}
							break;
						case K_QUEST_EVENT:
							dialog->code.push_back({ DT_IF_QUEST_EVENT, NULL });
							break;
						case K_QUEST_PROGRESS_RANGE:
							{
								int a = t.MustGetInt();
								t.Next();
								int b = t.MustGetInt();
								t.Next();
								if(a < 0 || a >= b)
									t.Throw("Invalid quest progress range {%d %d}.", a, b);
								int p = ((a & 0xFFFF) | ((p & 0xFFFF0000) >> 16));
								dialog->code.push_back({ DT_IF_QUEST_PROGRESS_RANGE, (cstring)p });
							}
							break;
						case K_CHOICES:
							{
								int count = t.MustGetInt();
								if(count < 0)
									t.Throw("Invalid choices count %d.", count);
								t.Next();
								dialog->code.push_back({ DT_IF_CHOICES, (cstring)count });
							}
							break;
						default:
							t.Unexpected();
							break;
						}
						//!!!
					}
					break;
				case K_ELSE:
					// !!!
					break;
				case K_CHECK_QUEST_TIMEOUT:
					{
						int type = t.MustGetInt();
						if(type < 0 || type > 2)
							t.Throw("Invalid quest type %d.", type);
						t.Next();
						dialog->code.push_back({ DT_CHECK_QUEST_TIMEOUT, (cstring)type });
					}
					break;
				case K_DO_QUEST:
					{
						int index = dialog->strs.size();
						dialog->strs.push_back(t.MustGetString());
						t.Next();
						dialog->code.push_back({ DT_DO_QUEST, (cstring)index });
					}
					break;
				case K_DO_QUEST_ITEM:
					{
						int index = dialog->strs.size();
						dialog->strs.push_back(t.MustGetString());
						t.Next();
						dialog->code.push_back({ DT_DO_QUEST_ITEM, (cstring)index });
					}
					break;
				case K_ESCAPE:
					//!!!
					break;
				case K_DO_QUEST2:
					{
						int index = dialog->strs.size();
						dialog->strs.push_back(t.MustGetString());
						t.Next();
						dialog->code.push_back({ DT_DO_QUEST2, (cstring)index });
					}
					break;
				case K_DO_ONCE:
					dialog->code.push_back({ DT_DO_ONCE, NULL });
					break;
				default:
					t.Unexpected();
					break;
				}

				if(is_inline)
					is_inline = false;
				else
				{

				}
			}
			else
				t.Unexpected();
		}

		return true;
	}
	catch(Tokenizer::Exception& e)
	{
		ERROR(Format("Failed to parse dialog '%s': %s", dialog->id.c_str(), e.ToString()));
		delete dialog;
		return false;
	}
}

void LoadDialogs()
{
	Tokenizer t;
	if(!t.FromFile(Format("%s/dialogs.txt", g_system_dir.c_str())))
		throw "Failed to open dialogs.txt.";

	t.AddKeyword("dialog", T_DIALOG, G_TOP);

	t.AddKeywords(G_KEYWORD, {
		{ "choice", K_CHOICE },
		{ "talk", K_TALK },
		{ "talk2", K_TALK2 },
		{ "end", K_END },
		{ "end2", K_END2 },
		{ "show_choices", K_SHOW_CHOICES },
		{ "special", K_SPECIAL },
		{ "set_quest_progress", K_SET_QUEST_PROGRESS },
		{ "if", K_IF },
		{ "quest_timeout", K_QUEST_TIMEOUT },
		{ "rand", K_RAND },
		{ "else", K_ELSE },
		{ "check_quest_timeout", K_CHECK_QUEST_TIMEOUT },
		{ "have_quest_item", K_HAVE_QUEST_ITEM },
		{ "do_quest", K_DO_QUEST },
		{ "do_quest_item", K_DO_QUEST_ITEM },
		{ "quest_progress", K_QUEST_PROGRESS },
		{ "need_talk", K_NEED_TALK },
		{ "escape", K_ESCAPE },
		{ "special", K_SPECIAL },
		{ "once", K_ONCE },
		{ "choices", K_CHOICES },
		{ "do_quest2", K_DO_QUEST2 },
		{ "have_item", K_HAVE_ITEM },
		{ "quest_progress_range", K_QUEST_PROGRESS_RANGE },
		{ "quest_event", K_QUEST_EVENT },
		{ "do_once", K_DO_ONCE }
	});

	int errors = 0;

	try
	{
		t.Next();

		while(!t.IsEof())
		{
			bool skip = false;

			if(t.IsKeywordGroup(G_TOP))
			{
				t.Next();

				if(!LoadDialog(t))
				{
					skip = true;
					++errors;
				}
			}
			else
			{
				int group = G_TOP;
				ERROR(t.FormatUnexpected(Tokenizer::T_KEYWORD_GROUP, &group));
				skip = true;
				++errors;
			}

			if(skip)
				t.SkipToKeywordGroup(G_TOP);
			else
				t.Next();
		}
	}
	catch(const Tokenizer::Exception& e)
	{
		ERROR(Format("Failed to load dialogs: %s", e.ToString()));
		++errors;
	}

	if(errors > 0)
		throw Format("Failed to load dialogs (%d errors), check log for details.", errors);
}
