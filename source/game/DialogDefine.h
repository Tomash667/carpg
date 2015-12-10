#pragma once

//-----------------------------------------------------------------------------
#define CHOICE(msg) DT_CHOICE, (cstring)msg
#define TRADE DT_TRADE, nullptr
#define END_CHOICE DT_END_CHOICE, nullptr
#define TALK(msg) DT_TALK, (cstring)msg
#define TALK2(msg) DT_TALK2, (cstring)msg
#define RESTART DT_RESTART, nullptr
#define SHOW_CHOICES DT_SHOW_CHOICES, nullptr
#define END DT_END, nullptr
#define END2 DT_END2, nullptr
#define SET_QUEST_PROGRESS(val) DT_SET_QUEST_PROGRESS, (cstring)val
#define SPECIAL(msg) DT_SPECIAL, msg
#define IF_QUEST_TIMEOUT DT_IF_QUEST_TIMEOUT, nullptr
#define END_IF DT_END_IF, nullptr
#define IF_RAND(val) DT_IF_RAND, (cstring)val
#define ELSE DT_ELSE, nullptr
#define CHECK_QUEST_TIMEOUT(id) DT_CHECK_QUEST_TIMEOUT, (cstring)id
#define IF_HAVE_QUEST_ITEM(name) DT_IF_HAVE_QUEST_ITEM, name
#define DO_QUEST(name) DT_DO_QUEST, name
#define DO_QUEST_ITEM(name) DT_DO_QUEST_ITEM, name
#define IF_QUEST_PROGRESS(val) DT_IF_QUEST_PROGRESS, (cstring)val
#define IF_NEED_TALK(name) DT_IF_NEED_TALK, name
#define ESCAPE_CHOICE DT_ESCAPE_CHOICE, nullptr
#define IF_SPECIAL(msg) DT_IF_SPECIAL, msg
#define IF_ONCE DT_IF_ONCE, nullptr
#define RANDOM_TEXT(ile) DT_RANDOM_TEXT, (cstring)ile
#define IF_CHOICES(ile) DT_IF_CHOICES, (cstring)ile
#define DO_QUEST2(name) DT_DO_QUEST2, name
#define IF_HAVE_ITEM(name) DT_IF_HAVE_ITEM, name
#define IF_QUEST_PROGRESS_RANGE(x,y) DT_IF_QUEST_PROGRESS_RANGE, (cstring)((x&0xFFFF)|((y&0xFFFF)<<16))
#define IF_QUEST_EVENT DT_IF_QUEST_EVENT, nullptr
#define END_OF_DIALOG DT_END_OF_DIALOG, nullptr
#define DO_ONCE DT_DO_ONCE, nullptr
#define NOT_ACTIVE DT_NOT_ACTIVE, nullptr
#define IF_QUEST_SPECIAL(msg) DT_IF_QUEST_SPECIAL, msg
#define QUEST_SPECIAL(msg) DT_QUEST_SPECIAL, msg
