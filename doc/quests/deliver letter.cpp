quest deliver_letter {
	type mayor
	progress {
		none
		accepted
		timeout
	}

	code ${
		Item@ quest_item;
	
		// called at init
		void on_init()
		{
			target_loc = World.GetRandomSettlement(start_loc);
			ContinueDialog("start");
		}

		// called when quest progress changes, progress & prev_progress are set
		void on_progress()
		{
			if(progress == accepted)
			{
				quest_item = CreateQuestItem("letter", "$letter");
				quest_item.name = text["letter_name"];
				player.unit.AddItem(quest_item);
				owner.AddDialog("check_timeout");
				owner.SetTimer(timeout, 30);
				target_loc.OnGenerate(add_dialog);
				AddQuestEntry(text["name"], text["accepted_text"], text["accepted_text2"]);
			}
			else if(progress == timeout)
			{
			}
			if(progress == started)
			{
				AddQuestEntry(text["name"], text["infoStart"]); // add journal entry (title, text)
				owner.RemoveGreet("start"); // remove greeting from owner
				owner.AddDialog("check_item"); // add dialog to check for item
				owner.SetTimer(timeout, 10); // add timeout after 10 days
			}
			else if(progress == complete)
			{
				QuestDone(text["done"]); // mark quest as done and add text to journal
				player.unit.RemoveItem("ziele"); // remove item from player
				owner.RemoveDialog("check_item"); // remove dialog
				owner.AddItem("ziele"); // add item to owner
				Team.GoldReward(300); // add gold reward to team
			}
			else if(progress == timeout)
			{
				QuestFail(text["timeout"]); // mark quest as failed and add text to journal
				owner.RemoveDialog("check_item"); // remove dialog
			}
		}
		
		void add_dialog()
		{
			target_loc.GetSpecialUnit("mayor").AddDialog(other_mayor);
		}
		
		/* default format_string, can write new on, on null it will be called)
		void format_string(string str)
		{
			target_mayor, target_locname
		}*/
	}$

	// dialogs
	dialog start {
		talk start
		choice yes { // universal keyword
			talk accepted
			talk accepted2
			set_progress accepted
			end
		}
		escape choice no // universal keyword
			end
		show_choices
	}
	
	dialog other_mayor {
	}
	
	// translation
	name = "Deliver letter"
	letter_name = "Letter for $target_mayor$ of $target_locname$"
	start = "I'm looking for someone who will deliver this letter to $target_mayor$ in $target_locname$. Are you interested in it?"
	accepted {
		"Good, here's the message. You have a month to deliver it and come back with the answer."
		"Good, take this letter. You have one month to provide me with an answer."
	}
	accepted2 "$target_locname$ is $target_dir$ from here."
	accepted_text "Obtained from $start_mayor$ of $start_locname$ at $date$."
	accepted_text2 "I got a letter that I need to provide to $target_mayor$ of $target_locname$. It's $target_dir$ from here. I have a month to go there and return."
	// po polsku
	// "Szukam kogoœ kto dostarczy ten list $target_mayor:przypadek$ $target_locname:przypadek$. Jesteœ zainteresowany?" z tymi przypadkami trzeba to dobrze zrobiæ, tu bêdzie burmistrzowi miasta Stonehange
	
}