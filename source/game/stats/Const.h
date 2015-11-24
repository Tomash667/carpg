#pragma once

// gra
const int FRAMES = 30; // iloœæ aktualizacji na sekundê w multiplayer
const float TICK = 1.f/FRAMES;

const int MIN_PLAYERS = 1; // minimum slotów na graczy na serwerze
const int MAX_PLAYERS = 8; // max slotów na graczy na serweerze
const int DEFAULT_PLAYERS = 6; // domyœlna liczba slotów
const int MAX_TEAM_SIZE = 8;
const int PORT = 37557; // port gry

// server flags
#define SERVER_PASSWORD 0x01
#define SERVER_SAVED 0x02

// czas oczekiwania
const float T_WAIT_FOR_DISCONNECT = 1.f;
const float T_CONNECT_PING = 1.f; // odstêp pomiêdzy pingowaniem serwera
const int I_CONNECT_TRIES = 5; // liczba prób po³¹czenia (ca³kowity czas ³¹czenia z serwerem = T_CONNECT_PING*T_CONNECT_TRIES)
const float T_CONNECT = 5.f; // czas na po³¹czenie do serwera
const int I_SHUTDOWN = 1000;

// czas odliczania do uruchomienia gry
#ifdef _DEBUG
const int STARTUP_TIMER = 1;
#else
const int STARTUP_TIMER = 3;
#endif
