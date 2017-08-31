#include "Pch.h"
#include "Core.h"
#include "Action.h"
#include "ResourceManager.h"

//-----------------------------------------------------------------------------
Action g_actions[] = {
	Action("bull_charge", 20.f, 20.f, 1, Action::LINE, Vec2(5.f, 0.6f), "charge.wav", 3.f, Action::F_IGNORE_UNITS),
	Action("dash", 0, 5.f, 3, Action::LINE, Vec2(7.5f, 0.25f), "dash.wav", 1.f, Action::F_PICK_DIR),
	Action("summon_wolf", 60.f, 60.f, 1, Action::POINT, Vec2(1.5f, 10.f), nullptr, 0, 0)
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
