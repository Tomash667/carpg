#pragma once

//-----------------------------------------------------------------------------
class Crc
{
public:
	Crc() : m_crc(CRC32_NEGL) {}
	void Update(const byte *input, size_t length);
	uint Get() const { return ~m_crc; }
	operator uint() const { return Get(); }

	template<typename T>
	void Update(const T& item)
	{
		Update((const byte*)&item, sizeof(item));
	}

	template<>
	void Update(const string& str)
	{
		if(!str.empty())
			Update((const byte*)str.c_str(), str.length());
	}

	template<>
	void Update(const cstring& str)
	{
		assert(str);
		Update((const byte*)str, strlen(str));
	}

	template<typename T>
	void Update(const vector<T>& v)
	{
		Update(v.size());
		if(!v.empty())
			Update((const byte*)v.data(), v.size() * sizeof(T));
	}

	template<>
	void Update(const vector<string>& v)
	{
		Update(v.size());
		for(const string& s : v)
			Update(s);
	}

	void Update0()
	{
		Update<byte>(0);
	}

	void Update1()
	{
		Update<byte>(1);
	}

private:
	static const uint CRC32_NEGL = 0xffffffffL;
	static const uint m_tab[256];
	uint m_crc;
};
