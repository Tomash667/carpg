#include "Pch.h"
#include "GameCore.h"
#include "Dialog.h"
#include "Game.h"
#include "Crc.h"
#include "Language.h"
#include "ScriptManager.h"
#include "Quest.h"

extern string g_system_dir;
typedef std::map<cstring, GameDialog*, CstringComparer> DialogsMap;
DialogsMap dialogs;

//=================================================================================================
void CheckText(cstring text, bool talk2)
{
	bool have_format = (strchr(text, '$') != nullptr);
	if(talk2 != have_format)
		Warn("Invalid dialog type for text \"%s\".", text);
}

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

//=================================================================================================
bool LoadDialog(Tokenizer& t, Crc& crc)
{
	GameDialog* dialog = new GameDialog;
	vector<IfState> if_state;
	bool line_block = false;
	dialog->max_index = -1;

	try
	{
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

		std::pair<DialogsMap::iterator, bool>& result = dialogs.insert(std::pair<cstring, GameDialog*>(dialog->id.c_str(), dialog));
		if(!result.second)
			t.Throw("Dialog with that id already exists.");

		return true;
	}
	catch(Tokenizer::Exception& e)
	{
		Error("Failed to parse dialog '%s': %s", dialog->id.c_str(), e.ToString());
		delete dialog;
		return false;
	}
}

//=================================================================================================
bool LoadGlobals(Tokenizer& t, uint& errors)
{
	string type, name;

	try
	{
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
			{
				Error(Format("Invalid type for global variable '%s%s %s'.", type.c_str(), is_ref ? "@" : "", name.c_str()));
				++errors;
			}
			else if(result == ScriptManager::AlreadyExists)
			{
				Error(Format("Global variable with name '%s' already declared.", name.c_str()));
				++errors;
			}
		}
		t.Next();
		return true;
	}
	catch(Tokenizer::Exception& e)
	{
		Error("Failed to parse globals: %s", e.ToString());
		return false;
	}
}

//=================================================================================================
uint LoadDialogs(uint& out_crc, uint& errors)
{
	Tokenizer t;
	if(!t.FromFile(Format("%s/dialogs.txt", g_system_dir.c_str())))
		throw "Failed to open dialogs.txt.";

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

	Crc crc;

	try
	{
		t.Next();

		while(!t.IsEof())
		{
			bool skip = false;

			if(t.IsKeywordGroup(G_TOP))
			{
				if(t.IsKeyword(T_GLOBALS, G_TOP))
				{
					t.Next();
					if(!LoadGlobals(t, errors))
					{
						skip = true;
						++errors;
					}
				}
				else
				{
					t.Next();
					if(!LoadDialog(t, crc))
					{
						skip = true;
						++errors;
					}
				}
			}
			else
			{
				int group = G_TOP;
				Error(t.FormatUnexpected(tokenizer::T_KEYWORD_GROUP, &group));
				skip = true;
				++errors;
			}

			if(skip)
				t.SkipToKeywordGroup(G_TOP);
		}
	}
	catch(const Tokenizer::Exception& e)
	{
		Error("Failed to load dialogs: %s", e.ToString());
		++errors;
	}

	out_crc = crc.Get();
	return dialogs.size();
}

//=================================================================================================
void CheckDialogText(GameDialog* dialog, int index)
{
	GameDialog::Text& text = dialog->texts[index];
	string& str = dialog->strs[text.index];
	int prev_script = -1;

	size_t pos = 0;
	while(true)
	{
		size_t pos2 = str.find_first_of('$', pos);
		if(pos2 == string::npos)
			return;
		++pos2;
		if(str[pos2] == '(')
		{
			pos = str.find_first_of(')', pos2);
			if(pos == string::npos)
			{
				Error("Broken game dialog text: %s", str.c_str());
				text.formatted = false;
				return;
			}
			int str_index = (int)dialog->strs.size();
			dialog->strs.push_back(str.substr(pos2 + 1, pos - pos2 - 1));
			++pos;
			if(prev_script == -1)
			{
				int script_index = (int)dialog->scripts.size();
				dialog->scripts.push_back(GameDialog::Script(str_index, GameDialog::Script::STRING));
				text.formatted = true;
				text.script = script_index;
				prev_script = script_index;
			}
			else
			{
				int script_index = (int)dialog->scripts.size();
				dialog->scripts.push_back(GameDialog::Script(str_index, GameDialog::Script::STRING));
				dialog->scripts[prev_script].next = script_index;
				prev_script = script_index;
			}
		}
		else
		{
			pos = str.find_first_of('$', pos2);
			if(pos == string::npos)
			{
				Error("Broken game dialog text: %s", str.c_str());
				text.formatted = false;
				return;
			}
			text.formatted = true;
			++pos;
		}
	}
}

//=================================================================================================
bool LoadDialogText(Tokenizer& t)
{
	GameDialog* dialog = nullptr;

	try
	{
		const string& id = t.MustGetItemKeyword();
		dialog = FindDialog(id.c_str());
		if(!dialog)
			t.Throw("Missing dialog '%s'.", id.c_str());

		t.Next();
		t.AssertSymbol('{');
		t.Next();

		while(!t.IsSymbol('}'))
		{
			int index = t.MustGetInt();
			if(index < 0 || index >= dialog->max_index)
				t.Throw("Invalid text index %d.", index);
			t.Next();

			if(t.IsSymbol('{'))
			{
				t.Next();
				int prev = -1;
				while(!t.IsSymbol('}'))
				{
					int str_idx = dialog->strs.size();
					dialog->strs.push_back(t.MustGetString());
					t.Next();
					if(prev == -1)
					{
						dialog->texts[index].index = str_idx;
						dialog->texts[index].exists = true;
						prev = index;
					}
					else
					{
						index = dialog->texts.size();
						dialog->texts[prev].next = index;
						dialog->texts.push_back(GameDialog::Text(str_idx));
						prev = index;
					}
					CheckDialogText(dialog, index);
				}
			}
			else
			{
				int str_idx = dialog->strs.size();
				dialog->strs.push_back(t.MustGetString());
				dialog->texts[index].index = str_idx;
				dialog->texts[index].exists = true;
				CheckDialogText(dialog, index);
			}

			t.Next();
		}

		t.Next();

		bool ok = true;
		int index = 0;
		for(GameDialog::Text& t : dialog->texts)
		{
			if(!t.exists)
			{
				Error("Dialog '%s' is missing text for index %d.", dialog->id.c_str(), index);
				ok = false;
			}
			++index;
		}

		return ok;
	}
	catch(Tokenizer::Exception& e)
	{
		if(dialog)
			Error("Failed to load dialog '%s' texts: %s", dialog->id.c_str(), e.ToString());
		else
			Error("Failed to load dialog texts: %s", e.ToString());
		return false;
	}
}

//=================================================================================================
void LoadDialogTexts()
{
	Tokenizer t;
	cstring path = Format("%s/lang/%s/dialogs.txt", g_system_dir.c_str(), Language::prefix.c_str());
	Info("Reading text file \"%s\".", path);

	if(!t.FromFile(path))
	{
		Error("Failed to load language file '%s'.", path);
		return;
	}

	t.AddKeyword("dialog", 0);

	int errors = 0;

	try
	{
		t.Next();

		while(!t.IsEof())
		{
			bool skip = false;

			if(t.IsKeyword(0))
			{
				t.Next();

				if(!LoadDialogText(t))
				{
					skip = true;
					++errors;
				}
			}
			else
			{
				int id = 0;
				Error(t.FormatUnexpected(tokenizer::T_KEYWORD, &id));
				skip = true;
				++errors;
			}

			if(skip)
				t.SkipToKeywordGroup(Tokenizer::EMPTY_GROUP);
		}
	}
	catch(const Tokenizer::Exception& e)
	{
		Error("Failed to load dialogs: %s", e.ToString());
		++errors;
	}

	if(errors > 0)
		Error("Failed to load dialog texts (%d errors), check log for details.", errors);
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
					dialog_s_text += Game::Get().FormatString(*this, str_part);
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
GameDialog* FindDialog(cstring id)
{
	auto it = dialogs.find(id);
	if(it == dialogs.end())
		return nullptr;
	else
		return it->second;
}

//=================================================================================================
void CleanupDialogs()
{
	for(auto& it : dialogs)
		delete it.second;
}

//=================================================================================================
void VerifyDialogs(uint& errors)
{
	Info("Test: Checking dialogs...");

	string str_output;

	for(auto& it : dialogs)
	{
		GameDialog* dialog = it.second;

		// verify scripts
		for(auto& script : dialog->scripts)
		{
			cstring str = dialog->strs[script.index].c_str();
			bool ok;
			switch(script.type)
			{
			default:
				assert(0);
				break;
			case GameDialog::Script::NORMAL:
				ok = SM.RunScript(str, true);
				break;
			case GameDialog::Script::IF:
				ok = SM.RunIfScript(str, true);
				break;
			case GameDialog::Script::STRING:
				ok = SM.RunStringScript(str, str_output, true);
				break;
			}
			if(!ok)
				++errors;
		}
	}
}
