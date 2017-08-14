/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za��� g�l� ja��
 * Files - Obs�uga plik�w i systemu plik�w
 * Dokumentacja: Patrz plik doc/Files.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#pragma once

#include "Stream.hpp"

namespace common
{

// Tryb otwarcia pliku
enum FILE_MODE
{
	// zapis: tak, odczyt: nie
	// pozycja: 0
	// nie istnieje: tworzy, istnieje: opr�nia
	// SetPos: dzia�a
	FM_WRITE,
	// zapis: tak, odczyt: tak
	// pozycja: 0
	// nie istnieje: tworzy, istnieje: opr�nia
	// SetPos: dzia�a
	FM_WRITE_PLUS,
	// zapis: nie, odczyt: tak
	// pozycja: 0
	// nie istnieje: b��d, istnieje: otwiera
	// SetPos: dzia�a
	FM_READ,
	// zapis: tak, odczyt: tak
	// pozycja: 0
	// nie istnieje: b��d, istnieje: otwiera
	// SetPos: dzia�a
	FM_READ_PLUS,
	// zapis: tak, odczyt: nie
	// pozycja: koniec
	// nie istnieje: tworzy, istnieje: otwiera
	// SetPos: w Linuksie nie dzia�a!
	FM_APPEND,
	// zapis: tak, odczyt: tak
	// pozycja: koniec
	// nie istnieje: tworzy, istnieje: otwiera
	// SetPos: w Linuksie nie dzia�a!
	FM_APPEND_PLUS,
};

class File_pimpl;

// Strumie� plikowy
class FileStream : public SeekableStream
{
private:
	scoped_ptr<File_pimpl> pimpl;

public:
	FileStream(const string &FileName, FILE_MODE FileMode, bool Lock = true);
	virtual ~FileStream();

	virtual void Write(const void *Data, size_t Size);
	virtual size_t Read(void *Data, size_t Size);
	virtual void Flush();
	virtual size_t GetSize();
	virtual int GetPos();
	virtual void SetPos(int pos);
	virtual void SetPosFromCurrent(int pos);
	virtual void SetPosFromEnd(int pos);
	virtual void SetSize(size_t Size);
	virtual void Truncate();
	virtual bool End();

	template <typename T>
	void ReadEx(T *x)
	{
		Read(x, sizeof(*x));
	}
	template<typename T>
	void Read(T& x)
	{
		Read(&x, sizeof(T));
	}
	template<typename T>
	T Read()
	{
		T x;
		Read(&x, sizeof(T));
		return x;
	}

	template<typename T>
	void Write(const T& x)
	{
		Write(&x, sizeof(T));
	}
};

} // namespace common
