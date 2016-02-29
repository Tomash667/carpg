quest find_herbs {
    // builtin quest properties
    int progress, prev_progress;
    Unit@ owner;

    progress {
        none
        started
        timeout
        complete
    }

    code ${
        // called at init
        void on_init()
        {
            owner.AddGreet("start"); // add greeting for owner
        }

        // called when quest progress changes, progress & prev_progress are set
        void on_progress()
        {
            if(progress == started)
            {
                AddQuestEntry(text["name"], text["infoStart"]); // add journal entry (title, text)
                owner.RemoveGreet("start"); // remove greeting from owner
                owner.AddDialog("check_item"); // add dialog to check for item
                SetTimer(timeout, 10); // add timeout after 10 days
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
    }$

    // dialogs  
    dialog start {
        talk greet0
        talk greet1
        talk greet2
        choice yes
            do_progress started // jak set_progress ale nie wraca
        escape choice no
            end // bêdzie gada³ przy ka¿dym rozpoczêciu dialogu
        show_choices
    }

    dialog accepted {
        talk accepted
        end
    }

    dialog check_item {
        if have_item "ziele" {
            choice give
                do_progress complete
        }
    }
}