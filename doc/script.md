Core library
-------------------------------------------------------------------------------------------------------------
### Global functions
* void Info(const string& in msg) - show info msg in console.
* void DevInfo(const string& in msg) - like info but only in dev mode.
* void Warn(const string& in msg) - show warning msg in console.
* void Error(const string& in msg) - show error msg in console.
* string Str(?& in) - convert value to string.
* string Format(const string& in formatStr, ?& in ...) - return formatted string, takes 0 to 8 any arguments.
* string Upper1(const string& in) - return string with first letter upper case.
* int Random(int a, int b) - returns random number in range <a,b>.
* int Rand() - returns random int number.
* void Sleep(float seconds) - resume script execution after some time, don't use in places that require return value instantly like callbacks or dialogIf. Console output don't work after sleep!

### Core types
* Int2 - 2d int point x, y.
* Vec2 - 2d vector x, y. Static methods:
	* float Distance(const Vec2& in v1, const Vec2& in v2);
* Vec3 - 3d vector x, y, z. Static methods:
	* float Distance(const Vec3& in v1, const Vec3& in v2);
* Vec4 - 4d vector x, y, z, w.
* Box2d - 2d area with two Vec2 v1 and v2. Methods:
	* float SizeX() const - return x size.
	* float SizeY() const - return y size.
* string
	* string Upper() const - return string with first letter uppercase.

### SpawnPoint type
Contain position and rotation.

Members:

* Vec3 pos
* float rot

### Var & Vars type
Used to store types in units/globals or pass between quests.

Example:

	Unit@ unit = ...;
	unit.vars["questVar"] = 42;
	int value = unit.vars["questVar"];

Static properties:

* Vars@ globals -  global variables.

Game enums & consts
-------------------------------------------------------------------------------------------------------------
### Funcdefs:
* float GetLocationCallback(Location@)

### Enum ENTRY_LOCATION
* ENTRY_NONE
* ENTRY_RANDOM
* ENTRY_FAR_FROM_ROOM
* ENTRY_BORDER
* ENTRY_FAR_FROM_PREV

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
* EVENT_USE - for item event handler, currently only works for books.

### Enum flags ITEM_FLAGS
* ITEM_NOT_SHOP - not generated in shop.
* ITEM_NOT_MERCHANT - not generated for merchant.
* ITEM_NOT_BLACKSMITH - not generated for blacksmith.

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
* LI_HUNTERS_CAMP
* LI_HILLS
* LI_VILLAGE_DESTROYED

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
* HUNTERS_CAMP
* HILLS
* VILLAGE_EMPTY

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
* ROOM_ENTRY_PREV
* ROOM_ENTRY_NEXT
* ROOM_TREASURY
* ROOM_PORTAL
* ROOM_PRISON
* ROOM_THRONE

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
* ORDER_GUARD - unit stays close to another unit and remove dontAttack flag when target is attacked.
* ORDER_AUTO_TALK - ai will talk with nearest player or leader.

Game system types
-------------------------------------------------------------------------------------------------------------
Loaded from file at startup, most of them are readonly.

### Ability type
Abilities or spells used by units.

Static methods:

* Ability@ Get(const string& in id) - return ability by id.

### BaseObject type
Object data like barrel, altar.

Static methods:

BaseObject@ Get(const string& in) - return object data by id.

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

Methods:

* Item* QuestCopy(Quest*) - create quest item from item.
* Item* QuestCopy(Quest*, const string& in name) - create quest item with name.

Static methods:

* Item@ Get(const string& in id) - return item with id.
* Item@ GetRandom(int maxValue) - returns random item with `value <= max_value`, can return quest item.

### ItemList type
List of items (swords, light armors).

Methods:

* Item@ Get() - return random item from list.
* Item@ Get(int index) - return item from list by index.
* int Size() - return size of list.
* 
Static methods:

* ItemList@ Get(const string& in id) - return item list with id.
* Item@ GetRandomItem(const string& in id) - return random item from list.

### Recipe type
Information how to craft items, learned by player.

Static methods:

* Recipe@ Get(const string& in id) - return recipe with id.

### UnitData type
Unit template.

Members:

* const string id

Static methods:

* UnitData@ Get(const string& in id) - return unit data with id.

### UnitGroup type
Group of template units. Used for spawn groups inside locations/encounters.

Members:

* const string name
* const bool female - used in polish language to determine correct spelling of some gender specific words.

Methods:

* UnitData@ GetLeader(int level) - get group leader that is closest to selected level.

Static properties:

* UnitGroup@ empty - represents empty group.

Static methods:

* UnitGroup@ Get(const string& in id) - return unit group with id.

Game entities
-------------------------------------------------------------------------------------------------------------
### Chest type
Chest object with items inside.

Methods:

bool AddItem(Item@, uint count = 1) - add item to chest, return true if stacked.

### CityBuilding type
Buildings inside city.

Properties:

* Vec3 unitPos - readonly, unit spawn pos.
* float unitRot - readonly, unit spawn rot.

### Encounter type
Special encounter on world map.

Properties:

* Vec2 pos
* bool dontAttack - spawned units have dontAttack set.
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
* Unit@ unit - used for: EVENT_DIE, EVENT_PICKUP, EVENT_UPDATE, EVENT_USE.
* GroundItem@ groundItem - used for EVENT_PICKUP.
* Item@ item - used for EVENT_USE.
* MapSettings@ mapSettings - used for EVENT_GENERATE.
* int stage - used for EVENT_GENERATE, stage 0 is before generating (can use mapSettings), stage 1 is after.
* bool cancel - set to true to cancel default handling of this event.

### GroundItem type
Item on ground that can be picked up.

Properties:

* Vec3 pos - readonly
* Item@ base - readonly

### Hero type
Hero is unit that can be recruited to team.

Properties:

* bool lostPvp - true if hero recently lost pvp.
* bool otherTeam - hero is in other team.
* bool wantJoin - true when join team without persuading.
* int persuasionCheck - skill level required to persuade to join.
* const int investment - value of investment (gold from quests over time).

### Location type
Location on world map. Currently locations can be added dynamicaly but not removed. Only camps support disappearing after some time.

Properties:

* Vec2 pos - readonly
* string name
* LOCATION type - readonly
* bool outside - readonly
* LOCATION_IMAGE image
* int st
* int levels - readonly, return count of location parts (for dungeon this is dungeon levels, for city enterable buildings + 1 for outside part)
* bool reset - when true reset locations (respawn units/loot) when team enters
* bool visited - readonly
* Quest@ activeQuest - quest assigned to location, prevent other quests from using this location.
* UnitGroup@ group - unit group that spawns in this location.
* LocationPart@ locPart - readonly, main part of location (for dungeons this is first level).

Method:

* bool IsCity() - true if location is city.
* bool IsVillage() - true if location is village.
* void SetKnown() - mark location as known.
* void AddEventHandler(Quest@, EVENT) - add event to location.
* void RemoveEventHandler(Quest@, EVENT = EVENT_ANY) - remove event handler from location.
* Unit@ GetMayor() - return mayor/soltys or null when not in city.
* Unit@ GetCaptain() - return guard captain or null when not in city.
* LocationPart@ GetLocationPart(int index) - get location part by index.
* LocationPart@ GetBuildingLocationPart(const string& id) - get inside building location part (by building group id).
* int GetRandomLevel() - return random dungeon level (higher chance for lower levels) or -1 when outside location.
* Unit@ FindQuestUnit(Quest@) - find unit with quest set.

### LocationPart type
Part of location - dungeon level, outside, inside of building.

Methods:

* bool RemoveItemFromChest(Item@) - return true if removed item.
* bool RemoveItemFromUnit(Item@) - remove single item from alive enemy, return true if removed.
* bool RemoveGroundItem(Item@) - remove single item from ground, return true if removed.

### MapSettings type
Used when generating dungeon.

Properties:

* ENTRY_LOCATION prevEntryLoc
* ENTRY_LOCATION nextEntryLoc

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
* bool AddRecipe(Recipe@) - add recipe, return true if not already added.

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
* int progress
* int timeout - readonly, days until timeout

Methods:

* void AddEntry(const string& in str) - add journal message.
* void SetStarted(const string& in title) - mark quest as started, add journal entry with quest title, can only be called once.
* void SetCompleted() - mark quest as completed, can only be called when quest is started.
* void SetFailed() - mark quest as failed, can only be called when quest is started.
* void SetTimeout(int days) - register quest timeout, can only be called once (removed when quest is completed or failed).
* Dialog@ GetDialog(const string& in id) - return quest dialog with this id.
* void AddRumor(const string& in str) - add quest rumor to available dialogs.
* void RemoveRumor() - remove quest rumor from available dialogs.
* void Start(Vars@) - start quest.

Static properties:

* Quest@ quest - active quest instance.

Static methods:

* Quest@ Find(const string& in id) - return quest with id (only works for unique quests).
* int CalculateReward(int st, const Int2& in stRange, const Int2& in reward) - calculate reward from value range.
* void AddItemEventHandler(Quest@, Item@) - add event handler for player using item.
* void RemoveItemEventHandler(Quest@, Item@) - remove event handler for player using item.

### Spawn type
Contains information about unit to spawn (template, level).

### SpawnGroup type
Used for spawning random group of units when you want to do something with each unit.

Example:

	SpawnGroup group;
	group.Fill(UnitGroup::Get("bandits"), 2, 8);
	for(uint i=0; i<group.count; ++i)
	{
		Unit@ unit = Level::SpawnUnit(mayor.locPart, group.Get(i));
		unit.dontAttack = true;
	}

Properties:

* uint count - readonly, rolled units count.

Methods:

* void Fill(UnitGroup@ group, uint count, uint level) - roll units from random list.
* Spawn Get(uint index) - get rolled unit by index.

### Unit type
Unit inside level, based on UnitData template.

Properties:

* UnitData@ data - readonly
* Vec3 pos - readonly
* Player@ player
* Hero@ hero
* int gold - modifying will show message.
* Vars@ vars - readonly
* string name - can only be set for heroes (UnitData have hero flag) at startup.
* string realName - readonly, return real name for heroes that don't have knownName.
* bool dontAttack - enemy ai don't attack.
* bool knownName - player known name, can't be changed from true to false.
* bool temporary - unit is removed when location is repopulated.
* UNIT_ORDER order - readonly, current unit order.
* LocationPart@ locPart - location part unit is in.
* const string& clas - readonly, class identifier.

Methods:

* bool IsTeamMember() - true if unit is team member.
* bool IsFollowing(Unit@) - true if following unit.
* bool IsEnemy(Unit@) - true if unit is enemy.
* float GetHpp() - get health percentage 0..1.
* bool HaveItem(Item@ item) - check if unit have item.
* void AddItem(Item@ item, uint count = 1) - add item, will show message.
* void AddTeamItem(Item@ item, uint count = 1) - add team item, will show message.
* uint RemoveItem(const string& in itemId, uint count = 1) - remove item by id, will show message. For count 0 remove all, return removed count.
* uint RemoveItem(Item@ item, uint count = 1) - like above but use item handle.
* void RemoveQuestItem(Quest@) - remove 1 quest item from unit inventory.
* void ConsumeItem(Item@) - unit consume item (food or drink) if not busy.
* void AddDialog(Quest@, const string& in dialogId, int priority = 0) - add quest dialog to unit.
* void RemoveDialog(Quest@) - remove quest dialog from unit.
* void AddEventHandler(Quest@, EVENT) - add event to unit.
* void RemoveEventHandler(Quest@, EVENT = EVENT_ANY) - remove event from unit.
* void OrderClear() - remove all unit orders.
* void OrderNext() - end current order and start next one.
* void OrderAttack() - orders unit to attack (crazies in this level will attack team, remove dontAttack).
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
* void ChangeBase(UnitData@ data, bool updateItems = false) - change unit base data, currently update items works only for team members.
* void MoveToLocation(LocationPart@ locPart, const Vec3& in pos) - move unit to location part, works between locations.
* void MoveOffscreen() - move unit to offscreen location.
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
* int dungeonLevel - readonly, starts from 0.

Static methods:

* bool IsSettlement() - true when inside city/village.
* bool IsCity() - true when inside city.
* bool IsVillage() - true when inside village.
* bool IsTutorial() - true when inside tutorial.
* bool IsSafe() - true when current location is safe.
* bool IsOutside() - true when current location is outside type.
* Unit@ FindUnit(UnitData@) - finds unit with this unit data.
* Unit@ GetNearestEnemy(Unit@) - finds nearest unit that is enemy of this unit.
* GroundItem@ FindItem(Item@) - finds first item.
* GroundItem@ FindNearestItem(Item@, const Vec3& in pos) - finds nearest item.
* GroundItem@ SpawnItem(Item@, const Vec3& in pos) - spawn item at position.
* GroundItem@ SpawnItem(LocationPart@, Item@) - spawn item inside building.
* GroundItem@ SpawnItem(Item@, Object@) - spawn item on object (require "spawn_pos" mesh attachment point - example object "book_holder").
* GroundItem@ SpawnItemInsideAnyRoom(Item@) - spawn item in random room on floor.
* void SpawnItemRandomly(Item@, uint count = 1) - spawns item inside level in random locations.
* Vec3 FindSpawnPos(Room@, Unit@) - return position for unit spawn/warp in room.
* Vec3 FindSpawnPos(LocationPart@, Unit@) - return position for unit spawn/warp in building location part.
* Unit@ SpawnUnitNearLocation(UnitData@, const Vec3& in pos, float range = 2, int level = -1) - spawns unit near position.
* Unit@ SpawnUnit(LocationPart@, Spawn) - spawns unit inside location part.
* Unit@ SpawnUnit(Room@, UnitData@, int level = -1) - spawn unit inside room.
* Unit@ GetMayor() - returns city mayor or village soltys or null.
* CityBuilding@ GetRandomBuilding(BuildingGroup@ group) - return random building with selected group.
* Room@ GetRoom(ROOM_TARGET) - get room with selected target.
* Room@ GetFarRoom() - get random room far from entrance.
* Object@ FindObject(Room@, BaseObject@) - return first object inside room or null.
* Chest@ GetRandomChest(Room@) - get random chest in room.
* Chest@ GetTreasureChest() - get silver chest in treasure room.
* array<Room@>@ FindPath(Room@ from, Room@ to) - find path from room to room.
* array<Unit@>@ GetUnits(Room@) - return all units inside room.
* bool FindPlaceNearWall(BaseObject@, SpawnPoint& out - search for place to spawn object near wall.
* Object@ SpawnObject(BaseObject@, const Vec3& in pos, float rot) - spawn object at position.

### StockScript component
Used in stock script - items to sell by shopkeepers.

Static methods:

* void AddItem(Item@, uint count = 1) - add item to stock.
* void AddRandomItem(ITEM_TYPE type, int priceLimit, int flags, int count = 1) - add random items to stock.

### Team component
Component used for player team.

Static properties:

* Unit@ leader - readonly
* uint size - readonly, active members count.
* uint maxSize - readonly, max active members count (currently 8).
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
* void Warp(const Vec3& in pos, const Vec3& in lookTarget) - warp team to position rotated towards look target.
* bool PersuasionCheck(int level) - do persuasion check, return true is succeeded.

### World component
Component used for world.

Static properties:

* Vec2 bounds - readonly, locations spawn bounds, map size with borders.
* Vec2 size - readonly, get worldmap size (in km).
* Vec2 pos - readonly, position on worldmap.
* int worldtime - readonly, number of days since start of game.
* const Date& date - readonly, current in game date.
* const Date& startDate - can only be set at startup.

Static methods:

* Box2d GetArea() - return world area.
* uint GetSettlements() - return count of settlements.
* Location@ GetLocation(uint index) - return location by index.
* string GetDirName(const Vec2& in pos1, const Vec2& in pos2) - get direction name string from pos1 to pos2.
* string GetDirName(Location@ loc1, Location@ loc2) - get direction name string from loc1 to loc2.
* float GetTravelDays(float distance) - convert world distance to days of travel required.
* Vec2 FindPlace(const Vec2& in pos, float range, bool allowExact = false) - find place for location inside range.
* Vec2 FindPlace(const Vec2& in pos, float minRange, float maxRange) - find place for location inside range.
* Vec2 FindPlace(const Box2d& in box) - find place for location inside region.
* bool TryFindPlace(Vec2& pos, float range, bool allowExact = false) - try to find place for location inside range.
* Vec2 GetRandomPlace() - get random pos for location.
* Location@ GetRandomCity() - returns random city (not village).
* Location@ GetRandomSettlementWithBuilding(const string& in buildingId) - returns random settlement that have this building.
* Location@ GetRandomSettlement(Location@) - returns random settlement that is not passed to function.
* Location@ GetRandomSettlement(GetLocationCallback@) - returns random settlement using callback that returns weight.
* Location@ GetRandomSpawnLocation(const Vec2& in pos, UnitGroup@ group, float range = 160) - get random location with selected unit group within range, if not found will create camp.
* Location@ GetClosestLocation(LOCATION type, const Vec2& in pos, LOCATION_TARGET target = -1, int flags = 0) - get closest location of this type (doesn't return quest locations). Available flags: ```F_ALLOW_ACTIVE``` - allow active quest locations, ```F_EXCLUDED``` - exclude target instead of include in search.
* Location@ GetClosestLocation(LOCATION type, const Vec2& in pos, array<LOCATION_TARGET> targets) - get closest location of this type and specified targets (doesn't return quest locations).
* Location@ CreateLocation(LOCATION type, const Vec2& in pos, LOCATION_TARGET target = -1, int dungeonLevels = -1) - create new location at position.
* Location@ CreateCamp(const Vec2& in pos, UnitGroup@) - create new camp at position.
* void AbadonLocation(Location@) - remove all units & items from location.
* Encounter@ AddEncounter(Quest@) - add new encounter attached to this quest.
* Encounter@ RecreateEncounter(Quest@, int) - recreate encounter, used for compatibility with old hardcoded quests.
* void RemoveEncounter(Quest@) - remove encounters attached to this quest.
* void SetStartLocation(Location@) - game start location, must be set in quest Startup.
* void AddNews(const string& in) - add news.
* Unit@ CreateUnit(UnitData@, int level = -1) - create offscreen unit.
