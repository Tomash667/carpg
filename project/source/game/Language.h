#pragma once

//-----------------------------------------------------------------------------
class Language
{
public:
	typedef std::map<string, string> LanguageMap;
	typedef std::map<string, LanguageMap*> LanguageSections;

	struct LanguageSection
	{
		LanguageSection(LanguageMap& lm, cstring name) : lm(lm), name(name) {}
		cstring Get(cstring str);
	private:
		LanguageMap& lm;
		cstring name;
	};

	static void Init();
	static void Cleanup();
	static bool LoadFile(cstring filename);
	static void LoadLanguages();
	static void LoadLanguageFiles();
	static cstring TryGetString(cstring str, bool err = true);
	static LanguageSection GetSection(cstring name);
	static vector<LanguageMap*>& GetLanguages() { return languages; }

	static string prefix;

private:
	static bool LoadFileInternal(cstring path, LanguageMap* lmap = nullptr);
	static void PrepareTokenizer(Tokenizer& t);
	static void LoadObjectFile(Tokenizer& t, cstring filename);

	static LanguageMap strs;
	static LanguageSections sections;
	static vector<LanguageMap*> languages;
	static string tstr;
};

//-----------------------------------------------------------------------------
inline cstring Str(cstring str)
{
	cstring s = Language::TryGetString(str);
	return s ? s : "";
}
template<int N>
inline void LoadArray(cstring (&var)[N], cstring str)
{
	for(uint i = 0; i < N; ++i)
		var[i] = Str(Format("%s%u", str, i));
}
