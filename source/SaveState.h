#pragma once

//-----------------------------------------------------------------------------
enum SAVE_VERSION
{
	// old versions - saves not compatibile
	V_0_2 = 0, // 0.2/0.2.1
	V_0_2_5 = 1, // 0.2.5
	V_0_2_10 = 2, // 0.2.10
	V_0_2_12 = 3, // 0.2.12
	V_0_2_20 = 4, // 0.2.20/21/22/23
	V_0_3 = 5, // 0.3
	V_0_4 = 6, // 0.4/0.4.10/0.4.20
	V_0_5 = 7, // 0.5
	V_0_5_1 = 8, // 0.5.1
	V_0_6 = 9, // 0.6/0.6.1
	V_0_6_2 = 10, // 0.6.2
	V_0_7 = 11, // 0.7

	MIN_SUPPORT_LOAD_VERSION = 12,
	V_0_7_1 = 12, // 0.7.1
	V_0_8 = 13, // 0.8
	V_0_9 = 14, // 0.9
	V_0_10 = 15, // 0.10
	V_0_11 = 16, // 0.11/0.11.1
	V_0_12 = 17, // 0.12/0.12.2
	V_0_13 = 18, // 0.13
	V_0_14 = 19, // 0.14
	V_0_14_1 = 20, // 0.14.1
	V_0_15 = 21, // 0.15
	V_0_16 = 22, // 0.16
	V_NEXT = 23,

	// save version used by saves
	V_CURRENT = V_NEXT,

	// use this versions in development on different branches
	V_MAIN = V_NEXT, // main bugfix branch
	V_DEV = V_NEXT // development/feature branch
};

//-----------------------------------------------------------------------------
struct SaveException
{
	SaveException(cstring localized_msg, cstring msg, bool missing_file = false) : localized_msg(localized_msg), msg(msg), missing_file(missing_file)
	{
	}

	cstring localized_msg, msg;
	bool missing_file;
};

//-----------------------------------------------------------------------------
extern int LOAD_VERSION;
