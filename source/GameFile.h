#pragma once

//-----------------------------------------------------------------------------
class GameReader final : public FileReader
{
public:
	explicit GameReader(HANDLE file) : FileReader(file), isLocal(false) {}
	explicit GameReader(cstring filename) : FileReader(filename), isLocal(false) {}
	explicit GameReader(const FileReader& f) : FileReader(f.GetHandle()), isLocal(false) {}

	using FileReader::operator >>;

	bool isLocal;
};

//-----------------------------------------------------------------------------
class GameWriter final : public FileWriter
{
public:
	explicit GameWriter(HANDLE file) : FileWriter(file), isLocal(false)
	{
	}

	explicit GameWriter(cstring filename) : FileWriter(filename), isLocal(false)
	{
	}

	using FileWriter::operator <<;

	bool isLocal;
};
