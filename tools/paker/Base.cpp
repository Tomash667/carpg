#include "Base.h"
#include <Shellapi.h>

string g_tmp_string;
DWORD tmp;
char BUF[256];
ObjectPool<string> StringPool;

//=================================================================================================
bool LoadFileToString(cstring path, string& str)
{
	HANDLE file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(file == INVALID_HANDLE_VALUE)
		return false;

	DWORD size = GetFileSize(file, NULL);
	str.resize(size);

	ReadFile(file, (char*)str.c_str(), size, &tmp, NULL);

	CloseHandle(file);

	return size == tmp;
}

//=================================================================================================
// Formatowanie ci¹gu znaków
//=================================================================================================
cstring Format(cstring str, ...)
{
	const uint FORMAT_STRINGS = 8;
	const uint FORMAT_LENGTH = 2048;

	assert(str);

	static char buf[FORMAT_STRINGS][FORMAT_LENGTH];
	static int marker = 0;

	va_list list;
	va_start(list, str);
	_vsnprintf_s((char*)buf[marker], FORMAT_LENGTH, FORMAT_LENGTH - 1, str, list);
	char* cbuf = buf[marker];
	cbuf[FORMAT_LENGTH - 1] = 0;

	marker = (marker + 1) % FORMAT_STRINGS;

	return cbuf;
}


//=================================================================================================
bool Unescape(const string& str_in, uint pos, uint size, string& str_out)
{
	str_out.clear();
	str_out.reserve(str_in.length());

	cstring unesc = "nt\\\"";
	cstring esc = "\n\t\\\"";
	uint end = pos + size;

	for(; pos<end; ++pos)
	{
		if(str_in[pos] == '\\')
		{
			++pos;
			if(pos == size)
			{
				//ERROR(Format("Unescape error in string \"%s\", character '\\' at end of string.", str_in.c_str()));
				return false;
			}
			int index = strchr_index(unesc, str_in[pos]);
			if(index != -1)
				str_out += esc[index];
			else
			{
				//ERROR(Format("Unescape error in string \"%s\", unknown escape sequence '\\%c'.", str_in.c_str(), str_in[pos]));
				return false;
			}
		}
		else
			str_out += str_in[pos];
	}

	return true;
}

int StringToNumber(cstring s, __int64& i, float& f)
{
	assert(s);

	i = 0;
	f = 0;
	uint diver = 10;
	uint digits = 0;
	char c;
	bool sign = false;
	if(*s == '-')
	{
		sign = true;
		++s;
	}

	while((c = *s) != 0)
	{
		if(c == '.')
		{
			++s;
			break;
		}
		else if(c >= '0' && c <= '9')
		{
			i *= 10;
			i += (int)c - '0';
		}
		else
			return 0;
		++s;
	}

	if(c == 0)
	{
		if(sign)
			i = -i;
		f = (float)i;
		return 1;
	}

	while((c = *s) != 0)
	{
		if(c == 'f')
		{
			if(digits == 0)
				return 0;
			break;
		}
		else if(c >= '0' && c <= '9')
		{
			++digits;
			f += ((float)((int)c - '0')) / diver;
			diver *= 10;
		}
		else
			return 0;
		++s;
	}
	f += (float)i;
	if(sign)
	{
		f = -f;
		i = -i;
	}
	return 2;
}

//=================================================================================================
// Sprawdza czy podany folder istnieje
//=================================================================================================
bool DirectoryExists(cstring file)
{
	if(!file)
		return false;

	DWORD attrib = GetFileAttributes(file);
	if(attrib == INVALID_FILE_ATTRIBUTES)
		return false;

	return IS_SET(attrib, FILE_ATTRIBUTE_DIRECTORY);
}

//=================================================================================================
bool DeleteDirectory(cstring filename)
{
	assert(filename);

	char* s = BUF;
	char c;
	while((c = *filename++) != 0)
		*s++ = c;
	*s++ = 0;
	*s = 0;

	SHFILEOPSTRUCT op = {
		NULL,
		FO_DELETE,
		BUF,
		NULL,
		FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT,
		FALSE,
		NULL,
		NULL
	};

	return SHFileOperation(&op) == 0;
}
