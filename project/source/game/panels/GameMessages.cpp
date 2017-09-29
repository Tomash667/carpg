#include "Pch.h"
#include "Core.h"
#include "GameMessages.h"
#include "Game.h"
#include "Language.h"

//=================================================================================================
GameMessages::GameMessages()
{
	txGamePausedBig = Str("gamePausedBig");
}

//=================================================================================================
void GameMessages::Draw(ControlDrawData*)
{
	for(list<GameMsg>::iterator it = msgs.begin(), end = msgs.end(); it != end; ++it)
	{
		int a = 255;
		if(it->fade < 0)
			a = 255 + int(it->fade * 2550);
		else if(it->fade > 0 && it->time < 0.f)
			a = 255 - int(it->fade * 2550);
		Rect rect = { 0, int(it->pos.y) - it->size.y / 2, GUI.wnd_size.x, int(it->pos.y) + it->size.y / 2 };
		GUI.DrawText(GUI.default_font, it->msg, DT_CENTER | DT_OUTLINE, COLOR_RGBA(255, 255, 255, a), rect);
	}

	Game& game = Game::Get();
	if(game.paused)
	{
		Rect r = { 0, 0, GUI.wnd_size.x, GUI.wnd_size.y };
		GUI.DrawText(GUI.fBig, txGamePausedBig, DT_CENTER | DT_VCENTER, BLACK, r);
	}
}

//=================================================================================================
void GameMessages::Update(float dt)
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

		float target_h = float(GUI.wnd_size.y) / 2 - float(total_h) / 2 + h;
		m.pos.y += (target_h - m.pos.y)*dt * 2;
	}
}

//=================================================================================================
void GameMessages::Reset()
{
	msgs.clear();
	msgs_h = 0;
}

//=================================================================================================
void GameMessages::Save(FileWriter& f) const
{
	f << msgs.size();
	for(auto& msg : msgs)
	{
		f.WriteString2(msg.msg);
		f << msg.time;
		f << msg.fade;
		f << msg.pos;
		f << msg.size;
		f << msg.type;
	}
	f << msgs_h;
}

//=================================================================================================
void GameMessages::Load(FileReader& f)
{
	msgs.resize(f.Read<uint>());
	for(auto& msg : msgs)
	{
		f.ReadString2(msg.msg);
		f >> msg.time;
		f >> msg.fade;
		f >> msg.pos;
		f >> msg.size;
		f >> msg.type;
	}
	f >> msgs_h;
}

//=================================================================================================
void GameMessages::AddMessage(cstring text, float time, int type)
{
	assert(text && time > 0.f);

	GameMsg& m = Add1(msgs);
	m.msg = text;
	m.type = type;
	m.time = time;
	m.fade = -0.1f;
	m.size = GUI.default_font->CalculateSize(text, GUI.wnd_size.x - 64);
	m.size.y += 6;

	if(msgs.size() == 1u)
		m.pos = Vec2(float(GUI.wnd_size.x) / 2, float(GUI.wnd_size.y) / 2);
	else
	{
		list<GameMsg>::reverse_iterator it = msgs.rbegin();
		++it;
		GameMsg& prev = *it;
		m.pos = Vec2(float(GUI.wnd_size.x) / 2, prev.pos.y + prev.size.y);
	}

	msgs_h += m.size.y;
}

//=================================================================================================
void GameMessages::AddMessageIfNotExists(cstring text, float time, int type)
{
	if(type >= 0)
	{
		for(GameMsg& msg : msgs)
		{
			if(msg.type == type)
			{
				msg.time = time;
				return;
			}
		}
	}
	else
	{
		for(GameMsg& msg : msgs)
		{
			if(msg.msg == text)
			{
				msg.time = time;
				return;
			}
		}
	}

	AddMessage(text, time, type);
}
