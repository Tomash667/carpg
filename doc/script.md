Core library
-------------------------------------------------------------------------------------------------------------
### Global functions
* void Info(const string& in msg) - show info msg in console.
* void DevInfo(const string& in msg) - like info but only in dev mode.
* void Warn(const string& in msg) - show warning msg in console.
* void Error(const string& in msg) - show error msg in console.
* string Format(const string& in format_str, ?& in ...) - return formatted string, takes 0 to 8 any arguments.
* int Random(int a, int b) - returns random number in range <a,b>.
* int Rand() - returns random int number.
* void Sleep(float seconds) - resume script execution after some time, don't use in places that require return value instantly like callbacks or dialog_if. Console output don't work after sleep!

### Core types
* Int2 - 2d int point x, y.
* Vec2 - 2d vector x, y. Static methods:
  * float Distance(const Vec2& in v1, const Vec2& in v2);
* Vec3 - 3d vector x, y, z. Static methods:
  * float Distance(const Vec3& in v1, const Vec3& in v2);
* Vec4 - 4d vector x, y, z, w.

### Var & Vars type
Used to store types in units/globals or pass between quests.

Example:

	Unit@ unit = ...;
	unit.vars["quest_var"] = 42;
	int value = unit.vars["quest_var"];

Static properties:

* Vars@ globals -  global variables.

Game enums & consts
-------------------------------------------------------------------------------------------------------------
### Funcdefs:
* float GetLocationCallback(Location@)

### Enum EVENT
* EVENT_ANY - used in RemoveEventHandler to remove all handlers.
* EVENT_ENTER - for locations, send when player enter location.
* EVENT_PICKUP - for locations, send when someone pickups ground item.
* EVENT_CLEARED - for locations, send when location is cleared from enemies.
* EVENT_GENERATE - for locations, send on first visit, currently only works for dungeons.
* EVENT_UPDATE - for units, send every frame.
* EVENT_DIE - for units, send when unit dies.
* EVENT_TIMEOUT - for quests, send when quest timeout expired.
* EVENT_ENCOUNTER - for quest encounter, send when team start encounter on world map.

### Enum flags ITEM_FLAGS
* ITEM_NOT_SHOP - not generated in shop.
* ITEM_NOT_MERCHANT - not generated for merchant.
* ITEM_NOT_BLACKSMITH - not generated for blacksmith.
* ITEM_NOT_ALCHEMIST - not generated for alchemist.

### Enum ITEM_TYPE
* IT_WEAPON
* IT_BOW
* IT_SHIELD
* IT_ARMOR
* IT_AMULET
* IT_RING
* IT_OTHER
* IT_CONSUMABLE
* IT_BOOK

### Enum LOCATION
* L_CITY
* L_CAVE
* L_CAMP
* L_DUNGEON
* L_OUTSIDE
* L_ENCOUNTER

### Enum LOCATION_IMAGE
* LI_CITY
* LI_VILLAGE
* LI_CAVE
* LI_CAMP
* LI_DUNGEON
* LI_CRYPT
* LI_FOREST
* LI_MOONWELL
* LI_TOWER
* LI_LABYRINTH
* LI_MINE
* LI_SAWMILL
* LI_DUNGEON2
* LI_ACADEMY
* LI_CAPITAL

### Enum LOCATION_TARGET
* FOREST
* MOONWELL
* ACADEMY
* VILLAGE
* CITY
* CAPITAL
* HUMAN_FORT
* DWARF_FORT
* MAGE_TOWER
* BANDITS_HIDEOUT
* HERO_CRYPT
* MONSTER_CRYPT
* OLD_TEMPLE
* VAULT
* NECROMANCER_BASE
* LABYRINTH
* ANCIENT_ARMORY
* TUTORIAL_FORT
* THRONE_FORT
* THRONE_VAULT

### Enum MOVE_TYPE
* MOVE_RUN - always run.
* MOVE_WALK - always walk.
* MOVE_RUN_WHEN_NEAR_TEAM - run when near team, otherwise walk.

### Enum QUEST_STATE
* Q_HIDDEN - don't show in journal
* Q_STARTED - black text in journal
* Q_COMPLETED - green text in journal
* Q_FAILED - red text in journal

### Enum ROOM_TARGET
* ROOM_NONE
* ROOM_CORRIDOR
* ROOM_STAIRS_UP
* ROOM_STAIRS_DOWN
* ROOM_TREASURY
* ROOM_PORTAL
* ROOM_PRISON
* ROOM_THRONE

### Enum STAIRS_LOCATION
* STAIRS_NONE
* STAIRS_RANDOM
* STAIRS_FAR_FROM_ROOM
* STAIRS_BORDER
* STAIRS_FAR_FROM_UP_STAIRS

### Enum UNIT_ORDER
* ORDER_NONE
* ORDER_WANDER - for heroes, they wander freely around city.
* ORDER_WAIT - for heroes, stay close to current position.
* ORDER_FOLLOW - for heroes, follow team leader.
* ORDER_LEAVE - unit goes to nearest location exit and leave.
* ORDER_MOVE - unit moves to position.
* ORDER_LOOK_AT - unit looks at position.
* ORDER_ESCAPE_TO - unit runs toward position and ignore enemies.
* ORDER_ESCAPE_TO_UNIT - unit runs toward other unit and ignore enemies.
* ORDER_GOTO_INN - unit goes to inn.
* ORDER_GUARD - unit stays close to another unit and remove dont_attack flag when target is attacked.
* ORDER_AUTO_TALK - ai will talk with nearest player or leader.

Game system types
-------------------------------------------------------------------------------------------------------------
Loaded from file at startup, most of them are readonly.

### Ability type
Abilities or spells used by units.

Static methods:

* Ability@ Get(const string& in id) - return ability by id.

### Dialog type
Dialogs between units.

Static properties:

* int var - global dialog context var, can be used inside dialog in "if var == ?"

Static methods:

* Dialog@ Get(const string& in id) - return dialog with id.

### Building type
Building like "inn", "village_house" etc.

### BuildingGroup type
Group of buildings: house, inn, hall etc.

Static methods:

* BuildingGroup@ Get(const string& in id) - return building group with id.

### Item type
Item template, can be dynamicaly created for quest items.

Properties:

* string name - can be changed, mostly changed for quest items.
* int value - readonly

Static methods:

* Item@ Get(const string& in id) - return item with id.
* Item@ GetRandom(int max_value) - returns random item with `value <= max_value`, can return quest item.

### ItemList type
List of items (swords, light armors).

Methods:

* Item@ Get() - return random item from list.
* Item@ Get(int index) - return item from list by index.
* int Size() - return size of list.
* 
Static methods:

* ItemList@ Get(const string& in id) - return item list with id.

### UnitData type
Unit template.

Static methods:

* UnitData@ Get(const string& in id) - return unit data with id.

### UnitGroup type
Group of template units. Used for spawn groups inside locations/encounters.

Static properties:

* UnitGroup@ empty - represents empty group.

Static methods:

* UnitGroup@ Get(const string& in id) - return unit group with id.

Game entities
-------------------------------------------------------------------------------------------------------------
### CityBuilding type
Buildings inside city.

Properties:

* Vec3 unit_pos - readonly, unit spawn pos.
* float unit_rot - readonly, unit spawn rot.

### Encounter type
Special encounter on world map.

Properties:

* Vec2 pos
* bool dont_attack - spawned units have dont_attack set.
* Quest@ quest
* Dialog@ dialog
* int st
* int chance
* UnitGroup@ group
* string text

### Event type
Used in quests for event handling.

Properties:

* EVENT event
* Location@ location - used for: EVENT_CLEARED, EVENT_ENTER, EVENT_GENERATE.
* Unit@ unit - used for: EVENT_DIE, EVENT_PICKUP, EVENT_UPDATE.
* GroundItem@ item - used for EVENT_PICKUP.
* MapSettings@ map_settings - used for EVENT_GENERATE.
* int stage - used for EVENT_GENERATE, stage 0 is before generating (can use map_settings), stage 1 is after.
* bool cancel - set to true to cancel default handling of this event.

### GroundItem type
Item on ground that can be picked up.

Properties:

* Vec3 pos - readonly
* Item@ base - readonly

### Hero type
Hero is unit that can be recruited to team.

Properties:

* bool lost_pvp - true if hero recently lost pvp.

### LevelArea type
Part of level - dungeon level, outside, inside of building.

### Location type
Location on world map. Currently locations can be added dynamicaly but not removed. Only camps support disappearing after some time.

Properties:

* Vec2 pos - readonly
* string name
* LOCATION type - readonly
* LOCATION_IMAGE image
* int st
* bool reset - when true reset locations (respawn units/loot) when team enters
* bool visited - readonly
* Quest@ active_quest - quest assigned to location, prevent other quests from using this location.
* UnitGroup@ group - unit group that spawns in this location.
* LevelArea@ area - readonly, main area of location (for dungeons this is first level).

Method:

* bool IsCity() - true if location is city.
* bool IsVillage() - true if location is village.
* void SetKnown() - mark location as known.
* void AddEventHandler(Quest@, EVENT) - add event to location.
* void RemoveEventHandler(Quest@, EVENT = EVENT_ANY) - remove event handler from location.
* Unit@ GetMayor() - return mayor/soltys or null when not in city.
* Unit@ GetCaptain() - return guard captain or null when not in city.

### MapSettings type
Used when generating dungeon.

Properties:

* STAIRS_LOCATION stairs_up_loc
* STAIRS_LOCATION stairs_down_loc

### Object type
Object inside level - barrel, chair etc.

### Player type
Player instance.

Properties:

* Unit@ unit - unit player.
* string name - readonly

Methods:

* bool IsLeader() - return true if player is leader.
* bool HaveAbility(Ability@) - return true if player have this ability.
* bool HavePerk(const string& in perk) - return true if player have this perk.
* bool AddAbility(Ability@) - add ability, return true if not already added.
* bool RemoveAbility(Ability@) - remove ability, return true if removed.

Static properties:

* Player@ pc - current player.

### Room type
Room inside dungeon.

Properties:

* ROOM_TARGET target
* Vec3 center - room center (y = 0).

### Quest type
Instance of quest.

Properties:

* QUEST_STATE state - readonly

Methods:

* void AddEntry(const string& in str) - add journal message.
* void SetStarted(const string& in title) - mark quest as started, add journal entry with quest title, can only be called once.
* void SetCompleted() - mark quest as completed, can only be called when quest is started.
* void SetFailed() - mark quest as failed, can only be called when quest is started.
* void SetTimeout(int days) - register quest timeout, can only be called once (removed when quest is completed or failed).
* Dialog@ GetDialog(const string& in id) - return quest dialog with this id.
* void AddRumor(const string& in str) - add quest rumor to available dialogs.
* void RemoveRumor() - remove quest rumor from available dialogs.

Static properties:

* Quest@ quest - active quest instance.

Static methods:

* Quest@ Find(const string& in id) - return quest with id (only works for unique quests).

### Spawn type
Contains information about unit to spawn (template, level).

### SpawnGroup type
Used for spawning random group of units when you want to do something with each unit.

Example:

	SpawnGroup group;
	group.Fill(UnitGroup::Get("bandits"), 2, 8);
	for(uint i=0; i<group.count; ++i)
	{
		Unit@ unit = Level::SpawnUnit(mayor.area, group.Get(i));
		unit.dont_attack = true;
	}

Properties:

* uint count - readonly, rolled units count.

Methods:

* void Fill(UnitGroup@ group, uint count, uint level) - roll units from random list.
* Spawn Get(uint index) - get rolled unit by index.

### Unit type
Unit inside level, based on UnitData template.

Properties:

* Vec3 pos - readonly
* Player@ player
* Hero@ hero
* int gold - modifying will show message.
* Vars@ vars - readonly
* string name - can only be set for heroes (UnitData have hero flag) at startup.
* string real_name - readonly, return real name for heroes that don't have known_name.
* bool dont_attack - enemy ai don't attack.
* bool known_name - player known name, can't be changed from true to false.
* UNIT_ORDER order - readonly, current unit order.
* LevelArea@ area - level area unit is in.

Methods:

* bool IsTeamMember() - true if unit is team member.
* bool IsFollowing(Unit@) - true if following unit.
* bool IsEnemy(Unit@) - true if unit is enemy.
* float GetHpp() - get health percentage 0..1.
* void AddItem(Item@ item, uint count = 1) - add item, will show message.
* void AddTeamItem(Item@ item, uint count = 1) - add team item, will show message.
* uint RemoveItem(const string& in item_id, uint count = 1) - remove item by id, will show message. For count 0 remove all, return removed count.
* uint RemoveItem(Item@ item, uint count = 1) - like above but use item handle.
* void RemoveQuestItem(Quest@) - remove 1 quest item from unit inventory.
* void ConsumeItem(Item@) - unit consume item (food or drink) if not busy.
* void AddDialog(Quest@, const string& in dialog_id, int priority = 0) - add quest dialog to unit.
* void RemoveDialog(Quest@) - remove quest dialog from unit.
* void AddEventHandler(Quest@, EVENT) - add event to unit.
* void RemoveEventHandler(Quest@, EVENT = EVENT_ANY) - remove event from unit.
* void OrderClear() - remove all unit orders.
* void OrderNext() - end current order and start next one.
* void OrderAttack() - orders unit to attack (crazies in this level will attack team, remove dont_attack).
* UnitOrderBuilder@ OrderWander() - order unit to wander.
* UnitOrderBuilder@ OrderWait() - order unit to wait.
* UnitOrderBuilder@ OrderFollow(Unit@) - order unit to follow target unit.
* UnitOrderBuilder@ OrderLeave() - order unit to leave current location.
* UnitOrderBuilder@ OrderMove(const Vec3& in pos) - order unit to move to position.
* UnitOrderBuilder@ OrderLookAt(const Vec3& in pos) - order unit to look at position.
* UnitOrderBuilder@ OrderEscapeTo(const Vec3& in pos) - order unit to escape to position (will ignore enemies).
* UnitOrderBuilder@ OrderEscapeToUnit(Unit@) - order unit to escape to unit (will ignore enemies).
* UnitOrderBuilder@ OrderGoToInn() - order unit to go to inn.
* UnitOrderBuilder@ OrderGuard(Unit@) - order unit to guard other unit and stay close, when attacked will defend target.
* UnitOrderBuilder@ OrderAutoTalk(bool leader = false, Dialog@=null, Quest@=null) - start dialog when close to player or leader, can use default dialog or selected.
* void Talk(const string& in text, int anim = -1) - unit talks text, animation (-1 random, 0 none, 1 what, 2 points).
* void RotateTo(const Vec3& in pos) - instantly rotates units too look at pos.
* void RotateTo(float rot) - instantly rotates units.
* void ChangeBase(UnitData@ data, bool update_items = false) - change unit base data.
* void MoveToArea(LevelArea@ area, const Vec3& in pos) - move unit to area, works between locations.
* void Kill() - used to spawn dead units.

Static properties:

* Unit@ target - global target unit, when running from console this is target before player, in dialog this is talking npc.

Static methods:

* Unit@ Id(int id) - return unit with id.

### UnitOrderBuilder type
Helper class used to set multiple unit orders one after another (for example: `unit.OrderWait().WithTimer(5).ThenFollow(player).WithTimer(5).ThenAutoTalk();`)

Methods:

* UnitOrderBuilder@ WithTimer(float timer) - set order timer.
* UnitOrderBuilder@ WithMoveType(MOVE_TYPE) - set move type for last order (currently only used for move order).
* UnitOrderBuilder@ WithRange(float range) - set range for last order (currently only used for move order).
* UnitOrderBuilder@ ThenWander()
* UnitOrderBuilder@ ThenWait()
* UnitOrderBuilder@ ThenFollow(Unit@)
* UnitOrderBuilder@ ThenLeave()
* UnitOrderBuilder@ ThenMove(const Vec3& in)
* UnitOrderBuilder@ ThenLookAt(const Vec3& in)
* UnitOrderBuilder@ ThenEscapeTo(const Vec3& in)
* UnitOrderBuilder@ ThenEscapeToUnit(Unit@)
* UnitOrderBuilder@ ThenGoToInn()
* UnitOrderBuilder@ ThenGuard(Unit@)
* UnitOrderBuilder@ ThenAutoTalk(bool=false, Dialog@=null, Quest@=null)

Game components
-------------------------------------------------------------------------------------------------------------
### Cutscene component
Used to display cutscenes - currently text & images only.

Static methods:

* void Start(bool instant = true) - start cutscene sequence.
* void Image(const string& in image) - queue image to show.
* void Text(const string& in text) - queue text to show.
* void End() - end cutscene sequence and suspend script until cutscene ended.

### Level component
Component used for current level manipulations.

Static properties:

* Location@ location - readonly
* int dungeon_level - readonly, starts from 0.

Static methods:

* bool IsSettlement() - true when inside city/village.
* bool IsCity() - true when inside city.
* bool IsVillage() - true when inside village.
* bool IsTutorial() - true when inside tutorial.
* bool IsSafe() - true when current location is safe.
* Unit@ FindUnit(UnitData@) - finds unit with this unit data.
* Unit@ GetNearestEnemy(Unit@) - finds nearest unit that is enemy of this unit.
* GroundItem@ FindItem(Item@) - finds first item.
* GroundItem@ FindNearestItem(Item@, const Vec3& in pos) - finds nearest item.
* GroundItem@ SpawnItem(Item@, const Vec3& in pos) - spawn item at position.
* GroundItem@ SpawnItem(Item@, Object@) - spawn item on object (require "spawn_pos" mesh attachment point - example object "book_holder").
* void SpawnItemRandomly(Item@, uint count = 1) - spawns item inside level in random locations.
* Vec3 FindSpawnPos(Room@, Unit@) - return position for unit spawn/warp.
* Unit@ SpawnUnitNearLocation(UnitData@, const Vec3& in pos, float range = 2) - spawns unit near position.
* Unit@ SpawnUnit(LevelArea@, Spawn) - spawns unit inside area.
* Unit@ SpawnUnit(Room@, UnitData@) - spawn unit inside room.
* Unit@ GetMayor() - returns city mayor or village soltys or null.
* CityBuilding@ GetRandomBuilding(BuildingGroup@ group) - return random building with selected group.
* Room@ GetRoom(ROOM_TARGET) - get room with selected target.
* Object@ FindObject(Room@, const string& in obj_id) - return first object inside room or null.
* array<Room@>@ FindPath(Room@ from, Room@ to) - find path from room to room.
* array<Unit@>@ GetUnits(Room@) - return all units inside room.

### StockScript component
Used in stock script - items to sell by shopkeepers.

Static methods:

* void AddItem(Item@, uint count = 1) - add item to stock.
* void AddRandomItem(ITEM_TYPE type, int price_limit, int flags, int count = 1) - add random items to stock.

### Team component
Component used for player team.

Static properties:

* Unit@ leader - readonly
* uint size - readonly, active members count.
* uint max_size - readonly, max active members count (currently 8).
* bool bandit - if true team will be attacked by citizens.

Static methods:

* bool HaveMember() - true if team have any more then 1 player or any npc.
* bool HavePcMember() - true if team have more then 1 player.
* bool HaveNpcMember() - true if team have any npc members.
* bool HaveItem(Item@) - true if anyone in team have this item.
* void AddGold(uint gold) - add gold divided between team members, shows message.
* void AddExp(int exp) - add experience to team, nagative value is unmodified, otherwise it depends on team size (1 player-200%, 2-150%, 3-125%, 4-100%, 5-75%, 6-60%, 7-50%, 8-40%).
* void AddReward(uint gold, uint exp = 0) - add gold and experience divided between team members.
* uint RemoveItem(Item@, uint count = 1) - remove items from team (count 0 = all).
* void AddMember(Unit@, int type = 0) - add team member, mode: 0-normal, 1-free (no gold), 2-visitor (no gold/exp).
* void RemoveMember(Unit@) - remove team member.
* void Warp(const Vec3& in pos, const Vec3& in look_target) - warp team to position rotated towards look target.

### World component
Component used for world.

Static properties:

* Vec2 size - readonly, get worldmap size (in km).
* Vec2 pos - readonly, position on worldmap.
* int worldtime - readonly, number of days since start of game.

Static methods:

* uint GetSettlements() - return count of settlements.
* Location@ GetLocation(uint index) - return location by index.
* string GetDirName(const Vec2& in pos1, const Vec2& in pos2) - get direction name string from pos1 to pos2.
* float GetTravelDays(float distance) - convert world distance to days of travel required.
* bool FindPlace(Vec2& inout pos, float range, bool allow_exact = false) - find place for location inside range.
* bool FindPlace(Vec2& inout pos, float min_range, float max_range) - find place for location inside range.
* Vec2 GetRandomPlace() - get random pos for location.
* Location@ GetRandomCity() - returns random city (not village).
* Location@ GetRandomSettlementWithBuilding(const string& in building_id) - returns random settlement that have this building.
* Location@ GetRandomSettlement(Location@) - returns random settlement that is not passed to function.
* Location@ GetRandomSettlement(GetLocationCallback@) - returns random settlement using callback that returns weight.
* Location@ GetClosestLocation(LOCATION type, const Vec2& in pos, int target = -1) - get closest location of this type (doesn't return quest locations).
* Location@ CreateLocation(LOCATION type, const Vec2& in pos, int target = 0, int dungeon_levels = -1) - create new location at position.
* Encounter@ AddEncounter(Quest@) - add new encounter attached to this quest.
* Encounter@ RecreateEncounter(Quest@, int) - recreate encounter, used for compatibility with old hardcoded quests.
* void RemoveEncounter(Quest@) - remove encounters attached to this quest.
* void SetStartLocation(Location@) - game start location, must be set in quest Startup.
* void AddNews(const string& in) - add news.
* Unit@ CreateUnit(UnitData@, int level = -1) - create offscreen unit.
