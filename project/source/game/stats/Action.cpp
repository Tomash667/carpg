#include "Pch.h"
#include "GameCore.h"
#include "Action.h"
#include "ResourceManager.h"
#include "SoundManager.h"
#include "Engine.h"

//-----------------------------------------------------------------------------
Action Action::actions[] = {
	Action("bull_charge", 20.f, 20.f, 1, Action::LINE, Vec2(5.f, 0.6f), "charge.wav", 2.f, Action::F_IGNORE_UNITS, 100.f),
	Action("dash", 0, 5.f, 3, Action::LINE, Vec2(7.5f, 0.25f), "dash.wav", 0.5f, Action::F_PICK_DIR, 25.f),
	Action("summon_wolf", 60.f, 60.f, 1, Action::POINT, Vec2(1.5f, 10.f), nullptr, 0, 0, 0.f)
};
const uint Action::n_actions = countof(Action::actions);

//=================================================================================================
Action* Action::Find(const string& id)
{
	for (auto& action : actions)
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
	bool load_sounds = !Engine::Get().sound_mgr->IsSoundDisabled();

	for (auto& action : actions)
	{
		action.tex = tex_mgr.GetLoaded(Format("%s.png", action.id));
		if (action.sound_id && load_sounds)
			action.sound = sound_mgr.GetLoaded(action.sound_id);
	}
}
