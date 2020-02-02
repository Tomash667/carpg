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
	Perk* ParsePerkId();
	void Finalize() override;
};
