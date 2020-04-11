#pragma once

// gra
const int FRAMES = 30; // iloœæ aktualizacji na sekundê w multiplayer
const float TICK = 1.f / FRAMES;
constexpr int MAX_ABILITIES = 3;

const uint MIN_PLAYERS = 1; // min players in create server
const uint MAX_PLAYERS_WARNING = 8; // show warning when creating server with >8 players
const uint MAX_PLAYERS = 64; // max players in create server
const int DEFAULT_PLAYERS = 6; // domyœlna liczba slotów
const int PORT = 37557; // port gry

// server flags
enum ServerFlags
{
	SERVER_PASSWORD = 0x01,
	SERVER_SAVED = 0x02
};

// czas oczekiwania
const float T_WAIT_FOR_DISCONNECT = 1.f;
const float T_CONNECT_PING = 1.f; // odstêp pomiêdzy pingowaniem serwera
const int I_CONNECT_TRIES = 5; // liczba prób po³¹czenia (ca³kowity czas ³¹czenia z serwerem = T_CONNECT_PING*T_CONNECT_TRIES)
const float T_CONNECT = 5.f; // czas na po³¹czenie do serwera

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
