#include "Pch.h"
#include "QuestLoader.h"
#include "QuestScheme.h"
#include "QuestList.h"
#include "QuestConsts.h"
#include "QuestManager.h"
#include "DialogLoader.h"
#include "GameDialog.h"
#include "ScriptManager.h"
#include <angelscript.h>

enum Group
{
	G_TOP,
	G_PROPERTY,
	G_QUEST_CATEGORY,
	G_FLAGS
};

enum TopKeyword
{
	TK_QUEST,
	TK_QUEST_LIST
};

enum Property
{
	P_TYPE,
	P_PROGRESS,
	P_CODE,
	P_DIALOG,
	P_FLAGS
};

enum QuestKeyword
{
	QK_QUEST,
	QK_DIALOG,
	QK_TEXTS
};

//=================================================================================================
void QuestLoader::DoLoading()
{
	engine = script_mgr->GetEngine();
	module = engine->GetModule("Quests", asGM_CREATE_IF_NOT_EXISTS);

	Load("quests.txt", G_TOP);
}

//=================================================================================================
void QuestLoader::Cleanup()
{
	DeleteElements(QuestScheme::schemes);
	DeleteElements(QuestList::lists);
}

//=================================================================================================
void QuestLoader::InitTokenizer()
{
	t.AddKeywords(G_TOP, {
		{ "quest", TK_QUEST },
		{ "quest_list", TK_QUEST_LIST }
		});

	t.AddKeywords(G_PROPERTY, {
		{ "type", P_TYPE },
		{ "progress", P_PROGRESS },
		{ "code", P_CODE },
		{ "dialog", P_DIALOG },
		{ "flags", P_FLAGS }
		});

	t.AddKeywords<QuestCategory>(G_QUEST_CATEGORY, {
		{ "mayor", QuestCategory::Mayor },
		{ "captain", QuestCategory::Captain },
		{ "random", QuestCategory::Random },
		{ "unique", QuestCategory::Unique }
		});

	t.AddKeywords(G_FLAGS, {
		{ "dont_count", QuestScheme::DONT_COUNT }
		});
}

//=================================================================================================
void QuestLoader::LoadEntity(int top, const string& id)
{
	switch(top)
	{
	case TK_QUEST:
		ParseQuest(id);
		break;
	case TK_QUEST_LIST:
		ParseQuestList(id);
		break;
	}
}

//=================================================================================================
void QuestLoader::ParseQuest(const string& id)
{
	if(quest_mgr->FindQuest(id))
		t.Throw("Id must be unique.");

	Ptr<QuestScheme> quest;
	quest->id = id;
	quest->dialogs.push_back(new GameDialog);
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
	{
		Property p = (Property)t.MustGetKeywordId(G_PROPERTY);
		t.Next();
		switch(p)
		{
		case P_TYPE:
			if(quest->category != QuestCategory::NotSet)
				t.Throw("Quest type already set.");
			quest->category = (QuestCategory)t.MustGetKeywordId(G_QUEST_CATEGORY);
			break;
		case P_PROGRESS:
			if(!quest->progress.empty())
				t.Throw("Progress already declared.");
			t.AssertSymbol('{');
			t.Next();
			while(!t.IsSymbol('}'))
			{
				const string& progress_id = t.MustGetItem();
				int p = quest->GetProgress(progress_id);
				if(p != -1)
					t.Throw("Progress %s already set.", progress_id.c_str());
				quest->progress.push_back(progress_id);
				t.Next();
			}
			if(quest->progress.empty())
				t.Throw("Empty progress list.");
			break;
		case P_CODE:
			quest->code = t.GetBlock('{', '}', false);
			break;
		case P_DIALOG:
			{
				GameDialog* dialog = dialog_loader->LoadSingleDialog(t, quest);
				if(quest->GetDialog(dialog->id))
				{
					cstring str = Format("Quest dialog '%s' already exists.", dialog->id.c_str());
					delete dialog;
					t.Throw(str);
				}
				quest->dialogs.push_back(dialog);
			}
			break;
		case P_FLAGS:
			t.ParseFlags(G_FLAGS, quest->flags);
			break;
		}
		t.Next();
	}

	if(quest->category == QuestCategory::NotSet)
		t.Throw("Quest type not set.");

	quest_mgr->AddScriptedQuest(quest.Get());
	QuestScheme::schemes.push_back(quest.Pin());
}

//=================================================================================================
void QuestLoader::ParseQuestList(const string& id)
{
	if(QuestList::TryGet(id))
		t.Throw("Id must be unique.");

	Ptr<QuestList> list;
	list->id = id;
	list->total = 0;
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
	{
		QuestInfo* info;
		const string& quest_id = t.MustGetItemKeyword();
		if(quest_id == "none")
			info = nullptr;
		else
		{
			info = quest_mgr->FindQuest(quest_id);
			if(!info)
				t.Throw("Missing quest '%s'.", quest_id.c_str());
		}
		for(QuestList::Entry& e : list->entries)
		{
			if(e.info == info)
				t.Throw("Quest '%s' is already on list.", quest_id.c_str());
		}
		t.Next();

		int chance = t.MustGetInt();
		if(chance < 1)
			t.Throw("Invalid quest chance %d.", chance);
		t.Next();

		QuestList::Entry& entry = Add1(list->entries);
		entry.info = info;
		entry.chance = chance;
		list->total += chance;
	}

	if(list->entries.empty())
		t.Throw("Quest list can't be empty.");

	QuestList::lists.push_back(list.Pin());
}

//=================================================================================================
void QuestLoader::LoadTexts()
{
	Tokenizer t;
	cstring path = FormatLanguagePath("quests.txt");
	Info("Reading text file \"%s\".", path);

	if(!t.FromFile(path))
	{
		Error("Failed to load language file '%s'.", path);
		return;
	}

	t.AddKeywords(0, {
		{ "quest", QK_QUEST },
		{ "dialog", QK_DIALOG },
		{ "texts", QK_TEXTS }
		});

	int errors = 0;

	try
	{
		t.Next();

		while(!t.IsEof())
		{
			bool skip = false;

			t.AssertKeyword(QK_QUEST, 0);
			t.Next();

			const string& quest_id = t.MustGetItemKeyword();
			QuestScheme* scheme = QuestScheme::TryGet(quest_id);
			if(!scheme)
				t.Throw("Missing quest '%s'.", quest_id.c_str());
			t.Next();

			t.AssertSymbol('{');
			t.Next();

			while(!t.IsSymbol('}'))
			{
				if(t.IsKeyword(QK_DIALOG, 0))
				{
					t.Next();
					dialog_loader->LoadText(t, scheme);
				}
				else if(t.IsKeyword(QK_TEXTS, 0))
				{
					GameDialog* dialog = scheme->dialogs[0];
					t.Next();
					t.AssertSymbol('{');
					t.Next();
					while(!t.IsSymbol('}'))
					{
						int index = t.MustGetInt();
						if(index < 0)
							t.Throw("Invalid text index %d.", index);
						t.Next();

						if(dialog->texts.size() <= (uint)index)
							dialog->texts.resize(index + 1, GameDialog::Text());

						if(dialog->texts[index].exists)
							t.Throw("Text %d already set.", index);

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
								dialog_loader->CheckDialogText(dialog, index, &scheme->scripts);
							}
						}
						else
						{
							int str_idx = dialog->strs.size();
							dialog->strs.push_back(t.MustGetString());
							dialog->texts[index].index = str_idx;
							dialog->texts[index].exists = true;
							dialog_loader->CheckDialogText(dialog, index, &scheme->scripts);
						}
						t.Next();
					}
					t.Next();
				}
				else
				{
					int k1 = QK_DIALOG,
						k2 = QK_TEXTS,
						gr = 0;
					Error(t.StartUnexpected()
						.Add(tokenizer::T_KEYWORD, &k1, &gr)
						.Add(tokenizer::T_KEYWORD, &k2, &gr)
						.Get());
					skip = true;
					++errors;
					break;
				}
			}

			if(skip)
				t.SkipToKeyword(QK_QUEST);
			else
			{
				GameDialog* dialog = scheme->GetDialog("");
				if(dialog)
				{
					int index = 0;
					for(GameDialog::Text& t : dialog->texts)
					{
						if(!t.exists)
						{
							Error("Quest '%s' is missing text for index %d.", scheme->id.c_str(), index);
							++errors;
						}
						++index;
					}
				}
				t.Next();
			}
		}
	}
	catch(const Tokenizer::Exception& e)
	{
		Error("Failed to load quest texts: %s", e.ToString());
		++errors;
	}

	if(errors > 0)
		Error("Failed to load quest texts (%d errors), check log for details.", errors);
}

//=================================================================================================
void QuestLoader::Finalize()
{
	for(QuestScheme* scheme : QuestScheme::schemes)
		BuildQuest(scheme);

	int r = module->Build();
	if(r < 0)
	{
		Error("Failed to build quest scripts (%d).", r);
		++content.errors;
		return;
	}
	//content.crc[(int)Content::Id::Units] = crc.Get();

	for(QuestScheme* scheme : QuestScheme::schemes)
	{
		asITypeInfo* type = module->GetTypeInfoByName(Format("quest_%s", scheme->id.c_str()));
		scheme->script_type = type;
		scheme->f_startup = type->GetMethodByDecl("void Startup()");
		if(!scheme->f_startup)
		{
			scheme->f_startup = type->GetMethodByDecl("void Startup(Vars@)");
			scheme->startup_use_vars = true;
		}
		else
			scheme->startup_use_vars = false;
		scheme->f_progress = type->GetMethodByDecl("void SetProgress()");
		if(!scheme->f_progress)
		{
			scheme->f_progress = type->GetMethodByDecl("void SetProgress(int)");
			scheme->set_progress_use_prev = true;
		}
		else
			scheme->set_progress_use_prev = false;
		scheme->f_event = type->GetMethodByDecl("void OnEvent(Event@)");
		scheme->f_upgrade = type->GetMethodByDecl("void OnUpgrade(dictionary&)");
		scheme->scripts.Set(type);

		if(!scheme->f_progress)
		{
			Error("Missing quest '%s' SetProgress method.", scheme->id.c_str());
			++content.errors;
		}
		if(scheme->category != QuestCategory::Unique && !scheme->GetDialog("start"))
		{
			Error("Missing quest '%s' dialog 'start'.", scheme->id.c_str());
			++content.errors;
		}

		uint props = type->GetPropertyCount();
		for(uint i = 0; i < props; ++i)
		{
			int type_id;
			bool is_ref;
			type->GetProperty(i, nullptr, &type_id, nullptr, nullptr, nullptr, &is_ref);
			if(!script_mgr->CheckVarType(type_id, is_ref))
			{
				Error("Quest '%s' invalid property declaration '%s'.", scheme->id.c_str(), type->GetPropertyDeclaration(i));
				++content.errors;
			}
		}
	}

	Info("Loaded quests (%u), lists (%u).", QuestScheme::schemes.size(), QuestList::lists.size());
}

//=================================================================================================
void QuestLoader::BuildQuest(QuestScheme* scheme)
{
	code.clear();
	code = "//===============================================================\n";
	if(!scheme->progress.empty())
	{
		code += "// progress values\n";
		int index = 0;
		for(const string& str : scheme->progress)
		{
			code += Format("const int %s = %d;\n", str.c_str(), index);
			++index;
		}
		code += "\n";
	}
	LocalString str;
	code += Format("class quest_%s {\n int get_progress()property{return quest.progress;}\n void set_progress(int value)property{quest.SetProgress(value);}\n"
		" string TEXT(int index){return quest.GetString(index);}\n", scheme->id.c_str());
	for(int i = 0; i < DialogScripts::F_MAX; ++i)
	{
		scheme->scripts.GetFormattedCode((DialogScripts::FUNC)i, str.get_ref());
		if(!str.empty())
		{
			code += str.get_ref();
			code += '\n';
		}
	}
	code += "// inner code\n";
	code += scheme->code;
	code += "\n}";

#ifdef _DEBUG
	io::CreateDirectory("debug");
	TextWriter::WriteAll(Format("debug/quests_%s.txt", scheme->id.c_str()), code);
#endif

	CHECKED(module->AddScriptSection(scheme->id.c_str(), code.c_str()));
}
