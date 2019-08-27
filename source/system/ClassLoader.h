#pragma once

//-----------------------------------------------------------------------------
#include "ContentLoader.h"
#include "Class.h"

//-----------------------------------------------------------------------------
class ClassLoader : public ContentLoader
{
	friend class Content;
private:
	void DoLoading() override;
	static void Cleanup();
	void InitTokenizer() override;
	void LoadEntity(int top, const string& id) override;
	void ParseLevelEntry(Class::LevelEntry& entry);
	void Finalize() override;
	void ApplyUnits();
};
