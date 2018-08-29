#pragma once

//-----------------------------------------------------------------------------
#include "Content.h"
#include "Crc.h"
#include "Tokenizer.h"

//-----------------------------------------------------------------------------
class ContentLoader
{
protected:
	ContentLoader() {}
	ContentLoader(int flags) : t(flags) {}
	virtual ~ContentLoader() {}
	void Load(cstring filename, int top_group, bool* require_id = nullptr);
	virtual void InitTokenizer() = 0;
	virtual void LoadEntity(int top, const string& id) = 0;
	virtual void Finalize() = 0;
	void LoadError(cstring msg);
	template<typename... Args>
	void LoadError(cstring msg, const Args&... args)
	{
		LoadError(Format(msg, args...));
	}

	Tokenizer t;
	Crc crc;

private:
	bool DoLoad(cstring filename, int top_group, bool* require_id);

	string local_id;
	int top_group, current_entity;
};
