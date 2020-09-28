#pragma once

#ifndef NPPSNAKE_PLDEF_H
#define NPPSNAKE_PLDEF_H

#include "PluginInterface.h"

const TCHAR PLUGIN_NAME[] = TEXT("NppSnake");

const int nbFunc = 3;

//
// Initialization of your plugin data
// It will be called while plugin loading
//
void pluginInit(HANDLE hModule);

//
// Cleaning of your plugin
// It will be called while plugin unloading
//
void pluginCleanUp();

//
//Initialization of your plugin commands
//
void commandMenuInit();

//
//Clean up your plugin commands allocation (if any)
//
void commandMenuCleanUp();

//
// Function which sets your command 
//
bool setCommand(size_t index, TCHAR* cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey* sk = NULL, bool check0nInit = false);

void newGame();
void openConfig();
void about();


#endif // NPPSNAKE_PLDEF_H
