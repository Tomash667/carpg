#pragma once

//-----------------------------------------------------------------------------
// Bufor
extern char BUF[256];
extern DWORD tmp;

//-----------------------------------------------------------------------------
namespace io
{
	// Delete directory.
	bool DeleteDirectory(cstring dir);
	// Check if directory exists.
	bool DirectoryExists(cstring dir);
	// Check if file exists.
	bool FileExists(cstring filename);
	// Find files matching pattern, return false from func to stop.
	bool FindFiles(cstring pattern, const std::function<bool(const WIN32_FIND_DATA&)>& func, bool exclude_special = true);
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
// Funkcje zapisuj¹ce/wczytuj¹ce z pliku
//-----------------------------------------------------------------------------
template<typename T>
inline void WriteString(HANDLE file, const string& s)
{
	assert(s.length() <= std::numeric_limits<T>::max());
	T len = (T)s.length();
	WriteFile(file, &len, sizeof(len), &tmp, nullptr);
	if(len)
		WriteFile(file, s.c_str(), len, &tmp, nullptr);
}

template<typename T>
inline void ReadString(HANDLE file, string& s)
{
	T len;
	ReadFile(file, &len, sizeof(len), &tmp, nullptr);
	if(len)
	{
		s.resize(len);
		ReadFile(file, (char*)s.c_str(), len, &tmp, nullptr);
	}
	else
		s.clear();
}

inline void WriteString1(HANDLE file, const string& s)
{
	WriteString<byte>(file, s);
}

inline void WriteString2(HANDLE file, const string& s)
{
	WriteString<word>(file, s);
}

inline void ReadString1(HANDLE file, string& s)
{
	ReadString<byte>(file, s);
}

inline void ReadString1(HANDLE file)
{
	byte len;
	ReadFile(file, &len, sizeof(len), &tmp, nullptr);
	if(len == 0)
		BUF[0] = 0;
	else
	{
		ReadFile(file, BUF, len, &tmp, nullptr);
		BUF[len] = 0;
	}
}

inline void ReadString2(HANDLE file, string& s)
{
	ReadString<word>(file, s);
}

template<typename COUNT_TYPE, typename STRING_SIZE_TYPE>
inline void WriteStringArray(HANDLE file, vector<string>& strings)
{
	COUNT_TYPE ile = (COUNT_TYPE)strings.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	for(vector<string>::iterator it = strings.begin(), end = strings.end(); it != end; ++it)
		WriteString<STRING_SIZE_TYPE>(file, *it);
}

template<typename COUNT_TYPE, typename STRING_SIZE_TYPE>
inline void ReadStringArray(HANDLE file, vector<string>& strings)
{
	COUNT_TYPE ile;
	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	strings.resize(ile);
	for(vector<string>::iterator it = strings.begin(), end = strings.end(); it != end; ++it)
		ReadString<STRING_SIZE_TYPE>(file, *it);
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
		T a;
		Read(&a, sizeof(T));
		return a;
	}
	template<typename T>
	void Read(T& a)
	{
		Read(&a, sizeof(a));
	}
	template<typename T>
	void operator >> (T& a)
	{
		Read(&a, sizeof(a));
	}

	template<typename T, typename T2>
	void ReadCasted(T2& a)
	{
		a = (T2)Read<T>();
	}

	template<typename SizeType>
	const string& ReadString()
	{
		SizeType len = Read<SizeType>();
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

	template<typename SizeType>
	void ReadString(string& s)
	{
		SizeType len = Read<SizeType>();
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

	template<typename T>
	void Skip(typename std::enable_if<(sizeof(T) <= 8)>::type* = 0)
	{
		T val;
		Read(val);
	}
	template<typename T>
	void Skip(typename std::enable_if<(sizeof(T) > 8)>::type* = 0)
	{
		Skip(sizeof(T));
	}

	template<typename SizeType, typename T>
	void ReadVector(vector<T>& v)
	{
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

	Buffer* ReadToBuffer(uint size)
	{
		if(!Ensure(size))
			return nullptr;
		Buffer* buf = BufferPool.Get();
		buf->Resize(size);
		Read(buf->Data(), size);
		return buf;
	}
	Buffer* ReadToBuffer(uint offset, uint size)
	{
		if(!SetPos(offset) || !Ensure(size))
			return nullptr;
		Buffer* buf = BufferPool.Get();
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
	FileReader() : file(INVALID_FILE_HANDLE), own_handle(false) {}
	explicit FileReader(FileHandle file) : file(file), own_handle(false) {}
	explicit FileReader(Cstring filename) { Open(filename); }
	~FileReader();
	void operator = (FileReader& f);

	bool Open(Cstring filename);
	using StreamReader::Read;
	void Read(void* ptr, uint size) override;
	void ReadToString(string& s);
	using StreamReader::ReadToBuffer;
	static Buffer* ReadToBuffer(Cstring path);
	static Buffer* ReadToBuffer(Cstring path, uint offset, uint size);
	using StreamReader::Skip;
	void Skip(uint size) override;
	uint GetSize() const override { return size; }
	uint GetPos() const override;
	FileHandle GetHandle() const { return file; }
	FileTime GetTime() const;
	bool IsOpen() const { return file != INVALID_FILE_HANDLE; }
	bool SetPos(uint pos) override;

protected:
	FileHandle file;
	uint size;
	bool own_handle;
};

//-----------------------------------------------------------------------------
class MemoryReader : public StreamReader
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
		Write(&a, sizeof(a));
	}
	template<typename T>
	void operator << (const T& a)
	{
		Write(&a, sizeof(a));
	}

	template<typename T, typename T2>
	void WriteCasted(const T2& a)
	{
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

	template<typename SizeType, typename T>
	void WriteVector(const vector<T>& v)
	{
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
		WriteVector<short>(v);
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

	template<typename T>
	uint BeginPatch(const T& a)
	{
		uint pos = GetPos();
		Write(a);
		return pos;
	}

	template<typename T>
	void Patch(uint pos, const T& a)
	{
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
	~FileWriter();

	bool Open(cstring filename);
	using StreamWriter::Write;
	void Write(const void* ptr, uint size) override;
	void Flush();
	uint GetPos() const override;
	uint GetSize() const;
	FileHandle GetHandle() const { return file; }
	bool IsOpen() const { return file != INVALID_FILE_HANDLE; }
	operator bool() const { return IsOpen(); }
	void operator = (FileWriter& f);
	void SetTime(FileTime file_time);
	bool SetPos(uint pos) override;

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
		file.Write(str, strlen(str));
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
