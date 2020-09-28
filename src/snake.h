/*
	Snake moves every cycle, followed by sleep which is determined by maxSleep - speed with minSleep as a limit.
	Drops operate under a maximum and spawn in spawnTime milliseconds, locations determined by rand().
*/

#pragma once

#ifndef NPPSNAKE_SNAKE
#define NPPSNAKE_SNAKE

#include <string>
#include <time.h>

typedef struct _SnakeGame {
	// Here are the settings for the game
	char bodyToken, headToken, fruitToken;
	int maxSleep, minSleep;
	int sizeX, sizeY;
	int snakeInitLen, snakeInitSpeed;
	int maxFruit, fruitSpawnTime, fruitSpeedValue, fruitLenValue; 
	HWND hCurrSci;
	TCHAR filePath[MAX_PATH];
} SnakeGame;

typedef struct _SnakePart {
	int x, y;
} SnakePart;

enum class SnakeGameState {
	NPPSNAKE_RUNNING,
	NPPSNAKE_PAUSED,
	NPPSNAKE_STOPPED
};

typedef enum { // no class because we don't care about scope of names or implicit type conversion
	NPPSNAKE_UP,
	NPPSNAKE_DOWN,
	NPPSNAKE_LEFT,
	NPPSNAKE_RIGHT
} SnakeDirection;

typedef struct _SnakePlayer {
	// x and y are positions of the head
	int x, y, len, speed, score;
	SnakePart * body; // array of body parts
	SnakeDirection direction;
	//char name[32]; // for scoreboard
} SnakePlayer;

DWORD WINAPI snake(void *);
void restoreUserStyles();
void assureUnPause();
void gameOver();

#endif // NPPSNAKE_SNAKE