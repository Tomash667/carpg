#include "Pch.h"
#include "Base.h"
#include "Game.h"

extern string g_system_dir;

enum RequiredType
{
	R_ITEM,
	R_UNIT,
	R_SPELL
};

void Game::LoadRequiredStats()
{
	Tokenizer t;
	if(!t.FromFile(Format("%s/required.txt", g_system_dir.c_str())))
		throw "Failed to open required.txt.";

	t.AddKeywords(0, {
		{ "item", R_ITEM },
		{ "unit", R_UNIT },
		{ "spell", R_SPELL }
	});

	int errors = 0;

	try
	{
		t.Next();

		while(!t.IsEof())
		{
			try
			{
				RequiredType type = (RequiredType)t.MustGetKeywordId(0);
				t.Next();

				const string& str = t.MustGetItemKeyword();

				switch(type)
				{
				case R_ITEM:
					{
						ItemListResult result;
						const Item* item = FindItem(str.c_str(), false, &result);
						if(!item)
						{
							ERROR(Format("Missing required item '%s'.", str.c_str()));
							++errors;
						}
						else if(result.lis)
						{
							ERROR(Format("Required item '%s' is list.", str.c_str()));
							++errors;
						}
					}
					break;
				case R_UNIT:
					{
						UnitData* ud = FindUnitData(str.c_str(), false);
						if(!ud)
						{
							ERROR(Format("Missing required unit '%s'.", str.c_str()));
							++errors;
						}
					}
					break;
				case R_SPELL:
					{
						Spell* spell = FindSpell(str.c_str());
						if(!spell)
						{
							ERROR(Format("Missing required spell '%s'.", str.c_str()));
							++errors;
						}
					}
					break;
				}

				t.Next();
			}
			catch(const Tokenizer::Exception& e)
			{
				ERROR(Format("Parse error: %s", e.ToString()));
				++errors;
				t.SkipToKeywordGroup(0);
			}
		}
	}
	catch(const Tokenizer::Exception& e)
	{
		ERROR(Format("Failed to load required entities: %s", e.ToString()));
		++errors;
	}

	if(errors > 0)
		throw Format("Failed to load required entities (%d errors), check log for details.", errors);
}
