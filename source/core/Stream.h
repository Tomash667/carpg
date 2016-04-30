#pragma once

//-----------------------------------------------------------------------------
// Buffer - used by MemoryStream
class Buffer
{
public:
	inline void* At(uint offset)
	{
		return data.data() + offset;
	}
	inline void* Data()
	{
		return data.data();
	}
	// decompress buffer to new buffer and return it, old one is freed
	Buffer* Decompress(uint real_size);
	inline void Resize(uint size)
	{
		data.resize(size);
	}
	inline uint Size() const
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

	inline operator bool() const
	{
		return buf != nullptr;
	}

	inline Buffer* operator -> ()
	{
		return buf;
	}

	inline Buffer* Pin()
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

	inline uint GetOffset() const { return offset; }
	inline uint GetSize() const { return size; }
	inline uint GetSizeLeft() const { return size - offset; }
	virtual bool IsFile() const = 0;
	inline bool IsValid() const { return valid; }
	inline bool Ensure(uint data_size) const
	{
		uint new_offset;
		return checked_add(offset, data_size, new_offset) && new_offset <= size;
	}
	inline bool Ensure(uint element_size, uint count) const
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
	inline static T* Get(U... args)
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
	inline static void Free(T* item)
	{
		static_assert(std::is_base_of<StreamSource, T>::value, "T must derive StreamSource");
		assert(item);
		item->~T();
		self.pool.push_back((byte*)item);
	}

	inline StreamSourcePool()
	{
		const uint to_reserve = 8;
		const uint to_create = 2;

		pool.reserve(to_reserve);
		pool.resize(to_create);
		for(uint i = 0; i < to_create; ++i)
			pool[i] = new byte[size];
	}

	inline ~StreamSourcePool()
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

	inline bool IsFile() const { return true; }
	HANDLE PinFile();
	bool Read(void* ptr, uint data_size);
	bool Skip(uint data_size);
	void Write(const void* ptr, uint data_size);

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

	inline bool IsFile() const { return false; }
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

	inline bool IsFile() const { return false; }
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

	inline StreamSource* GetSource() { return source; }
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

	inline operator bool() const { return ok; }

	inline uint GetSize() const { return source->GetSize(); }
	inline bool Ensure(uint size) const { return ok && source->Ensure(size); }
	inline bool Ensure(uint element_size, uint count) const { return ok && source->Ensure(element_size, count); }
	BufferHandle Read(uint size);
	inline bool Read(void* ptr, uint size) { return ok && source->Read(ptr, size); }
	bool Read(string& s);
	Buffer* ReadAll();
	bool ReadString1();
	bool ReadString1(string& s) { return Read(s); }
	inline bool Skip(uint count) { return ok && source->Skip(count); }

	template<typename T>
	inline bool Read(T& obj)
	{
		return Read(&obj, sizeof(T));
	}

	template<typename T>
	inline bool operator >> (T& obj)
	{
		return Read(&obj, sizeof(T));
	}

	static StreamReader LoadAsMemoryStream(const string& path);
	static StreamReader LoadAsMemoryStream(HANDLE file, uint offset = 0, uint size = 0);
	static Buffer* LoadToBuffer(const string& path);
	static Buffer* LoadToBuffer(HANDLE file, uint offset = 0, uint size = 0);

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

	inline void Write(const void* ptr, uint size) { source->Write(ptr, size); }

	template<typename T>
	inline void Write(const T& obj)
	{
		Write(&obj, sizeof(T));
	}

	inline bool Ensure(uint size) const { return source->Ensure(size); }
	inline bool Ensure(uint element_size, uint count) const { return source->Ensure(element_size, count); }

	template<typename T>
	inline void operator << (const T& obj)
	{
		Write(&obj, sizeof(T));
	}
};
