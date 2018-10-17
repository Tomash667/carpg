#include "Pch.h"
#include "GameCore.h"
#include "DialogLoader.h"
#include "GameDialog.h"
#include "Item.h"
#include "ScriptManager.h"

enum Group
{
	G_TOP,
	G_KEYWORD
};

enum TopKeyword
{
	T_DIALOG,
	T_GLOBALS
};

enum Keyword
{
	K_CHOICE,
	K_TRADE,
	K_TALK,
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
	K_DO_ONCE,
	K_NOT_ACTIVE,
	K_QUEST_SPECIAL,
	K_NOT,
	K_SCRIPT
};

enum IfState
{
	IFS_IF,
	IFS_INLINE_IF,
	IFS_ELSE,
	IFS_INLINE_ELSE,
	IFS_CHOICE,
	IFS_INLINE_CHOICE,
	IFS_ESCAPE
};

void DialogLoader::InitTokenizer()
{
	t.AddKeywords(G_TOP, {
		{ "dialog", T_DIALOG },
		{ "globals", T_GLOBALS }
	});

	t.AddKeywords(G_KEYWORD, {
		{ "choice", K_CHOICE },
		{ "trade", K_TRADE },
		{ "talk", K_TALK },
		{ "restart", K_RESTART },
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
		{ "once", K_ONCE },
		{ "choices", K_CHOICES },
		{ "do_quest2", K_DO_QUEST2 },
		{ "have_item", K_HAVE_ITEM },
		{ "quest_progress_range", K_QUEST_PROGRESS_RANGE },
		{ "quest_event", K_QUEST_EVENT },
		{ "do_once", K_DO_ONCE },
		{ "not_active", K_NOT_ACTIVE },
		{ "quest_special", K_QUEST_SPECIAL },
		{ "not", K_NOT },
		{ "script", K_SCRIPT }
	});
}

void DialogLoader::DoLoading()
{
	bool require_id[2] = { true, false };
	Load("dialogs.txt", G_TOP, require_id);
}

void DialogLoader::LoadEntity(int top, const string& id)
{
	if(top == T_DIALOG)
		LoadDialog(id);
	else
		LoadGlobals();
}

void DialogLoader::LoadDialog(const string& id)
{
	Ptr<GameDialog> dialog;
	bool line_block = false;
	dialog->max_index = -1;

	dialog->id = t.MustGetItemKeyword();
	crc.Update(dialog->id);
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	while(true)
	{
		if(t.IsSymbol('}'))
		{
			t.Next();
			if(if_state.empty())
				break;
			switch(if_state.back())
			{
			case IFS_INLINE_CHOICE:
			case IFS_INLINE_IF:
			case IFS_INLINE_ELSE:
				t.Unexpected();
				break;
			case IFS_IF:
				if(t.IsKeyword(K_ELSE, G_KEYWORD))
				{
					// if { ... } else
					t.Next();
					dialog->code.push_back({ DTF_ELSE, 0 });
					crc.Update(DTF_ELSE);
					if(t.IsSymbol('{'))
					{
						if_state.back() = IFS_ELSE;
						t.Next();
					}
					else
						if_state.back() = IFS_INLINE_ELSE;
				}
				else
				{
					dialog->code.push_back({ DTF_END_IF, 0 });
					if_state.pop_back();
					crc.Update(DTF_END_IF);
				}
				break;
			case IFS_ELSE:
				dialog->code.push_back({ DTF_END_IF, 0 });
				if_state.pop_back();
				crc.Update(DTF_END_IF);
				break;
			case IFS_CHOICE:
				dialog->code.push_back({ DTF_END_CHOICE, 0 });
				if_state.pop_back();
				crc.Update(DTF_END_CHOICE);
				if(!if_state.empty() && if_state.back() == IFS_ESCAPE)
				{
					dialog->code.push_back({ DTF_ESCAPE_CHOICE, 0 });
					if_state.pop_back();
					crc.Update(DTF_ESCAPE_CHOICE);
				}
				break;
			}
		}
		else if(t.IsKeywordGroup(G_KEYWORD))
		{
			Keyword k = (Keyword)t.GetKeywordId(G_KEYWORD);
			t.Next();

			switch(k)
			{
			case K_CHOICE:
			case K_ESCAPE:
				{
					if(k == K_ESCAPE)
					{
						crc.Update(DTF_ESCAPE_CHOICE);
						t.AssertKeyword(K_CHOICE, G_KEYWORD);
						t.Next();
						if_state.push_back(IFS_ESCAPE);
					}

					int index = t.MustGetInt();
					if(index < 0)
						t.Throw("Invalid text index %d.", index);
					t.Next();
					dialog->code.push_back({ DTF_CHOICE, index });
					if(t.IsSymbol('{'))
					{
						if_state.push_back(IFS_CHOICE);
						t.Next();
					}
					else
						if_state.push_back(IFS_INLINE_CHOICE);
					++index;
					if(index > dialog->max_index)
					{
						dialog->texts.resize(index, GameDialog::Text());
						dialog->max_index = index;
					}
					dialog->texts[index - 1].exists = true;
					line_block = true;
					crc.Update(DTF_CHOICE);
					crc.Update(index);
				}
				break;
			case K_TRADE:
				dialog->code.push_back({ DTF_TRADE, 0 });
				crc.Update(DTF_TRADE);
				break;
			case K_TALK:
				{
					int index = t.MustGetInt();
					if(index < 0)
						t.Throw("Invalid text index %d.", index);
					t.Next();
					dialog->code.push_back({ DTF_TALK, index });
					++index;
					if(index > dialog->max_index)
					{
						dialog->texts.resize(index, GameDialog::Text());
						dialog->max_index = index;
					}
					dialog->texts[index - 1].exists = true;
					crc.Update(DTF_TALK);
					crc.Update(index);
				}
				break;
			case K_RESTART:
				dialog->code.push_back({ DTF_RESTART, 0 });
				crc.Update(DTF_RESTART);
				break;
			case K_END:
				dialog->code.push_back({ DTF_END, 0 });
				crc.Update(DTF_END);
				break;
			case K_END2:
				dialog->code.push_back({ DTF_END2, 0 });
				crc.Update(DTF_END2);
				break;
			case K_SHOW_CHOICES:
				dialog->code.push_back({ DTF_SHOW_CHOICES, 0 });
				crc.Update(DTF_SHOW_CHOICES);
				break;
			case K_SPECIAL:
				{
					int index = dialog->strs.size();
					dialog->strs.push_back(t.MustGetString());
					t.Next();
					dialog->code.push_back({ DTF_SPECIAL, index });
					crc.Update(DTF_SPECIAL);
					crc.Update(dialog->strs.back());
				}
				break;
			case K_SET_QUEST_PROGRESS:
				{
					int p = t.MustGetInt();
					if(p < 0)
						t.Throw("Invalid quest progress %d.", p);
					t.Next();
					dialog->code.push_back({ DTF_SET_QUEST_PROGRESS, p });
					crc.Update(DTF_SET_QUEST_PROGRESS);
					crc.Update(p);
				}
				break;
			case K_IF:
				{
					k = (Keyword)t.MustGetKeywordId(G_KEYWORD);
					t.Next();

					if(k == K_NOT)
					{
						dialog->code.push_back({ DTF_NOT, 0 });
						crc.Update(DTF_NOT);
						k = (Keyword)t.MustGetKeywordId(G_KEYWORD);
						t.Next();
					}

					switch(k)
					{
					case K_QUEST_TIMEOUT:
						dialog->code.push_back({ DTF_IF_QUEST_TIMEOUT, 0 });
						crc.Update(DTF_IF_QUEST_TIMEOUT);
						break;
					case K_RAND:
						{
							int chance = t.MustGetInt();
							if(chance <= 0 || chance >= 100)
								t.Throw("Invalid chance %d.", chance);
							t.Next();
							dialog->code.push_back({ DTF_IF_RAND, chance });
							crc.Update(DTF_IF_RAND);
							crc.Update(chance);
						}
						break;
					case K_HAVE_QUEST_ITEM:
						{
							if(t.IsKeyword(K_NOT_ACTIVE, G_KEYWORD))
							{
								t.Next();
								dialog->code.push_back({ DTF_NOT_ACTIVE, 0 });
								crc.Update(DTF_NOT_ACTIVE);
							}
							int index = dialog->strs.size();
							dialog->strs.push_back(t.MustGetString());
							t.Next();
							dialog->code.push_back({ DTF_IF_HAVE_QUEST_ITEM, index });
							crc.Update(DTF_IF_HAVE_QUEST_ITEM);
							crc.Update(dialog->strs.back());
						}
						break;
					case K_QUEST_PROGRESS:
						{
							int p = t.MustGetInt();
							if(p < 0)
								t.Throw("Invalid quest progress %d.", p);
							t.Next();
							dialog->code.push_back({ DTF_IF_QUEST_PROGRESS, p });
							crc.Update(DTF_IF_QUEST_PROGRESS);
							crc.Update(p);
						}
						break;
					case K_NEED_TALK:
						{
							int index = dialog->strs.size();
							dialog->strs.push_back(t.MustGetString());
							t.Next();
							dialog->code.push_back({ DTF_IF_NEED_TALK, index });
							crc.Update(DTF_IF_NEED_TALK);
							crc.Update(dialog->strs.back());
						}
						break;
					case K_SPECIAL:
						{
							int index = dialog->strs.size();
							dialog->strs.push_back(t.MustGetString());
							t.Next();
							dialog->code.push_back({ DTF_IF_SPECIAL, index });
							crc.Update(DTF_IF_SPECIAL);
							crc.Update(dialog->strs.back());
						}
						break;
					case K_ONCE:
						dialog->code.push_back({ DTF_IF_ONCE, 0 });
						crc.Update(DTF_IF_ONCE);
						break;
					case K_HAVE_ITEM:
						{
							const string& id = t.MustGetItemKeyword();
							const Item* item = Item::TryGet(id);
							if(item)
							{
								t.Next();
								dialog->code.push_back({ DTF_IF_HAVE_ITEM, (int)item });
							}
							else
								t.Throw("Invalid item '%s'.", id.c_str());
							crc.Update(DTF_SPECIAL);
							crc.Update(item->id);
						}
						break;
					case K_QUEST_EVENT:
						dialog->code.push_back({ DTF_IF_QUEST_EVENT, 0 });
						crc.Update(DTF_IF_QUEST_EVENT);
						break;
					case K_QUEST_PROGRESS_RANGE:
						{
							int a = t.MustGetInt();
							t.Next();
							int b = t.MustGetInt();
							t.Next();
							if(a < 0 || a >= b)
								t.Throw("Invalid quest progress range {%d %d}.", a, b);
							int p = ((a & 0xFFFF) | ((b & 0xFFFF) << 16));
							dialog->code.push_back({ DTF_IF_QUEST_PROGRESS_RANGE, p });
							crc.Update(DTF_IF_QUEST_PROGRESS_RANGE);
							crc.Update(p);
						}
						break;
					case K_CHOICES:
						{
							int count = t.MustGetInt();
							if(count < 0)
								t.Throw("Invalid choices count %d.", count);
							t.Next();
							dialog->code.push_back({ DTF_IF_CHOICES, count });
							crc.Update(DTF_IF_CHOICES);
							crc.Update(count);
						}
						break;
					case K_QUEST_SPECIAL:
						{
							int index = dialog->strs.size();
							dialog->strs.push_back(t.MustGetString());
							t.Next();
							dialog->code.push_back({ DTF_IF_QUEST_SPECIAL, index });
							crc.Update(DTF_IF_QUEST_SPECIAL);
							crc.Update(dialog->strs.back());
						}
						break;
					case K_SCRIPT:
						{
							int index = dialog->strs.size();
							dialog->strs.push_back(t.MustGetString());
							t.Next();
							dialog->code.push_back({ DTF_IF_SCRIPT, index });
							dialog->scripts.push_back(GameDialog::Script(index, GameDialog::Script::IF));
							crc.Update(DTF_IF_SCRIPT);
							crc.Update(dialog->strs.back());
						}
						break;
					default:
						t.Unexpected();
						break;
					}

					if(t.IsSymbol('{'))
					{
						if_state.push_back(IFS_IF);
						t.Next();
					}
					else
						if_state.push_back(IFS_INLINE_IF);
					line_block = true;
				}
				break;
			case K_CHECK_QUEST_TIMEOUT:
				{
					int type = t.MustGetInt();
					if(type < 0 || type > 2)
						t.Throw("Invalid quest type %d.", type);
					t.Next();
					dialog->code.push_back({ DTF_CHECK_QUEST_TIMEOUT, type });
					crc.Update(DTF_CHECK_QUEST_TIMEOUT);
					crc.Update(type);
				}
				break;
			case K_DO_QUEST:
				{
					int index = dialog->strs.size();
					dialog->strs.push_back(t.MustGetString());
					t.Next();
					dialog->code.push_back({ DTF_DO_QUEST, index });
					crc.Update(DTF_DO_QUEST);
					crc.Update(dialog->strs.back());
				}
				break;
			case K_DO_QUEST_ITEM:
				{
					int index = dialog->strs.size();
					dialog->strs.push_back(t.MustGetString());
					t.Next();
					dialog->code.push_back({ DTF_DO_QUEST_ITEM, index });
					crc.Update(DTF_DO_QUEST_ITEM);
					crc.Update(dialog->strs.back());
				}
				break;
			case K_DO_QUEST2:
				{
					int index = dialog->strs.size();
					dialog->strs.push_back(t.MustGetString());
					t.Next();
					dialog->code.push_back({ DTF_DO_QUEST2, index });
					crc.Update(DTF_DO_QUEST2);
					crc.Update(dialog->strs.back());
				}
				break;
			case K_DO_ONCE:
				dialog->code.push_back({ DTF_DO_ONCE, 0 });
				crc.Update(DTF_DO_ONCE);
				break;
			case K_QUEST_SPECIAL:
				{
					int index = dialog->strs.size();
					dialog->strs.push_back(t.MustGetString());
					t.Next();
					dialog->code.push_back({ DTF_QUEST_SPECIAL, index });
					crc.Update(DTF_QUEST_SPECIAL);
					crc.Update(dialog->strs.back());
				}
				break;
			case K_SCRIPT:
				{
					int index = dialog->strs.size();
					dialog->strs.push_back(t.MustGetString());
					t.Next();
					dialog->code.push_back({ DTF_SCRIPT, index });
					dialog->scripts.push_back(GameDialog::Script(index, GameDialog::Script::NORMAL));
					crc.Update(DTF_SCRIPT);
					crc.Update(dialog->strs.back());
				}
				break;
			default:
				t.Unexpected();
				break;
			}
		}
		else
			t.Unexpected();

		if(line_block)
			line_block = false;
		else
		{
			while(!if_state.empty())
			{
				bool b = false;
				switch(if_state.back())
				{
				case IFS_IF:
				case IFS_ELSE:
				case IFS_CHOICE:
					b = true;
					break;
				case IFS_INLINE_IF:
					if(t.IsKeyword(K_ELSE, G_KEYWORD))
					{
						dialog->code.push_back({ DTF_ELSE, 0 });
						crc.Update(DTF_ELSE);
						t.Next();
						if(t.IsSymbol('{'))
						{
							if_state.back() = IFS_ELSE;
							t.Next();
						}
						else
							if_state.back() = IFS_INLINE_ELSE;
						b = true;
					}
					else
					{
						dialog->code.push_back({ DTF_END_IF, 0 });
						if_state.pop_back();
					}
					break;
				case IFS_INLINE_ELSE:
					dialog->code.push_back({ DTF_END_IF, 0 });
					if_state.pop_back();
					crc.Update(DTF_END_IF);
					break;
				case IFS_INLINE_CHOICE:
					dialog->code.push_back({ DTF_END_CHOICE, 0 });
					crc.Update(DTF_END_CHOICE);
					if_state.pop_back();
					if(!if_state.empty() && if_state.back() == IFS_ESCAPE)
					{
						dialog->code.push_back({ DTF_ESCAPE_CHOICE, 0 });
						if_state.pop_back();
						crc.Update(DTF_ESCAPE_CHOICE);
					}
					break;
				}

				if(b)
					break;
			}
		}
	}

	std::pair<GameDialog::Map::iterator, bool>& result = GameDialog::dialogs.insert(std::pair<cstring, GameDialog*>(dialog->id.c_str(), dialog));
	if(!result.second)
		t.Throw("Dialog with that id already exists.");
}

void DialogLoader::LoadGlobals()
{
	string type, name;

	t.AssertSymbol('{');
	t.Next();
	while(!t.IsSymbol('}'))
	{
		type = t.MustGetItem();
		t.Next();
		bool is_ref = t.IsSymbol('@');
		if(is_ref)
			t.Next();
		name = t.MustGetItem();
		t.Next();
		t.AssertSymbol(';');
		t.Next();
		ScriptManager::RegisterResult result = SM.RegisterGlobalVar(type, is_ref, name);
		if(result == ScriptManager::InvalidType)
			LoadError("Invalid type for global variable '%s%s %s'.", type.c_str(), is_ref ? "@" : "", name.c_str());
		else if(result == ScriptManager::AlreadyExists)
			LoadError("Global variable with name '%s' already declared.", name.c_str());
	}
}

void DialogLoader::Finalize()
{
	content::crc[(int)content::Id::Dialogs] = crc.Get();
	Info("Loaded dialogs (%u) - crc %p.", GameDialog::dialogs.size(), content::crc[(int)content::Id::Dialogs]);
}

void content::LoadDialogs()
{
	DialogLoader loader;
	loader.DoLoading();
}

void content::CleanupDialogs()
{
	GameDialog::Cleanup();
}
