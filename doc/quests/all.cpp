// quest properties
int progress, prev_progress; // default 0
	Unit* owner; // automatyicaly set from type on generation
	Location* start_loc, // automaticaly set
		*target_loc; // default null
QuestEntry* quest_entry; // default null, set in quest functions		
						
// globals
Player* player; // player talking to unit

// quest types
mayor
captain
traveler
other

// world functions
World.GetRandomSettlement(Location@ loc = null); // get random city or village that is not loc

// quest functions
ContinueDialog(string dialog_name); // required in on_init for mayor, captain & traveler quest
AddQuestEntry(string name, string text, string text2=null); // add quest entry (set quest_entry) with name and text, set quest_entry_state to 2
// for quest functions
int quest_entry_state;
void Quest::OnInit()
{
	quest_entry_state = 0;
	quest_obj->on_init();
	if(quest_entry_state != 0)
		game->AddGameMsg3(GMS_JOURNAL_UPDATED);
	// journal update
}
