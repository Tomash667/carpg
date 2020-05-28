#pragma once

//-----------------------------------------------------------------------------
enum class Msg
{
	LearnRecipe,
	Max
};

//-----------------------------------------------------------------------------
class Messenger
{
public:
	typedef delegate<void()> Delegate;

	void Register(Msg msg, Delegate handler);
	void Post(Msg msg);
	void Process();

private:
	vector<Delegate> handlers[(int)Msg::Max];
	vector<Msg> messages;
};
