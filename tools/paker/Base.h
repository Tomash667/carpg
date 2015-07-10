#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define STRICT

#include <Windows.h>
#include <vector>
#include <string>
#include <cassert>
#include <cstdarg>
#include <map>
#include <algorithm>

using std::vector;
using std::string;
using std::min;
using std::max;

typedef unsigned char byte;
typedef unsigned int uint;
typedef const char* cstring;

#define IS_SET(flaga,bit) (((flaga) & (bit)) != 0)

extern string g_tmp_string;
extern DWORD tmp;
extern char BUF[256];

bool LoadFileToString(cstring path, string& str);
cstring Format(cstring fmt, ...);
bool Unescape(const string& str_in, uint pos, uint length, string& str_out);
inline bool Unescape(const string& str_in, string& str_out)
{
	return Unescape(str_in, 0u, str_in.length(), str_out);
}
int StringToNumber(cstring s, __int64& i, float& f);
bool DirectoryExists(cstring filename);
bool DeleteDirectory(cstring filename);

template<typename T>
inline T& Add1(vector<T>& v)
{
	v.resize(v.size() + 1);
	return v.back();
}

// return index of character in cstring
inline int strchr_index(cstring chrs, char c)
{
	int index = 0;
	do
	{
		if(*chrs == c)
			return index;
		++index;
		++chrs;
	} while(*chrs);

	return -1;
}

// Usuwanie elementów wektora
template<typename T>
inline void DeleteElements(vector<T>& v)
{
	for(vector<T>::iterator it = v.begin(), end = v.end(); it != end; ++it)
		delete *it;
	v.clear();
}

//-----------------------------------------------------------------------------
// kontener u¿ywany na tymczasowe obiekty które s¹ potrzebne od czasu do czasu
//-----------------------------------------------------------------------------
#ifdef _DEBUG
//#	define CHECK_POOL_LEAK
#endif
template<typename T>
struct ObjectPool
{
	~ObjectPool()
	{
		DeleteElements(pool);
#ifdef CHECK_POOL_LEAK
#endif
	}

	inline T* Get()
	{
		T* t;
		if(pool.empty())
			t = new T;
		else
		{
			t = pool.back();
			pool.pop_back();
		}
		return t;
	}

	inline void Free(T* t)
	{
		assert(t);
#ifdef CHECK_POOL_LEAK
		delete t;
#else
		pool.push_back(t);
#endif
	}

	inline void Free(vector<T*>& elems)
	{
		if(elems.empty())
			return;
#ifdef _DEBUG
		for(vector<T*>::iterator it = elems.begin(), end = elems.end(); it != end; ++it)
		{
			assert(*it);
#ifdef CHECK_POOL_LEAK
			delete *it;
#endif
		}
#endif
#ifndef CHECK_POOL_LEAK
		pool.insert(pool.end(), elems.begin(), elems.end());
#endif
		elems.clear();
	}

private:
	vector<T*> pool;
};

// tymczasowe stringi
extern ObjectPool<string> StringPool;
extern ObjectPool<vector<void*> > VectorPool;

//-----------------------------------------------------------------------------
// Lokalny string który wykorzystuje StringPool
//-----------------------------------------------------------------------------
struct LocalString
{
	LocalString()
	{
		s = StringPool.Get();
		s->clear();
	}

	LocalString(cstring str)
	{
		s = StringPool.Get();
		*s = str;
	}

	LocalString(const string& str)
	{
		s = StringPool.Get();
		*s = str;
	}

	~LocalString()
	{
		StringPool.Free(s);
	}

	inline void operator = (cstring str)
	{
		*s = str;
	}

	inline void operator = (const string& str)
	{
		*s = str;
	}

	inline char at_back(uint offset) const
	{
		assert(offset < s->size());
		return s->at(s->size() - 1 - offset);
	}

	inline void pop(uint count)
	{
		assert(s->size() > count);
		s->resize(s->size() - count);
	}

	inline void operator += (cstring str)
	{
		*s += str;
	}

	inline void operator += (const string& str)
	{
		*s += str;
	}

	inline void operator += (char c)
	{
		*s += c;
	}

	inline operator cstring() const
	{
		return s->c_str();
	}

	inline string& get_ref()
	{
		return *s;
	}

	inline string* get_ptr()
	{
		return s;
	}

	inline string* operator -> ()
	{
		return s;
	}

	inline const string* operator -> () const
	{
		return s;
	}

	inline bool operator == (cstring str) const
	{
		return *s == str;
	}

	inline bool operator == (const string& str) const
	{
		return *s == str;
	}

	inline bool operator == (const LocalString& str) const
	{
		return *s == *str.s;
	}

	inline bool operator != (cstring str) const
	{
		return *s != str;
	}

	inline bool operator != (const string& str) const
	{
		return *s != str;
	}

	inline bool operator != (const LocalString& str) const
	{
		return *s != *str.s;
	}

	inline bool empty() const
	{
		return s->empty();
	}

	inline cstring c_str() const
	{
		return s->c_str();
	}

	inline void clear()
	{
		s->clear();
	}

private:
	string* s;
};
