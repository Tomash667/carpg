#pragma once

typedef std::map<string, string> LanguageMap;
typedef std::map<string, LanguageMap*> LanguageSections;

//-----------------------------------------------------------------------------
extern string g_lang_prefix;
extern LanguageMap g_language;
extern LanguageSections g_language2;
extern vector<LanguageMap*> g_languages;

//-----------------------------------------------------------------------------
void LoadLanguageFile(cstring filename);
bool LoadLanguageFile2(cstring filename, cstring section, LanguageMap* lmap = nullptr);
void LoadLanguages();
void ClearLanguages();
void LoadLanguageFiles();

//-----------------------------------------------------------------------------
inline cstring StrT(cstring str, bool err=true)
{
	LanguageMap::const_iterator it = g_language.find(str);
	if(it != g_language.end())
		return it->second.c_str();
	else
	{
		if(err)
			ERROR(Format("Missing text string for '%s'.", str));
		return nullptr;
	}
}

//-----------------------------------------------------------------------------
inline const cstring Str(cstring str)
{
	cstring s = StrT(str);
	return s ? s : "";
}
