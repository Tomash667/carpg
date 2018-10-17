#include "Pch.h"
#include "GameCore.h"
#include "GameDialog.h"
#include "ScriptManager.h"

GameDialog::Map GameDialog::dialogs;

//=================================================================================================
GameDialog* GameDialog::TryGet(cstring id)
{
	auto it = dialogs.find(id);
	if(it == dialogs.end())
		return nullptr;
	else
		return it->second;
}

//=================================================================================================
void GameDialog::Cleanup()
{
	for(auto& it : dialogs)
		delete it.second;
}

//=================================================================================================
void GameDialog::Verify(uint& errors)
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
