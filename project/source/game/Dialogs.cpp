#include "Pch.h"
#include "GameCore.h"
#include "GameDialog.h"
#include "Game.h"
#include "Crc.h"
#include "Language.h"
#include "ScriptManager.h"
#include "Quest.h"

extern string g_system_dir;

//=================================================================================================
void CheckText(cstring text, bool talk2)
{
	bool have_format = (strchr(text, '$') != nullptr);
	if(talk2 != have_format)
		Warn("Invalid dialog type for text \"%s\".", text);
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
		dialog = GameDialog::TryGet(id.c_str());
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
