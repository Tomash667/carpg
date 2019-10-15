 _______  _______  _______  _______  _______
(  ____ \(  ___  )(  ____ )(  ____ )(  ____ \
| (    \/| (   ) || (    )|| (    )|| (    \/
| |      | (___) || (____)|| (____)|| |
| |      |  ___  ||     __)|  _____)| | ____
| |      | (   ) || (\ (   | (      | | \_  )
| (____/\| )   ( || ) \ \__| )      | (___) |
(_______/|/     \||/   \__/|/       (_______)

Website: http://carpg.pl/en
Version: 0.12
Date: 2019-09-22

===============================================================================
1) Table of contents
	1... Table of contents
	2... About
	3... Controls
	4... How to play
	5... Multiplayer mode
	6... Changes
	7... Console commands
	8... Configuration file
	9... Command line
	10.. Report bug
	11.. Credits

===============================================================================
2) About
CaRpg is a combination of the type of game action rpg / hack-n-slash like Gothic
or TES: Morrowind and rougelike games like Slash'EM or Dungeon Crawl Stone Soup.
In randomly generated world, we begin in randomly generated city, recruit random
characters and go to the randomly generated dungeons to kill randomly generated
enemies :3 This is not about everything to be random but also matched to each
other. There are several unique tasks (more precisely eight) whose performance
is the goal in the current version. It is available multiplayer mode for up to 8
people but let so many people walking in the underworld is a little awkward
because you have to scramble to kill someone so it is proposed to play for 4
people. Expect changes for the better!

===============================================================================
3) Controls - Can be changed in options.
3.1. Common
	Esc - menu / close window
	Alt + F4 - quit from game
	Print Screen - do screenshot (with shift without gui)
	F5 - quicksave
	F9 - quickload
	Pause - pause game
	Ctrl + U - unlock cursor in windowed mode
	~ - console
3.2. In game
	Z / left mouse button - use, attack, loot, talk
	R - use, loot, talk with while holding weapon
	X / right mouse button - block
	W / up arrow - move forward
	S / down arrow - move backward
	A / left arrow - move left
	D / right arrow - move right
	Q - rotate left
	E - rotate right
	1 - equip melee weapon and shield
	2 - equip ranged weapon
	3 - use action
	4 - drink best healing potion
	F - auto walk forward
	Caps Lock - toggle run/walk
	Y - yell
	TAB - character screen, team, inventory
	J - journal
	M - minimap
	N - talk box
	Enter - input text in multiplayer
	Mouse wheel - change camera distance
	F2 - show/hide fps
3.3. Equipment
	left mouse button - use, wear, sell, buy
	right mouse button - drop
	click + shift - apply action to all items (drop/pickup)
	click + ctrl - apply action to single item (drop/pickup)
	click + alt - show dialog window with number to enter for how many items do action
	F - pick all items
3.4. World map
	left mouse button - travel to location / enter
	Tab - open / close multiplayer input panel
	Enter - input text in multiplayer
3.5. Journal
	A / left arrow - previous page
	D / right arrow - next page
	W / up arrow - exit from quest details
	Q - previous quest
	E - next quest
	1 - quests
	2 - rumors
	3 - notes

===============================================================================
4) How to play
* First play the tutorial, everything is there to find out. If out accidentally
	turned it off in carpg.cfg change "skip_tutorial = 1" to 0
* Combat - The easiest way to attack the enemy is using melee weapon. Try to
	surround and attack them from behind while they are busy fighting with the
	others, then you deal more damage. Blocking is not currently as useful, you do
	not get such damage but the enemy does not receive any injuries. It is useful
	for blocking spells that ignore armor. Archery is useful as long as you have
	someone in the team who will stop the enemy from coming to you. In combat you
	can perform a powerful attack, you have to hold the attack for a while, so
	that it will be stronger. This is useful against resistant enemies which
	attack normal asking little or nothing. Attack on the run is like 0.25 of
	powerful attack but you cannot stop.
* Actions - Use action by pressing key 3 (by default), it will draw area of effect
	used by this action. Press left mouse button to use, right to cancel. Red
	area means that action can be used in this place.
* Stamina - Attacking uses stamina, if you run out of stamina you can't run or
	attack. Blocking uses stamina too, and if you loss all stamina block is
	broken. Stop attacking to restore stamina, not walking restore it faster.
* Often, save the game, especially at the beginning when you are weak. Later,
	before the fight with the boss. Remember, you can always happen some
	unrepaired bug and the game may hang or crash :( Before saving the previous
	file is copied to other file (X.sav -> X.sav.bak) so in case of problems
	during saving you can copy it.
* Defeat - If the character with your team loses all hp he will fall to the
	ground. When there will be no enemies in the area he will recover with 1 hp.
	When all die there will be the end of the game. The game also ends in year
	160, your character is retiring.
* The division of loot - Each team member receives a portion of the loot for
	themselves. Each NPC receives 10% of loot, the rest is divided equally between
	the players. Heroes who for some reason were in town get nothing. Items found
	in the dungeon are owned by team, when they sold the profit will be split. You
	can take for yourself a team item but then in the future you have to pay the
	remainder of the market value for the other heroes - it is marked as a credit.
	NPCs can afford to take an item if it is better than what we have currently,
	or you ask for it in the city. You can give heroes an item, if it is better
	than what they have to try to buy it from you. NPCs can take from you every
	potion, for free.
* Hardcore mode - In this mode, when you save you exit to menu and load deletes
	a save. Standard mode for rougelike games. It is not recommended so
	far because if the game freeze or crashes you will need to start new game.
* If the NPC will block your path you can yell at him (default key Y) so he
	will move out of the road.

===============================================================================
5) Multiplayer mode
* General information - The multiplayer mode has been tested only on a LAN. Thanks
	to master server it is now possible to connect two computers behind router.
	But if it fails you need to manualy change router settings (NAT). If the
	character's movement lags let the server change option in console
	'mp_interp'. Default is 0.05, raising it until you find the character movement
	is adequate. Not forget to boast on the forum how it went! :)
* The leader - Man that determines where to go on a world map. Only he can kick
	out heroes and give them orders. NPC cannot be an leader. You can pass
	command to another from team panel (default T key). The server also can change
	the commander.
* The time - in singleplayer mode time passes normally when the player is
	resting or training. In multiplayer mode where one person is resting /
	training other players receive the same amount of days to use. How many days
	you can see in the team panel (default T key). When someone exceeds this number
	it wil automatically increase it for all. The day changes only when the number
	is exceeded. Free days are decreasing during travel that players do not keep
	them indefinitely.
* Saving - Don't save game while in combat or during important dialogue
	because something might went wrong. You can only load game from menu.
* If the load on the client takes too long you can change / add file
	configuration value of "timeout = X" where X = 1-3600 in seconds after which
	the player is kicked for timeout.
* Ports - game uses port udp 37557, can be changed in configuration file. Master
	servers uses ports tcp 8080 and udp 60481 for communication.

===============================================================================
6) Changes
See changelog_eng.txt file.

===============================================================================
7) Console command
To open console press ~ [to the left from 1]. Some commands are only available
in multiplayer on lobby. Devmode must be activated (devmode 1).
Available commands:
	add_effect - add effect to selected unit (add_effect effect <value_type> power [source [perk/time]]).
	add_exp - add experience to team (add_exp value).
	add_gold - give gold to player (add_gold count).
	add_team_gold - give gold to team (add_team_gold count).
	add_item - add item to player inventory (add_item id [count]).
	add_learning_points - add learning point to selected unit [count - default 1].
	add_team_item - add team item to player inventory (add_team_item id [count]).
	arena - spawns enemies on arena (example arena 3 rat vs 2 wolf).
	break_action - break unit current action ('break 1' targets self)
	citizen - citizens/crazies don't attack player or his team.
	clean_level - remove all corpses and blood from level (clean_level [building_id]).
	clear - clear text.
	cmds - show commands and write them to file commands.txt, with all show unavailable commands too (cmds [all]).
	crash - crash game to death!
	devmode - developer mode (devmode 0/1).
	dont_wander - citizens don't wander around city (dont_wander 0/1).
	draw_col - draw colliders (draw_col 0/1).
	draw_flags - set which elements of game draw (draw_flags int).
	draw_hitbox - draw weapons hitbox (draw_hitbox 0/1).
	draw_particle_sphere - draw particle extents sphere (draw_particle_sphere 0/1).
	draw_phy - draw physical colliders (draw_phy 0/1).
	draw_unit_radius - draw units radius (draw_unit_radius 0/1).
	exit - exit to menu.
	fall - unit fall on ground for some time ('fall 1' targets self)
	fog - draw fog (fog 0/1).
	force_quest - force next random quest to select (use list quest or none/reset)
	godmode - player can't be killed (godmode 0/1).
	goto_map - transport player to world map.
	grass_range - grass draw range (grass_range 0-100).
	heal - heal player.
	heal_unit - heal unit in front of player.
	help - display information about command (help [command]).
	hurt - deal 100 damage to unit ('hurt 1' targets self)
	invisible - ai can't see player (invisible 0/1).
	kick - kick player from server (kick nick).
	kill - kill unit in front of player.
	killall - kills all enemy units in current level, with 1 it kills allies too, with 2 kill inside buildings (killall [1/2]).
	leader - change team leader (leader nick).
	lighting - use lighting (lighting 0/1).
	list - display list of types, don't enter type to list possible choices (list type [filter]).
	list_effects - display selected unit effects.
	list_perks - display selected unit perks.
	list_stats - display selected unit stats.
	load - load game (load 1-10 or name).
	map2console - draw dungeon map in console.
	mod_stat - modify player statistics (modstat stat value), use modstat ? to get list of stats.
	mp_interp - interpolation interval (mp_interp 0.f-1.f).
	mp_use_interp - set use of interpolation (mp_use_interp 0/1).
	multisampling - sets multisampling (multisampling type [quality]).
	next_seed - random seed used in next map generation.
	noai - disable ai (noai 0/1).
	noclip - turn off player collisions (noclip 0/1).
	pause - pause/unpause.
	play_music - sets if play music (play_music 0/1).
	play_sound - sets if play sound (play_sound 0/1).
	player_devmode - allow/disallow developer mode for player in multiplayer (player_devmode nick/all 0/1).
	profiler - profiler execution: 0-disabled, 1-update, 2-rendering.
	qs - pick random character, get ready and start game.
	quickload - load game from last slot.
	quicksave - save game on last slot.
	quit - quit from game.
	random - roll random number 1-100 or pick random character (random, random [warrior/hunter/rogue]).
	ready - set player as ready/unready.
	refresh_cooldown - refresh action cooldown/charges.
	reload_shaders - reload shaders.
	remove_effect - remove effect from selected unit (remove_effect effect/source [perk] [value_type]).
	remove_perk - remove perk from selected unit (remove_perk perk).
	remove_unit - remove selected unit.
	resolution - show or change display resolution (resolution [w h hz]).
	reveal - reveal all locations on world map.
	reveal_minimap - reveal dungeon minimap.
	refresh_cooldown - refresh action cooldown/charges.
	s - send message to all players, short from say (say msg).
	save - save game (save 1-10 [text] or name).
	say - send message to all players (say msg).
	scare - enemies escape.
	screenshot - save screenshot.
	select - select and display currently selected target (select [me/show/target] - use target or show by default).
	server - send message from server to all players (server msg).
	set_stat - set player statistics (setstat stat value), use setstat ? to get list of stats.
	set_seed - set randomness seed (set_seed uint).
	shader_version - force shader version (shader_version 2/3).
	skip_days - skip days [skip_days [count]).
	spawn_unit - create unit in front of player (spawn_unit id [level count arena]).
	speed - game speed (speed 0-10).
	start - start server.
	stun - stun unit for time (stun [length=1] [1 = self]).
	suicide - kill player.
	tile_info - write current terrain tile info to console.
	version - displays game version.
	w - send private message to player, short from whisper (w nick msg).
	warp - move player into building (warp inn/arena/hall).
	whisper - send private message to player (whisper nick msg).

===============================================================================
8) Config file
In configuration file (by default carpg.cfg) you can use such options:
	* autopick (random warrior hunter rogue) - automatically picks character in
		multiplayer and set as ready. Works only once
	* autostart (count>=1) - automatically starts up when specified number of players
		join and are ready
	* change_title (true [false]) - change window title depending on game mode.
	* check_updates ([true] false) - check for game updates
	* class (warrior hunter rogue) - selected class in quick singleplayer game
	* con_pos Int2 - console position x, y
	* console (true [false]) - windows console
	* crash_mode (none [normal] dataseg full) - mode to save crash information
	* grass_range (0-100) - grass draw range
	* force_seed (true [false]) - force randomness seed on all levels
	* fullscreen ([true] false) - fullscreen mode
	* inactive_update (true [false]) - update singleplayer game even if window is
		not active
	* log ([true] false) - logging to file
	* log_filename ["log.txt"] - logging to file name
	* name - player name in quick game
	* next_seed - next randomness seed
	* nick - nick in multiplayer game
	* nomusic (true [false]) - don't load music, can't turn it on
	* nosound (true [false]) - don't load sound, can't turn it on
	* packet_logger (true [false]) - logging MP packets
	* play_music ([true] false) - play music
	* play_sound ([true] false) - play sound
	* port ([37557]) - port in multiplayer
	* quickstart (single host join joinip) - automatically start game in
		selected mode (single - singleplayer mode, use nick and class), in
		multiplayer uses (nick, server_name, server_player, server_pswd, server_ip),
		if options not set it won't work
	* resolution (800x600 [1024x768]) - screen resolution
	* screenshot_format - ustawia rozszerzenie screenshotów (jpg, bmp, tga, png)
	* seed - randomness seed
	* server_ip - last server ip address
	* server_lan - if true server won't be registered on master server
	* server_name - last server name
	* server_players - last server max players
	* server_pswd - last server password
	* shader_version - force shader version 2/3
	* skip_tutorial (true [false]) - skip tutorial
	* stream_log_file ["log.stream"] - file to log mp information
	* stream_log_mode (none [errors] full) - mode to log mp information
	* timeout (1-3600) - time before kicking player from server when loading if
		no response (default 10)
	* vsync ([true] false) - force vertical synchronization
	* wnd_pos Int2 - window position x, y
	* wnd_size - window size x, y

===============================================================================
9) Command line
Command line switches are used for exe shortcut or from window shell.
-config file.cfg - use this config file
-console - open windows console
-delay-1 - start delay for other game instance
-delay-2 - wait for other game instance to finish loading
-fullscreen - run in fullscreen
-host - start hosting LAN server
-hostip - start hosting Internet server
-join - join LAN server
-joinip - join Internet server
-nomusic - no music
-nosound - no sound
-single - start quick singleplayer game
-windowed - windowed mode
-test - test game data

===============================================================================
10) Report bug
Errors and bugs can and should be reported to the appropriate forum section in
http://carpg.pl/en forum. Do not forget to give all possible details that may
help in its repair. If game crash with created minidump file you can attach it.
Log file can be helpful too. Before reporting a bug check if someone has not
reported it in the appropriate topic.

===============================================================================
11) Credits
Tomashu - Programming, models, textures, ideas, story.
Leinnan - Models, textures, ideas, testing.
MarkK - Models and textures of food and other objects.
Shdorsh - English translation fixes.
Zielu - Testing.

Thanks for bug founding:
	Ampped
	darktorq
	Docucat
	fire
	Harorri
	Medarc
	MikelkCZ
	MildlyPhilosophicalHobbit
	Minister of Death
	Paradox Edge
	Savagesheep
	thebard88
	Vinur_Gamall
	XNautPhD
	xweert123
	Zettaton
	Zinny
	
Audio Engine: FMOD Studio by Firelight Technologies.

Models:
	* kaangvl - fish
	* yd - sausage
Textures:
	* http://cgtextures.com
	* Cube
	* texturez.com
	* texturelib.com
	* SwordAxeIcon by Raindropmemory
	* Class icons - http://game-icons.net/
Music:
	* £ukasz Xwokloiz
	* Celestial Aeon Project
	* Project Divinity
	* For Originz - Kevin MacLeod (incompetech.com)
	* http://www.deceasedsuperiortechnician.com/
Sounds:
	* http://www.freesound.org/
	* http://www.pacdv.com/sounds/
	* http://www.soundjay.com
	* http://www.grsites.com/archive/sounds/
	* http://www.wavsource.com
