#pragma once

//-----------------------------------------------------------------------------
// Macros
#define BIT(bit) (1<<(bit))
#define IS_SET(flags,bit) (((flags) & (bit)) != 0)
#define IS_CLEAR(flags,bit) (((flags) & (bit)) == 0)
#define IS_ALL_SET(flags,bity) (((flags) & (bity)) == (bity))
#define SET_BIT(flags,bit) ((flags) |= (bit))
#define CLEAR_BIT(flags,bit) ((flags) &= ~(bit))
#define SET_BIT_VALUE(flags,bit,wartos) { if(wartos) SET_BIT(flags,bit); else CLEAR_BIT(flags,bit); }
#define COPY_BIT(flags,flags2,bit) { if(((flags2) & (bit)) != 0) SET_BIT(flags,bit); else CLEAR_BIT(flags,bit); }
#define FLT10(x) (float(int((x)*10))/10)
#define FLT100(x) (float(int((x)*100))/100)
#define OR2_EQ(var,val1,val2) (((var) == (val1)) || ((var) == (val2)))
#define OR3_EQ(var,val1,val2,val3) (((var) == (val1)) || ((var) == (val2)) || ((var) == (val3)))
#ifndef STRING
#	define _STRING(str) #str
#	define STRING(str) _STRING(str)
#endif
#define _JOIN(a,b) a##b
#define JOIN(a,b) _JOIN(a,b)

//-----------------------------------------------------------------------------
template<class T, size_t N>
constexpr size_t countof(T(&)[N]) { return N; }

//-----------------------------------------------------------------------------
// Debug helpers
#ifdef _DEBUG
#	define DEBUG_DO(x) (x)
#	define C(x) assert(x)
#else
#	define DEBUG_DO(x)
#	define C(x) x
#endif
#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC2__ __FILE__ "("__STR1__(__LINE__)") : "
#ifndef _DEBUG
#	define FIXME __pragma(message(__LOC2__ "error: FIXME in release build!"))
#else
#	define FIXME
#endif

//-----------------------------------------------------------------------------
// Core variable types
typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int uint;
typedef __int64 int64;
typedef unsigned __int64 uint64;
typedef const char* cstring;

//-----------------------------------------------------------------------------
// Forward declarations
class Buffer;
class FileReader;
class FileWriter;
class MemoryReader;
class MemoryWriter;
class StreamReader;
class StreamWriter;
class TextWriter;
namespace tokenizer
{
	class Tokenizer;
}
using tokenizer::Tokenizer;

//-----------------------------------------------------------------------------
// Delegates
#include "FastFunc.h"
template<typename T>
using delegate = ssvu::FastFunc<T>;
typedef delegate<void()> VoidDelegate;
typedef delegate<void()> VoidF;
typedef delegate<void(cstring)> PrintMsgFunc;

//-----------------------------------------------------------------------------
// Allocators
template<typename T>
struct IAllocator
{
	virtual T* Create() = 0;
	virtual void Destroy(T* item) = 0;
};

template<typename T>
struct IVectorAllocator
{
	virtual void Destroy(vector<T*>& items) = 0;
};

namespace internal
{
	template<typename T>
	class AllocateHelper
	{
	public:
		template<typename Q = T>
		static typename std::enable_if<std::is_abstract<Q>::value || !std::is_default_constructible<T>::value, Q>::type* Allocate()
		{
			return nullptr;
		}

		template<typename Q = T>
		static typename std::enable_if<!std::is_abstract<Q>::value && std::is_default_constructible<T>::value, Q>::type* Allocate()
		{
			return new T;
		}
	};

	template<typename T>
	struct StandardAllocator : IAllocator<T>
	{
		T* Create()
		{
			return AllocateHelper<T>::Allocate();
		}

		void Destroy(T* item)
		{
			delete item;
		}
	};
}

//-----------------------------------------------------------------------------
template<typename T>
class SmartPtr
{
public:
	SmartPtr() : ptr(nullptr) {}
	SmartPtr(T* p) : ptr(p)
	{
		if(ptr)
			ptr->AddRef();
	}
	SmartPtr(const SmartPtr<T>& sp) : ptr(sp.ptr)
	{
		if(ptr)
			ptr->AddRef();
	}
	~SmartPtr()
	{
		if(ptr)
			ptr->Release();
	}

	operator T* () { return ptr; }
	operator const T* () const { return ptr; }
	T* operator -> () { return ptr; }
	const T* operator -> () const { return ptr; }

	operator bool() const
	{
		return ptr != nullptr;
	}

	SmartPtr& operator = (T* p)
	{
		if(ptr)
			ptr->Release();
		ptr = p;
		if(ptr)
			ptr->AddRef();
		return *this;
	}

	SmartPtr& operator = (nullptr_t)
	{
		if(ptr)
			ptr->Release();
		ptr = nullptr;
		return *this;
	}

	SmartPtr& operator = (const SmartPtr& sptr)
	{
		if(ptr)
			ptr->Release();
		ptr = sptr.ptr;
		if(ptr)
			ptr->AddRef();
		return *this;
	}

	bool operator == (T* ptr2) const
	{
		return ptr == ptr2;
	}

	bool operator != (T* ptr2) const
	{
		return ptr != ptr2;
	}

private:
	T* ptr;
};

template<typename T>
inline bool operator == (T* ptr, SmartPtr<T>& sptr)
{
	return sptr == ptr;
}
template<typename T>
inline bool operator != (T* ptr, SmartPtr<T>& sptr)
{
	return sptr != ptr;
}

//-----------------------------------------------------------------------------
// RAII for simple pointer
template<typename T, typename Allocator = internal::StandardAllocator<T>>
class Ptr
{
	static_assert(std::is_base_of<IAllocator<T>, Allocator>::value, "Allocator must inherit from IAllocator.");
public:
	Ptr(nullptr_t) : ptr(nullptr)
	{
	}
	Ptr(T* ptr) : ptr(ptr)
	{
	}
	template<typename U = T>
	Ptr(typename std::enable_if<!std::is_abstract<U>::value && std::is_default_constructible<U>::value>::type* = nullptr)
	{
		ptr = allocator.Create();
	}
	~Ptr()
	{
		if(ptr)
			allocator.Destroy(ptr);
	}
	void operator = (T* new_ptr)
	{
		if(ptr)
			allocator.Destroy(ptr);
		ptr = new_ptr;
	}
	operator T* ()
	{
		return ptr;
	}
	T* operator -> ()
	{
		return ptr;
	}
	void Ensure()
	{
		if(!ptr)
			ptr = allocator.Create();
	}
	T* Pin()
	{
		T* t = ptr;
		ptr = nullptr;
		return t;
	}
	T*& Get()
	{
		return ptr;
	}

private:
	T* ptr;
	Allocator allocator;
};

//-----------------------------------------------------------------------------
// RAII for vector of pointers
template<typename T, typename Allocator>
class VectorPtr
{
public:
	static_assert(std::is_base_of<IVectorAllocator<T>, Allocator>::value, "Allocator must inherit from IVectorAllocator.");

	VectorPtr() : pinned(false)
	{
	}

	~VectorPtr()
	{
		if(!pinned)
			allocator.Destroy(items);
	}

	vector<T*>* operator -> ()
	{
		return &items;
	}

	vector<T*>&& Pin()
	{
		pinned = true;
		return std::move(items);
	}

private:
	vector<T*> items;
	Allocator allocator;
	bool pinned;
};

//-----------------------------------------------------------------------------
// In debug it uses dynamic_cast and asserts if cast is valid
// In release it uses C style cast
template<typename T, typename T2>
inline T checked_cast(T2& a)
{
#ifdef _DEBUG
	T b = dynamic_cast<T>(a);
	assert(b);
#else
	T b = (T)a;
#endif
	return b;
}

//-----------------------------------------------------------------------------
// Offset cast
template<typename T>
inline T& offset_cast(void* data, uint offset)
{
	byte* b = ((byte*)data) + offset;
	return *(T*)b;
}

//-----------------------------------------------------------------------------
// Cast using union
template<typename To, typename From>
inline To union_cast(const From& f)
{
	union
	{
		To to;
		From from;
	} a;

	a.from = f;
	return a.to;
}

//-----------------------------------------------------------------------------
// Absolute cast
template <typename destT, typename srcT>
destT &absolute_cast(srcT &v)
{
	return reinterpret_cast<destT&>(v);
}
template <typename destT, typename srcT>
const destT &absolute_cast(const srcT &v)
{
	return reinterpret_cast<const destT&>(v);
}

//-----------------------------------------------------------------------------
template<typename T>
class Singleton
{
	static T instance;
public:
	static T& Get() { return instance; }
};

//-----------------------------------------------------------------------------
template<typename T>
class SingletonPtr
{
	static T* instance;
public:
	static T& Get()
	{
		if(!instance)
			instance = new T;
		return *instance;
	}

	static T* TryGet()
	{
		return instance;
	}

	static void Free()
	{
		delete instance;
		instance = nullptr;
	}
};

//-----------------------------------------------------------------------------
// Return true if any element matches condition
template<typename T, typename Arg>
inline bool Any(const T& item, const Arg& arg)
{
	return item == arg;
}

template<typename T, typename Arg, typename... Args>
inline bool Any(const T& item, const Arg& arg, const Args&... args)
{
	return item == arg || Any(item, args...);
}

//-----------------------------------------------------------------------------
struct FileTime
{
	uint64 time;

	bool operator == (const FileTime& file_time) const;
	bool operator != (const FileTime& file_time) const
	{
		return !operator ==(file_time);
	}
};

//-----------------------------------------------------------------------------
#include "Containers.h"
#include "CoreMath.h"
#include "Color.h"
#include "Logger.h"
#include "Text.h"
