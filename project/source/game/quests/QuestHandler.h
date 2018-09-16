#pragma once

struct QuestHandler
{
	virtual ~QuestHandler() {}
	virtual bool Special(DialogContext& ctx, cstring msg) { return false; }
	virtual bool SpecialIf(DialogContext& ctx, cstring msg) { return false; }
	virtual cstring FormatString(const string& str) { return nullptr; }
};
