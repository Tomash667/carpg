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
	K_SCRIPT,
	K_BETWEEN,
	K_AND
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
		{ "script", K_SCRIPT },
		{ "between", K_BETWEEN },
		{ "and", K_AND }
		});
}

//=================================================================================================
void DialogLoader::LoadEntity(int top, const string& id)
{
	if(top == T_DIALOG)
	{
		GameDialog* dialog = LoadDialog(id);
		pair<GameDialog::Map::iterator, bool>& result = GameDialog::dialogs.insert(pair<cstring, GameDialog*>(dialog->id.c_str(), dialog));
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
	current_dialog = dialog.Get();
	dialog->max_index = -1;
	dialog->id = id;
	dialog->quest = nullptr;
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	Pooled<Node> root(GetNode());
	root->node_op = NodeOp::Block;

	while(true)
	{
		if(t.IsSymbol('}'))
			break;
		root->childs.push_back(ParseStatement());
	}

	if(!BuildDialog(root.Get()))
		LoadWarning("Missing dialog end.");
	dialog->code.push_back(DialogEntry(DTF_END_OF_DIALOG));

	return dialog.Pin();
}

//=================================================================================================
DialogLoader::Node* DialogLoader::ParseStatement()
{
	Keyword k = (Keyword)t.MustGetKeywordId(G_KEYWORD);
	switch(k)
	{
	case K_IF:
		return ParseIf();
	case K_CHOICE:
	case K_ESCAPE:
		return ParseChoice();
	case K_DO_ONCE:
	case K_END:
	case K_END2:
	case K_RESTART:
	case K_SHOW_CHOICES:
	case K_TRADE:
		{
			t.Next();
			DialogType type;
			switch(k)
			{
			default:
			case K_DO_ONCE:
				type = DTF_DO_ONCE;
				break;
			case K_END:
				type = DTF_END;
				break;
			case K_END2:
				type = DTF_END2;
				break;
			case K_RESTART:
				type = DTF_RESTART;
				break;
			case K_SHOW_CHOICES:
				type = DTF_SHOW_CHOICES;
				break;
			case K_TRADE:
				type = DTF_TRADE;
				break;
			}
			Node* node = GetNode();
			node->node_op = NodeOp::Statement;
			node->type = type;
			node->op = OP_NONE;
			node->value = 0;
			return node;
		}
	case K_CHECK_QUEST_TIMEOUT:
		{
			t.Next();
			int type = t.MustGetInt();
			if(type < 0 || type > 2)
				t.Throw("Invalid quest type %d.", type);
			t.Next();
			Node* node = GetNode();
			node->node_op = NodeOp::Statement;
			node->type = DTF_CHECK_QUEST_TIMEOUT;
			node->op = OP_NONE;
			node->value = type;
			return node;
		}
	case K_SET_QUEST_PROGRESS:
		{
			t.Next();
			int value = ParseProgress();
			Node* node = GetNode();
			node->node_op = NodeOp::Statement;
			node->type = DTF_SET_QUEST_PROGRESS;
			node->op = OP_NONE;
			node->value = value;
			return node;
		}
	case K_TALK:
		{
			t.Next();
			int index = t.MustGetInt();
			if(index < 0)
				t.Throw("Invalid text index %d.", index);
			t.Next();
			Node* node = GetNode();
			node->node_op = NodeOp::Statement;
			node->type = DTF_TALK;
			node->op = OP_NONE;
			node->value = index;
			++index;
			if(index > current_dialog->max_index)
			{
				current_dialog->texts.resize(index, GameDialog::Text());
				current_dialog->max_index = index;
			}
			current_dialog->texts[index - 1].exists = true;
			return node;
		}
	case K_DO_QUEST:
	case K_DO_QUEST2:
	case K_DO_QUEST_ITEM:
	case K_QUEST_SPECIAL:
	case K_SPECIAL:
		{
			t.Next();
			DialogType type;
			switch(k)
			{
			default:
			case K_DO_QUEST:
				type = DTF_DO_QUEST;
				break;
			case K_DO_QUEST2:
				type = DTF_DO_QUEST2;
				break;
			case K_DO_QUEST_ITEM:
				type = DTF_DO_QUEST_ITEM;
				break;
			case K_QUEST_SPECIAL:
				type = DTF_QUEST_SPECIAL;
				break;
			case K_SPECIAL:
				type = DTF_SPECIAL;
				break;
			}
			int index = current_dialog->strs.size();
			current_dialog->strs.push_back(t.MustGetString());
			t.Next();
			Node* node = GetNode();
			node->node_op = NodeOp::Statement;
			node->type = type;
			node->op = OP_NONE;
			node->value = index;
			return node;
		}
	case K_SCRIPT:
		{
			t.Next();
			int index = scripts->AddCode(DialogScripts::F_SCRIPT, t.MustGetString());
			t.Next();
			Node* node = GetNode();
			node->node_op = NodeOp::Statement;
			node->type = DTF_SCRIPT;
			node->op = OP_NONE;
			node->value = index;
			return node;
		}
	default:
		t.Unexpected();
		break;
	}
}

//=================================================================================================
DialogLoader::Node* DialogLoader::ParseBlock()
{
	if(t.IsSymbol('{'))
	{
		t.Next();
		Pooled<Node> block(GetNode());
		while(!t.IsSymbol('}'))
			block->childs.push_back(ParseStatement());
		t.Next();
		if(block->childs.size() == 1u)
		{
			Node* node = block->childs[0];
			block->childs.clear();
			return node;
		}
		else
		{
			block->node_op = NodeOp::Block;
			return block.Pin();
		}
	}
	else
		return ParseStatement();
}

//=================================================================================================
DialogLoader::Node* DialogLoader::ParseIf()
{
	Pooled<Node> node(GetNode());
	node->node_op = NodeOp::If;
	t.Next();

	bool not = false;
	if(t.IsKeyword(K_NOT, G_KEYWORD))
	{
		t.Next();
		not = true;
	}

	Keyword k = (Keyword)t.MustGetKeywordId(G_KEYWORD);
	switch(k)
	{
	case K_HAVE_ITEM:
	case K_HAVE_QUEST_ITEM:
	case K_NEED_TALK:
	case K_ONCE:
	case K_QUEST_EVENT:
	case K_QUEST_SPECIAL:
	case K_QUEST_TIMEOUT:
	case K_RAND:
	case K_SCRIPT:
	case K_SPECIAL:
		{
			t.Next();
			node->op = OP_EQUAL;
			node->value = 0;
			switch(k)
			{
			case K_HAVE_ITEM:
				{
					node->type = DTF_IF_HAVE_ITEM;
					const string& id = t.MustGetItemKeyword();
					const Item* item = Item::TryGet(id);
					if(!item)
						t.Throw("Invalid item '%s'.", id.c_str());
					node->value = (int)item;
					t.Next();
				}
				break;
			case K_HAVE_QUEST_ITEM:
				if(quest)
					node->type = DTF_IF_HAVE_QUEST_ITEM_CURRENT;
				else
				{
					if(t.IsKeyword(K_NOT_ACTIVE, G_KEYWORD))
					{
						node->type = DTF_IF_HAVE_QUEST_ITEM_NOT_ACTIVE;
						t.Next();
					}
					else
						node->type = DTF_IF_HAVE_QUEST_ITEM;
					node->value = current_dialog->strs.size();
					current_dialog->strs.push_back(t.MustGetString());
					t.Next();
				}
				break;
			case K_NEED_TALK:
				node->type = DTF_IF_NEED_TALK;
				node->value = current_dialog->strs.size();
				current_dialog->strs.push_back(t.MustGetString());
				t.Next();
				break;
			case K_ONCE:
				node->type = DTF_IF_ONCE;
				break;
			case K_QUEST_EVENT:
				node->type = DTF_IF_QUEST_EVENT;
				break;
			case K_QUEST_SPECIAL:
				{
					int index = current_dialog->strs.size();
					current_dialog->strs.push_back(t.MustGetString());
					node->type = DTF_IF_QUEST_SPECIAL;
					node->value = index;
					t.Next();
				}
				break;
			case K_QUEST_TIMEOUT:
				node->type = DTF_IF_QUEST_TIMEOUT;
				break;
			case K_RAND:
				{
					node->type = DTF_IF_RAND;
					int chance = t.MustGetInt();
					if(chance <= 0 || chance >= 100)
						t.Throw("Invalid chance %d.", chance);
					node->value = chance;
					t.Next();
				}
				break;
			case K_SCRIPT:
				node->type = DTF_IF_SCRIPT;
				node->value = scripts->AddCode(DialogScripts::F_IF_SCRIPT, t.MustGetString());
				t.Next();
				break;
			case K_SPECIAL:
				{
					int index = current_dialog->strs.size();
					current_dialog->strs.push_back(t.MustGetString());
					node->type = DTF_IF_SPECIAL;
					node->value = index;
					t.Next();
				}
				break;
			}
		}
		break;
	case K_CHOICES:
	case K_QUEST_PROGRESS:
		{
			t.Next();
			node->type = (k == K_CHOICES ? DTF_IF_CHOICES : DTF_IF_QUEST_PROGRESS);
			if(t.IsKeyword(K_BETWEEN, G_KEYWORD))
			{
				node->op = OP_BETWEEN;
				t.Next();
				int a = ParseProgressOrInt(k);
				t.AssertKeyword(K_AND, G_KEYWORD);
				t.Next();
				int b = ParseProgressOrInt(k);
				if(a >= b || a >= std::numeric_limits<word>::max() || b >= std::numeric_limits<word>::max())
					t.Throw("Invalid range values %d and %d.", a, b);
				node->value = (a | (b << 16));
			}
			else
			{
				node->op = ParseOp();
				node->value = ParseProgressOrInt(k);
			}
		}
		break;
	default:
		t.Unexpected();
		break;
	}

	if(not)
		node->op = GetNegatedOp(node->op);

	node->childs.push_back(ParseBlock());
	if(t.IsKeyword(K_ELSE, G_KEYWORD))
	{
		t.Next();
		node->childs.push_back(ParseBlock());
	}
	return node.Pin();
}

//=================================================================================================
DialogLoader::Node* DialogLoader::ParseChoice()
{
	bool escape = false;
	if(t.IsKeyword(K_ESCAPE, G_KEYWORD))
	{
		escape = true;
		t.Next();
	}

	t.AssertKeyword(K_CHOICE, G_KEYWORD);
	t.Next();

	int index = t.MustGetInt();
	if(index < 0)
		t.Throw("Invalid text index %d.", index);
	Pooled<Node> node(GetNode());
	node->node_op = NodeOp::Choice;
	node->type = DTF_CHOICE;
	node->op = (escape ? OP_ESCAPE : OP_NONE);
	node->value = index;
	t.Next();

	++index;
	if(index > current_dialog->max_index)
	{
		current_dialog->texts.resize(index, GameDialog::Text());
		current_dialog->max_index = index;
	}
	current_dialog->texts[index - 1].exists = true;

	node->childs.push_back(ParseBlock());
	return node.Pin();
}

//=================================================================================================
DialogOp DialogLoader::GetNegatedOp(DialogOp op)
{
	switch(op)
	{
	case OP_EQUAL:
		return OP_NOT_EQUAL;
	case OP_NOT_EQUAL:
		return OP_EQUAL;
	case OP_GREATER:
		return OP_LESS_EQUAL;
	case OP_GREATER_EQUAL:
		return OP_LESS;
	case OP_LESS:
		return OP_GREATER_EQUAL;
	case OP_LESS_EQUAL:
		return OP_GREATER;
	case OP_BETWEEN:
		return OP_NOT_BETWEEN;
	case OP_NOT_BETWEEN:
		return OP_BETWEEN;
	default:
		assert(0);
		return OP_EQUAL;
	}
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
int DialogLoader::ParseProgressOrInt(int keyword)
{
	if(keyword == K_CHOICES)
	{
		uint val = t.MustGetUint();
		t.Next();
		return val;
	}
	else
		return ParseProgress();
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
bool DialogLoader::BuildDialog(Node* node)
{
	vector<DialogEntry>& code = current_dialog->code;
	switch(node->node_op)
	{
	case NodeOp::Statement:
		code.push_back(DialogEntry(node->type, node->op, node->value));
		return Any(node->type, DTF_END, DTF_END2, DTF_RESTART, DTF_TRADE, DTF_SHOW_CHOICES);
	case NodeOp::Block:
		return BuildDialogBlock(node);
	case NodeOp::If:
		if(node->childs.size() == 1u)
		{
			// if
			code.push_back(DialogEntry(node->type, node->op, node->value));
			uint jmp_pos = code.size();
			code.push_back(DialogEntry(DTF_CJMP));
			BuildDialog(node->childs[0]);
			uint pos = code.size();
			code[jmp_pos].value = pos;
			return false;
		}
		else
		{
			// if else
			code.push_back(DialogEntry(node->type, node->op, node->value));
			uint jmp_pos = code.size();
			code.push_back(DialogEntry(DTF_CJMP));
			bool result = BuildDialog(node->childs[0]);
			uint jmp_end_pos = code.size();
			code.push_back(DialogEntry(DTF_JMP));
			uint pos = code.size();
			code[jmp_pos].value = pos;
			result = BuildDialog(node->childs[1]) && result;
			pos = code.size();
			code[jmp_end_pos].value = pos;
			return result;
		}
	case NodeOp::Choice:
		{
			code.push_back(DialogEntry(node->type, node->op, node->value));
			uint jmp_pos = code.size();
			code.push_back(DialogEntry(DTF_JMP));
			if(!BuildDialogBlock(node))
			{
#ifdef _DEBUG
				LoadWarning("Missing choice end at line %d.", node->line);
#else
				LoadWarning("Missing choice end.");
#endif
			}
			uint pos = code.size();
			code[jmp_pos].value = pos;
			return false;
		}
	default:
		assert(0);
		return false;
	}
}

//=================================================================================================
bool DialogLoader::BuildDialogBlock(Node* node)
{
	int have_exit = 0;
	for(Node* child : node->childs)
	{
		if(have_exit == 1)
		{
			have_exit = 2;
#ifdef _DEBUG
			LoadWarning("Unreachable code found at line %d.", child->line);
#else
			LoadWarning("Unreachable code found.");
#endif
		}
		bool result = BuildDialog(child);
		if(result && have_exit == 0)
			have_exit = 1;
	}
	return (have_exit >= 1);
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
		ScriptManager::RegisterResult result = script_mgr->RegisterGlobalVar(type, is_ref, name);
		if(result == ScriptManager::InvalidType)
			LoadError("Invalid type for global variable '%s%s %s'.", type.c_str(), is_ref ? "@" : "", name.c_str());
		else if(result == ScriptManager::AlreadyExists)
			LoadError("Global variable with name '%s' already declared.", name.c_str());
	}
}

//=================================================================================================
void DialogLoader::Finalize()
{
	content.errors += DialogScripts::global.Build();

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
	SetLocalId(id);
	GameDialog* dialog = LoadDialog(id);
	dialog->quest = quest;
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
