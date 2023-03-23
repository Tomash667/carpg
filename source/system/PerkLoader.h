#pragma once

//-----------------------------------------------------------------------------
#include "ContentLoader.h"

//-----------------------------------------------------------------------------
class PerkLoader : public ContentLoader
{
	friend class Content;

private:
	void DoLoading() override;
	static void Cleanup();
	void InitTokenizer() override;
	void LoadEntity(int top, const string& id) override;
	void ParsePerk(const string& id);
	Perk* ParsePerkId();
	void ParseAlias(const string& id);
	void Finalize() override;
	Perk* GetHardcodedPerk(cstring name);
};
