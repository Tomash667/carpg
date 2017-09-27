#pragma once

//-----------------------------------------------------------------------------
// Bufor
extern char BUF[256];
extern DWORD tmp;

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
class FileReader
{
public:
	FileReader() : file(INVALID_HANDLE_VALUE), own_handle(false)
	{
	}

	explicit FileReader(HANDLE file) : file(file), own_handle(false)
	{
	}

	explicit FileReader(cstring filename) : own_handle(true)
	{
		Open(filename);
	}

	~FileReader()
	{
		if(own_handle && file != INVALID_HANDLE_VALUE)
		{
			CloseHandle(file);
			file = INVALID_HANDLE_VALUE;
		}
	}

	bool Open(cstring filename)
	{
		assert(filename);
		file = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		own_handle = true;
		return (file != INVALID_HANDLE_VALUE);
	}

	bool IsOpen() const
	{
		return file != INVALID_HANDLE_VALUE;
	}

	bool Read(void* ptr, uint size)
	{
		ReadFile(file, ptr, size, &tmp, nullptr);
		return size == tmp;
	}

	template<typename T>
	bool operator >> (T& a)
	{
		return Read(&a, sizeof(a));
	}

	template<typename T>
	T Read()
	{
		T a;
		Read(&a, sizeof(T));
		return a;
	}

	template<typename T>
	bool Read(T& a)
	{
		return Read(&a, sizeof(a));
	}

	template<typename T, typename T2>
	bool ReadCasted(T2& a)
	{
		T b;
		if(!Read<T>(b))
			return false;
		a = (T2)b;
		return true;
	}

	bool ReadStringBUF()
	{
		byte len = Read<byte>();
		if(len == 0)
		{
			BUF[0] = 0;
			return true;
		}
		else
		{
			BUF[len] = 0;
			return Read(BUF, len);
		}
	}

	template<typename T>
	void Skip()
	{
		SetFilePointer(file, sizeof(T), nullptr, FILE_CURRENT);
	}

	void Skip(int bytes)
	{
		SetFilePointer(file, bytes, nullptr, FILE_CURRENT);
	}

	bool ReadString1(string& s)
	{
		byte len;
		if(!Read(len))
			return false;
		s.resize(len);
		return Read((char*)s.c_str(), len);
	}

	bool ReadString2(string& s)
	{
		word len;
		if(!Read(len))
			return false;
		s.resize(len);
		return Read((char*)s.c_str(), len);
	}

	bool operator >> (string& s)
	{
		return ReadString1(s);
	}

	operator bool() const
	{
		return file != INVALID_HANDLE_VALUE;
	}

	void ReadToString(string& s)
	{
		DWORD size = GetFileSize(file, nullptr);
		s.resize(size);
		ReadFile(file, (char*)s.c_str(), size, &tmp, nullptr);
		assert(size == tmp);
	}

	template<typename T>
	void ReadVector1(vector<T>& v)
	{
		byte count;
		Read(count);
		v.resize(count);
		if(count)
			Read(&v[0], sizeof(T)*count);
	}

	template<typename T>
	void ReadVector2(vector<T>& v)
	{
		word count;
		Read(count);
		v.resize(count);
		if(count)
			Read(&v[0], sizeof(T)*count);
	}

	uint GetSize() const
	{
		return GetFileSize(file, nullptr);
	}

	HANDLE file;
	bool own_handle;
};

//-----------------------------------------------------------------------------
class FileWriter
{
public:
	FileWriter() : file(INVALID_HANDLE_VALUE), own_handle(true)
	{
	}

	explicit FileWriter(HANDLE file) : file(file), own_handle(false)
	{
	}

	explicit FileWriter(cstring filename) : own_handle(true)
	{
		Open(filename);
	}

	~FileWriter()
	{
		if(own_handle && file != INVALID_HANDLE_VALUE)
		{
			CloseHandle(file);
			file = INVALID_HANDLE_VALUE;
		}
	}

	bool Open(cstring filename)
	{
		assert(filename);
		file = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		return (file != INVALID_HANDLE_VALUE);
	}

	bool IsOpen() const
	{
		return file != INVALID_HANDLE_VALUE;
	}

	void Write(const void* ptr, uint size)
	{
		WriteFile(file, ptr, size, &tmp, nullptr);
		assert(size == tmp);
	}

	template<typename T>
	void operator << (const T& a)
	{
		Write(&a, sizeof(a));
	}

	template<typename T>
	void Write(const T& a)
	{
		Write(&a, sizeof(a));
	}

	template<typename T, typename T2>
	void WriteCasted(const T2& a)
	{
		Write(&a, sizeof(T));
	}

	void WriteString1(const string& s)
	{
		int length = s.length();
		assert(length < 256);
		WriteCasted<byte>(length);
		if(length)
			Write(s.c_str(), length);
	}

	void WriteString1(cstring str)
	{
		assert(str);
		int length = strlen(str);
		assert(length < 256);
		WriteCasted<byte>(length);
		if(length)
			Write(str, length);
	}

	void WriteString2(const string& s)
	{
		int length = s.length();
		assert(length < 256 * 256);
		WriteCasted<word>(length);
		if(length)
			Write(s.c_str(), length);
	}

	void WriteString2(cstring str)
	{
		assert(str);
		int length = strlen(str);
		assert(length < 256 * 256);
		Write<word>(length);
		if(length)
			Write(str, length);
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

	operator bool() const
	{
		return file != INVALID_HANDLE_VALUE;
	}

	void Write0()
	{
		WriteCasted<byte>(0);
	}

	template<typename T>
	void WriteVector1(const vector<T>& v)
	{
		WriteCasted<byte>(v.size());
		if(!v.empty())
			Write(&v[0], sizeof(T)*v.size());
	}

	template<typename T>
	void WriteVector2(const vector<T>& v)
	{
		WriteCasted<word>(v.size());
		if(!v.empty())
			Write(&v[0], sizeof(T)*v.size());
	}

	void Flush()
	{
		FlushFileBuffers(file);
	}

	uint GetSize() const
	{
		return GetFileSize(file, nullptr);
	}

	HANDLE file;
	bool own_handle;
};

//-----------------------------------------------------------------------------
// Funkcje do ³atwiejszej edycji bufora
//-----------------------------------------------------------------------------
template<typename T, typename T2>
inline T* ptr_shift(T2* ptr, uint shift)
{
	return ((T*)(((byte*)ptr) + shift));
}

template<typename T>
void* ptr_shift(T* ptr, uint shift)
{
	return (((byte*)ptr) + shift);
}

template<typename T, typename T2>
inline T& ptr_shiftd(T2* ptr, uint shift)
{
	return *((T*)(((byte*)ptr) + shift));
}

//-----------------------------------------------------------------------------
// New io reader/writer
namespace io2
{
	//-----------------------------------------------------------------------------
	// Output source
	class Source
	{
	public:
		virtual ~Source()
		{
		}
		virtual void Write(const void* data, uint size) = 0;
		virtual bool Read(void* data, uint size) = 0;
		virtual bool Skip(uint size) = 0;
		virtual bool Ensure(uint size) = 0;
		virtual bool IsOk() const = 0;
	};

	//-----------------------------------------------------------------------------
	// File output source
	class FileSource final : public Source
	{
	public:
		FileSource() : file(INVALID_HANDLE_VALUE), own_handle(true), ok(false)
		{
		}

		explicit FileSource(cstring path, bool write) : own_handle(true)
		{
			Open(path, write);
		}

		explicit FileSource(HANDLE file) : file(file), own_handle(false), ok(true)
		{
		}

		explicit FileSource(FileSource&& f) : file(f.file), own_handle(f.own_handle), ok(f.ok)
		{
			f.file = INVALID_HANDLE_VALUE;
			f.ok = false;
		}

		~FileSource()
		{
			if(file != INVALID_HANDLE_VALUE && own_handle)
			{
				CloseHandle(file);
				file = INVALID_HANDLE_VALUE;
			}
		}

		bool Open(cstring path, bool write)
		{
			assert(path);
			if(write)
				file = CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			else
				file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			ok = (file != INVALID_HANDLE_VALUE);
			return ok;
		}

		void Write(const void* data, uint size) override
		{
			DWORD tmp;
			WriteFile(file, data, size, &tmp, nullptr);
			assert(size == tmp);
		}

		bool Read(void* data, uint size) override
		{
			DWORD tmp;
			ReadFile(file, data, size, &tmp, nullptr);
			ok = (tmp == size && ok);
			return ok;
		}

		bool Skip(uint size) override
		{
			DWORD pos = SetFilePointer(file, size, nullptr, FILE_CURRENT);
			ok = !(pos == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) && ok;
			return ok;
		}

		bool Ensure(uint size_required) override
		{
			if(!ok)
				return false;
			DWORD pos = SetFilePointer(file, 0, nullptr, FILE_CURRENT);
			DWORD size = GetFileSize(file, nullptr);
			return (size - pos >= size_required);
		}

		bool IsOpen() const
		{
			return file != INVALID_HANDLE_VALUE;
		}

		bool IsOk() const override
		{
			return ok;
		}

		void Flush()
		{
			FlushFileBuffers(file);
		}

		uint GetSize() const
		{
			return GetFileSize(file, nullptr);
		}

	private:
		HANDLE file;
		bool own_handle, ok;
	};

	//-----------------------------------------------------------------------------
	// Generic source, use virtual call to dynamicaly select output source
	class GenericSource final : public Source
	{
	public:
		GenericSource() : source(nullptr)
		{
		}

		GenericSource(Source* source, bool owned) : source(source), owned(owned)
		{
		}

		~GenericSource()
		{
			if(owned)
				delete source;
		}

		void Write(const void* data, uint size) override
		{
			source->Write(data, size);
		}

		bool Read(void* data, uint size) override
		{
			return source->Read(data, size);
		}

		bool Skip(uint size) override
		{
			return source->Skip(size);
		}

		bool Ensure(uint size) override
		{
			return source->Ensure(size);
		}

		bool IsOk() const override
		{
			return source->IsOk();
		}

	private:
		Source* source;
		bool owned;
	};

	//-----------------------------------------------------------------------------
	// Base writer
	template<typename SourceType>
	class BaseWriter
	{
	public:
		SourceType& GetSource()
		{
			return source;
		}

		void Write(const void* data, uint size)
		{
			source.Write(data, size);
		}

		template<typename T>
		void Write(const T& data)
		{
			Write(&data, sizeof(T));
		}
		template<>
		void Write(const string& s)
		{
			WriteString1(s);
		}

		template<typename DestType, typename SourceType>
		void WriteCasted(const SourceType& a)
		{
			DestType val = (DestType)a;
			Write(&val, sizeof(DestType));
		}

		template<typename SizeType>
		void WriteString(const string& s)
		{
			assert(s.length() <= std::numeric_limits<SizeType>::max());
			SizeType len = (SizeType)s.length();
			Write(len);
			if(len)
				Write(s.c_str(), len);
		}
		template<typename SizeType>
		void WriteString(cstring s)
		{
			assert(s && strlen(s) <= std::numeric_limits<SizeType>::max());
			SizeType len = (SizeType)strlen(s);
			Write(len);
			if(len)
				Write(s, len);
		}
		void WriteString1(const string& s)
		{
			WriteString<byte>(s);
		}
		void WriteString1(cstring s)
		{
			WriteString<byte>(s);
		}
		void WriteString2(const string& s)
		{
			WriteString<word>(s);
		}
		void WriteString2(cstring s)
		{
			WriteString<word>(s);
		}
		void WriteString(const string& s)
		{
			WriteString<uint>(s);
		}
		void WriteString(cstring s)
		{
			WriteString<uint>(s);
		}

		template<typename SizeType, typename T>
		void WriteVector(const vector<T>& v)
		{
			assert(v.count() <= std::numeric_limits<SizeType>::max());
			WriteCasted<SizeType>(v.size());
			if(!v.empty())
				Write(&v[0], sizeof(T)*v.size());
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
		void WriteVector(const vector<T>& v)
		{
			WriteVector<uint>(v);
		}

		void Write0()
		{
			WriteCasted<byte>(0);
		}

		template<typename T>
		void operator << (const T& a)
		{
			Write(&a, sizeof(T));
		}
		void operator << (const string& s)
		{
			WriteString1(s);
		}
		void operator << (cstring s)
		{
			WriteString1(s);
		}
		template<typename T>
		void operator << (vector<T>& v)
		{
			WriteVector(v);
		}

		template<>
		void operator << (vector<string>& v)
		{
			WriteCasted<byte>(v.size());
			for(string& s : v)
				WriteString1(s);
		}

	protected:
		BaseWriter()
		{
		}

		explicit BaseWriter(SourceType&& source) : source(std::move(source))
		{
		}

		SourceType source;
	};

	//-----------------------------------------------------------------------------
	// File writer
	class FileWriter final : public BaseWriter<FileSource>
	{
	public:
		FileWriter()
		{
		}

		explicit FileWriter(cstring path) : BaseWriter(FileSource(path, true))
		{
		}

		void Flush()
		{
			source.Flush();
		}

		uint GetSize() const
		{
			return source.GetSize();
		}

		bool IsOpen() const
		{
			return source.IsOpen();
		}

		bool Open(cstring path)
		{
			return source.Open(path, true);
		}

		operator bool() const
		{
			return IsOpen();
		}
	};

	//-----------------------------------------------------------------------------
	// Writer to generic source
	class StreamWriter final : public BaseWriter<GenericSource>
	{
	public:
		StreamWriter()
		{
		}

		template<typename T>
		explicit StreamWriter(BaseWriter<T>& writer) : BaseWriter(GenericSource(&writer.GetSource(), false))
		{
		}
	};

	//-----------------------------------------------------------------------------
	// Base reader
	template<typename SourceType>
	class BaseReader
	{
	public:
		SourceType& GetSource()
		{
			return source;
		}

		bool Read(void* data, uint size)
		{
			return source.Read(data, size);
		}

		template<typename T>
		bool Read(T& data)
		{
			return Read(&data, sizeof(T));
		}

		template<typename DestType, typename SourceType>
		bool ReadCasted(SourceType& a)
		{
			DestType b;
			if(!Read<DestType>(b))
				return false;
			a = (SourceType)b;
			return true;
		}

		template<typename SizeType>
		bool ReadString(string& s)
		{
			SizeType len;
			if(!Read(len))
				return false;
			if(len)
			{
				s.resize(len);
				if(!Read((char*)s.data(), len))
					return false;
			}
			else
				s.clear();
			return true;
		}
		bool ReadString1(string& s)
		{
			return ReadString<byte>(s);
		}
		bool ReadString2(string& s)
		{
			return ReadString<word>(s);
		}
		bool ReadString(string& s)
		{
			return ReadString<uint>(s);
		}

		template<typename SizeType, typename T>
		bool ReadVector(vector<T>& v)
		{
			SizeType count;
			if(!Read(count))
				return false;
			v.resize(count);
			if(count)
				return Read(&v[0], sizeof(T)*count);
			return true;
		}
		template<typename T>
		bool ReadVector1(vector<T>& v)
		{
			return ReadVector<byte>(v);
		}
		template<typename T>
		bool ReadVector2(vector<T>& v)
		{
			return ReadVector<word>(v);
		}
		template<typename T>
		bool ReadVector(vector<T>& v)
		{
			return ReadVector<uint>(v);
		}

		template<typename T>
		bool operator >> (T& a)
		{
			return Read(&a, sizeof(T));
		}
		bool operator >> (string& s)
		{
			return ReadString1(s);
		}
		template<typename T>
		bool operator >> (vector<T>& v)
		{
			return ReadVector(v);
		}
		template<>
		bool operator >> (vector<string>& v)
		{
			byte count;
			if(!Read(count))
				return false;
			v.resize(count);
			for(string& s : v)
			{
				if(!ReadString1(s))
					return false;
			}
			return true;
		}

		bool Ensure(uint size)
		{
			return source.Ensure(size);
		}

	protected:
		BaseReader()
		{
		}

		BaseReader(SourceType&& source) : source(std::move(source))
		{
		}

		SourceType source;
	};

	//-----------------------------------------------------------------------------
	// File reader
	class FileReader final : public BaseReader<FileSource>
	{
	public:
		FileReader()
		{
		}

		FileReader(cstring path) : BaseReader(FileSource(path, false))
		{
		}

		uint GetSize() const
		{
			return source.GetSize();
		}

		bool IsOpen() const
		{
			return source.IsOpen();
		}

		bool IsOk() const
		{
			return source.IsOk();
		}

		bool Open(cstring path)
		{
			return source.Open(path, false);
		}

		bool Skip(uint size)
		{
			return source.Skip(size);
		}
		template<typename T>
		bool Skip()
		{
			return source.Skip(sizeof(T));
		}

		operator bool() const
		{
			return IsOk();
		}
	};

	//-----------------------------------------------------------------------------
	// Reader from generic source
	class StreamReader final : public BaseReader<GenericSource>
	{
	public:
		StreamReader()
		{
		}

		template<typename T>
		StreamReader(BaseReader<T>& reader) : BaseReader(GenericSource(&reader.GetSource(), false))
		{
		}
	};
};
