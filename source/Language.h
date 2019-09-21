#pragma once

//-----------------------------------------------------------------------------
class Language
{
public:
	typedef std::map<string, string> Map;
	typedef std::map<string, Map*> Sections;

	struct Section
	{
		Section(Map& lm, cstring name) : lm(lm), name(name) {}
		cstring Get(cstring str);
		Section& operator = (Section& s)
		{
			lm = s.lm;
			name = s.name;
			return *this;
		}
	private:
		std::reference_wrapper<Map> lm;
		cstring name;
	};

	static void Init();
	static void Cleanup();
	static bool LoadFile(cstring filename);
	static void LoadLanguages();
	static void LoadLanguageFiles();
	static cstring TryGetString(cstring str, bool err = true);
	static Section GetSection(cstring name);
	static vector<Map*>& GetLanguages() { return languages; }
	static uint GetErrors() { return errors; }

	static string prefix;

private:
	static bool LoadFileInternal(Tokenizer& t, cstring path, Map* lmap = nullptr);
	static void PrepareTokenizer(Tokenizer& t);
	static void LoadObjectFile(Tokenizer& t, cstring filename);
	static void ParseObject(Tokenizer& t);

	static Map strs;
	static Sections sections;
	static vector<Map*> languages;
	static string tstr;
	static uint errors;
};

//-----------------------------------------------------------------------------
inline cstring Str(cstring str)
{
	cstring s = Language::TryGetString(str);
	return s ? s : "";
}
template<int N>
inline void LoadArray(cstring(&var)[N], cstring str)
{
	for(uint i = 0; i < N; ++i)
		var[i] = Str(Format("%s%u", str, i));
}
