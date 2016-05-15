#pragma once

enum class ContentType
{
	Building
};

class ContentLoader
{
	friend class ContentManager;
public:
	enum Flags
	{
		HaveDatafile = 1 << 0,
		HaveResources = 1 << 1,
		HaveText = 1 << 2
	};

	ContentLoader(ContentType type, cstring name) : type(type), name(name) {}
	ContentLoader(ContentType type, cstring name, std::initializer_list<ContentType> const & dependency) : type(type), name(name), dependency(dependency) {}
	virtual ~ContentLoader() {}

	virtual void Init() = 0;
	virtual int Load() = 0;
	virtual void Cleanup() = 0;

	inline static cstring GetItemString(cstring type, const string& str)
	{
		if(str.empty())
			return type;
		else
			return Format("%s '%s'", type, str.c_str());
	}

protected:
	Tokenizer t;

private:
	cstring name;
	ContentType type;
	vector<ContentType> dependency;
	vector<ContentLoader*> edge_from, edge_to;
	uint incoming_edges;
	bool marked;
};
