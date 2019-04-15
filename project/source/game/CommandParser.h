#pragma once

//-----------------------------------------------------------------------------
#include "GameComponent.h"

//-----------------------------------------------------------------------------
class CommandParser : public GameComponent
{
public:
	bool ParseStream(BitStreamReader& f, PlayerInfo& info);
	void ParseStringCommand(int cmd, const string& s, PlayerInfo& info);

	void RemoveEffect(Unit* u, EffectId effect, EffectSource source, int source_id, int value);
	void ListEffects(Unit* u);
	void AddPerk(PlayerController* pc, Perk perk, int value);
	void RemovePerk(PlayerController* pc, Perk perk, int value);
	void ListPerks(PlayerController* pc);
	void ListStats(Unit* u);
	void ArenaCombat(cstring str);

private:
	void Cleanup() override { delete this; }
	bool ParseStreamInner(BitStreamReader& f);
};
