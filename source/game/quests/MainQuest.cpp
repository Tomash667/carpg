#include "Pch.h"
#include "Base.h"
#include "MainQuest.h"
#include "Dialog.h"
#include "DialogDefine.h"

DialogEntry dialog_main[] = {
	TALK2(1307),
	TALK(1308),
	TALK(1309),
	TALK(1310),
	DO_QUEST("main"),
	TALK(1311),
	END
};
