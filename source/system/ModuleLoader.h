#pragma once

#include <Tokenizer.h>

class ModuleLoader
{
public:
	void LoadModules();

private:
	void InitTokenizer();
	void LoadModule(cstring path);
	void ParseLocalizedString(string& str);

	Tokenizer t;
};
