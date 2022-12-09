#pragma once

//-----------------------------------------------------------------------------
#include "ConsoleCommands.h"
#include "NetChange.h"
#include "Tokenizer.h"

//-----------------------------------------------------------------------------
class CommandParser
{
public:
	typedef delegate<void(cstring)> PrintMsgFunc;

	void AddCommands();
	void ParseCommand(const string& commandStr, PrintMsgFunc printMsgFunc, PARSE_SOURCE source = PS_UNKNOWN);
	bool ParseStream(BitStreamReader& f, PlayerInfo& info);
	void ParseStringCommand(int cmd, const string& s, PlayerInfo& info);
	const vector<ConsoleCommand>& GetCommands() { return cmds; }

	inline void Msg(cstring msg)
	{
		printMsgFunc(msg);
	}
	template<typename... Args>
	inline void Msg(cstring msg, const Args&... args)
	{
		printMsgFunc(Format(msg, args...));
	}

private:
	void ParseScript();
	void RunCommand(ConsoleCommand& cmd, PARSE_SOURCE source);
	bool ParseStreamInner(BitStreamReader& f, PlayerController* player);
	NetChangeWriter PushGenericCmd(CMD cmd);
	// commands
	void HealUnit(Unit& unit);
	void RemoveEffect(Unit* u, EffectId effect, EffectSource source, int sourceId, int value);
	void ListEffects(Unit* u);
	void AddPerk(PlayerController* pc, Perk* perk, int value);
	void RemovePerk(PlayerController* pc, Perk* perk, int value);
	void ListPerks(PlayerController* pc);
	void ListStats(Unit* u);
	void ArenaCombat(cstring str);
	void CmdList();
	void Scare(PlayerController* player);

	Tokenizer t;
	vector<ConsoleCommand> cmds;
	PrintMsgFunc printMsgFunc;
};
