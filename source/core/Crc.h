#pragma once

//-----------------------------------------------------------------------------
const uint CRC32_NEGL = 0xffffffffL;

//-----------------------------------------------------------------------------
#ifdef IS_LITTLE_ENDIAN
#define CRC32_INDEX(c) (c & 0xff)
#define CRC32_SHIFTED(c) (c >> 8)
#else
#define CRC32_INDEX(c) (c >> 24)
#define CRC32_SHIFTED(c) (c << 8)
#endif

//-----------------------------------------------------------------------------
class CRC32
{
public:
	CRC32()
	{
		m_crc = CRC32_NEGL;
	}
	void Update(const byte *input, size_t length);
	uint Get()
	{
		return m_crc;
	}

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
	void UpdateVector(const vector<T>& v)
	{
		Update(v.size());
		if(!v.empty())
			Update((const byte*)v.data(), v.size()*sizeof(T));
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
	static const uint m_tab[256];
	uint m_crc;
};
