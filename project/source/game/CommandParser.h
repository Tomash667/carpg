#pragma once

//-----------------------------------------------------------------------------
#include "GameComponent.h"
#include "ConsoleCommands.h"

//-----------------------------------------------------------------------------
class CommandParser : public GameComponent
{
public:
	void AddCommands();
	void ParseCommand(const string& command_str, PrintMsgFunc print_func, PARSE_SOURCE source = PS_UNKNOWN);
	bool ParseStream(BitStreamReader& f, PlayerInfo& info);
	void ParseStringCommand(int cmd, const string& s, PlayerInfo& info);
	const vector<ConsoleCommand>& GetCommands() { return cmds; }

	void RemoveEffect(Unit* u, EffectId effect, EffectSource source, int source_id, int value);
	void ListEffects(Unit* u);
	void AddPerk(PlayerController* pc, Perk perk, int value);
	void RemovePerk(PlayerController* pc, Perk perk, int value);
	void ListPerks(PlayerController* pc);
	void ListStats(Unit* u);
	void ArenaCombat(cstring str);
	void CmdList(Tokenizer& t);

private:
	void ParseScript(Tokenizer& t);
	void RunCommand(ConsoleCommand& cmd, Tokenizer& t, PARSE_SOURCE source);
	void Cleanup() override { delete this; }
	bool ParseStreamInner(BitStreamReader& f);

	vector<ConsoleCommand> cmds;
};
