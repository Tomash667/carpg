#include <windows.h>
#include <cstdio>
#include <conio.h>
#include <vector>
#include <string>

using namespace std;

struct File
{
	string name;
	int size, offset;
};

void Crypt(char *inp, DWORD inplen, char* key, DWORD keylen)
{
	//we will consider size of sbox 256 bytes
	//(extra byte are only to prevent any mishep just in case)
	char Sbox[257], Sbox2[257];
	unsigned long i, j, t, x;

	//this unsecured key is to be used only when there is no input key from user
	char temp , k;
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

	j = 0;
	//initialize the sbox2 with user key
	for(i = 0; i < 256U ; i++)
	{
		if(j == keylen)
		{
			j = 0;
		}
		Sbox2[i] = key[j++];
	}    

	j = 0 ; //Initialize j
	//scramble sbox1 with sbox2
	for(i = 0; i < 256; i++)
	{
		j = (j + (unsigned long) Sbox[i] + (unsigned long) Sbox2[i]) % 256U ;
		temp =  Sbox[i];                    
		Sbox[i] = Sbox[j];
		Sbox[j] =  temp;
	}

	i = j = 0;
	for(x = 0; x < inplen; x++)
	{
		//increment i
		i = (i + 1U) % 256U;
		//increment j
		j = (j + (unsigned long) Sbox[i]) % 256U;

		//Scramble SBox #1 further so encryption routine will
		//will repeat itself at great interval
		temp = Sbox[i];
		Sbox[i] = Sbox[j] ;
		Sbox[j] = temp;

		//Get ready to create pseudo random  byte for encryption key
		t = ((unsigned long) Sbox[i] + (unsigned long) Sbox[j]) %  256U ;

		//get the random byte
		k = Sbox[t];

		//xor with the data and done
		inp[x] = (inp[x] ^ k);
	}    
}

int main(int argc, char** argv)
{
	bool crypt = false;
	int keylen = 0;

	if(argc == 3 && strcmp(argv[1], "-c") == 0)
	{
		crypt = true;
		keylen = strlen(argv[2]);
	}

	HANDLE pak = CreateFile("data.pak", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(pak == INVALID_HANDLE_VALUE)
	{
		printf("Nie mozna utworzyc pliku PAK!\n\n(ok)");
		_getch();
		return 1;
	}

	vector<File> files;

	WIN32_FIND_DATA find;
	HANDLE handle = FindFirstFile("*", &find);

	int offset = 16;

	while(handle != INVALID_HANDLE_VALUE)
	{
		if((find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			if(strcmp(find.cFileName, "pak.exe") != 0 && strcmp(find.cFileName, "data.pak") != 0)
			{
				File f;
				f.name = find.cFileName;
				f.size = find.nFileSizeLow;
				files.push_back(f);

				offset += 9+f.name.length();
			}
		}

		if(FindNextFile(handle, &find) == 0)
			break;
	}

	int header_offset = offset;

	FindClose(handle);

	for(vector<File>::iterator it = files.begin(), end = files.end(); it != end; ++it)
	{
		it->offset = offset;
		offset += it->size;
	}

	char sign[4] = {'P','A','K',0};
	DWORD tmp;

	int flagi = 0;
	if(crypt)
		flagi = 1;

	WriteFile(pak, sign, 4, &tmp, NULL);

	WriteFile(pak, &flagi, sizeof(flagi), &tmp, NULL);
	header_offset -= 16;
	WriteFile(pak, &header_offset, sizeof(header_offset), &tmp, NULL);

	UINT n = files.size();
	WriteFile(pak, &n, 4, &tmp, NULL);

	byte len;
	int size;

	vector<byte> buf;

	if(crypt)
	{
		for(vector<File>::iterator it = files.begin(), end = files.end(); it != end; ++it)
		{
			len = it->name.length();
			size = 9+len;
			unsigned int off = buf.size();
			buf.resize(off+size);
			memcpy(&buf[off], &len, 1);
			memcpy(&buf[off+1], it->name.c_str(), len);
			memcpy(&buf[off+1+len], &it->size, 4);
			memcpy(&buf[off+5+len], &it->offset, 4);
		}

		Crypt((char*)&buf[0], buf.size(), argv[2], keylen);
		WriteFile(pak, &buf[0], buf.size(), &tmp, NULL);
	}
	else
	{
		for(vector<File>::iterator it = files.begin(), end = files.end(); it != end; ++it)
		{
			len = it->name.length();
			WriteFile(pak, &len, 1, &tmp, NULL);
			WriteFile(pak, it->name.c_str(), len, &tmp, NULL);
			WriteFile(pak, &it->size, 4, &tmp, NULL);
			WriteFile(pak, &it->offset, 4, &tmp, NULL);
		}
	}

	for(vector<File>::iterator it = files.begin(), end = files.end(); it != end; ++it)
	{
		if((int)buf.size() < it->size)
			buf.resize(it->size);
		HANDLE file = CreateFile(it->name.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		ReadFile(file, &buf[0], it->size, &tmp, NULL);
		CloseHandle(file);
		WriteFile(pak, &buf[0], it->size, &tmp, NULL);
	}

	CloseHandle(pak);

	return 0;
}
