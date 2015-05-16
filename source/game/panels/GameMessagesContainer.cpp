#include "Pch.h"
#include "Base.h"
#include "GameMessagesContainer.h"
#include "GamePanel.h"
#include "Game.h"

//=================================================================================================
void GameMessagesContainer::Draw(ControlDrawData*)
{
	for(list<GameMsg>::iterator it = msgs.begin(), end = msgs.end(); it != end; ++it)
	{
		int a = 255;
		if(it->fade < 0)
			a = 255 + int(it->fade*2550);
		else if(it->fade > 0 && it->time < 0.f)
			a = 255 - int(it->fade*2550);
		RECT rect = {0, int(it->pos.y)-it->size.y/2, GUI.wnd_size.x, int(it->pos.y)+it->size.y/2};
		GUI.DrawText(GUI.default_font, it->msg, DT_CENTER|DT_OUTLINE, COLOR_RGBA(255,255,255,a), rect);
	}

	if(Game::Get().paused)
	{
		RECT r = {0, 0, GUI.wnd_size.x, GUI.wnd_size.y};
		GUI.DrawText(GUI.fBig, Game::Get().game_gui->txGamePausedBig, DT_CENTER|DT_VCENTER, BLACK, r);
	}

	if(GamePanel::menu.visible)
		GamePanel::menu.Draw();
}

//=================================================================================================
void GameMessagesContainer::Update(float dt)
{
	int h = 0, total_h = msgs_h;

	for(list<GameMsg>::iterator it = msgs.begin(), end = msgs.end(); it != end; ++it)
	{
		GameMsg& m = *it;
		m.time -= dt;
		h += m.size.y;

		if(m.time >= 0.f)
		{
			m.fade += dt;
			if(m.fade > 0.f)
				m.fade = 0.f;
		}
		else
		{
			m.fade -= dt;
			if(m.fade < -0.1f)
			{
				msgs_h -= m.size.y;
				it = msgs.erase(it);
				end = msgs.end();
				if(it == end)
					break;
				else
					continue;
			}
		}

		float target_h = float(GUI.wnd_size.y)/2 - float(total_h)/2 + h;
		m.pos.y += (target_h - m.pos.y)*dt*2;
	}
}

//=================================================================================================
void GameMessagesContainer::Reset()
{
	msgs.clear();
	msgs_h = 0;
}
