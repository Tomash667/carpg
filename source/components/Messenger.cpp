#include "Pch.h"
#include "Messenger.h"

Messenger* messenger;

//=================================================================================================
void Messenger::Register(Msg msg, Delegate handler)
{
	handlers[(int)msg].push_back(handler);
}

//=================================================================================================
void Messenger::Post(Msg msg)
{
	for(Msg m : messages)
	{
		if(m == msg)
			return;
	}
	messages.push_back(msg);
}

//=================================================================================================
void Messenger::Process()
{
	for(Msg m : messages)
	{
		for(Delegate& handler : handlers[(int)m])
			handler();
	}
	messages.clear();
}
