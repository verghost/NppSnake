/*
	Most of the Code for the game
*/

#include "PluginInterface.h"
#include "plDef.h"
#include "menuCmdID.h"
#include "snake.h"

extern NppData nppData;

// Config stuff
extern TCHAR iniFilePath[MAX_PATH];
extern const TCHAR sectionName[] = TEXT("Game Settings");

SnakeGame GameInfo;
SnakePlayer player;
SnakeGameState gameState = SnakeGameState::NPPSNAKE_STOPPED;
bool directionChanged;
TCHAR lastFilePath[MAX_PATH];

std::string strInit;
int numFruit, timeSinceLastFruit;

int iUserCaretStyle;
bool iUserCaretLineVisible;

const int margin = 2;

static inline char getChar(int x, int y)  {
	int pos = x + ((GameInfo.sizeX + margin) * y);
	return (char)::SendMessage(GameInfo.hCurrSci, SCI_GETCHARAT, (WPARAM)pos, NULL);
}

static inline void setChar(int x, int y, char c, bool override = false) {
	int iPosStart = x + ((GameInfo.sizeX + margin) * y);
	int iPosEnd = iPosStart + 1;
	char tmp[2]; tmp[0] = c; tmp[1] = '\0';
	::SendMessage(GameInfo.hCurrSci, SCI_SETSEL, (WPARAM)iPosStart, (LPARAM)iPosEnd);
	if (gameState == SnakeGameState::NPPSNAKE_RUNNING || override) { // are we running?
		::SendMessage(GameInfo.hCurrSci, SCI_REPLACESEL, NULL, (LPARAM)tmp);
	}
	::SendMessage(GameInfo.hCurrSci, SCI_SETSEL, (WPARAM)-1, (LPARAM)-1); // set position to end of doc
}

static inline bool drawSnake(SnakePart prevTail, bool grow /*= false*/) {
	SnakePart* tmp;
	if (grow) {
		player.len += GameInfo.fruitLenValue;
		if ((GameInfo.maxSleep - player.speed) > GameInfo.minSleep)
			player.speed += GameInfo.fruitSpeedValue;
		tmp = (SnakePart*)realloc(player.body, sizeof(SnakePart) * player.len);

		if (tmp == NULL) { // realloc failed
			return false;
		}
		player.body = tmp;
		player.body[player.len-1].x = prevTail.x;
		player.body[player.len-1].y = prevTail.y;
	} else {
		// Erase tail
		setChar(prevTail.x, prevTail.y, ' ');
	}
	setChar(player.body[0].x, player.body[0].y, GameInfo.headToken); // Draw new head
	return true;
}

static inline void spawnFruit() {
	int iFruitX, iFruitY;
	do {
		iFruitX = rand() % GameInfo.sizeX;
		iFruitY = rand() % GameInfo.sizeY;
	} while (getChar(iFruitX, iFruitY) != ' ');
	setChar(iFruitX, iFruitY, GameInfo.fruitToken);
}

void restoreUserStyles() {
	::SendMessage(GameInfo.hCurrSci, SCI_SETCARETSTYLE, (WPARAM)iUserCaretStyle, NULL);
	::SendMessage(GameInfo.hCurrSci, SCI_SETCARETLINEVISIBLE, (WPARAM)iUserCaretLineVisible, NULL);
}

void applySnakeStyles() {
	::SendMessage(GameInfo.hCurrSci, SCI_SETCARETSTYLE, (WPARAM)CARETSTYLE_INVISIBLE, NULL);
	::SendMessage(GameInfo.hCurrSci, SCI_SETCARETLINEVISIBLE, (WPARAM)false, NULL);
}

// Unpause with the assurance that no artifacts are left behind from the buggy pause.
void assureUnPause() {
	::SendMessage(GameInfo.hCurrSci, SCI_SETTEXT, 0, (LPARAM)strInit.c_str());
	setChar(player.body[0].x, player.body[0].y, GameInfo.headToken, true);
	for (int i = 1; i < player.len; i++) {
		setChar(player.body[i].x, player.body[i].y, GameInfo.bodyToken, true);
	}
	numFruit = 0, timeSinceLastFruit = 0;
	applySnakeStyles();
	gameState = SnakeGameState::NPPSNAKE_RUNNING;
}

void gameOver() {
	if (gameState == SnakeGameState::NPPSNAKE_RUNNING) { // no game over screen
		std::string endString = "Game Over!";
		std::string scoreString = "Your Score was: " + std::to_string(player.len);
		int esX = (GameInfo.sizeX / 2) - (endString.length() / 2),
			esY = GameInfo.sizeY / 2;
		if (esY < 0) esY = 1;
		for (int i = 0; i < endString.length(); i++) setChar(esX + i, esY, endString[i]);
		esX = (GameInfo.sizeX / 2) - (scoreString.length() / 2);
		for (int i = 0; i < scoreString.length(); i++) setChar(esX + i, esY + 1, scoreString[i]);
		gameState = SnakeGameState::NPPSNAKE_STOPPED;
	}
}

DWORD WINAPI snake(void*) {
	// Get the current scintilla
	int which = -1;
	::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
	if (which == -1)
		return 0;
	HWND hSci = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
	GameInfo.hCurrSci = hSci; // store scintilla handle

	// Decide whether to open a new file, or use the one we're in
	::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, MAX_PATH, (LPARAM)GameInfo.filePath);
	if (lastFilePath && (wcscmp(lastFilePath, GameInfo.filePath) == 0))
		::SendMessage(hSci, SCI_SETTEXT, (WPARAM)"", NULL);
	else 
	{
		::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_NEW);
		::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, MAX_PATH, (LPARAM)GameInfo.filePath);
		wcscpy_s(lastFilePath, MAX_PATH, GameInfo.filePath);
	}

	// Set caret and caretline to be invisible (this will be reset at the end of the game)
	iUserCaretStyle = (int)::SendMessage(hSci, SCI_GETCARETSTYLE, NULL, NULL);
	iUserCaretLineVisible = (bool)::SendMessage(hSci, SCI_GETCARETLINEVISIBLE, NULL, NULL);
	applySnakeStyles();
	
	// Get setting from config file (TODO: replace with loop)
	GameInfo.sizeX = ::GetPrivateProfileInt(sectionName, L"size_x", 50, iniFilePath);
	GameInfo.sizeY = ::GetPrivateProfileInt(sectionName, L"size_y", 25, iniFilePath);
	GameInfo.maxSleep = ::GetPrivateProfileInt(sectionName, L"max_sleep", 200, iniFilePath);
	GameInfo.minSleep = ::GetPrivateProfileInt(sectionName, L"min_sleep", 40, iniFilePath);
	GameInfo.snakeInitLen = ::GetPrivateProfileInt(sectionName, L"player_init_length", 4, iniFilePath);
	GameInfo.snakeInitSpeed = ::GetPrivateProfileInt(sectionName, L"player_init_speed", 80, iniFilePath);
	TCHAR buff[2];
	GetPrivateProfileString(sectionName, L"token_head", L"#", buff, 2, iniFilePath);
	GameInfo.headToken = (char)buff[0];
	GetPrivateProfileString(sectionName, L"token_body", L"O", buff, 2, iniFilePath);
	GameInfo.bodyToken = (char)buff[0];
	GetPrivateProfileString(sectionName, L"token_fruit", L"0", buff, 2, iniFilePath);
	GameInfo.fruitToken = (char)buff[0];
	GameInfo.maxFruit = ::GetPrivateProfileInt(sectionName, L"fruit_max", 4, iniFilePath);
	GameInfo.fruitLenValue = ::GetPrivateProfileInt(sectionName, L"fruit_length_value", 1, iniFilePath);
	GameInfo.fruitSpeedValue = ::GetPrivateProfileInt(sectionName, L"fruit_speed_value", 5, iniFilePath);
	GameInfo.fruitSpawnTime = ::GetPrivateProfileInt(sectionName, L"fruit_spawn_time_ms", 2000, iniFilePath);

	// Make init frame
	int i;
	strInit = ""; std::string strRow = "";
	for (i = 0; i < GameInfo.sizeX; i++) strRow.append(" ");
	strRow.append("|\n");
	for (i = 0; i < GameInfo.sizeY; i++) strInit.append(strRow);
	strInit.append("\nWelcome to snake v0.1 (buggy!).\nUse WASD or Arrow Keys to move and close the file to exit!\n");
	::SendMessage(hSci, SCI_SETTEXT, 0, (LPARAM)strInit.c_str());

	// Setup player
	int initX, initY;
	player.len = GameInfo.snakeInitLen;
	player.speed = GameInfo.snakeInitSpeed;
	player.direction = SnakeDirection::NPPSNAKE_RIGHT;
	player.score = 0;
	player.body = (SnakePart*)malloc(sizeof(SnakePart) * player.len);
	
	initX = player.len + 1;
	initY = GameInfo.sizeY / 2;
	for (i = 0; i < player.len; i++) {
		player.body[i].x = initX - i;
		player.body[i].y = initY;
	}

	srand(time(NULL)); // init prng
	gameState = SnakeGameState::NPPSNAKE_RUNNING;

	// Spawn snake
	setChar(player.body[0].x, player.body[0].y, GameInfo.headToken);
	for (int i = 1; i < player.len; i++) {
		setChar(player.body[i].x, player.body[i].y, GameInfo.bodyToken);
	}

	// Local game vars
	bool grow;
	SnakePart prevTail;
	numFruit = 0, timeSinceLastFruit = 0;
	while(gameState != SnakeGameState::NPPSNAKE_STOPPED) { // Game loop
		if (gameState == SnakeGameState::NPPSNAKE_PAUSED) {
			Sleep(GameInfo.maxSleep - player.speed);
			continue;
		}

		grow = false;
		directionChanged = false;
		prevTail.x = player.body[player.len-1].x;
		prevTail.y = player.body[player.len-1].y;
		
		// Update all body positions
		for (i = player.len - 1; i > 0; i--) { // body
			player.body[i].x = player.body[i - 1].x;
			player.body[i].y = player.body[i - 1].y;
		}

		// Move snake according to which direction it's headed.
		setChar(player.body[0].x, player.body[0].y, GameInfo.bodyToken);
		switch(player.direction) {
			case NPPSNAKE_UP:
				player.body[0].y -= 1;
				break;
			case NPPSNAKE_DOWN:
				player.body[0].y += 1;
				break;
			case NPPSNAKE_LEFT:
				player.body[0].x -= 1;
				break;
			case NPPSNAKE_RIGHT:
				player.body[0].x += 1;
				break;
			default:
				break;
		}
		
		if ((player.body[0].x > (GameInfo.sizeX - 1)) || (player.body[0].x < 0) ||
			(player.body[0].y > (GameInfo.sizeY - 1)) || (player.body[0].y < 0)) {
			break; // collision
		} else if (getChar(player.body[0].x, player.body[0].y) == GameInfo.bodyToken) {
			break; // wrap-around
		} else if (getChar(player.body[0].x, player.body[0].y) == GameInfo.fruitToken) {
			grow = true; // fruit
			numFruit--;
		}

		if (!drawSnake(prevTail, grow)) break;
		
		// spawn more fruit or increment timeSinceSpawn
		if(numFruit < GameInfo.maxFruit) {
			if(timeSinceLastFruit <= 0) { // spawn new fruit
				spawnFruit();
				numFruit++;
				timeSinceLastFruit = GameInfo.fruitSpawnTime;
			} else {
				timeSinceLastFruit -= (GameInfo.maxSleep - player.speed);
			}
		}
		Sleep(GameInfo.maxSleep - player.speed);
	}
	gameOver();

	// Reset scintilla styles and clean up!
	restoreUserStyles();
	free(player.body);

	return 0;
}