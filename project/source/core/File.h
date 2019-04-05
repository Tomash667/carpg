#pragma once

//-----------------------------------------------------------------------------
// Bufor
extern char BUF[256];

//-----------------------------------------------------------------------------
namespace io
{
	struct FileInfo
	{
		cstring filename;
		bool is_dir;
	};

	// Delete directory.
	bool DeleteDirectory(cstring dir);
	// Check if directory exists.
	bool DirectoryExists(cstring dir);
	// Check if file exists.
	bool FileExists(cstring filename);
	// Find files matching pattern, return false from func to stop.
	bool FindFiles(cstring pattern, delegate<bool(const FileInfo&)> func);
	// Call ShellExecute on file
	void Execute(cstring file);
	// get filename from path, returned string use same string as argument
	cstring FilenameFromPath(const string& path);
	cstring FilenameFromPath(cstring path);
	// load text file to string (whole or up to max size)
	bool LoadFileToString(cstring path, string& str, uint max_size = (uint)-1);
	// simple encryption (pass encrypted to decrypt data)
	void Crypt(char* inp, uint inplen, cstring key, uint keylen);
	// open url in default web browser
	void OpenUrl(Cstring url);
}

//-----------------------------------------------------------------------------
typedef void* FileHandle;
const FileHandle INVALID_FILE_HANDLE = (FileHandle)(int*)-1;

//-----------------------------------------------------------------------------
class StreamReader
{
public:
	StreamReader() : ok(false) {}
	StreamReader(const StreamReader&) = delete;
	virtual ~StreamReader() {}

	virtual uint GetSize() const = 0;
	virtual uint GetPos() const = 0;
	virtual bool SetPos(uint pos) = 0;
	virtual void Read(void* ptr, uint size) = 0;
	virtual void Skip(uint size) = 0;

	bool IsOk() const { return ok; }
	operator bool() const { return IsOk(); }
	const string& GetStringBuffer() const { return buf; }

	bool Ensure(uint elements_size) const
	{
		auto pos = GetPos();
		uint offset;
		return ok && CheckedAdd(pos, elements_size, offset) && offset <= GetSize();
	}
	bool Ensure(uint count, uint element_size) const
	{
		auto pos = GetPos();
		uint offset;
		return ok && CheckedMultiplyAdd(count, element_size, pos, offset) && offset <= GetSize();
	}
	template<typename T>
	T Read()
	{
		static_assert(!std::is_pointer<T>::value, "Value is pointer!");
		T a;
		Read(&a, sizeof(T));
		return a;
	}
	template<typename T>
	void Read(T& a)
	{
		static_assert(!std::is_pointer<T>::value, "Value is pointer!");
		Read(&a, sizeof(a));
	}
	template<typename T>
	void operator >> (T& a)
	{
		static_assert(!std::is_pointer<T>::value, "Value is pointer!");
		Read(&a, sizeof(a));
	}

	template<typename T, typename T2>
	void ReadCasted(T2& a)
	{
		static_assert(!std::is_pointer<T>::value && !std::is_pointer<T2>::value, "Value is pointer!");
		a = (T2)Read<T>();
	}

	template<typename LengthType>
	const string& ReadString()
	{
		LengthType len = Read<LengthType>();
		if(!ok || len == 0)
			buf.clear();
		else
		{
			buf.resize(len);
			Read((char*)buf.c_str(), len);
		}
		return buf;
	}
	const string& ReadString1()
	{
		return ReadString<byte>();
	}
	const string& ReadString2()
	{
		return ReadString<word>();
	}
	const string& ReadString4()
	{
		return ReadString<uint>();
	}

	template<typename LengthType>
	void ReadString(string& s)
	{
		LengthType len = Read<LengthType>();
		if(!ok || len == 0)
			s.clear();
		else
		{
			s.resize(len);
			Read((char*)s.c_str(), len);
		}
	}
	void ReadString1(string& s)
	{
		ReadString<byte>(s);
	}
	void ReadString2(string& s)
	{
		ReadString<word>(s);
	}
	void ReadString4(string& s)
	{
		ReadString<uint>(s);
	}
	template<>
	void Read(string& s)
	{
		ReadString1(s);
	}
	void operator >> (string& s)
	{
		ReadString1(s);
	}

	template<typename CountType, typename LengthType>
	void ReadStringArray(vector<string>& strs)
	{
		CountType count = Read<CountType>();
		if(!ok || count == 0)
			strs.clear();
		else
		{
			strs.resize(count);
			for(string& str : strs)
				ReadString<LengthType>(str);
		}
	}

	template<typename T>
	void Skip(typename std::enable_if<(sizeof(T) <= 8)>::type* = 0)
	{
		static_assert(!std::is_pointer<T>::value, "Value is pointer!");
		T val;
		Read(val);
	}
	template<typename T>
	void Skip(typename std::enable_if<(sizeof(T) > 8)>::type* = 0)
	{
		static_assert(!std::is_pointer<T>::value, "Value is pointer!");
		Skip(sizeof(T));
	}
	template<typename LengthType>
	void SkipData()
	{
		LengthType length = Read<LengthType>();
		if(!ok || length == 0)
			return;
		Skip(length);
	}
	template<typename LengthType>
	void SkipString()
	{
		SkipData<LengthType>();
	}
	void SkipString1()
	{
		SkipString<byte>();
	}
	template<typename CountType, typename LengthType>
	void SkipStringArray()
	{
		CountType count = Read<CountType>();
		if(!ok || count == 0)
			return;
		for(CountType i = 0; i < count; ++i)
			SkipString<LengthType>();
	}

	bool Read0()
	{
		return Read<byte>() == 0;
	}
	bool Read1()
	{
		return Read<byte>() == 1;
	}

	template<typename SizeType, typename T>
	void ReadVector(vector<T>& v)
	{
		static_assert(!std::is_pointer<T>::value, "Value is pointer!");
		SizeType size = Read<SizeType>();
		if(!ok || size == 0)
			v.clear();
		else
		{
			v.resize(size);
			Read(v.data(), sizeof(T) * size);
		}
	}
	template<typename T>
	void ReadVector1(vector<T>& v)
	{
		ReadVector<byte>(v);
	}
	template<typename T>
	void ReadVector2(vector<T>& v)
	{
		ReadVector<word>(v);
	}
	template<typename T>
	void ReadVector4(vector<T>& v)
	{
		ReadVector<uint>(v);
	}
	template<typename T>
	void Read(vector<T>& v)
	{
		ReadVector4(v);
	}
	template<typename T>
	void operator >> (vector<T>& v)
	{
		ReadVector4(v);
	}

	template<typename CastType, typename SizeType = uint, typename T>
	void ReadVectorCasted(vector<T>& v)
	{
		static_assert(!std::is_pointer<T>::value, "Value is pointer!");
		SizeType size = Read<SizeType>();
		if(!ok || size == 0)
			v.clear();
		else if(!Ensure(sizeof(CastType) * size))
		{
			ok = false;
			v.clear();
		}
		else
		{
			v.resize(size);
			Read((char*)v.data(), sizeof(CastType)*size);
			T* to = v.data() + size - 1;
			CastType* from = ((CastType*)v.data()) + size - 1;
			while(size > 0)
			{
				*to = (T)*from;
				--to;
				--from;
				--size;
			}
		}
	}

	Buffer* ReadToBuffer(uint size)
	{
		if(!Ensure(size))
			return nullptr;
		Buffer* buf = Buffer::Get();
		buf->Resize(size);
		Read(buf->Data(), size);
		return buf;
	}
	Buffer* ReadToBuffer(uint offset, uint size)
	{
		if(!SetPos(offset) || !Ensure(size))
			return nullptr;
		Buffer* buf = Buffer::Get();
		buf->Resize(size);
		Read(buf->Data(), size);
		return buf;
	}

protected:
	bool ok;
	static string buf;
};

//-----------------------------------------------------------------------------
class FileReader : public StreamReader
{
public:
	FileReader() : file(INVALID_FILE_HANDLE), size(0), own_handle(false) {}
	explicit FileReader(FileHandle file);
	explicit FileReader(Cstring filename) { Open(filename); }
	~FileReader();
	void operator = (FileReader& f);

	bool Open(Cstring filename);
	using StreamReader::Read;
	void Read(void* ptr, uint size) override final;
	void ReadToString(string& s);
	using StreamReader::ReadToBuffer;
	static Buffer* ReadToBuffer(Cstring path);
	static Buffer* ReadToBuffer(Cstring path, uint offset, uint size);
	using StreamReader::Skip;
	void Skip(uint size) override final;
	uint GetSize() const override final { return size; }
	uint GetPos() const override final;
	FileHandle GetHandle() const { return file; }
	FileTime GetTime() const;
	bool IsOpen() const { return file != INVALID_FILE_HANDLE; }
	bool SetPos(uint pos) override final;

protected:
	FileHandle file;
	uint size;
	bool own_handle;
};

//-----------------------------------------------------------------------------
class MemoryReader final : public StreamReader
{
public:
	MemoryReader(BufferHandle& buf);
	MemoryReader(Buffer* buf);
	~MemoryReader();

	using StreamReader::Read;
	void Read(void* ptr, uint size) override;
	using StreamReader::Skip;
	void Skip(uint size) override;
	uint GetSize() const override { return buf.Size(); }
	uint GetPos() const override { return pos; }
	bool SetPos(uint pos) override;

private:
	Buffer& buf;
	uint pos;
};

//-----------------------------------------------------------------------------
class StreamWriter
{
public:
	virtual ~StreamWriter() {}

	virtual void Write(const void* ptr, uint size) = 0;
	virtual uint GetPos() const = 0;
	virtual bool SetPos(uint pos) = 0;

	template<typename T>
	void Write(const T& a)
	{
		static_assert(!std::is_pointer<T>::value, "Value is pointer!");
		Write(&a, sizeof(a));
	}
	template<typename T>
	void operator << (const T& a)
	{
		static_assert(!std::is_pointer<T>::value, "Value is pointer!");
		Write(&a, sizeof(a));
	}

	template<typename T, typename T2>
	void WriteCasted(const T2& a)
	{
		static_assert(!std::is_pointer<T>::value && !std::is_pointer<T2>::value, "Value is pointer!");
		Write(&a, sizeof(T));
	}

	template<typename SizeType>
	void WriteString(const string& s)
	{
		assert(s.length() <= std::numeric_limits<SizeType>::max());
		SizeType length = (SizeType)s.length();
		Write(length);
		Write(s.c_str(), length);
	}
	void WriteString1(const string& s)
	{
		WriteString<byte>(s);
	}
	void WriteString2(const string& s)
	{
		WriteString<word>(s);
	}
	void WriteString4(const string& s)
	{
		WriteString<uint>(s);
	}

	template<typename SizeType>
	void WriteString(cstring str)
	{
		assert(str);
		uint length = strlen(str);
		assert(length <= std::numeric_limits<SizeType>::max());
		WriteCasted<SizeType>(length);
		Write(str, length);
	}
	void WriteString1(cstring str)
	{
		WriteString<byte>(str);
	}
	void WriteString2(cstring str)
	{
		WriteString<word>(str);
	}
	void WriteString4(cstring str)
	{
		WriteString<uint>(str);
	}
	void WriteStringF(cstring str)
	{
		uint length = strlen(str);
		Write(str, length);
	}

	template<typename CountType, typename LengthType>
	void WriteStringArray(const vector<string>& strs)
	{
		assert(strs.size() <= (uint)std::numeric_limits<CountType>::max());
		WriteCasted<CountType>(strs.size());
		for(const string& str : strs)
			WriteString<LengthType>(str);
	}

	template<>
	void Write(const string& s)
	{
		WriteString1(s);
	}
	void Write(cstring str)
	{
		WriteString1(str);
	}
	void operator << (const string& s)
	{
		WriteString1(s);
	}
	void operator << (cstring str)
	{
		assert(str);
		WriteString1(str);
	}

	void Write0()
	{
		WriteCasted<byte>(0);
	}
	void Write1()
	{
		WriteCasted<byte>(1);
	}

	template<typename SizeType, typename T>
	void WriteVector(const vector<T>& v)
	{
		static_assert(!std::is_pointer<T>::value, "Value is pointer!");
		assert(v.size() <= (size_t)std::numeric_limits<SizeType>::max());
		SizeType size = (SizeType)v.size();
		Write(size);
		Write(v.data(), size * sizeof(T));
	}
	template<typename T>
	void WriteVector1(const vector<T>& v)
	{
		WriteVector<byte>(v);
	}
	template<typename T>
	void WriteVector2(const vector<T>& v)
	{
		WriteVector<word>(v);
	}
	template<typename T>
	void WriteVector4(const vector<T>& v)
	{
		WriteVector<uint>(v);
	}
	template<typename T>
	void Write(const vector<T>& v)
	{
		WriteVector4(v);
	}
	template<typename T>
	void operator << (const vector<T>& v)
	{
		WriteVector4(v);
	}

	template<typename CastType, typename SizeType = uint, typename T>
	void WriteVectorCasted(const vector<T>& v)
	{
		static_assert(!std::is_pointer<T>::value, "Value is pointer!");
		assert(v.size() <= (size_t)std::numeric_limits<SizeType>::max());
		SizeType size = (SizeType)v.size();
		Write(size);
		for(const T& e : v)
			WriteCasted<CastType>(e);
	}

	template<typename T>
	uint BeginPatch(const T& a)
	{
		static_assert(!std::is_pointer<T>::value, "Value is pointer!");
		uint pos = GetPos();
		Write(a);
		return pos;
	}

	template<typename T>
	void Patch(uint pos, const T& a)
	{
		static_assert(!std::is_pointer<T>::value, "Value is pointer!");
		uint current_pos = GetPos();
		SetPos(pos);
		Write(a);
		SetPos(current_pos);
	}
};

//-----------------------------------------------------------------------------
class FileWriter : public StreamWriter
{
public:
	FileWriter() : file(INVALID_FILE_HANDLE), own_handle(true) {}
	explicit FileWriter(FileHandle file) : file(file), own_handle(false) {}
	explicit FileWriter(cstring filename) : own_handle(true) { Open(filename); }
	explicit FileWriter(const string& filename) : FileWriter(filename.c_str()) {}
	~FileWriter();

	bool Open(cstring filename);
	using StreamWriter::Write;
	void Write(const void* ptr, uint size) override final;
	void Flush();
	uint GetPos() const override final;
	uint GetSize() const;
	FileHandle GetHandle() const { return file; }
	bool IsOpen() const { return file != INVALID_FILE_HANDLE; }
	operator bool() const { return IsOpen(); }
	void operator = (FileWriter& f);
	void SetTime(FileTime file_time);
	bool SetPos(uint pos) override final;

protected:
	FileHandle file;
	bool own_handle;
};

//-----------------------------------------------------------------------------
class TextWriter
{
public:
	explicit TextWriter(Cstring filename) : file(filename)
	{
	}

	operator bool() const
	{
		return file.IsOpen();
	}

	void Flush()
	{
		file.Flush();
	}
	void Write(cstring str)
	{
		file.Write(str, strlen(str));
	}

	void operator << (const string& str)
	{
		file.Write(str.c_str(), str.length());
	}
	void operator << (cstring str)
	{
		Write(str);
	}
	void operator << (char c)
	{
		file << c;
	}
	void operator << (int i)
	{
		Write(Format("%d", i));
	}
	void operator << (float f)
	{
		Write(Format("%g", f));
	}

private:
	FileWriter file;
};
