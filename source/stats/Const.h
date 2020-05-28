#pragma once

const int FRAMES = 30; // multiplayer update frames
const float TICK = 1.f / FRAMES;
constexpr int MAX_ABILITIES = 3;

const uint MIN_PLAYERS = 1; // min players in create server
const uint MAX_PLAYERS_WARNING = 8; // show warning when creating server with >8 players
const uint MAX_PLAYERS = 64; // max players in create server
const int DEFAULT_PLAYERS = 6; // default number of players when creating server
const int PORT = 37557; // default server port

// server flags
enum ServerFlags
{
	SERVER_PASSWORD = 0x01,
	SERVER_SAVED = 0x02
};

// waiting time
const float T_WAIT_FOR_DISCONNECT = 1.f;
const float T_CONNECT_PING = 1.f; // delay between server pinging
const int I_CONNECT_TRIES = 5; // no of tries to connect to server
const float T_CONNECT = 5.f; // wait time when trying to connect to server

const int MAX_LEVEL = 25;

extern const float ALERT_RANGE;
extern const float ALERT_SPAWN_RANGE;
extern const float PICKUP_RANGE;
extern const float ARROW_TIMER;
extern const float MIN_H;

extern const float HIT_SOUND_DIST;
extern const float ARROW_HIT_SOUND_DIST;
extern const float SHOOT_SOUND_DIST;
extern const float SPAWN_SOUND_DIST;
extern const float MAGIC_SCROLL_SOUND_DIST;
