/*
    Plugin definition
*/

#include "plDef.h"
#include "menuCmdID.h"
#include <shlwapi.h>
#include <fileapi.h>
#include "snake.h"

FuncItem funcItem[nbFunc]; // The plugin data that Notepad++ uses
NppData nppData; // The data of Notepad++

const TCHAR configFileName[] = TEXT("NppSnake.ini");
const TCHAR sectionName[] = TEXT("Game Settings");
TCHAR iniFilePath[MAX_PATH];
const TCHAR * configDefaults[] = {
            L"size_x", L"50",
            L"size_y", L"25",
            L"max_sleep", L"200",
            L"min_sleep", L"20",
            L"token_head", L"#",
            L"token_body", L"O",
            L"token_fruit", L"0",
            L"player_init_length", L"4",
            L"player_init_speed", L"80",
            L"fruit_max", L"4",
            L"fruit_spawn_time_ms", L"2000",
            L"fruit_length_value", L"1",
            L"fruit_speed_value", L"5"
};
const int numSettings = 13; // update this when adding more settings!

extern SnakeGame GameInfo;
extern SnakeGameState gameState;

// Called while the plugin is loading
void pluginInit(HANDLE /*hModule*/)
{}

// Do clean up, save the parameters (if any) for the next session
void pluginCleanUp()
{}

// Make sure we have a valid config file available.
void assureIni(bool init) {
    if (init) {
        ::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)iniFilePath);
        if (PathFileExists(iniFilePath) == FALSE)
            ::CreateDirectory(iniFilePath, NULL);
        PathAppend(iniFilePath, configFileName);
    }
    HANDLE f = CreateFile(iniFilePath, GENERIC_ALL, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
    bool fnf = GetLastError() == ERROR_FILE_NOT_FOUND;
    CloseHandle(f);
    if (fnf) {  // Initialize config file if it doesn't already exist
        for (int i = 0; i < (numSettings * 2) - 1; i += 2) {
            WritePrivateProfileString(sectionName, configDefaults[i], configDefaults[i + 1], iniFilePath);
        }
    }
}

void commandMenuInit()
{
    // Config setup
    assureIni(true);
    gameState = SnakeGameState::NPPSNAKE_STOPPED;
    setCommand(0, (TCHAR *)TEXT("New Game"), newGame, NULL, false);
    setCommand(1, (TCHAR *)TEXT("Settings"), openConfig, NULL, false);
    setCommand(2, (TCHAR*)TEXT("About"), about, NULL, false);
}

// deallocate shortcuts here
void commandMenuCleanUp()
{}

// int index,                      zero based number to indicate the order of command
// TCHAR *commandName,             the command name that you want to see in plugin menu
// PFUNCPLUGINCMD functionPointer, the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
// ShortcutKey *shortcut,          optional. Define a shortcut to trigger this command
// bool check0nInit                optional. Make this menu item be checked visually
bool setCommand(size_t index, TCHAR* cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey* sk, bool check0nInit)
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}

// If a game is running on a thread, then end it and wait for it to end.
void endRemoteGame() {
    if (gameState != SnakeGameState::NPPSNAKE_STOPPED) {
        if (::MessageBox(NULL, L"This will end the current game!", L"NppSnake", MB_OKCANCEL) == IDCANCEL) return;
        gameState = SnakeGameState::NPPSNAKE_STOPPED;
        Sleep(GameInfo.maxSleep + 500); // give the current enough game time to end
    }
}

// TODO: Only hook when we want to play snake (unhook on gameOver)
// TODO: Work out bugs in pause mechanism (data race?)
// TODO: Add display element to paused game
// TODO: Config file checks (size values must be < RAND_MAX)
// TODO: Detect window size change and issue warning or end game?
void newGame() {
    endRemoteGame();
    assureIni(false);
    HANDLE hThread = ::CreateThread(NULL, 0, snake, 0, 0, NULL);
    if (!hThread) return;
    ::CloseHandle(hThread);
}

void openConfig() {
    endRemoteGame();
    assureIni(false);
    ::SendMessage(nppData._nppHandle, NPPM_DOOPEN, 0, (LPARAM)iniFilePath);
}

void about() {
    ::MessageBox(NULL, L"A Notepad++ version of the classic arcade game \"Snake\"", L"About", MB_OK);
}