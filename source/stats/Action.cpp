#include "Pch.h"
#include "GameCore.h"
#include "Action.h"
#include "ResourceManager.h"
#include "SoundManager.h"
#include "Engine.h"

//-----------------------------------------------------------------------------
Action Action::actions[] = {
	Action("bull_charge", 20.f, 20.f, 1, Action::LINE, Vec2(5.f, 0.6f), "charge.wav", 2.f, Action::F_IGNORE_UNITS, 100.f, false, false),
	Action("dash", 0, 5.f, 3, Action::LINE, Vec2(7.5f, 0.25f), "dash.wav", 0.5f, Action::F_PICK_DIR, 25.f, false, false),
	Action("summon_wolf", 60.f, 60.f, 1, Action::POINT, Vec2(1.5f, 10.f), nullptr, 0, 0, 0.f, false, true),
	Action("heal", 5.f, 5.f, 1, Action::TARGET, Vec2(7.5f, 0), nullptr, 0, 0, 100.f, true, true)
};
const uint Action::n_actions = countof(Action::actions);

//=================================================================================================
Action* Action::Find(const string& id)
{
	for(Action& action : actions)
	{
		if(id == action.id)
			return &action;
	}

	return nullptr;
}

//=================================================================================================
void Action::LoadData()
{
	for(Action& action : actions)
	{
		action.tex = res_mgr->Load<Texture>(Format("%s.png", action.id));
		if(action.sound_id)
			action.sound = res_mgr->Load<Sound>(action.sound_id);
	}
}
