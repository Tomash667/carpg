#include "Pch.h"
#include "Core.h"
#include "Action.h"
#include "ResourceManager.h"

//-----------------------------------------------------------------------------
Action g_actions[] = {
	Action("bull_charge", 25.f, 25.f, 1, Action::LINE, Vec2(4.f, 0.6f), "charge.wav", 3.f, false),
	Action("dash", 0.5f, 10.f, 2, Action::LINE, Vec2(5.f, 0.25f), "dash.wav", 1.f, true),
	Action("summon_wolf", 90.f, 90.f, 1, Action::POINT, Vec2(1.5f, 10.f), nullptr, 0, false)
};
extern const uint n_actions = countof(g_actions);

//=================================================================================================
Action* Action::Find(const string& id)
{
	for (auto& action : g_actions)
	{
		if (id == action.id)
			return &action;
	}

	return nullptr;
}

//=================================================================================================
void Action::LoadData()
{
	auto& tex_mgr = ResourceManager::Get<Texture>();
	auto& sound_mgr = ResourceManager::Get<Sound>();

	for (auto& action : g_actions)
	{
		action.tex = tex_mgr.GetLoaded(Format("%s.png", action.id));
		if (action.sound_id)
			action.sound = sound_mgr.GetLoaded(action.sound_id);
	}
}
