 _______  _______  _______  _______  _______
(  ____ \(  ___  )(  ____ )(  ____ )(  ____ \
| (    \/| (   ) || (    )|| (    )|| (    \/
| |      | (___) || (____)|| (____)|| |
| |      |  ___  ||     __)|  _____)| | ____
| |      | (   ) || (\ (   | (      | | \_  )
| (____/\| )   ( || ) \ \__| )      | (___) |
(_______/|/     \||/   \__/|/       (_______)

Website: https://carpg.pl/en
Version: 0.18
Date: 2021-07-02

===================================================================================================
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

===================================================================================================
2) About
CaRpg is a combination of the type of game action rpg / hack-n-slash like Gothic or TES: Morrowind
and roguelike games like Slash'EM or Dungeon Crawl Stone Soup. In randomly generated world, we
begin in randomly generated city, recruit random characters and go to the randomly generated
dungeons to kill randomly generated enemies :3 This is not about everything to be random but also
matched to each other. There are several unique tasks (more precisely eight) whose performance
is the goal in the current version. It is available multiplayer mode for up to 8 people but let so
many people walking in the underworld is a little awkward because you have to scramble to kill
someone so it is proposed to play for 4 people. Expect changes for the better!

===================================================================================================
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
	E - use, loot, talk with while holding weapon
	left mouse button - use, attack, loot, talk
	Z / 4 mouse button - secondary attack
	X - cancel attack
	right mouse button - block
	W - move forward
	S - move backward
	A - move left
	D - move right
	1..0 - ability/item shortcuts
	F - auto walk forward
	Caps Lock - toggle run/walk
	Y - yell
	C - character screen
	I - inventory
	T - team
	K - abilities
	J - journal
	Tab - minimap
	'" - talk box
	Enter - input text in multiplayer
	Mouse wheel - change camera distance
	F1 - accept notification
	F3 - decline notification
	Ctrl + F1 - toggle fps
	Ctrl + F2 - toggle debug info
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
	F - search for location
3.5. Journal
	A / left arrow - previous page
	D / right arrow - next page
	W / up arrow - exit from quest details
	Q - previous quest
	E - next quest
	1 - quests
	2 - rumors
	3 - notes

===================================================================================================
4) How to play
* First play the tutorial, everything is there to find out. If out accidentally turned it off in
	carpg.cfg change "skipTutorial = 1" to 0
* Combat - The easiest way to attack the enemy is using melee weapon. Try to surround and attack
	them from behind while they are busy fighting with the others, then you deal more damage.
	Blocking is not currently as useful, you do not get such damage but the enemy does not receive
	any injuries. It is useful for blocking spells that ignore armor. Archery is useful as long as
	you have someone in the team who will stop the enemy from coming to you. In combat you can
	perform a powerful attack, you have to hold the attack for a while, so that it will be
	stronger. This is useful against resistant enemies which attack normal asking little or
	nothing. Attack on the run is like 0.25 of powerful attack but you cannot stop.
* Abilities - Use ability by pressing key 3 (by default), it will draw area of effect used by this
	ability. Press left mouse button to use, right to cancel. Red area means that ability can be
	used in this place. Some spells can be cast on yourself by holding walk back, player will be
	highlighted.
* Stamina - Attacking uses stamina, if you run out of stamina you can't run or attack. Blocking
	uses stamina too, and if you loss all stamina block is broken. Stop attacking to restore
	stamina, not walking restore it faster.
* Often, save the game, especially at the beginning when you are weak. Later, before the fight with
	the boss. Remember, you can always happen some unrepaired bug and the game may hang or crash :(
	Before saving the previous file is copied to other file (X.sav -> X.sav.bak) so in case of
	problems during saving you can copy it.
* Defeat - If the character with your team loses all hp he will fall to the ground. When there will
	be no enemies in the area he will recover with 1 hp. When all die there will be the end of the
	game. The game also ends when your character is retiring.
* The division of loot - Each team member receives a portion of the loot for themselves. Each NPC
	receives 10% of loot, the rest is divided equally between the players. Heroes who for some
	reason were in town get nothing. Items found in the dungeon are owned by team, when they sold
	the profit will be split. You can take for yourself a team item but then in the future you have
	to pay the remainder of the market value for the other heroes - it is marked as a credit. NPCs
	can afford to take an item if it is better than what we have currently, or you ask for it in
	the city. You can give heroes an item, if it is better than what they have to try to buy it
	from you. NPCs can take from you every potion, for free.
* Hardcore mode - In this mode, when you save you exit to menu and load deletes a save. Standard
	mode for roguelike games. It is not recommended so far because if the game freeze or crashes
	you will need to start new game.
* If the NPC will block your path you can yell at him (default key Y) so he will move out of the
	road.

===================================================================================================
5) Multiplayer mode
* General information - The multiplayer mode has been tested only on a LAN. Thanks to master server
	it is now possible to connect two computers behind router. But if it fails you need to manually
	change router settings (NAT). If the character's movement lags let the server change option in
	console 'mpInterp'. Default is 0.05, raising it until you find the character movement is
	adequate. Not forget to boast on the forum how it went! :)
* The leader - Man that determines where to go on a world map. Only he can kick out heroes and give
	them orders. NPC cannot be an leader. You can pass command to another from team panel (default
	T key). The server also can change the commander.
* The time - in singleplayer mode time passes normally when the player is resting or training. In
	multiplayer mode where one person is resting/training other players receive the same amount of
	days to use. How many days you can see in the team panel (default T key). When someone exceeds
	this number it will automatically increase it for all. The day changes only when the number is
	exceeded. Free days are decreasing during travel that players do not keep them indefinitely.
* Saving - Don't save game while in combat or during important dialogue because something might
	went wrong.
* If the load on the client takes too long you can change/add file configuration value of
	"timeout = X" where X = 1-3600 in seconds after which the player is kicked for timeout.
* Ports - game uses port udp 37557, can be changed in configuration file. Master servers uses ports
	tcp 8080 and udp 60481 for communication.

===================================================================================================
6) Changes
See changelog_eng.txt file.

===================================================================================================
7) Console command
To open console press ~ [to the left from 1]. Some commands are only available in multiplayer on
	lobby. Devmode must be activated (devmode 1). Available commands:
	addEffect - add effect to selected unit (addEffect effect <valueType> power [source [perk/time]]).
	addExp - add experience to team (addExp value).
	addGold - give gold to player (addGold count).
	addItem - add item to player inventory (addItem id [count]).
	addLearningPoints - add learning point to selected unit [count - default 1].
	addPerk - add perk to selected unit (addPerk perk).
	addTeamGold - give gold to team (addTeamGold count).
	addTeamItem - add team item to player inventory (addTeamItem id [count]).
	arena - spawns enemies on arena (example arena 3 rat vs 2 wolf).
	breakAction - break unit current action ('break 1' targets self).
	citizen - citizens/crazies don't attack player or his team.
	cleanLevel - remove all corpses and blood from level (cleanLevel [buildingId]).
	clear - clear text.
	cmds - show commands and write them to file commands.txt, with all show unavailable commands too (cmds [all]).
	crash - crash game to death!
	devmode - developer mode (devmode 0/1).
	dontWander - citizens don't wander around city (dontWander 0/1).
	drawCol - draw colliders (drawCol 0/1).
	drawFlags - set which elements of game draw (drawFlags int).
	drawHitbox - draw weapons hitbox (drawHitbox 0/1).
	drawParticleSphere - draw particle extents sphere (drawParticleSphere 0/1).
	drawPath - draw debug pathfinding, look at target.
	drawPhy - draw physical colliders (drawPhy 0/1).
	drawUnitRadius - draw units radius (drawUnitRadius 0/1).
	exit - exit to menu.
	fall - unit fall on ground for some time ('fall 1' targets self).
	find - find nearest entity (find type id).
	forceQuest - force next random quest to select (use list quest or none/reset).
	godmode - player can't be killed (godmode 0/1).
	gotoMap - transport player to world map.
	grassRange - grass draw range.
	heal - heal player.
	healUnit - heal unit in front of player.
	help - display information about command (help [command]).
	hurt - deal 100 damage to unit ('hurt 1' targets self).
	invisible - ai can't see player (invisible 0/1).
	kill - kill unit in front of player.
	killall - kills all enemy units in current level, with 1 it kills allies too, ignore unit in front of player (killall [0/1]).
	list - display list of types, don't enter type to list possible choices (list type [filter]).
	listEffects - display selected unit effects.
	listPerks - display selected unit perks.
	listStats - display selected unit stats.
	load - load game (load 1-11 or filename).
	map2console - draw dungeon map in console.
	modStat - modify player statistics, use ? to get list (modStat stat value).
	mpInterp - interpolation interval (mpInterp 0.f-1.f).
	mpUseInterp - set use of interpolation (mpUseInterp 0/1).
	multisampling - sets multisampling (multisampling type [quality]).
	nextSeed - random seed used in next map generation.
	noai - disable ai (noai 0/1).
	nocd - player abilities have no cooldown & use no mana/stamina (nocd 0/1).
	noclip - turn off player collisions (noclip 0/1).
	pause - pause/unpause.
	playerDevmode - get/set player developer mode in multiplayer (playerDevmode nick/all [0/1]).
	qs - pick random character, get ready and start game.
	quit - quit from game.
	random - roll random number 1-100 or pick random character (random, random [name], use ? to get list).
	refreshCooldown - refresh action cooldown/charges.
	reloadShaders - reload shaders.
	removeEffect - remove effect from selected unit (removeEffect effect/source [perk] [valueType]).
	removePerk - remove perk from selected unit (removePerk perk).
	removeUnit - remove selected unit.
	resolution - show or change display resolution (resolution [w h]).
	reveal - reveal all locations on world map.
	revealMinimap - reveal dungeon minimap.
	save - save game (save 1-11 [text] or filename).
	scare - enemies escape.
	screenshot - save screenshot.
	select - select and display currently selected target (select [me/show/target] - use target or show by default).
	setStat - set player statistics, use ? to get list (setStat stat value).
	skipDays - skip days [skipDays [count]).
	spawnUnit - create unit in front of player (spawnUnit id [level count arena]).
	speed - game speed (speed 0-10).
	stun - stun unit for time (stun [length=1] [1 = self]).
	suicide - kill player.
	tileInfo - display info about map tile.
	useFog - draw fog (useFog 0/1).
	useGlow - use glow (useGlow 0/1).
	useLighting - use lighting (useLighting 0/1).
	useNormalmap - use normal mapping (useNormalmap 0/1).
	usePostfx - use post effects (usePostfx 0/1).
	useSpecularmap - use specular mapping (useSpecularmap 0/1).
	uvMod - terrain uv mod (uvMod 1-256).
	verify - verify game state integrity.
	version - displays game version.
	warp - move player into building (warp building/group [front]).
	whisper/w - send private message to player (whisper nick msg).

===================================================================================================
8) Config file
In configuration file (by default carpg.cfg) you can use such options:
	* autopick (random warrior hunter rogue) - automatically picks character in multiplayer and set
		as ready. Works only once
	* autostart (count>=1) - automatically starts up when specified number of players join and are
		ready
	* changeTitle (true [false]) - change window title depending on game mode.
	* checkUpdates ([true] false) - check for game updates
	* class (warrior hunter rogue) - selected class in quick singleplayer game
	* conPos Int2 - console position x, y
	* conSize Int2 - console size x, y
	* console (true [false]) - windows console
	* crashMode (none [normal] dataseg full) - mode to save crash information
	* grassRange (0-100) - grass draw range
	* featureLevel ("10.0" "10.1" "11.0") - directx feature level
	* fullscreen ([true] false) - fullscreen mode
	* inactiveUpdate (true [false]) - update singleplayer game even if window is not active
	* log ([true] false) - logging to file
	* logFilename ["log.txt"] - logging to file name
	* name - player name in quick game
	* nick - nick in multiplayer game
	* nosound (true [false]) - don't load sounds & music, can't turn it on
	* packetLogger (true [false]) - logging MP packets
	* port ([37557]) - port in multiplayer
	* quickstart (single host join joinip) - automatically start game in selected mode (single -
		singleplayer mode, use nick and class), in multiplayer uses (nick, serverName,
		serverPlayers, serverPswd, serverIp), if options not set it won't work
	* resolution (800x600 [1024x768]) - screen resolution
	* screenshotFormat – set screenshot format (jpg, bmp, tif, gif, png, dds)
	* serverIp - last server ip address
	* serverLan - if true server won't be registered on master server
	* serverName - last server name
	* serverPlayers - last server max players
	* serverPswd - last server password
	* skipTutorial (true [false]) - skip tutorial
	* timeout (1-3600) - time before kicking player from server when loading if no response
		(default 10)
	* vsync ([true] false) - force vertical synchronization
	* wndPos Int2 - window position x, y
	* wndSize - window size x, y

===================================================================================================
9) Command line
Command line switches are used for exe shortcut or from window shell. All config settings can be
used. Aditionaly you can change which config file to use: -config file.cfg - use this config file

===================================================================================================
10) Report bug 🐜
Errors and bugs can and should be reported to the appropriate forum section in https://carpg.pl/en
forum. Do not forget to give all possible details that may help in its repair. If game crash with
created minidump file you can attach it. Log file can be helpful too. Before reporting a bug check
if someone has not reported it in the appropriate topic.

===================================================================================================
11) Credits
Tomashu - Programming, models, textures, ideas, story.
Leinnan - Models, textures, ideas, testing.
MarkK - Models and textures of food and other objects.
Groszek - Models, textures and gui.
Shdorsh - English translation fixes.
Zielu - Testing and models.
BottledByte - Bug reports and coding.

❤ Thanks for testing & bug founding:
	Ampped
	AnimeIsWrong
	darktorq
	Docucat
	fire
	Groszek
	Harorri
	Lemiczek
	Medarc
	MikelkCZ
	MildlyPhilosophicalHobbit
	Minister of Death
	mishka
	Mokai
	Paradox Edge
	Savagesheep
	SwagOfficerSuccubus
	thebard88
	Vinur_Gamall
	Woltvint
	XNautPhD
	xweert123
	Zettaton
	Zinny
🎵 Audio Engine: FMOD Studio by Firelight Technologies.
🗃 Models:
	* https://opengameart.org
		* CDmir - chest
		* Clint Bellanger - naraphim sword
		* Enetheru - windmill
		* Grefuntor - slime
		* johndh - cutlass
		* kindland - shield of death
		* Lucian Pavel - short sword, kite shield
		* Mophs - scarecrow
		* nubux - ogre
		* p0ss - long sword
		* rubberduck - shrine
		* sandsound - morningstar
		* System G6 - shield
		* TiZiana - vodka, gray potion
		* Wind astella - battle axe
	* kaangvl - fish
	* yd - sausage
🎨 Textures:
	* https://www.textures.com/
	* Cube
	* texturez.com
	* texturelib.com
	* SwordAxeIcon by Raindropmemory
	* Class icons - https://game-icons.net/
🎧 Music:
	* Łukasz Xwokloiz
	* Celestial Aeon Project
	* Project Divinity
	* HorrorPen - Winds of Stories
	* For Originz - Kevin MacLeod (incompetech.com)
	* http://www.deceasedsuperiortechnician.com/
🔈 Sounds:
	* https://opengameart.org
		* artisticdude - ogre sounds
		* OwlishMedia - coughs
	* https://www.freesound.org/
		* DrMinky - Slime Death, Slime Land
		* InspectorJ (www.jshaw.co.uk) - Boiling Water, Large
		* newagesoup - wolf growl
		* wubitog - Slime attack or movement
	* https://www.pacdv.com/sounds/
	* https://www.soundjay.com
	* http://www.grsites.com/archive/sounds/
	* http://www.wavsource.com
