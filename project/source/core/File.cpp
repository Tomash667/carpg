#include "Pch.h"
#include "Base.h"

DWORD tmp;
char BUF[256];

//=================================================================================================
bool io::DeleteDirectory(cstring dir)
{
	assert(dir);

	char* s = BUF;
	char c;
	while((c = *dir++) != 0)
		*s++ = c;
	*s++ = 0;
	*s = 0;

	SHFILEOPSTRUCT op = {
		nullptr,
		FO_DELETE,
		BUF,
		nullptr,
		FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT,
		FALSE,
		nullptr,
		nullptr
	};

	return SHFileOperation(&op) == 0;
}

//=================================================================================================
bool io::DirectoryExists(cstring dir)
{
	assert(dir);

	DWORD attrib = GetFileAttributes(dir);
	if(attrib == INVALID_FILE_ATTRIBUTES)
		return false;

	return IS_SET(attrib, FILE_ATTRIBUTE_DIRECTORY);
}

//=================================================================================================
bool io::FileExists(cstring filename)
{
	assert(filename);

	DWORD attrib = GetFileAttributes(filename);
	if(attrib == INVALID_FILE_ATTRIBUTES)
		return false;

	return !IS_SET(attrib, FILE_ATTRIBUTE_DIRECTORY);
}

//=================================================================================================
bool io::FindFiles(cstring pattern, const std::function<bool(const WIN32_FIND_DATA&)>& func, bool exclude_special)
{
	assert(pattern);

	WIN32_FIND_DATA find_data;
	HANDLE find = FindFirstFile(pattern, &find_data);
	if(find == INVALID_HANDLE_VALUE)
		return false;

	do
	{
		// exclude special files or directories
		if((exclude_special && (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0))
			|| IS_SET(find_data.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
			continue;

		// callback
		if(!func(find_data))
			break;

	} while(FindNextFile(find, &find_data) != 0);

	DWORD result = GetLastError();
	FindClose(find);
	return (result == ERROR_NO_MORE_FILES);
}

//=================================================================================================
void io::Execute(cstring file)
{
	assert(file);

	ShellExecute(nullptr, "open", file, nullptr, nullptr, SW_SHOWNORMAL);
}

//=================================================================================================
bool io::LoadFileToString(cstring path, string& str, uint max_size)
{
	HANDLE file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if(file == INVALID_HANDLE_VALUE)
		return false;

	uint file_size = (uint)GetFileSize(file, nullptr);
	uint size = min(file_size, max_size);
	str.resize(size);

	ReadFile(file, (char*)str.c_str(), size, &tmp, nullptr);

	CloseHandle(file);

	return size == tmp;
}

//=================================================================================================
void io::Crypt(char* inp, uint inplen, cstring key, uint keylen)
{
	//we will consider size of sbox 256 bytes
	//(extra byte are only to prevent any mishep just in case)
	char Sbox[257], Sbox2[257];
	uint i, j, t, x;

	//this unsecured key is to be used only when there is no input key from user
	char temp, k;
	i = j = t = x = 0;
	temp = k = 0;

	//always initialize the arrays with zero
	ZeroMemory(Sbox, sizeof(Sbox));
	ZeroMemory(Sbox2, sizeof(Sbox2));

	//initialize sbox i
	for(i = 0; i < 256U; i++)
	{
		Sbox[i] = (char)i;
	}

	//initialize the sbox2 with user key
	for(i = 0; i < 256U; i++)
	{
		if(j == keylen)
		{
			j = 0;
		}
		Sbox2[i] = key[j++];
	}

	j = 0; //Initialize j
		   //scramble sbox1 with sbox2
	for(i = 0; i < 256; i++)
	{
		j = (j + (uint)Sbox[i] + (uint)Sbox2[i]) % 256U;
		temp = Sbox[i];
		Sbox[i] = Sbox[j];
		Sbox[j] = temp;
	}

	i = j = 0;
	for(x = 0; x < inplen; x++)
	{
		//increment i
		i = (i + 1U) % 256U;
		//increment j
		j = (j + (uint)Sbox[i]) % 256U;

		//Scramble SBox #1 further so encryption routine will
		//will repeat itself at great interval
		temp = Sbox[i];
		Sbox[i] = Sbox[j];
		Sbox[j] = temp;

		//Get ready to create pseudo random  byte for encryption key
		t = ((uint)Sbox[i] + (uint)Sbox[j]) % 256U;

		//get the random byte
		k = Sbox[t];

		//xor with the data and done
		inp[x] = (inp[x] ^ k);
	}
}

//=================================================================================================
cstring io::FilenameFromPath(const string& path)
{
	uint pos = path.find_last_of('/');
	if(pos == string::npos)
		return path.c_str();
	else
		return path.c_str() + pos + 1;
}

//=================================================================================================
cstring io::FilenameFromPath(cstring path)
{
	assert(path);
	cstring filename = strrchr(path, '/');
	if(filename)
		return filename + 1;
	else
		return path;
}

//=================================================================================================
string io::GetParentDir(const string& path)
{
	std::size_t pos = path.find_last_of('/');
	string part = path.substr(0, pos);
	return part;
}

//=================================================================================================
string io::GetExt(const string& filename)
{
	std::size_t pos = filename.find_last_of('.');
	if(pos == string::npos)
		return string();
	string ext = filename.substr(pos + 1);
	return ext;
}
