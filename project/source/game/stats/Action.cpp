#include "Pch.h"
#include "Core.h"
#include "Action.h"
#include "ResourceManager.h"

//-----------------------------------------------------------------------------
Action g_actions[] = {
	Action("bull_charge", 25.f, 25.f, 1),
	Action("dash", 0.5f, 10.f, 2),
	Action("summon_wolf", 90.f, 90.f, 1)
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
void Action::LoadImages()
{
	auto& tex = ResourceManager::Get<Texture>();

	for (auto& action : g_actions)
		action.tex = tex.GetLoaded(Format("%s.png", action.id));
}
