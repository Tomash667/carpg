#pragma once

//-----------------------------------------------------------------------------
#include "ContentLoader.h"

//-----------------------------------------------------------------------------
class AbilityLoader : public ContentLoader
{
	friend class Content;

	void DoLoading() override;
	static void Cleanup();
	void InitTokenizer() override;
	void LoadEntity(int top, const string& id) override;
	void ParseAbility(const string& id);
	void ParseAlias(const string& id);
	void Finalize() override;
};
