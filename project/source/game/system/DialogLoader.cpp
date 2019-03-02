#include "Pch.h"
#include "GameCore.h"
#include "DialogLoader.h"
#include "GameDialog.h"
#include "Item.h"
#include "ScriptManager.h"
#include "QuestScheme.h"

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

//=================================================================================================
void DialogLoader::DoLoading()
{
	quest = nullptr;
	scripts = &DialogScripts::global;
	bool require_id[2] = { true, false };
	Load("dialogs.txt", G_TOP, require_id);
}

//=================================================================================================
void DialogLoader::Cleanup()
{
	GameDialog::Cleanup();
}

//=================================================================================================
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

//=================================================================================================
void DialogLoader::LoadEntity(int top, const string& id)
{
	if(top == T_DIALOG)
	{
		GameDialog* dialog = LoadDialog(id);
		std::pair<GameDialog::Map::iterator, bool>& result = GameDialog::dialogs.insert(std::pair<cstring, GameDialog*>(dialog->id.c_str(), dialog));
		if(!result.second)
		{
			delete dialog;
			t.Throw("Dialog with that id already exists.");
		}
	}
	else
		LoadGlobals();
}

//=================================================================================================
GameDialog* DialogLoader::LoadDialog(const string& id)
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
			if(if_state.empty())
				break;
			t.Next();
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
					dialog->code.push_back(DTF_ELSE);
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
					dialog->code.push_back(DTF_END_IF);
					if_state.pop_back();
					crc.Update(DTF_END_IF);
				}
				break;
			case IFS_ELSE:
				dialog->code.push_back(DTF_END_IF);
				if_state.pop_back();
				crc.Update(DTF_END_IF);
				break;
			case IFS_CHOICE:
				dialog->code.push_back(DTF_END_CHOICE);
				if_state.pop_back();
				crc.Update(DTF_END_CHOICE);
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
					bool escape = false;
					if(k == K_ESCAPE)
					{
						escape = true;
						t.AssertKeyword(K_CHOICE, G_KEYWORD);
						t.Next();
					}

					int index = t.MustGetInt();
					if(index < 0)
						t.Throw("Invalid text index %d.", index);
					t.Next();
					DialogEntry entry(DTF_CHOICE, index);
					if(escape)
						entry.op = OP_ESCAPE;
					dialog->code.push_back(entry);
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
					crc.Update(entry.op);
					crc.Update(index);
				}
				break;
			case K_TRADE:
				dialog->code.push_back(DTF_TRADE);
				crc.Update(DTF_TRADE);
				break;
			case K_TALK:
				{
					int index = t.MustGetInt();
					if(index < 0)
						t.Throw("Invalid text index %d.", index);
					t.Next();
					dialog->code.push_back(DialogEntry(DTF_TALK, index));
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
				dialog->code.push_back(DTF_RESTART);
				crc.Update(DTF_RESTART);
				break;
			case K_END:
				dialog->code.push_back(DTF_END);
				crc.Update(DTF_END);
				break;
			case K_END2:
				dialog->code.push_back(DTF_END2);
				crc.Update(DTF_END2);
				break;
			case K_SHOW_CHOICES:
				dialog->code.push_back(DTF_SHOW_CHOICES);
				crc.Update(DTF_SHOW_CHOICES);
				break;
			case K_SPECIAL:
				{
					int index = dialog->strs.size();
					dialog->strs.push_back(t.MustGetString());
					t.Next();
					dialog->code.push_back(DialogEntry(DTF_SPECIAL, index));
					crc.Update(DTF_SPECIAL);
					crc.Update(dialog->strs.back());
				}
				break;
			case K_SET_QUEST_PROGRESS:
				{
					int p = ParseProgress();
					dialog->code.push_back(DialogEntry(DTF_SET_QUEST_PROGRESS, p));
					crc.Update(DTF_SET_QUEST_PROGRESS);
					crc.Update(p);
				}
				break;
			case K_IF:
				{
					k = (Keyword)t.MustGetKeywordId(G_KEYWORD);
					t.Next();

					bool not = false;
					if(k == K_NOT)
					{
						not = true;
						k = (Keyword)t.MustGetKeywordId(G_KEYWORD);
						t.Next();
					}

					switch(k)
					{
					case K_QUEST_TIMEOUT:
						dialog->code.push_back(DTF_IF_QUEST_TIMEOUT);
						crc.Update(DTF_IF_QUEST_TIMEOUT);
						break;
					case K_RAND:
						{
							int chance = t.MustGetInt();
							if(chance <= 0 || chance >= 100)
								t.Throw("Invalid chance %d.", chance);
							t.Next();
							dialog->code.push_back(DialogEntry(DTF_IF_RAND, chance));
							crc.Update(DTF_IF_RAND);
							crc.Update(chance);
						}
						break;
					case K_HAVE_QUEST_ITEM:
						if(quest)
						{
							dialog->code.push_back(DTF_IF_HAVE_QUEST_ITEM_CURRENT);
							crc.Update(DTF_IF_HAVE_QUEST_ITEM_CURRENT);
						}
						else
						{
							if(t.IsKeyword(K_NOT_ACTIVE, G_KEYWORD))
							{
								t.Next();
								dialog->code.push_back(DTF_NOT_ACTIVE);
								crc.Update(DTF_NOT_ACTIVE);
							}
							int index = dialog->strs.size();
							dialog->strs.push_back(t.MustGetString());
							t.Next();
							dialog->code.push_back(DialogEntry(DTF_IF_HAVE_QUEST_ITEM, index));
							crc.Update(DTF_IF_HAVE_QUEST_ITEM);
							crc.Update(dialog->strs.back());
						}
						break;
					case K_QUEST_PROGRESS:
						{
							DialogOp op = ParseOp();
							int p = ParseProgress();
							DialogEntry entry(DTF_IF_QUEST_PROGRESS, p);
							entry.op = op;
							dialog->code.push_back(entry);
							crc.Update(DTF_IF_QUEST_PROGRESS);
							crc.Update(p);
						}
						break;
					case K_NEED_TALK:
						{
							int index = dialog->strs.size();
							dialog->strs.push_back(t.MustGetString());
							t.Next();
							dialog->code.push_back(DialogEntry(DTF_IF_NEED_TALK, index));
							crc.Update(DTF_IF_NEED_TALK);
							crc.Update(dialog->strs.back());
						}
						break;
					case K_SPECIAL:
						{
							int index = dialog->strs.size();
							dialog->strs.push_back(t.MustGetString());
							t.Next();
							dialog->code.push_back(DialogEntry(DTF_IF_SPECIAL, index));
							crc.Update(DTF_IF_SPECIAL);
							crc.Update(dialog->strs.back());
						}
						break;
					case K_ONCE:
						dialog->code.push_back(DTF_IF_ONCE);
						crc.Update(DTF_IF_ONCE);
						break;
					case K_HAVE_ITEM:
						{
							const string& id = t.MustGetItemKeyword();
							const Item* item = Item::TryGet(id);
							if(item)
							{
								t.Next();
								dialog->code.push_back(DialogEntry(DTF_IF_HAVE_ITEM, (int)item));
							}
							else
								t.Throw("Invalid item '%s'.", id.c_str());
							crc.Update(DTF_SPECIAL);
							crc.Update(item->id);
						}
						break;
					case K_QUEST_EVENT:
						dialog->code.push_back(DTF_IF_QUEST_EVENT);
						crc.Update(DTF_IF_QUEST_EVENT);
						break;
					case K_QUEST_PROGRESS_RANGE:
						{
							int a = ParseProgress();
							int b = ParseProgress();
							if(a < 0 || a >= b)
								t.Throw("Invalid quest progress range {%d %d}.", a, b);
							int p = ((a & 0xFFFF) | ((b & 0xFFFF) << 16));
							dialog->code.push_back(DialogEntry(DTF_IF_QUEST_PROGRESS_RANGE, p));
							crc.Update(DTF_IF_QUEST_PROGRESS_RANGE);
							crc.Update(p);
						}
						break;
					case K_CHOICES:
						{
							DialogOp op = ParseOp();
							int count = t.MustGetInt();
							if(count < 0)
								t.Throw("Invalid choices count %d.", count);
							t.Next();
							DialogEntry entry(DTF_IF_CHOICES, count);
							entry.op = op;
							dialog->code.push_back(entry);
							crc.Update(DTF_IF_CHOICES);
							crc.Update(count);
						}
						break;
					case K_QUEST_SPECIAL:
						{
							int index = dialog->strs.size();
							dialog->strs.push_back(t.MustGetString());
							t.Next();
							dialog->code.push_back(DialogEntry(DTF_IF_QUEST_SPECIAL, index));
							crc.Update(DTF_IF_QUEST_SPECIAL);
							crc.Update(dialog->strs.back());
						}
						break;
					case K_SCRIPT:
						{
							int index = scripts->AddCode(DialogScripts::F_IF_SCRIPT, t.MustGetString());
							t.Next();
							dialog->code.push_back(DialogEntry(DTF_IF_SCRIPT, index));
							crc.Update(DTF_IF_SCRIPT);
							crc.Update(index);
						}
						break;
					default:
						t.Unexpected();
						break;
					}

					DialogEntry& entry = dialog->code.back();
					if(not)
					{
						if(entry.type == DTF_IF_QUEST_PROGRESS || entry.type == DTF_IF_CHOICES)
						{
							switch(entry.op)
							{
							case OP_EQUAL:
								entry.op = OP_NOT_EQUAL;
								break;
							case OP_NOT_EQUAL:
								entry.op = OP_EQUAL;
								break;
							case OP_GREATER:
								entry.op = OP_LESS_EQUAL;
								break;
							case OP_GREATER_EQUAL:
								entry.op = OP_LESS;
								break;
							case OP_LESS:
								entry.op = OP_GREATER_EQUAL;
								break;
							case OP_LESS_EQUAL:
								entry.op = OP_LESS;
								break;
							}
						}
						else
							entry.op = OP_NOT_EQUAL;
					}
					crc.Update(entry.op);

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
					dialog->code.push_back(DialogEntry(DTF_CHECK_QUEST_TIMEOUT, type));
					crc.Update(DTF_CHECK_QUEST_TIMEOUT);
					crc.Update(type);
				}
				break;
			case K_DO_QUEST:
				{
					int index = dialog->strs.size();
					dialog->strs.push_back(t.MustGetString());
					t.Next();
					dialog->code.push_back(DialogEntry(DTF_DO_QUEST, index));
					crc.Update(DTF_DO_QUEST);
					crc.Update(dialog->strs.back());
				}
				break;
			case K_DO_QUEST_ITEM:
				{
					int index = dialog->strs.size();
					dialog->strs.push_back(t.MustGetString());
					t.Next();
					dialog->code.push_back(DialogEntry(DTF_DO_QUEST_ITEM, index));
					crc.Update(DTF_DO_QUEST_ITEM);
					crc.Update(dialog->strs.back());
				}
				break;
			case K_DO_QUEST2:
				{
					int index = dialog->strs.size();
					dialog->strs.push_back(t.MustGetString());
					t.Next();
					dialog->code.push_back(DialogEntry(DTF_DO_QUEST2, index));
					crc.Update(DTF_DO_QUEST2);
					crc.Update(dialog->strs.back());
				}
				break;
			case K_DO_ONCE:
				dialog->code.push_back(DTF_DO_ONCE);
				crc.Update(DTF_DO_ONCE);
				break;
			case K_QUEST_SPECIAL:
				{
					int index = dialog->strs.size();
					dialog->strs.push_back(t.MustGetString());
					t.Next();
					dialog->code.push_back(DialogEntry(DTF_QUEST_SPECIAL, index));
					crc.Update(DTF_QUEST_SPECIAL);
					crc.Update(dialog->strs.back());
				}
				break;
			case K_SCRIPT:
				{
					int index = scripts->AddCode(DialogScripts::F_SCRIPT, t.MustGetString());
					t.Next();
					dialog->code.push_back(DialogEntry(DTF_SCRIPT, index));
					crc.Update(DTF_SCRIPT);
					crc.Update(index);
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
						dialog->code.push_back(DTF_ELSE);
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
						dialog->code.push_back(DTF_END_IF);
						if_state.pop_back();
					}
					break;
				case IFS_INLINE_ELSE:
					dialog->code.push_back(DTF_END_IF);
					if_state.pop_back();
					crc.Update(DTF_END_IF);
					break;
				case IFS_INLINE_CHOICE:
					dialog->code.push_back(DTF_END_CHOICE);
					crc.Update(DTF_END_CHOICE);
					if_state.pop_back();
					break;
				}

				if(b)
					break;
			}
		}
	}

	return dialog.Pin();
}

//=================================================================================================
DialogOp DialogLoader::ParseOp()
{
	if(t.IsSymbol())
	{
		switch(t.GetSymbol())
		{
		case '>':
			t.Next();
			if(!t.IsSymbol())
				return OP_GREATER;
			else if(t.IsSymbol('='))
			{
				t.Next();
				return OP_GREATER_EQUAL;
			}
			else
				t.Unexpected();
			break;
		case '<':
			t.Next();
			if(!t.IsSymbol())
				return OP_LESS;
			else if(t.IsSymbol('='))
			{
				t.Next();
				return OP_LESS_EQUAL;
			}
			else
				t.Unexpected();
			break;
		case '=':
			t.Next();
			if(t.IsSymbol('='))
			{
				t.Next();
				return OP_EQUAL;
			}
			else
				t.Unexpected();
			break;
		case '!':
			t.Next();
			if(t.IsSymbol('='))
			{
				t.Next();
				return OP_NOT_EQUAL;
			}
			else
				t.Unexpected();
			break;
		default:
			t.Unexpected();
			break;
		}
	}
	return OP_EQUAL;
}

//=================================================================================================
int DialogLoader::ParseProgress()
{
	if(t.IsItem())
	{
		const string& progress = t.GetItem();
		if(quest)
		{
			int p = quest->GetProgress(progress);
			if(p != -1)
			{
				t.Next();
				return p;
			}
		}
		t.Throw("Invalid quest progress '%s'.", progress.c_str());
	}
	else if(t.IsInt())
	{
		int p = t.GetInt();
		if(p < 0)
			t.Throw("Invalid quest progress '%d'.", p);
		t.Next();
		return p;
	}
	else
		t.ThrowExpecting("quest progress value");
}

//=================================================================================================
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

//=================================================================================================
void DialogLoader::Finalize()
{
	DialogScripts::global.Build();

	content.crc[(int)Content::Id::Dialogs] = crc.Get();
	Info("Loaded dialogs (%u) - crc %p.", GameDialog::dialogs.size(), content.crc[(int)Content::Id::Dialogs]);
}

//=================================================================================================
GameDialog* DialogLoader::LoadSingleDialog(Tokenizer& parent_t, QuestScheme* quest)
{
	this->quest = quest;
	scripts = &quest->scripts;
	t.FromTokenizer(parent_t);
	const string& id = t.MustGetItem();
	GameDialog* dialog = LoadDialog(id);
	parent_t.MoveTo(t.GetPos());
	return dialog;
}

//=================================================================================================
void DialogLoader::LoadTexts()
{
	Tokenizer t;
	cstring path = FormatLanguagePath("dialogs.txt");
	Info("Reading text file \"%s\".", path);

	if(!t.FromFile(path))
	{
		LoadError("Failed to load language file '%s'.", path);
		return;
	}

	t.AddKeyword("dialog", 0);

	try
	{
		t.Next();

		while(!t.IsEof())
		{
			bool skip = false;

			if(t.IsKeyword(0))
			{
				t.Next();

				if(!LoadText(t))
					skip = true;
			}
			else
			{
				int id = 0;
				LoadError(t.FormatUnexpected(tokenizer::T_KEYWORD, &id));
				skip = true;
			}

			if(skip)
				t.SkipToKeywordGroup(Tokenizer::EMPTY_GROUP);
		}
	}
	catch(const Tokenizer::Exception& e)
	{
		LoadError("Failed to load dialogs: %s", e.ToString());
	}
}

//=================================================================================================
bool DialogLoader::LoadText(Tokenizer& t, QuestScheme* scheme)
{
	GameDialog* dialog = nullptr;

	try
	{
		DialogScripts* scripts;
		const string& id = t.MustGetItemKeyword();
		if(scheme)
		{
			dialog = scheme->GetDialog(id);
			scripts = &scheme->scripts;
		}
		else
		{
			dialog = GameDialog::TryGet(id.c_str());
			scripts = &DialogScripts::global;
		}
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
					CheckDialogText(dialog, index, scripts);
				}
			}
			else
			{
				int str_idx = dialog->strs.size();
				dialog->strs.push_back(t.MustGetString());
				dialog->texts[index].index = str_idx;
				dialog->texts[index].exists = true;
				CheckDialogText(dialog, index, scripts);
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
				LoadError("Dialog '%s' is missing text for index %d.", dialog->id.c_str(), index);
				ok = false;
			}
			++index;
		}

		return ok;
	}
	catch(Tokenizer::Exception& e)
	{
		if(dialog)
			LoadError("Failed to load dialog '%s' texts: %s", dialog->id.c_str(), e.ToString());
		else
			LoadError("Failed to load dialog texts: %s", e.ToString());
		return false;
	}
}

//=================================================================================================
void DialogLoader::CheckDialogText(GameDialog* dialog, int index, DialogScripts* scripts)
{
	GameDialog::Text& text = dialog->texts[index];
	string& str = dialog->strs[text.index];
	LocalString result;
	bool replaced = false;

	size_t pos = 0;
	while(true)
	{
		size_t pos2 = str.find_first_of('$', pos);
		if(pos2 == string::npos)
		{
			result += str.substr(pos);
			break;
		}
		result += str.substr(pos, pos2 - pos);
		++pos2;
		if(str.at(pos2) == '(')
		{
			pos = FindClosingPos(str, pos2);
			if(pos == string::npos)
			{
				Error("Broken game dialog text: %s", str.c_str());
				text.formatted = false;
				return;
			}
			string code = str.substr(pos2 + 1, pos - pos2 - 1);
			int index = scripts->AddCode(DialogScripts::F_FORMAT, code);
			result += Format("$(%d)", index);
			replaced = true;
			text.formatted = true;
			++pos;
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
			result += str.substr(pos2 - 1, pos - pos2 + 1);
		}
	}

	if(replaced)
		str = result;
}
