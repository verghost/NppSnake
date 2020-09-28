/*
	DLL Main file, where the direction keys are detected and player.direction is modified 
*/

#include "plDef.h"
#include "menuCmdID.h"
#include "snake.h"

extern FuncItem funcItem[nbFunc];
extern NppData nppData;

extern SnakeGame GameInfo; // snake stuff
extern SnakePlayer player;
extern SnakeGameState gameState;
extern bool directionChanged;

bool snakeInForeground = true;

HHOOK hook;
KBDLLHOOKSTRUCT hkStruct;

void changeDirection(SnakeDirection d) {
	if (gameState == SnakeGameState::NPPSNAKE_RUNNING) {
		switch (d) {
		case SnakeDirection::NPPSNAKE_UP:
			if (player.direction != SnakeDirection::NPPSNAKE_DOWN && !directionChanged) {
				player.direction = SnakeDirection::NPPSNAKE_UP;
				directionChanged = true;
			}
			break;
		case SnakeDirection::NPPSNAKE_DOWN:
			if (player.direction != SnakeDirection::NPPSNAKE_UP && !directionChanged) {
				player.direction = SnakeDirection::NPPSNAKE_DOWN;
				directionChanged = true;
			}
			break;
		case SnakeDirection::NPPSNAKE_LEFT:
			if (player.direction != SnakeDirection::NPPSNAKE_RIGHT && !directionChanged) {
				player.direction = SnakeDirection::NPPSNAKE_LEFT;
				directionChanged = true;
			}
			break;
		case SnakeDirection::NPPSNAKE_RIGHT:
			if (player.direction != SnakeDirection::NPPSNAKE_LEFT && !directionChanged) {
				player.direction = SnakeDirection::NPPSNAKE_RIGHT;
				directionChanged = true;
			}
			break;
		default:
			break;
		}
	}
}

bool checkPath(unsigned long long bufferId) {
	int ret;
	TCHAR* buff;
	if (bufferId != -16) {
		ret = (int)::SendMessage(nppData._nppHandle, NPPM_GETFULLPATHFROMBUFFERID, bufferId, NULL);
		if (ret == -1) return false; // bufferId does not exist
		buff = new TCHAR[++ret];
		ret = (int)::SendMessage(nppData._nppHandle, NPPM_GETFULLPATHFROMBUFFERID, bufferId, (LPARAM)buff);
		if (ret == -1) return false;
	} 
	else // check current full path
	{
		buff = new TCHAR[MAX_PATH];
		::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, MAX_PATH, (LPARAM)buff);
	}
	ret = wcscmp(buff, GameInfo.filePath);
	delete[] buff;
	return (ret == 0);
}

//Arrow Controls
LRESULT hcb(int nc, WPARAM wParam, LPARAM lParam) {
	if ((gameState != SnakeGameState::NPPSNAKE_STOPPED) && nc >= 0 && wParam == WM_KEYDOWN) // check if there is a KEYDOWN message
	{
		hkStruct = *((KBDLLHOOKSTRUCT*)lParam);
		// Process the virtual key code
		switch (hkStruct.vkCode) {
		case VK_ESCAPE: // pause
			if (gameState == SnakeGameState::NPPSNAKE_RUNNING) {
				gameState = SnakeGameState::NPPSNAKE_PAUSED;
				restoreUserStyles();
				break;
			}
			if (gameState == SnakeGameState::NPPSNAKE_PAUSED && checkPath(-16)) {
				assureUnPause();
			}
			break;
		case VK_LEFT:
			changeDirection(SnakeDirection::NPPSNAKE_LEFT);
			break;
		case VK_RIGHT:
			changeDirection(SnakeDirection::NPPSNAKE_RIGHT);
			break;
		case VK_UP:
			changeDirection(SnakeDirection::NPPSNAKE_UP);
			break;
		case VK_DOWN:
			changeDirection(SnakeDirection::NPPSNAKE_DOWN);
			break;
		default:
			break;
		}
	}
	return CallNextHookEx(hook, nc, wParam, lParam);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		// Hook procedure
		if (!(hook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)hcb, NULL, 0)))
		{
			MessageBoxA(NULL, "Hook Failed!\nArrow key control will not be enabled!", "Error", MB_ICONERROR);
			ExitThread(0);
		}
        pluginInit(hModule);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        pluginCleanUp();
        break;
    }
    return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	nppData = notpadPlusData;
	commandMenuInit();
}

extern "C" __declspec(dllexport) const TCHAR * getName()
{
	return PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int* nbF)
{
	*nbF = nbFunc;
	return funcItem;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification * notifyCode)
{
	switch (notifyCode->nmhdr.code)
	{
	case SCN_CHARADDED:
	{
		// for whatever reason, I can't seem to get a reliable value from notifyCode->ch
		if (gameState == SnakeGameState::NPPSNAKE_RUNNING) {
			int it = (int)::SendMessage(GameInfo.hCurrSci, SCI_GETCURRENTPOS, NULL, NULL) - 1;
			char ctmp = (char)::SendMessage(GameInfo.hCurrSci, SCI_GETCHARAT, (WPARAM)it, NULL);
			switch (ctmp) {
			case 'a':
				changeDirection(SnakeDirection::NPPSNAKE_LEFT);
				break;
			case 'd':
				changeDirection(SnakeDirection::NPPSNAKE_RIGHT);
				break;
			case 'w':
				changeDirection(SnakeDirection::NPPSNAKE_UP);
				break;
			case 's':
				changeDirection(SnakeDirection::NPPSNAKE_DOWN);
				break;
			default:
				break;
			}
			::SendMessage(GameInfo.hCurrSci, SCI_SETSEL, (WPARAM)it, (LPARAM)it+1);
			::SendMessage(GameInfo.hCurrSci, SCI_REPLACESEL, NULL, (LPARAM)"");
		}
	}
	break;

	case NPPN_FILEBEFORECLOSE: // if the player closes the file, then they want to quit
	{
		if (gameState != SnakeGameState::NPPSNAKE_STOPPED && snakeInForeground) {
			gameState = SnakeGameState::NPPSNAKE_STOPPED;
			::MessageBox(NULL, L"You have ended the game!", L"NppSnake", MB_OK);
		}
	}
	break;

	case NPPN_BUFFERACTIVATED:
	{
		if (gameState == SnakeGameState::NPPSNAKE_RUNNING) {
			gameState = SnakeGameState::NPPSNAKE_PAUSED;
			restoreUserStyles();
			//checkPath(notifyCode->nmhdr.idFrom);
			break;
		}
		if (gameState == SnakeGameState::NPPSNAKE_PAUSED && checkPath(notifyCode->nmhdr.idFrom)) {
			assureUnPause();
		}
	}
	break;

	case NPPN_SHUTDOWN:
	{
		commandMenuCleanUp();
	}
	break;

	default:
		return;
	}
}

// To request a message be added: http://sourceforge.net/forum/forum.php?forum_id=482781
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message) {
	case IDM_FILE_CLOSE:
		if (gameState == SnakeGameState::NPPSNAKE_RUNNING) {
			gameState = SnakeGameState::NPPSNAKE_STOPPED;
		}
		break;
	default:
		break;
	}
	return TRUE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
	return TRUE;
}
#endif //UNICODE
