// póki nie mam w³asnej klasy BitStream to musz¹ byæ zewnêtrzne funkcje
#pragma once

//=================================================================================================
// ZAPIS
//=================================================================================================
// zapisywanie stringa
template<typename LENGTH_TYPE>
inline void WriteString(BitStream& stream, const string& str)
{
	LENGTH_TYPE len = (LENGTH_TYPE)str.length();
	assert(str.length() == (uint)len);
	stream.Write(len);
	if(len)
		stream.Write(str.c_str(), len);
}

inline void WriteString1(BitStream& stream, const string& str)
{
	WriteString<byte>(stream, str);
}

inline void WriteString2(BitStream& stream, const string& str)
{
	WriteString<word>(stream, str);
}

// zapisywanie wektora stringów
template<typename COUNT_TYPE, typename LENGTH_TYPE>
inline void WriteStringArray(BitStream& stream, const vector<string>& strs)
{
	COUNT_TYPE count = (COUNT_TYPE)strs.size();
	assert(strs.size() == (uint)count);
	stream.Write(count);
	for(vector<string>::const_iterator it = strs.begin(), end = strs.end(); it != end; ++it)
		WriteString<LENGTH_TYPE>(stream, *it);
}

// zapisywanie boola
// po co to? móg³by ktoœ zapytaæ - ano nie chce zapisywaæ czegoœ na pojedyñczym bicie bo
// zepsuje siê alingment, póŸniej nie bêdzie wogóle takiej opcji
inline void WriteBool(BitStream& stream, bool b)
{
	stream.WriteCasted<byte>(b ? 1 : 0);
}

template<typename COUNT_TYPE, typename T>
inline void WriteVector(BitStream& stream, const vector<T>& v)
{
	uint count = v.size();
	assert(count <= std::numeric_limits<COUNT_TYPE>::max());
	stream.WriteCasted<COUNT_TYPE>(count);
	stream.Write(v.data(), sizeof(T)*count);
}

template<typename CAST_TYPE, typename COUNT_TYPE=uint, typename T>
inline void WriteVectorCast(BitStream& stream, const vector<T>& v)
{
	static_assert(sizeof(CAST_TYPE) < sizeof(T), "CAST_TYPE must be smaller then T");
	uint count = v.size();
	assert(count <= std::numeric_limits<COUNT_TYPE>::max());
	stream.WriteCasted<COUNT_TYPE>(count);
	for(T value : v)
		stream.WriteCasted<CAST_TYPE>(value);
}

//=================================================================================================
// ODCZYT
//=================================================================================================
// odczytywanie stringa
template<typename LENGTH_TYPE>
inline bool ReadString(BitStream& stream, string& str)
{
	LENGTH_TYPE len;
	if(!stream.Read(len))
		return false;
	if(len)
	{
		if(stream.GetNumberOfUnreadBits() / 8 < len)
			return false;
		str.resize(len);
		return stream.Read((char*)str.c_str(), len);
	}
	else
	{
		str.clear();
		return true;
	}
}

inline bool ReadString1(BitStream& stream, string& str)
{
	return ReadString<byte>(stream, str);
}

inline bool ReadString2(BitStream& stream, string& str)
{
	return ReadString<word>(stream, str);
}

// odczytywanie wektora stringów
template<typename COUNT_TYPE, typename LENGTH_TYPE>
inline bool ReadStringArray(BitStream& stream, vector<string>& strs)
{
	COUNT_TYPE count;
	if(!stream.Read(count))
		return false;
	if(stream.GetNumberOfUnreadBits() / 8 < sizeof(LENGTH_TYPE)*count)
		return false;
	strs.resize(count);
	for(vector<string>::iterator it = strs.begin(), end = strs.end(); it != end; ++it)
	{
		if(!ReadString<LENGTH_TYPE>(stream, *it))
			return false;
	}
	return true;
}

// odczytywanie stringa do BUF
inline bool ReadString1(BitStream& stream)
{
	byte len;
	if(!stream.Read(len))
		return false;
	BUF[len] = 0;
	if(len == 0)
		return true;
	return stream.Read(BUF, len);
}

// odczytywanie boola
// po co? - patrz na WriteBool
inline bool ReadBool(BitStream& stream, bool& boo)
{
	byte b;
	if(!stream.Read(b))
		return false;
	boo = (b == 1);
	return true;
}

template<typename COUNT_TYPE, typename T>
inline void ReadVector(BitStream& stream, vector<T>& v)
{
	COUNT_TYPE count;
	if(!stream.Read(count)
		|| !EnsureSize(stream, sizeof(T)*count))
		return false;
	if(count)
	{
		v.resize(count);
		stream.Read(v.data(), sizeof(T)*count);
	}
	return true;
}

template<typename CAST_TYPE, typename COUNT_TYPE = uint, typename T>
inline bool ReadVectorCast(BitStream& stream, vector<T>& v)
{
	static_assert(sizeof(CAST_TYPE) < sizeof(T), "CAST_TYPE must be smaller then T");
	COUNT_TYPE count;
	if(!stream.Read(count)
		|| !EnsureSize(stream, count*sizeof(CAST_TYPE)))
		return false;
	if(count == 0)
	{
		v.clear();
		return true;
	}
	v.resize(count);
	stream.Read((char*)v.data(), sizeof(CAST_TYPE)*count);
	T* to = v.data() + count - 1;
	CAST_TYPE* from = ((CAST_TYPE*)v.data()) + count - 1;
	while(count > 0)
	{
		*to = (T)*from;
		--to;
		--from;
		--count;
	}
	return true;
}

//=================================================================================================
// POMIJANIE
//=================================================================================================
// pomijanie dowolej liczby bajtów
inline bool Skip(BitStream& stream, uint count)
{
	count <<= 3;
	if(stream.GetNumberOfUnreadBits() >= count)
	{
		stream.SetReadOffset(stream.GetReadOffset()+count);
		return true;
	}
	else
		return false;
}

// pomija string
template<typename LENGTH_TYPE>
inline bool SkipString(BitStream& stream)
{
	LENGTH_TYPE len;
	if(!stream.Read(len))
		return false;
	return Skip(stream, len);
}

inline bool SkipString1(BitStream& stream)
{
	return SkipString<byte>(stream);
}

inline bool SkipString2(BitStream& stream)
{
	return SkipString<word>(stream);
}

// pomija wektor stringów
template<typename COUNT_TYPE, typename LENGTH_TYPE>
inline bool SkipStringArray(BitStream& stream)
{
	COUNT_TYPE count;
	if(!stream.Read(count))
		return false;
	for(COUNT_TYPE i=0; i<count; ++i)
	{
		if(!SkipString<LENGTH_TYPE>(stream))
			return false;
	}
	return true;
}

// pomija strukture
template<typename T>
inline bool SkipStruct(BitStream& stream)
{
	return Skip(stream, sizeof(T));
}

//=================================================================================================
// OTHER
//=================================================================================================
inline bool EnsureSize(BitStream& stream, uint count)
{
	return stream.GetNumberOfUnreadBits() / 8 >= count;
}

inline uint PatchByte(BitStream& stream)
{
	uint pos = stream.GetNumberOfBytesUsed();
	stream.WriteCasted<byte>(0);
	return pos;
}

inline void PatchByteApply(BitStream& stream, uint pos, byte value)
{
	uint old_pos = stream.GetNumberOfBytesUsed();
	stream.SetWriteOffset(pos * 8);
	stream.Write(value);
	stream.SetWriteOffset(old_pos * 8);
}
