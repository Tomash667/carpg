#include "Pch.h"
#include "Core.h"
#include "GameMessages.h"
#include "Game.h"
#include "Language.h"
#include "SaveState.h"

//=================================================================================================
GameMessages::GameMessages()
{
	txGamePausedBig = Str("gamePausedBig");
}

//=================================================================================================
void GameMessages::Draw(ControlDrawData*)
{
	for(auto p_msg : msgs)
	{
		auto& msg = *p_msg;
		int a = 255;
		if(msg.fade < 0)
			a = 255 + int(msg.fade * 2550);
		else if(msg.fade > 0 && msg.time < 0.f)
			a = 255 - int(msg.fade * 2550);
		Rect rect = { 0, int(msg.pos.y) - msg.size.y / 2, GUI.wnd_size.x, int(msg.pos.y) + msg.size.y / 2 };
		GUI.DrawText(GUI.default_font, msg.msg, DT_CENTER | DT_OUTLINE, COLOR_RGBA(255, 255, 255, a), rect);
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

	for(vector<GameMsg*>::iterator it = msgs.begin(), end = msgs.end(); it != end; ++it)
	{
		GameMsg& m = **it;
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
	GameMsg::Free(msgs);
	msgs.clear();
	msgs_h = 0;
}

//=================================================================================================
void GameMessages::Save(FileWriter& f) const
{
	f << msgs.size();
	for(auto p_msg : msgs)
	{
		auto& msg = *p_msg;
		if(msg.pattern.empty())
		{
			f.Write0();
			f.WriteString2(msg.msg);
		}
		else
		{
			f.Write1();
			f.WriteString2(msg.pattern);
			f << msg.id;
			f << msg.value;
		}
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
	uint count;
	f >> count;
	msgs.reserve(count);

	for(uint i = 0; i < count; ++i)
	{
		GameMsg* p_msg = GameMsg::Get();
		msgs.push_back(p_msg);

		auto& msg = *p_msg;
		if(LOAD_VERSION >= V_CURRENT)
		{
			if(f.Read0())
			{
				f.ReadString2(msg.msg);
				msg.pattern.clear();
			}
			else
			{
				f.ReadString2(msg.pattern);
				f >> msg.id;
				f >> msg.value;
				msg.msg = Format(msg.pattern.c_str(), msg.value);
			}
		}
		else
		{
			msg.pattern.clear();
			f.ReadString2(msg.msg);
		}

		f >> msg.time;
		f >> msg.fade;
		f >> msg.pos;
		f >> msg.size;
		f >> msg.type;
	}

	f >> msgs_h;
}

//=================================================================================================
GameMsg& GameMessages::AddMessage(cstring text, float time, int type)
{
	assert(text && time > 0.f);

	auto p_msg = GameMsg::Get();
	msgs.push_back(p_msg);

	GameMsg& m = *p_msg;
	m.msg = text;
	m.pattern.clear();
	m.type = type;
	m.time = time;
	m.fade = -0.1f;
	m.size = GUI.default_font->CalculateSize(text, GUI.wnd_size.x - 64);
	m.size.y += 6;

	if(msgs.size() == 1u)
		m.pos = Vec2(float(GUI.wnd_size.x) / 2, float(GUI.wnd_size.y) / 2);
	else
	{
		GameMsg& prev = **(msgs.end() - 2);
		m.pos = Vec2(float(GUI.wnd_size.x) / 2, prev.pos.y + prev.size.y);
	}

	msgs_h += m.size.y;

	return m;
}

//=================================================================================================
void GameMessages::AddMessageIfNotExists(cstring text, float time, int type)
{
	if(type >= 0)
	{
		for(GameMsg* msg : msgs)
		{
			if(msg->type == type)
			{
				msg->time = time;
				return;
			}
		}
	}
	else
	{
		for(GameMsg* msg : msgs)
		{
			if(msg->msg == text)
			{
				msg->time = time;
				return;
			}
		}
	}

	AddMessage(text, time, type);
}

//=================================================================================================
void GameMessages::AddOrUpdateMessageWithPattern(cstring pattern, int type, int id, int value, float time)
{
	assert(pattern && type >= 0 && value > 0);

	for(auto msg : msgs)
	{
		if(msg->type == type && msg->id == id)
		{
			msg->value += value;
			msg->time = time;
			msg->msg = Format(pattern, msg->value);
			return;
		}
	}

	auto& msg = AddMessage(Format(pattern, value), time, type);
	msg.pattern = pattern;
	msg.id = id;
	msg.value = value;
}
