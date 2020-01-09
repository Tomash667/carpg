#pragma once

//-----------------------------------------------------------------------------
// Game packet identifiers sent by SLikeNet
enum GamePacket : byte
{
	/* Welcome message
	int - version
	uint[] - system crc
	string1 - player name
	*/
	ID_HELLO = ID_USER_PACKET_ENUM + 1,

	/* Lobby update
	byte - changes
	{
		byte - change type (LobbyUpdate enum)
		if(type == Lobby_UpdatePlayer)
		{
			byte - id
			bool - is ready
			byte - class
		}
		if(type == Lobby_AddPlayer)
		{
			byte - id
			string1 - name
		}
		if(type == Lobby_RemovePlayer || type == Lobby_KickPlayer)
			byte - id
		if(type == Lobby_ChangeCount)
			byte - players count
		if(type == Lobby_ChangeLeader)
			byte - leader id
	}
	*/
	ID_LOBBY_UPDATE,

	/* Server refused player to join
	byte - reason (JoinResult enum)
	... - extra data
	*/
	ID_CANT_JOIN,

	/* Player joined lobby
	byte - player id
	byte - players count
	byte - active player
	byte - leader id
	[for each active player]
	{
		byte - id
		bool - is ready
		string1 - class
		string1 - nick
	}
	*/
	ID_JOIN,

	/* Player changed readyness
	byte - id
	bool - ready
	*/
	ID_CHANGE_READY,

	/* Message from client to everyone
	byte - player id
	string1 - text
	*/
	ID_SAY,

	/* Private message
	byte - player id (send to when sent, sent from when receiving)
	string1 - text
	*/
	ID_WHISPER,

	/* Message from server to everyone
	string1 - text
	*/
	ID_SERVER_SAY,

	/* Player leaving server
	*/
	ID_LEAVE,

	/* Closing server
	byte(ServerCloseReason) - reason
	*/
	ID_SERVER_CLOSE,

	/* Send to server when changed class
	CharacterData
	bool - ready
	------------------------------------
	Response:
	byte - id
	bool - ok
	*/
	ID_PICK_CHARACTER,

	/* Server startup timer
	byte - left seconds
	*/
	ID_TIMER,

	/* Server startup canceled
	*/
	ID_END_TIMER,

	/* Server starting
	bool - start on worldmap
	*/
	ID_STARTUP,

	/* Info about current server task
	byte - id
	byte - state (0-generating world, 1-sending world, 2-waiting for players)
	*/
	ID_STATE,

	/* World data sent to all players
	WriteWorldData
	ReadWorldData
	Client repond with ID_READY
	*/
	ID_WORLD_DATA,

	/* One time send data per player
	WritePlayerStartData
	ReadPlayerStartData
	Client repond with ID_READY
	*/
	ID_PLAYER_START_DATA,

	/* Sent by clients after loading data to inform server
	byte - action id
	*/
	ID_READY,

	/* Info about changing location level
	byte - location id
	byte - dungeon level
	bool - is encounter location
	Client send ack response
	*/
	ID_CHANGE_LEVEL,

	/* Level data sent to client
	byte - id
	WriteLevelData
	ReadLevelData
	*/
	ID_LEVEL_DATA,

	/* Player data sent per player
	WritePlayerData
	ReadPlayerData
	*/
	ID_PLAYER_DATA,

	/* Sent to all players when everyone is loaded to start level
	*/
	ID_START,

	/* Player controls sent to server
	...
	*/
	ID_CONTROL,

	/* Game changes sent to all players
	...
	*/
	ID_CHANGES,

	/* Game changes specific to single player
	...
	*/
	ID_PLAYER_CHANGES,

	// Connected to master server, send info about this server
	ID_MASTER_HOST,

	// Update info about this server on master server
	ID_MASTER_UPDATE,

	// Game is quickloaded
	// bool - load on worldmap
	ID_LOADING,

	// Sent by clients when loading takes some time
	ID_PROGRESS
};

//-----------------------------------------------------------------------------
enum ServerCloseReason
{
	ServerClose_Closing,
	ServerClose_Kicked
};
