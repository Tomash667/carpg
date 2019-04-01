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
	virtual void DoLoading() = 0;
	virtual void LoadTexts() {}
	void Load(cstring filename, int top_group, bool* require_id = nullptr);
	virtual void InitTokenizer() = 0;
	virtual void LoadEntity(int top, const string& id) = 0;
	virtual void Finalize() = 0;
	void LoadWarning(cstring msg);
	template<typename... Args>
	void LoadWarning(cstring msg, const Args&... args)
	{
		LoadWarning(Format(msg, args...));
	}
	void LoadError(cstring msg);
	template<typename... Args>
	void LoadError(cstring msg, const Args&... args)
	{
		LoadError(Format(msg, args...));
	}
	cstring FormatLanguagePath(cstring filename);
	void SetLocalId(const string& id) { local_id = id; }

	Tokenizer t;
	Crc crc;

private:
	bool DoLoad(cstring filename, int top_group, bool* require_id);

	string local_id;
	int top_group, current_entity;
};
