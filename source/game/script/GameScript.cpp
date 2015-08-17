#include "Pch.h"
#include "Base.h"
#include "CaScript.h"
#include "Attribute.h"
#include "Skill.h"
#include "Unit.h"

void Unit_Mod(Unit* u, Attribute a, int value)
{

}

void Unit_Mod(Unit* u, Skill s, int value)
{

}

struct A
{
	int a;
};

struct B
{
	int b;

	int dodaj(int a)
	{
		return a + b;
	}
};

struct C
{
	int dodaj(int a);
	float dodaj(float b);
};

struct D
{
	static int dodaj(int a, int b);
};

int dodaj(int a, int b)
{
	return a + b;
}

float dodajf(float a, float b)
{
	return a + b;
}

int dodaja(A& a, int b)
{
	return a.a + b;
}

enum Type
{
	Func,
	FuncObj,
	FuncThis
};

struct CallContext
{
	Type type;
	void* f;
	void* args;
	void* obj;
	int paramsSize;
	bool returnFloat;
};


int CallFunction(void* f, void* args, int paramsSize)
{
	int result = 0;

	__asm
	{
		// save registers
		push ecx;

		// clear fpu stack ?
		fninit;

		// copy arguments
		mov ecx, paramsSize;
		mov eax, args;
		add eax, ecx;
		cmp ecx, 0;
		je endcopy;
	copyloop:
		sub eax, 4;
		push dword ptr[eax];
		sub ecx, 4;
		jne copyloop;
	endcopy:

		// call
		call[f];

		// pop arguments
		add esp, paramsSize;

		// copy return value
		mov result, eax;

		// restore registers
		pop ecx;
	};

	return result;
}

int CallFunctionObj(void* f, void* obj, void* args, int paramsSize)
{
	int result = 0;

	__asm
	{
		// save registers
		push ecx;

		// clear fpu stack ?
		fninit;

		// copy arguments
		mov ecx, paramsSize;
		mov eax, args;
		add eax, ecx;
		cmp ecx, 0;
		je endcopy;
	copyloop:
		sub eax, 4;
		push dword ptr[eax];
		sub ecx, 4;
		jne copyloop;
	endcopy:

		// push obj
		push obj;

		// call
		call[f];

		// pop arguments
		add esp, paramsSize;
		add esp, 4;

		// copy return value
		mov result, eax;

		// restore registers
		pop ecx;
	};

	return result;
}

int CallFunctionThis(void* f, void* obj, void* args, int paramsSize)
{
	int result = 0;

	__asm
	{
		// save registers
		push ecx;

		// clear fpu stack ?
		fninit;

		// copy arguments
		mov ecx, paramsSize;
		mov eax, args;
		add eax, ecx;
		cmp ecx, 0;
		je endcopy;
	copyloop:
		sub eax, 4;
		push dword ptr[eax];
		sub ecx, 4;
		jne copyloop;
	endcopy:

		// set this
		mov ecx, obj;

		// call
		call[f];
		
		// copy return value
		mov result, eax;

		// restore registers
		pop ecx;
	};

	return result;
}

int CallFunction(CallContext& cc)
{
	int result;

	switch(cc.type)
	{
	case Func:
		result = CallFunction(cc.f, cc.args, cc.paramsSize);
		break;
	case FuncObj:
		result = CallFunctionObj(cc.f, cc.obj, cc.args, cc.paramsSize);
		break;
	case FuncThis:
		result = CallFunctionThis(cc.f, cc.obj, cc.args, cc.paramsSize);
		break;
	default:
		assert(0);
		return -1;
	}

	if(cc.returnFloat)
	{
		__asm
		{
			fstp dword ptr[result];
		};
	}

	return result;
}

float int2float(int a)
{
	union C
	{
		int i;
		float f;
	};

	C c;
	c.i = a;
	return c.f;
}

void InitGameScript()
{
	/*cas::Engine script;

	cas::Enum* attributes = script.RegisterEnum("Attribute");
	for(int i = 0; i < (int)Attribute::MAX; ++i)
		attributes->Add(g_attributes[i].id, i);

	cas::Enum* skills = script.RegisterEnum("Skill");
	for(int i = 0; i < (int)Skill::MAX; ++i)
		skills->Add(g_skills[i].id, i);

	cas::Type* unit = script.RegisterType("Unit");
	unit->RegisterFunction("void Mod(Attribute, int)", (void*)&Unit_Mod);
	unit->RegisterFunction("void AddItem(Item)", (void*)&Unit::AddItemAndEquipIfNone);*/

	/*cas::Engine script;

	//script.RegisterFunction("int dodaj(int,int)", FUNCTION(dodaj));
	{
		int args[] = { 10, 5 };

		CallContext cc;
		cc.type = Func;
		cc.f = dodaj;
		cc.args = args;
		cc.paramsSize = 8;
		cc.returnFloat = false;
		
		int result = CallFunction(cc);
	}
	{
		float args[] = { 12.5f, 7.5f };
		CallContext cc;
		cc.type = Func;
		cc.f = dodaj;
		cc.args = args;
		cc.paramsSize = 8;
		cc.returnFloat = true;

		int result = CallFunction(cc);
		float f = int2float(result);
		float x = f;
	}
	{
		A a;
		a.a = 10;
		int args[] = { 15 };
		CallContext cc;
		cc.type = FuncObj;
		cc.f = dodaja;
		cc.args = args;
		cc.paramsSize = 4;
		cc.obj = &a;
		cc.returnFloat = false;

		int result = CallFunction(cc);
	}

	script.RegisterFunction("dodaj", FUNCTION_EX(Unit_Mod, (Unit*, Attribute, int), void));
	script.RegisterFunction("", METHOD(B, dodaj));
	script.RegisterFunction("", METHOD_EX(B, dodaj, (int), int));
	script.RegisterFunction("", METHOD_EX(B, dodaj, (float), float));
	script.RegisterFunction("", FUNCTION(D::dodaj));*/
}

struct XXX
{
	XXX()
	{
		InitGameScript();
	}
};

/*
XXX xxxx;
*/
