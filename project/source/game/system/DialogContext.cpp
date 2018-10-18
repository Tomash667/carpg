#include "Pch.h"
#include "GameCore.h"
#include "DialogContext.h"
#include "ScriptManager.h"
#include "Quest.h"
#include "Game.h"
#include "PlayerInfo.h"

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
