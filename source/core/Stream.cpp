#include "Pch.h"
#include "Base.h"
#include "Stream.h"

//-----------------------------------------------------------------------------
ObjectPool<Buffer> BufferPool;
StreamSourcePool StreamSourcePool::self;

//=================================================================================================
Buffer* Buffer::Decompress(uint real_size)
{
	Buffer* buf = BufferPool.Get();
	buf->Resize(real_size);
	uLong size = real_size;
	uncompress((Bytef*)buf->Data(), &size, (const Bytef*)Data(), Size());
	BufferPool.Free(this);
	return buf;
}

//=================================================================================================
FileSource::FileSource(bool write, const string& path)
{
	if(write)
	{
		file = CreateFile(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		size = 0;
	}
	else
	{
		file = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if(file != INVALID_HANDLE_VALUE)
			size = GetFileSize(file, nullptr);
		else
			size = 0;
	}
	offset = 0;
	own_handle = true;
	valid = (file != INVALID_HANDLE_VALUE);
	write_mode = write;
	real_offset = 0;
	real_size = size;
}

//=================================================================================================
FileSource::FileSource(bool write, HANDLE _file, uint clamp_offset, uint clamp_size)
{
	file = _file;
	write_mode = write;
	own_handle = false;
	offset = 0;
	if(file != INVALID_HANDLE_VALUE)
	{
		real_size = GetFileSize(file, nullptr);
		size = size;
		if(!write && (clamp_offset > real_size || clamp_size > real_size))
		{
			// invalid clamp values
			assert(0);
			valid = false;
			real_offset = 0;
		}
		else
		{
			valid = true;
			if(!write && clamp_size != 0)
			{
				// clamp mode
				real_offset = clamp_offset;
				size = clamp_size;
			}
			SetFilePointer(file, real_offset, nullptr, FILE_BEGIN);
		}
	}
	else
	{
		valid = false;
		size = real_size = 0;
		real_offset = 0;
	}
}

//=================================================================================================
FileSource::~FileSource()
{
	if(own_handle && file != INVALID_HANDLE_VALUE)
		CloseHandle(file);
}

//=================================================================================================
HANDLE FileSource::PinFile()
{
	HANDLE h = file;
	file = INVALID_HANDLE_VALUE;
	StreamSourcePool::Free(this);
	return h;
}

//=================================================================================================
bool FileSource::Read(void* ptr, uint data_size)
{
	assert(ptr && valid && !write_mode);
	if(!Ensure(data_size))
		return false;
	ReadFile(file, ptr, data_size, &tmp, nullptr);
	offset += data_size;
	real_offset += data_size;
	return true;
}

//=================================================================================================
bool FileSource::Skip(uint data_size)
{
	assert(valid);
	if(!Ensure(data_size))
		return false;
	offset += data_size;
	real_offset += data_size;
	SetFilePointer(file, data_size, nullptr, FILE_CURRENT);
	return true;
}

//=================================================================================================
void FileSource::Write(const void* ptr, uint data_size)
{
	assert(ptr && valid && write_mode);
	WriteFile(file, ptr, data_size, &tmp, nullptr);
	offset += data_size;
	real_offset += data_size;
}

//=================================================================================================
MemorySource::MemorySource(Buffer* _buf)
{
	buf = _buf;
	if(buf != nullptr)
	{
		valid = true;
		size = buf->Size();
	}
	else
	{
		valid = false;
		size = 0;
	}
	offset = 0;
}

//=================================================================================================
MemorySource::~MemorySource()
{
	if(buf)
		BufferPool.Free(buf);
}

//=================================================================================================
Buffer* MemorySource::PinBuffer()
{
	Buffer* b = buf;
	buf = nullptr;
	StreamSourcePool::Free(this);
	return b;
}

//=================================================================================================
bool MemorySource::Read(void* ptr, uint data_size)
{
	assert(ptr && valid);
	if(!Ensure(data_size))
		return false;
	memcpy(ptr, buf->At(offset), data_size);
	offset += data_size;
	return true;
}

//=================================================================================================
bool MemorySource::Skip(uint data_size)
{
	assert(valid);
	if(!Ensure(data_size))
		return false;
	offset += data_size;
	return true;
}

//=================================================================================================
void MemorySource::Write(const void* ptr, uint data_size)
{
	assert(ptr && valid);
	size += data_size;
	buf->Resize(size);
	memcpy(buf->At(offset), ptr, data_size);
	offset += data_size;
}

//=================================================================================================
BitStreamSource::BitStreamSource(BitStream* bitstream, bool write) : bitstream(bitstream), write(write)
{
	assert(bitstream);
	size = bitstream->GetNumberOfBytesUsed();
	offset = (write ? bitstream->GetWriteOffset() : bitstream->GetReadOffset()) / 8;
	valid = true;
}

//=================================================================================================
bool BitStreamSource::Read(void* ptr, uint data_size)
{
	assert(ptr && !write);
	if(!Ensure(data_size))
		return false;
	bitstream->Read((char*)ptr, data_size);
	offset += data_size;
	return true;
}

//=================================================================================================
bool BitStreamSource::Skip(uint data_size)
{
	assert(!write);
	if(!Ensure(data_size))
		return false;
	offset += data_size;
	uint pos = offset * 8;
	if(write)
		bitstream->SetWriteOffset(pos);
	else
		bitstream->SetReadOffset(pos);
	return true;
}

//=================================================================================================
void BitStreamSource::Write(const void* ptr, uint data_size)
{
	assert(ptr && write);
	bitstream->Write((const char*)ptr, data_size);
	size += data_size;
	offset += data_size;
}

//=================================================================================================
Stream::~Stream()
{
	if(source)
		StreamSourcePool::Free(source);
}

//=================================================================================================
Buffer* Stream::PinBuffer()
{
	if(source && source->IsValid() && !source->IsFile())
	{
		MemorySource* mem = (MemorySource*)source;
		Buffer* buf = mem->PinBuffer();
		source = nullptr;
		return buf;
	}
	else
		return nullptr;
}

//=================================================================================================
HANDLE Stream::PinFile()
{
	if(source && source->IsValid() && source->IsFile())
	{
		FileSource* file = (FileSource*)source;
		HANDLE handle = file->PinFile();
		source = nullptr;
		return handle;
	}
	else
		return INVALID_HANDLE_VALUE;
}

//=================================================================================================
StreamReader::StreamReader(const string& path)
{
	source = StreamSourcePool::Get<FileSource>(false, path);
	ok = source->IsValid();
}

//=================================================================================================
StreamReader::StreamReader(Buffer* buf)
{
	source = StreamSourcePool::Get<MemorySource>(buf);
	ok = source->IsValid();
}

//=================================================================================================
StreamReader::StreamReader(BufferHandle& buf)
{
	source = StreamSourcePool::Get<MemorySource>(buf.Pin());
	ok = source->IsValid();
}

//=================================================================================================
StreamReader::StreamReader(HANDLE file, uint clamp_offset, uint clamp_size)
{
	source = StreamSourcePool::Get<FileSource>(false, file, clamp_offset, clamp_size);
	ok = source->IsValid();
}

//=================================================================================================
StreamReader::StreamReader(BitStream& bitstream)
{
	source = StreamSourcePool::Get<BitStreamSource>(&bitstream, false);
	ok = source->IsValid();
}

//=================================================================================================
bool StreamReader::Read(string& s)
{
	byte len;
	if(!Read(len) || !Ensure(len))
		return false;
	s.resize(len);
	if(len != 0)
		Read((void*)s.data(), len);
	return true;
}

//=================================================================================================
BufferHandle StreamReader::Read(uint size)
{
	if(!Ensure(size))
		return BufferHandle(nullptr);
	Buffer* buf = BufferPool.Get();
	buf->Resize(size);
	Read(buf->Data(), size);
	return BufferHandle(buf);
}

//=================================================================================================
// read rest of stream to buffer
Buffer* StreamReader::ReadAll()
{
	Buffer* buf = BufferPool.Get();
	uint size = source->GetSizeLeft();
	buf->Resize(size);
	source->Read(buf->Data(), size);
	return buf;
}

//=================================================================================================
// read to global BUF, clear if failed
bool StreamReader::ReadString1()
{
	byte len;
	if(!Read(len) || !Ensure(len))
	{
		BUF[0] = 0;
		return false;
	}
	Read(BUF, len);
	BUF[len] = 0;
	return true;
}

//=================================================================================================
StreamReader StreamReader::LoadAsMemoryStream(const string& path)
{
	StreamReader reader(path);
	if(!reader)
		return reader;
	else
		return StreamReader(reader.ReadAll());
}

//=================================================================================================
StreamReader StreamReader::LoadAsMemoryStream(HANDLE file, uint offset, uint size)
{
	StreamReader reader(file, offset, size);
	return StreamReader(reader.ReadAll());
}

//=================================================================================================
Buffer* StreamReader::LoadToBuffer(const string& path)
{
	StreamReader reader(path);
	if(!reader)
		return nullptr;
	else
		return reader.ReadAll();
}

//=================================================================================================
Buffer* StreamReader::LoadToBuffer(HANDLE file, uint offset, uint size)
{
	StreamReader reader(file, offset, size);
	return reader.ReadAll();
}

//=================================================================================================
StreamWriter::StreamWriter(HANDLE file)
{
	source = StreamSourcePool::Get<FileSource>(true, file);
}

//=================================================================================================
StreamWriter::StreamWriter(BitStream& bitstream)
{
	source = StreamSourcePool::Get<BitStreamSource>(&bitstream, true);
}
