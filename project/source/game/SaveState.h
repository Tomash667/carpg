#pragma once

//-----------------------------------------------------------------------------
enum SAVE_VERSION
{
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
	V_0_7 = 10, // 0.7
	V_FEATURE = 10, // 0.7?

	V_CURRENT = V_FEATURE
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
extern const int SAVE_VERSION;
extern int LOAD_VERSION;
extern const int MIN_SUPPORT_LOAD_VERSION;
