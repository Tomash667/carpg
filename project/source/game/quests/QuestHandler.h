#pragma once

struct QuestHandler
{
	virtual ~QuestHandler() {}
	virtual void Special(DialogContext& ctx, cstring msg) {}
	virtual bool SpecialIf(DialogContext& ctx, cstring msg) { return false; }
	virtual cstring FormatString(const string& str) { return nullptr; }
};
