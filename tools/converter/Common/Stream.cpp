/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za¿ó³æ gêœl¹ jaŸñ
 * Stream - Hierarchia klas strumieni
 * Dokumentacja: Patrz plik doc/Stream.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#include "Base.hpp"
#include <typeinfo.h>
#include <memory.h> // dla memcpy
#include "Error.hpp"
#include "Stream.hpp"


namespace common
{

const size_t BUFFER_SIZE = 4096;


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Funkcje globalne wewnêtrzne

void _ThrowBufEndError(const char *File, int Line)
{
	throw Error("Napotkano koniec strumienia.", File, Line);
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa Stream

void Stream::Write(const void *Data, size_t Size)
{
	throw Error(Format("Strumieñ klasy # nie obs³uguje zapisywnia") % typeid(this).name(), __FILE__, __LINE__);
}

void Stream::WriteString1(const string &s)
{
	typedef uint1 T;
	if (s.length() > static_cast<size_t>(std::numeric_limits<T>::max()))
		throw Error(Format("Nie mo¿na zapisaæ do strumienia ³añcucha - d³u¿szy, ni¿ # znaków") % static_cast<size_t>(std::numeric_limits<T>::max()), __FILE__, __LINE__);
	T Length = static_cast<T>(s.length());
	Write(&Length, sizeof(T));
	Write(s.data(), s.length());
}

void Stream::WriteString2(const string &s)
{
	typedef uint2 T;
	if (s.length() > static_cast<size_t>(std::numeric_limits<T>::max()))
		throw Error(Format("Nie mo¿na zapisaæ do strumienia ³añcucha - d³u¿szy, ni¿ # znaków") % static_cast<size_t>(std::numeric_limits<T>::max()), __FILE__, __LINE__);
	T Length = static_cast<T>(s.length());
	Write(&Length, sizeof(T));
	Write(s.data(), s.length());
}

void Stream::WriteString4(const string &s)
{
	size_t Length = s.length();
	Write(&Length, sizeof(Length));
	Write(s.data(), s.length());
}

void Stream::WriteStringF(const string &s)
{
	Write(s.data(), s.length());
}

void Stream::WriteBool(bool b)
{
	uint1 bt = (b ? 1 : 0);
	Write(&bt, sizeof(bt));
}

size_t Stream::Read(void *Data, size_t Size)
{
	throw Error(Format("Strumieñ klasy # nie obs³uguje odczytywania") % typeid(this).name(), __FILE__, __LINE__);
}

void Stream::MustRead(void *Data, size_t Size)
{
	if (Size == 0) return;
	size_t i = Read(Data, Size);
	if (i != Size)
		throw Error(Format("Odczytano ze strumienia #/# B") % i % Size, __FILE__, __LINE__);
}

size_t Stream::Skip(size_t MaxLength)
{
	// Implementacja dla klasy Stream nie posiadaj¹cej kursora.
	// Trzeba wczytywaæ dane po kawa³ku.
	// MaxLength bêdzie zmniejszane. Oznacza liczbê pozosta³ych bajtów do pominiêcia.
	
	if (MaxLength == 0)
		return 0;

	char Buf[BUFFER_SIZE];
	uint BlockSize, ReadSize, Sum = 0;
	while (MaxLength > 0)
	{
		BlockSize = std::min(MaxLength, BUFFER_SIZE);
		ReadSize = Read(Buf, BlockSize);
		Sum += ReadSize;
		MaxLength -= ReadSize;
		if (ReadSize < BlockSize)
			break;
	}
	return Sum;
}

void Stream::ReadString1(string *s)
{
	typedef uint1 T;
	T Length;
	MustRead(&Length, sizeof(T));
	scoped_ptr<char, DeleteArrayPolicy> Buf(new char[Length]);
	MustRead(Buf.get(), Length);
	s->clear();
	s->append(Buf.get(), Length);
}

void Stream::ReadString2(string *s)
{
	typedef uint2 T;
	T Length;
	MustRead(&Length, sizeof(T));
	scoped_ptr<char, DeleteArrayPolicy> Buf(new char[Length]);
	MustRead(Buf.get(), Length);
	s->clear();
	s->append(Buf.get(), Length);
}

void Stream::ReadString4(string *s)
{
	typedef uint4 T;
	T Length;
	MustRead(&Length, sizeof(T));
	scoped_ptr<char, DeleteArrayPolicy> Buf(new char[Length]);
	MustRead(Buf.get(), Length);
	s->clear();
	s->append(Buf.get(), Length);
}

void Stream::ReadStringF(string *s, size_t Length)
{
	scoped_ptr<char, DeleteArrayPolicy> Buf(new char[Length]);
	MustRead(Buf.get(), Length);
	s->clear();
	s->append(Buf.get(), Length);
}

void Stream::ReadStringToEnd(string *s)
{
	s->clear();
	scoped_ptr<char, DeleteArrayPolicy> Buf(new char[BUFFER_SIZE]);
	size_t Size = BUFFER_SIZE;
	do
	{
		Size = Read(Buf.get(), Size);
		if (Size > 0)
			s->append(Buf.get(), Size);
	} while (Size == BUFFER_SIZE);
}

void Stream::ReadBool(bool *b)
{
	uint1 bt;
	MustRead(&bt, sizeof(bt));
	*b = (bt != 0);
}

void Stream::MustSkip(size_t Length)
{
	if (Skip(Length) != Length)
		throw Error(Format("Nie mo¿na pomin¹æ # bajtów - wczeœniej napotkany koniec strumienia.") % Length, __FILE__, __LINE__);
}

bool Stream::End()
{
	throw Error(Format("Strumieñ klasy # nie obs³uguje sprawdzania koñca") % typeid(this).name(), __FILE__, __LINE__);
}

size_t Stream::CopyFrom(Stream *s, size_t Size)
{
	if (Size == 0) return 0;
	// Size - liczba bajtów, jaka zosta³a do odczytania
	scoped_ptr<char, DeleteArrayPolicy> Buf(new char[BUFFER_SIZE]);
	size_t ReqSize, ReadSize, BytesRead = 0;
	do
	{
		ReadSize = ReqSize = std::min(BUFFER_SIZE, Size);
		ReadSize = s->Read(Buf.get(), ReadSize);
		if (ReadSize > 0)
		{
			Write(Buf.get(), ReadSize);
			Size -= ReadSize;
			BytesRead += ReadSize;
		}
	} while (ReadSize == ReqSize && Size > 0);
	return BytesRead;
}

void Stream::MustCopyFrom(Stream *s, size_t Size)
{
	if (Size == 0) return;
	// Size - liczba bajtów, jaka zosta³a do odczytania
	scoped_ptr<char, DeleteArrayPolicy> Buf(new char[BUFFER_SIZE]);
	size_t ReqSize;
	do
	{
		ReqSize = std::min(BUFFER_SIZE, Size);
		s->MustRead(Buf.get(), ReqSize);
		if (ReqSize > 0)
		{
			Write(Buf.get(), ReqSize);
			Size -= ReqSize;
		}
	} while (Size > 0);
}

void Stream::CopyFromToEnd(Stream *s)
{
	scoped_ptr<char, DeleteArrayPolicy> Buf(new char[BUFFER_SIZE]);
	size_t Size = BUFFER_SIZE;
	do
	{
		Size = s->Read(Buf.get(), Size);
		if (Size > 0)
			Write(Buf.get(), Size);
	} while (Size == BUFFER_SIZE);
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa SeekableStream

size_t SeekableStream::GetSize()
{
	throw Error(Format("Strumieñ klasy # nie obs³uguje zwracania rozmiaru") % typeid(this).name(), __FILE__, __LINE__);
}

int SeekableStream::GetPos()
{
	throw Error(Format("Strumieñ klasy # nie obs³uguje zwracania pozycji kursora") % typeid(this).name(), __FILE__, __LINE__);
}

void SeekableStream::SetPos(int pos)
{
	throw Error(Format("Strumieñ klasy # nie obs³uguje ustawiania pozycji kursora") % typeid(this).name(), __FILE__, __LINE__);
}

void SeekableStream::SetPosFromCurrent(int pos)
{
	SetPos(GetPos()+pos);
}

void SeekableStream::SetPosFromEnd(int pos)
{
	SetPos((int)GetSize()+pos);
}

void SeekableStream::Rewind()
{
	SetPos(0);
}

void SeekableStream::SetSize(size_t Size)
{
	throw Error(Format("Strumieñ klasy # nie obs³uguje ustawiania rozmiaru") % typeid(this).name(), __FILE__, __LINE__);
}

void SeekableStream::Truncate()
{
	SetSize((size_t)GetPos());
}

void SeekableStream::Clear()
{
	SetSize(0);
}

bool SeekableStream::End()
{
	return (GetPos() == (int)GetSize());
}

size_t SeekableStream::Skip(size_t MaxLength)
{
	uint Pos = GetPos();
	uint Size = GetSize();
	uint BytesToSkip = std::min(MaxLength, Size - Pos);
	SetPosFromCurrent((int)BytesToSkip);
	return BytesToSkip;
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa CharReader

bool CharReader::EnsureNewChars()
{
	assert(m_BufBeg == m_BufEnd);
	uint ReadSize = m_Stream->Read(&m_Buf[0], BUFFER_SIZE);
	m_BufBeg = 0;
	m_BufEnd = ReadSize;
	return (ReadSize > 0);
}

size_t CharReader::ReadString(string *Out, size_t MaxLength)
{
	uint BlockSize, i, Sum = 0, Out_i = 0;
	Out->clear();

	// MaxLength bêdzie zmniejszane.
	// Tu nie mogê zrobiæ Out->resize(MaxLength), bo jako MaxLength podaje siê czêsto ogromne liczby.

	while (MaxLength > 0)
	{
		if (m_BufBeg == m_BufEnd)
		{
			if (!EnsureNewChars())
				return Sum;
		}
		BlockSize = std::min(m_BufEnd - m_BufBeg, MaxLength);
		Out->resize(Out_i + BlockSize);
		for (i = 0; i < BlockSize; i++)
		{
			(*Out)[Out_i] = m_Buf[m_BufBeg];
			Out_i++;
			m_BufBeg++;
		}
		MaxLength -= BlockSize;
		Sum += BlockSize;
	}
	return Sum;
}

void CharReader::MustReadString(string *Out, size_t Length)
{
	uint BlockSize, i, Out_i = 0;
	Out->clear();
	Out->resize(Length);

	// Length bêdzie zmniejszane.

	while (Length > 0)
	{
		if (m_BufBeg == m_BufEnd)
		{
			if (!EnsureNewChars())
				_ThrowBufEndError(__FILE__, __LINE__);
		}
		BlockSize = std::min(m_BufEnd - m_BufBeg, Length);
		for (i = 0; i < BlockSize; i++)
		{
			(*Out)[Out_i] = m_Buf[m_BufBeg];
			Out_i++;
			m_BufBeg++;
		}
		Length -= BlockSize;
	}
}

size_t CharReader::ReadString(char *Out, size_t MaxLength)
{
	uint BlockSize, i, Sum = 0;

	// MaxLength bêdzie zmniejszane.
	// Out bêdzie przesuwane.

	while (MaxLength > 0)
	{
		if (m_BufBeg == m_BufEnd)
		{
			if (!EnsureNewChars())
				return Sum;
		}
		BlockSize = std::min(m_BufEnd - m_BufBeg, MaxLength);
		for (i = 0; i < BlockSize; i++)
		{
			*Out = m_Buf[m_BufBeg];
			Out++;
			m_BufBeg++;
		}
		MaxLength -= BlockSize;
		Sum += BlockSize;
	}
	return Sum;
}

void CharReader::MustReadString(char *Out, size_t Length)
{
	uint BlockSize, i;

	// Length bêdzie zmniejszane.
	// Out bêdzie przesuwane.

	while (Length > 0)
	{
		if (m_BufBeg == m_BufEnd)
		{
			if (!EnsureNewChars())
				_ThrowBufEndError(__FILE__, __LINE__);
		}
		BlockSize = std::min(m_BufEnd - m_BufBeg, Length);
		for (i = 0; i < BlockSize; i++)
		{
			*Out = m_Buf[m_BufBeg];
			Out++;
			m_BufBeg++;
		}
		Length -= BlockSize;
	}
}

size_t CharReader::ReadData(void *Out, size_t MaxLength)
{
	char *OutChars = (char*)Out;
	uint BlockSize, i, Sum = 0;

	// MaxLength bêdzie zmniejszane.
	// OutChars bêdzie przesuwane.

	while (MaxLength > 0)
	{
		if (m_BufBeg == m_BufEnd)
		{
			if (!EnsureNewChars())
				return Sum;
		}
		BlockSize = std::min(m_BufEnd - m_BufBeg, MaxLength);
		for (i = 0; i < BlockSize; i++)
		{
			*OutChars = m_Buf[m_BufBeg];
			OutChars++;
			m_BufBeg++;
		}
		MaxLength -= BlockSize;
		Sum += BlockSize;
	}
	return Sum;
}

void CharReader::MustReadData(void *Out, size_t Length)
{
	char *OutChars = (char*)Out;
	uint BlockSize, i;

	// Length bêdzie zmniejszane.
	// OutChars bêdzie przesuwane.

	while (Length > 0)
	{
		if (m_BufBeg == m_BufEnd)
		{
			if (!EnsureNewChars())
				_ThrowBufEndError(__FILE__, __LINE__);
		}
		BlockSize = std::min(m_BufEnd - m_BufBeg, Length);
		for (i = 0; i < BlockSize; i++)
		{
			*OutChars = m_Buf[m_BufBeg];
			OutChars++;
			m_BufBeg++;
		}
		Length -= BlockSize;
	}
}

size_t CharReader::Skip(size_t MaxLength)
{
	uint BlockSize, Sum = 0;

	// MaxLength bêdzie zmniejszane.

	while (MaxLength > 0)
	{
		if (m_BufBeg == m_BufEnd)
		{
			if (!EnsureNewChars())
				return Sum;
		}
		BlockSize = std::min(m_BufEnd - m_BufBeg, MaxLength);
		m_BufBeg += BlockSize;
		MaxLength -= BlockSize;
		Sum += BlockSize;
	}
	return Sum;
}

void CharReader::MustSkip(size_t Length)
{
	uint BlockSize;

	// Length bêdzie zmniejszane.

	while (Length > 0)
	{
		if (m_BufBeg == m_BufEnd)
		{
			if (!EnsureNewChars())
				_ThrowBufEndError(__FILE__, __LINE__);
		}
		BlockSize = std::min(m_BufEnd - m_BufBeg, Length);
		m_BufBeg += BlockSize;
		Length -= BlockSize;
	}
}

bool CharReader::ReadLine(string *Out)
{
	Out->clear();
	char Ch;
	bool WasEol = false;
	while (ReadChar(&Ch))
	{
		if (Ch == '\r')
		{
			WasEol = true;
			if (PeekChar(&Ch) && Ch == '\n')
				MustSkipChar();
			break;
		}
		else if (Ch == '\n')
		{
			WasEol = true;
			break;
		}
		else
			(*Out) += Ch;
	}
	return !Out->empty() || WasEol;
}

void CharReader::MustReadLine(string *Out)
{
	Out->clear();
	char Ch;
	while (ReadChar(&Ch))
	{
		if (Ch == '\r')
		{
			if (PeekChar(&Ch) && Ch == '\n')
				MustSkipChar();
			break;
		}
		else if (Ch == '\n')
			break;
		else
			(*Out) += Ch;
	}
}


//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa MemoryStream

MemoryStream::MemoryStream(size_t Size, void *Data)
{
	m_Data = (char*)Data;
	m_Size = Size;
	m_Pos = 0;
	m_InternalAlloc = (m_Data == 0);

	if (m_InternalAlloc)
		m_Data = new char[m_Size];
}

MemoryStream::~MemoryStream()
{
	if (m_InternalAlloc)
		delete [] m_Data;
}

void MemoryStream::Write(const void *Data, size_t Size)
{
	if (m_Pos >= 0 && m_Pos + (int)Size <= (int)m_Size)
	{
		memcpy(&m_Data[m_Pos], Data, Size);
		m_Pos += (int)Size;
	}
	else
		throw Error(Format("Nie mo¿na zapisaæ # bajtów do strumienia pamiêciowego - pozycja poza zakrsem (pozycja: #, rozmiar: #)") % Size % m_Pos % m_Size, __FILE__, __LINE__);
}

size_t MemoryStream::Read(void *Data, size_t Size)
{
	if (m_Pos >= 0 && m_Pos <= (int)m_Size)
	{
		Size = std::min(Size, m_Size-m_Pos);
		memcpy(Data, &m_Data[m_Pos], Size);
		m_Pos += (int)Size;
	}
	else
		throw Error(Format("Nie mo¿na odczytaæ # bajtów ze strumienia pamiêciowego - pozycja poza zakrsem (pozycja: #, rozmiar: #)") % Size % m_Pos % m_Size, __FILE__, __LINE__);
	return Size;
}

void MemoryStream::MustRead(void *Data, size_t Size)
{
	if (m_Pos >= 0 && m_Pos + (int)Size <= (int)m_Size)
	{
		memcpy(Data, &m_Data[m_Pos], Size);
		m_Pos += (int)Size;
	}
	else
		throw Error(Format("Nie mo¿na odczytaæ (2) # bajtów ze strumienia pamiêciowego - pozycja poza zakrsem (pozycja: #, rozmiar: #)") % Size % m_Pos % m_Size, __FILE__, __LINE__);
}

size_t MemoryStream::GetSize()
{
	return m_Size;
}

int MemoryStream::GetPos()
{
	return m_Pos;
}

void MemoryStream::SetPos(int pos)
{
	m_Pos = pos;
}

void MemoryStream::Rewind()
{
	m_Pos = 0;
}

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa StringStream

StringStream::StringStream(string *Data)
{
	m_Data = Data;
	m_Pos = 0;
	m_InternalAlloc = (m_Data == 0);

	if (m_InternalAlloc)
		m_Data = new string;
}

StringStream::~StringStream()
{
	if (m_InternalAlloc)
		delete m_Data;
}

void StringStream::Write(const void *Data, size_t Size)
{
	if (m_Pos + Size > m_Data->length())
		m_Data->resize(m_Pos + Size);

	for (size_t i = 0; i < Size; i++)
		(*m_Data)[m_Pos+i] = reinterpret_cast<const char*>(Data)[i];

	m_Pos += (int)Size;
}

size_t StringStream::Read(void *Data, size_t Size)
{
	if (m_Pos < 0)
		throw Error(Format("Nie mo¿na zapisaæ # bajtów do strumienia StringStream - pozycja poza zakrsem (pozycja: #, rozmiar: #)") % Size % m_Pos % m_Data->length(), __FILE__, __LINE__);

	Size = std::min(Size, m_Data->length()-m_Pos);
	for (size_t i = 0; i < Size; i++)
	{
		reinterpret_cast<char*>(Data)[i] = (*m_Data)[m_Pos];
		m_Pos++;
	}
	return Size;
}

void StringStream::MustRead(void *Data, size_t Size)
{
	if (m_Pos < 0 || m_Pos + Size > m_Data->length())
		throw Error(Format("Nie mo¿na odczytaæ (2) # bajtów ze strumienia StringStream - pozycja poza zakrsem (pozycja: #, rozmiar: #)") % Size % m_Pos % m_Data->length(), __FILE__, __LINE__);

	for (size_t i = 0; i < Size; i++)
		reinterpret_cast<char*>(Data)[i] = (*m_Data)[m_Pos+i];

	m_Pos += (int)Size;
}

} // namespace common
