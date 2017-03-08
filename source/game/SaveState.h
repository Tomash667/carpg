#pragma once

enum SAVE_VERSION
{
	V_0_2 = 0, // 0.2/0.2.1
	V_0_2_5 = 1, // 0.2.5
	V_0_2_10 = 2, // 0.2.10
	V_0_2_12 = 3, // 0.2.12
	V_0_2_20 = 4, // 0.2.20/21/22/23
	V_0_3 = 5, // 0.3
	V_0_4 = 6, // 0.4/0.4.10/0.4.20
	V_0_10 = 7, // 0.10

	V_CURRENT = V_0_10
};

//-----------------------------------------------------------------------------
extern const int SAVE_VERSION;
extern int LOAD_VERSION;
extern const INT2 SUPPORT_LOAD_VERSION;
