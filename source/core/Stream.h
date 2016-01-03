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
	inline bool IsValid() const { return valid; }
	inline bool Ensure(uint data_size) const { return offset + data_size <= size; }
	virtual bool Read(void* ptr, uint data_size) = 0;
	virtual bool Skip(uint data_size) = 0;
	virtual void Write(void* ptr, uint data_size) = 0;

protected:
	uint offset, size;
	bool valid;
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

	bool Read(void* ptr, uint data_size);
	bool Skip(uint data_size);
	void Write(void* ptr, uint data_size);

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

	bool Read(void* ptr, uint data_size);
	bool Skip(uint data_size);
	void Write(void* ptr, uint data_size);

private:
	Buffer* buf;
};

//-----------------------------------------------------------------------------
// Base stream
class Stream
{
public:
	inline StreamSource* GetSource() { return source; }

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
	~StreamReader();

	inline operator bool() const { return ok; }

	bool Ensure(uint size) { return ok && source->Ensure(size); }
	inline bool Read(void* ptr, uint size) { return ok && source->Read(ptr, size); }
	bool Read(string& s);
	Buffer* ReadAll();
	bool ReadString1();
	inline bool Skip(uint count) { return ok && source->Skip(count); }
	
	template<typename T>
	inline bool Read(T& obj)
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
};
