#pragma once

//-----------------------------------------------------------------------------
#include "ContentLoader.h"

//-----------------------------------------------------------------------------
class ParticleLoader : public ContentLoader
{
	friend class Content;

	void DoLoading();
	void InitTokenizer();
	void LoadEntity(int top, const string& id);
};
