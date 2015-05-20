/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za¿ó³æ gêœl¹ jaŸñ
 * Error - Klasy wyj¹tków do obs³ugi b³êdów
 * Dokumentacja: Patrz plik doc/Error.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#include "Base.hpp"
#include "Error.hpp"

namespace common
{

void Error::Push(const string &msg, const string &file, int line)
{
	if (msg.empty()) return;

	string file_name;
	size_t pos = file.find_last_of("/\\");
	if (pos != string::npos)
		file_name = file.substr(pos+1);
	else
		file_name = file;

	if (file_name.empty() && line == 0)
		m_Msgs.push_back(msg);
	else if (file_name.empty())
		m_Msgs.push_back("[" + IntToStrR(line) + "] " + msg);
	else if (line == 0)
		m_Msgs.push_back("[" + file_name + "] " + msg);
	else
		m_Msgs.push_back("[" + file_name + "," + IntToStrR(line) + "] " + msg);
}

void Error::GetMessage_(string *Msg, const string &Indent, const string &EOL) const
{
	Msg->clear();
	for (
		std::vector<string>::const_reverse_iterator crit = m_Msgs.rbegin();
		crit != m_Msgs.rend();
		++crit)
	{
		if (!Msg->empty())
			*Msg += EOL;
		*Msg += Indent;
		*Msg += *crit;
	}
}

Win32Error::Win32Error(const string &msg, const string &file, int line)
{
	DWORD Code = GetLastError();
	string s = "(Win32Error," + UintToStrR<uint4>(Code) + ") ";
	char *Message = 0;
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, Code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&Message, 0, 0);
	Push(s + Message);
	LocalFree(Message);

	Push(msg, file, line);
}

} // namespace common
