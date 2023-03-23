#pragma once

//-----------------------------------------------------------------------------
#include "ContentLoader.h"

//-----------------------------------------------------------------------------
class AbilityLoader : public ContentLoader
{
	friend class Content;

public:
	void ApplyUnits();

private:
	void DoLoading() override;
	static void Cleanup();
	void InitTokenizer() override;
	void LoadEntity(int top, const string& id) override;
	void ParseAbility(const string& id);
	void ParseAlias(const string& id);
	void Finalize() override;
	Ability* GetHardcodedAbility(cstring name);

	Texture* tPlaceholder;
};
