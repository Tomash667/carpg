#pragma once

//-----------------------------------------------------------------------------
#include "ContentLoader.h"

//-----------------------------------------------------------------------------
class RequiredLoader : public ContentLoader
{
	friend class Content;

	void DoLoading() override;
	void InitTokenizer() override;
	void LoadEntity(int type, const string& id) override;
	void Finalize() override;
	void CheckStartItems(SkillId skill, bool required);
	void CheckBaseItems();
	void CheckBaseItem(cstring name, int num);
};
