/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za¿ó³æ gêœl¹ jaŸñ
 * Files - Obs³uga plików i systemu plików
 * Dokumentacja: Patrz plik doc/Files.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#include "Base.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h> // dla unlink [Win, Linux ma go w unistd.h], rename [obydwa]
extern "C" {
	#include <sys/utime.h> // dla utime
	#include <direct.h> // dla mkdir (Linux ma go w sys/stat.h + sys/types.h)
}
#include <windows.h>
#include <stack>

#include "Error.hpp"
#include "Files.hpp"


namespace common
{

//HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
// Klasa FileStream

class File_pimpl
{
public:
	HANDLE m_File;
};

FileStream::FileStream(const string &FileName, FILE_MODE FileMode, bool Lock) :
	pimpl(new File_pimpl)
{
	uint4 DesiredAccess, ShareMode, CreationDisposition;
	switch (FileMode)
	{
	case FM_WRITE:
		DesiredAccess = GENERIC_WRITE;
		ShareMode = 0;
		CreationDisposition = CREATE_ALWAYS;
		break;
	case FM_WRITE_PLUS:
		DesiredAccess = GENERIC_WRITE | GENERIC_READ;
		ShareMode = 0;
		CreationDisposition = CREATE_ALWAYS;
		break;
	case FM_READ:
		DesiredAccess = GENERIC_READ;
		ShareMode = FILE_SHARE_READ;
		CreationDisposition = OPEN_EXISTING;
		break;
	case FM_READ_PLUS:
		DesiredAccess = GENERIC_WRITE | GENERIC_READ;
		ShareMode = 0;
		CreationDisposition = OPEN_EXISTING;
		break;
	case FM_APPEND:
		DesiredAccess = GENERIC_WRITE;
		ShareMode = 0;
		CreationDisposition = OPEN_ALWAYS;
		break;
	case FM_APPEND_PLUS:
		DesiredAccess = GENERIC_WRITE | GENERIC_READ;
		ShareMode = 0;
		CreationDisposition = OPEN_ALWAYS;
		break;
	}
	if (!Lock)
		ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;

	pimpl->m_File = CreateFileA(FileName.c_str(), DesiredAccess, ShareMode, 0, CreationDisposition, FILE_ATTRIBUTE_NORMAL, 0);
	if (pimpl->m_File == INVALID_HANDLE_VALUE)
		throw Win32Error("Nie mo¿na otworzyæ pliku: "+FileName, __FILE__, __LINE__);

	if (FileMode == FM_APPEND || FileMode == FM_APPEND_PLUS)
		SetPosFromEnd(0);
}

FileStream::~FileStream()
{
	CloseHandle(pimpl->m_File);
}

void FileStream::Write(const void *Data, size_t Size)
{
	uint4 WrittenSize;
	if (WriteFile(pimpl->m_File, Data, Size, (LPDWORD)&WrittenSize, 0) == 0)
		throw Win32Error(Format("Nie mo¿na zapisaæ # B do pliku") % Size, __FILE__, __LINE__);
	if (WrittenSize != Size)
		throw Error(Format("Nie mo¿na zapisaæ do pliku - zapisanych bajtów #/#") % WrittenSize % Size, __FILE__, __LINE__);
}

size_t FileStream::Read(void *Data, size_t Size)
{
	uint4 ReadSize;
	if (ReadFile(pimpl->m_File, Data, Size, (LPDWORD)&ReadSize, 0) == 0)
		throw Win32Error(Format("Nie mo¿na odczytaæ # B z pliku") % Size, __FILE__, __LINE__);
	return ReadSize;
}

void FileStream::Flush()
{
	FlushFileBuffers(pimpl->m_File);
}

size_t FileStream::GetSize()
{
	return (size_t)GetFileSize(pimpl->m_File, 0);
}

int FileStream::GetPos()
{
	// Fuj! Ale to nieeleganckie. Szkoda, ¿e nie ma lepszego sposobu.
	uint4 r = SetFilePointer(pimpl->m_File, 0, 0, FILE_CURRENT);
	if (r == INVALID_SET_FILE_POINTER)
		throw Win32Error("Nie mo¿na odczytaæ pozycji w strumieniu plikowym", __FILE__, __LINE__);
	return (int)r;
}

void FileStream::SetPos(int pos)
{
	if (SetFilePointer(pimpl->m_File, pos, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		throw Win32Error(Format("Nie mo¿na przesun¹æ pozycji w strumieniu plikowym do # od pocz¹tku") % pos, __FILE__, __LINE__);
}

void FileStream::SetPosFromCurrent(int pos)
{
	if (SetFilePointer(pimpl->m_File, pos, 0, FILE_CURRENT) == INVALID_SET_FILE_POINTER)
		throw Win32Error(Format("Nie mo¿na przesun¹æ pozycji w strumieniu plikowym do # od bie¿¹cej") % pos, __FILE__, __LINE__);
}

void FileStream::SetPosFromEnd(int pos)
{
	if (SetFilePointer(pimpl->m_File, pos, 0, FILE_END) == INVALID_SET_FILE_POINTER)
		throw Win32Error(Format("Nie mo¿na przesun¹æ pozycji w strumieniu plikowym do # od koñca") % pos, __FILE__, __LINE__);
}

void FileStream::SetSize(size_t Size)
{
	SetPos((int)Size);
	Truncate();
}

void FileStream::Truncate()
{
	if (SetEndOfFile(pimpl->m_File) == 0)
		throw Win32Error("Nie mo¿na obci¹æ pliku", __FILE__, __LINE__);
}

bool FileStream::End()
{
	// Fuj! Ale to nieeleganckie. Szkoda, ¿e nie ma lepszego sposobu.
	return ((int)GetSize() == GetPos());
}

} // namespace common

