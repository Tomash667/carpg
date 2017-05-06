#pragma once

#include <type_traits>

//-----------------------------------------------------------------------------
// Buffer - used by MemoryStream
class Buffer
{
public:
	void* At(uint offset)
	{
		return data.data() + offset;
	}
	void* Data()
	{
		return data.data();
	}
	// decompress buffer to new buffer and return it, old one is freed
	Buffer* Decompress(uint real_size);
	void Resize(uint size)
	{
		data.resize(size);
	}
	uint Size() const
	{
		return data.size();
	}

private:
	vector<byte> data;
};
extern ObjectPool<Buffer> BufferPool;

//-----------------------------------------------------------------------------
class BufferHandle
{
public:
	BufferHandle(Buffer* buf) : buf(buf) {}
	~BufferHandle()
	{
		if(buf)
			BufferPool.Free(buf);
	}

	operator bool() const
	{
		return buf != nullptr;
	}

	Buffer* operator -> ()
	{
		return buf;
	}

	Buffer* Pin()
	{
		Buffer* b = buf;
		buf = nullptr;
		return b;
	}

private:
	Buffer* buf;
};

//-----------------------------------------------------------------------------
// Base stream source
class StreamSource
{
public:
	virtual ~StreamSource() {}

	uint GetOffset() const { return offset; }
	uint GetSize() const { return size; }
	uint GetSizeLeft() const { return size - offset; }
	virtual bool IsFile() const = 0;
	bool IsValid() const { return valid; }
	bool Ensure(uint data_size) const
	{
		uint new_offset;
		return checked_add(offset, data_size, new_offset) && new_offset <= size;
	}
	bool Ensure(uint element_size, uint count) const
	{
		uint new_offset;
		return checked_multiply_add(element_size, count, offset, new_offset) && new_offset <= size;
	}
	virtual bool Read(void* ptr, uint data_size) = 0;
	virtual bool Skip(uint data_size) = 0;
	virtual void Write(const void* ptr, uint data_size) = 0;

protected:
	uint offset, size;
	bool valid;
};

//-----------------------------------------------------------------------------
// Pool used to create and destroy stream sources
class StreamSourcePool
{
public:
	template<typename T, typename ...U>
	static T* Get(U... args)
	{
		static_assert(std::is_base_of<StreamSource, T>::value, "T must derive StreamSource");
		static_assert(sizeof(T) <= size, "T size can't be larger then StreamSourcePool::size");
		byte* mem;
		if(self.pool.empty())
			mem = new byte[size];
		else
		{
			mem = self.pool.back();
			self.pool.pop_back();
		}
		T* item = (T*)mem;
		new (item)T(args...);
		return item;
	}

	template<typename T>
	static void Free(T* item)
	{
		static_assert(std::is_base_of<StreamSource, T>::value, "T must derive StreamSource");
		assert(item);
		item->~T();
		self.pool.push_back((byte*)item);
	}

	StreamSourcePool()
	{
		const uint to_reserve = 8;
		const uint to_create = 2;

		pool.reserve(to_reserve);
		pool.resize(to_create);
		for(uint i = 0; i < to_create; ++i)
			pool[i] = new byte[size];
	}

	~StreamSourcePool()
	{
		DeleteElements(pool);
	}

private:
	static const uint size = 36u;
	static StreamSourcePool self;
	vector<byte*> pool;
};

//-----------------------------------------------------------------------------
// File stream source
// allow to clamp size offset for reading file from pak
class FileSource : public StreamSource
{
public:
	// Open file in selected mode
	FileSource(bool write, const string& path);
	// Copy handle, if read mode and size is valid, file will act as clamped (can't read outside of specified region)
	FileSource(bool write, HANDLE file, uint clamp_offset = 0, uint clamp_size = 0);
	~FileSource();

	bool IsFile() const { return true; }
	HANDLE PinFile();
	bool Read(void* ptr, uint data_size);
	bool Skip(uint data_size);
	void Write(const void* ptr, uint data_size);
	void Refresh();

private:
	HANDLE file;
	DWORD tmp;
	uint real_size, real_offset;
	bool own_handle, write_mode;
};

//-----------------------------------------------------------------------------
// Memory stream source
class MemorySource : public StreamSource
{
public:
	MemorySource(Buffer* buf);
	~MemorySource();

	bool IsFile() const { return false; }
	Buffer* PinBuffer();
	bool Read(void* ptr, uint data_size);
	bool Skip(uint data_size);
	void Write(const void* ptr, uint data_size);

private:
	Buffer* buf;
};

//-----------------------------------------------------------------------------
// BitStream source
class BitStreamSource : public StreamSource
{
public:
	BitStreamSource(BitStream* bitstream, bool write);

	bool IsFile() const { return false; }
	bool Read(void* ptr, uint data_size);
	bool Skip(uint data_size);
	void Write(const void* ptr, uint data_size);

private:
	BitStream* bitstream;
	bool write;
};

//-----------------------------------------------------------------------------
// Base stream
class Stream
{
public:
	~Stream();

	StreamSource* GetSource() { return source; }
	Buffer* PinBuffer();
	HANDLE PinFile();

protected:
	StreamSource* source;
};

//-----------------------------------------------------------------------------
// Stream reader
class StreamReader : public Stream
{
public:
	StreamReader(const string& path);
	StreamReader(HANDLE file, uint clamp_offset = 0, uint clamp_size = 0);
	StreamReader(Buffer* buf);
	StreamReader(BufferHandle& buf);
	StreamReader(BitStream& bitstream);

	operator bool() const { return ok; }

	uint GetSize() const { return source->GetSize(); }
	bool Ensure(uint size) const { return ok && source->Ensure(size); }
	bool Ensure(uint element_size, uint count) const { return ok && source->Ensure(element_size, count); }
	BufferHandle ReadToBuffer(uint size);
	bool Read(void* ptr, uint size) { return ok && source->Read(ptr, size); }
	bool Read(string& s);
	Buffer* ReadAll();
	bool ReadString1();
	cstring ReadString1C();
	bool ReadString1(string& s) { return Read(s); }
	bool Skip(uint count) { return ok && source->Skip(count); }

	template<typename T>
	bool Read(T& obj)
	{
		return Read(&obj, sizeof(T));
	}

	template<typename T>
	bool operator >> (T& obj)
	{
		return Read(obj);
	}

	static StreamReader LoadAsMemoryStream(const string& path);
	static StreamReader LoadAsMemoryStream(HANDLE file, uint offset = 0, uint size = 0);
	static Buffer* LoadToBuffer(const string& path);
	static Buffer* LoadToBuffer(HANDLE file, uint offset = 0, uint size = 0);

	template<typename SizeType, typename T>
	bool ReadVector(vector<T>& v)
	{
		SizeType count;
		if(!Read(count))
			return false;
		v.resize(count);
		return Read(v.data(), sizeof(T) * count);
	}

	template<typename T>
	bool operator >> (vector<T>& v)
	{
		return ReadVector<uint>(v);
	}

	void Refresh();

	template<typename T>
	T Read()
	{
		T obj;
		Read(obj);
		return obj;
	}

private:
	bool ok;
};

//-----------------------------------------------------------------------------
// Stream writer
class StreamWriter : public Stream
{
public:
	StreamWriter(HANDLE file);
	StreamWriter(BitStream& bitstream);

	void Write(const void* ptr, uint size) { source->Write(ptr, size); }

	template<typename T>
	void Write(const T& obj)
	{
		Write(&obj, sizeof(T));
	}

	bool Ensure(uint size) const { return source->Ensure(size); }
	bool Ensure(uint element_size, uint count) const { return source->Ensure(element_size, count); }

	template<typename T>
	void operator << (const T& obj)
	{
		Write(obj);
	}

	template<typename SizeType, typename T>
	void WriteVector(const vector<T>& v)
	{
		assert(v.size() <= std::numeric_limits<SizeType>::max());
		SizeType count = (SizeType)v.size();
		Write(count);
		if(!v.empty())
			Write(v.data(), sizeof(T) * count);
	}

	template<typename T>
	void operator << (const vector<T>& v)
	{
		WriteVector<uint>(v);
	}

	void Refresh();

	template<typename LengthType>
	void WriteString(const string& s)
	{
		assert(std::numeric_limits<LengthType>::max() > s.length());
		LengthType length = (LengthType)s.length();
		Write(length);
		Write(s.data(), length);
	}

	template<>
	void operator << (const string& s)
	{
		WriteString<byte>(s);
	}
};
