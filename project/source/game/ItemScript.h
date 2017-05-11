#pragma once

//-----------------------------------------------------------------------------
// pseudo skrypt obs³uguj¹cy przedmiot postaci
enum ParseScript
{
	PS_END,
	PS_ONE,
	PS_CHANCE,
	PS_ONE_OF_MANY,
	PS_IF_LEVEL,
	PS_ELSE,
	PS_END_IF,
	PS_CHANCE2,
	PS_IF_CHANCE,
	PS_MANY,
	PS_RANDOM,
	PS_ITEM,
	PS_LIST,
	PS_LEVELED_LIST,
	PS_LEVELED_LIST_MOD
};
