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

const float ALERT_RANGE = 20.f;
const float ALERT_RANGE_ATTACK = 5.f;
const float ALERT_SPAWN_RANGE = 25.f;
const float AGGRO_TIMER = 5.f;
const float PICKUP_RANGE = 2.f;
const float ARROW_TIMER = 5.f;
const float TRAP_ARROW_SPEED = 45.f;
const float MIN_H = 1.5f;
const float TRAIN_KILL_RATIO = 0.1f;

const float HIT_SOUND_DIST = 1.5f;
const float ARROW_HIT_SOUND_DIST = 1.5f;
const float SHOOT_SOUND_DIST = 1.f;
const float SPAWN_SOUND_DIST = 1.5f;
const float MAGIC_SCROLL_SOUND_DIST = 1.5f;
const float AGGRO_SOUND_DIST = 1.f;
