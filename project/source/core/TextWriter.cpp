#include "Pch.h"
#include "Core.h"
#include "TextWriter.h"

void TextWriter::WriteFlags(std::initializer_list<FlagsGroup> const& flag_groups)
{
	int set = 0;
	for(auto& flag_group : flag_groups)
	{
		set += CountBits(flag_group.flags);
		if(set >= 2)
			break;
	}
	assert(set != 0);

	TextWriter& f = *this;
	if(set > 1)
		f << " {";

	for(auto& flag_group : flag_groups)
	{
		for(auto& flag : flag_group.flags_def)
		{
			if(IS_SET(flag_group.flags, flag.value))
			{
				f << " ";
				f << flag.name;
			}
		}
	}

	if(set > 1)
		f << " }";
}
