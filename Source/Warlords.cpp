#include "Warlords.h"
#include "Game.h"
#include "Editor.h"
#include "Screens/MapScreen.h"
#include "Screens/Battle.h"

#include "MFFileSystem.h"
#include "FileSystem/MFFileSystemNative.h"
#include "MFModel.h"
#include "MFView.h"
#include "MFRenderer.h"
#include "MFSystem.h"

/*** Global Stuff ***/
InputManager *pInputManager = NULL;

Game *pGame;

Editor *pEditor = NULL;
Battle *pBattle = NULL;

/*** Game Functions ***/


void Game_Init()
{
	MFCALLSTACK;

	// mount the game directory
	MFFileSystemHandle hNative = MFFileSystem_GetInternalFileSystemHandle(MFFSH_NativeFileSystem);
	MFMountDataNative mountData;
	mountData.cbSize = sizeof(MFMountDataNative);
	mountData.priority = MFMP_Normal;
	mountData.flags = MFMF_DontCacheTOC;
	mountData.pMountpoint = "game";
	mountData.pPath = MFFile_SystemPath("Data/");
	MFFileSystem_Mount(hNative, &mountData);

	pInputManager = new InputManager;

#if 1
	pGame = new Game("Map");
	Game::SetCurrent(pGame);
	pGame->BeginGame();
#else
	pGame = new Game("Map", true);
	Game::SetCurrent(pGame);

	pEditor = new Editor(pGame);
	Screen::SetNext(pEditor);
#endif
}

void Game_Update()
{
	MFCALLSTACKc;

	pInputManager->Update();

	Screen::UpdateScreen();
}

void Game_Draw()
{
	MFCALLSTACKc;

	// Set identity camera (no camera)
	MFView_Push();
	MFView_ConfigureProjection(MFDEGREES(60.0f), 0.1f, 1000.0f);

	MFRect rect = { 0, 0, (float)gDefaults.display.displayWidth, (float)gDefaults.display.displayHeight };
	MFView_SetOrtho(&rect);

	MFRenderer_ClearScreen();

	Screen::DrawScreen();

	MFView_Pop();
}

void Game_Deinit()
{
	MFCALLSTACK;

	if(pEditor)
		delete pEditor;

	if(pGame)
		delete pGame;
}

int GameMain(MFInitParams *pInitParams)
{
	MFRand_Seed((uint32)MFSystem_ReadRTC());

#if 0
	gDefaults.display.displayWidth = 480;
	gDefaults.display.displayHeight = 320;
#else
	gDefaults.display.displayWidth = 800;
	gDefaults.display.displayHeight = 480;
#endif

//	gDefaults.input.useDirectInputKeyboard = false;
	gDefaults.input.useXInput = false;
//	gDefaults.system.threadPriority = MFPriority_AboveNormal;
	gDefaults.display.pWindowTitle = "Warlords";

	MFSystem_RegisterSystemCallback(MFCB_InitDone, Game_Init);
	MFSystem_RegisterSystemCallback(MFCB_Update, Game_Update);
	MFSystem_RegisterSystemCallback(MFCB_Draw, Game_Draw);
	MFSystem_RegisterSystemCallback(MFCB_Deinit, Game_Deinit);

	return MFMain(pInitParams);
}

void DivideRect_Horiz(const MFRect &rect, float split, float margin, MFRect *pLeft, MFRect *pRight, bool bSplitPixels)
{
	float s = MFFloor(bSplitPixels ? split : rect.width*split);

	pLeft->x = rect.x + margin;
	pLeft->y = rect.y + margin;
	pLeft->width = s - margin*2;
	pLeft->height = rect.height - margin*2;

	pRight->x = rect.x + s + margin;
	pRight->y = rect.y + margin;
	pRight->width = rect.width - s - margin*2;
	pRight->height = rect.height - margin*2;
}

void DivideRect_Vert(const MFRect &rect, float split, float margin, MFRect *pTop, MFRect *pBottom, bool bSplitPixels)
{
	float s = MFFloor(bSplitPixels ? split : rect.height*split);

	pTop->x = rect.x + margin;
	pTop->y = rect.y + margin;
	pTop->width = rect.width - margin*2;
	pTop->height = s - margin*2;

	pBottom->x = rect.x + margin;
	pBottom->y = rect.y + s + margin;
	pBottom->width = rect.width - margin*2;
	pBottom->height = rect.height - s - margin*2;
}

void DivideRect_Quad(const MFRect &rect, float hSplit, float vSplit, float margin, MFRect *pTL, MFRect *pTR, MFRect *pBL, MFRect *pBR, bool bSplitPixels)
{
	float hs = MFFloor(bSplitPixels ? hSplit : rect.width*hSplit);
	float vs = MFFloor(bSplitPixels ? vSplit : rect.height*vSplit);

	pTL->x = rect.x + margin;
	pTL->y = rect.y + margin;
	pTL->width = hs - margin*2;
	pTL->height = vs - margin*2;

	pTR->x = rect.x + hs + margin;
	pTR->y = rect.y + margin;
	pTR->width = rect.width - hs - margin*2;
	pTR->height = vs - margin*2;

	pBL->x = rect.x + margin;
	pBL->y = rect.y + vs + margin;
	pBL->width = hs - margin*2;
	pBL->height = rect.height - vs - margin*2;

	pBR->x = rect.x + hs + margin;
	pBR->y = rect.y + vs + margin;
	pBR->width = rect.width - hs - margin*2;
	pBR->height = rect.height - vs - margin*2;
}

void AdjustRect_Margin(MFRect *pRect, float margin, bool bPixels)
{
	float hMargin = bPixels ? margin : pRect->width * margin;
	float vMargin = bPixels ? margin : pRect->width * margin;

	pRect->x += hMargin;
	pRect->y += vMargin;
	pRect->width -= hMargin*2;
	pRect->height -= vMargin*2;
}

#if defined(MF_WINDOWS) || defined(_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow)
{
	MFInitParams initParams;
	MFZeroMemory(&initParams, sizeof(MFInitParams));
	initParams.hInstance = hInstance;
	initParams.pCommandLine = lpCmdLine;

	return GameMain(&initParams);
}

#elif defined(MF_PSP) || defined(_PSP)
#include <pspkernel.h>

int main(int argc, char *argv[])
{
	MFInitParams initParams;
	MFZeroMemory(&initParams, sizeof(MFInitParams));
	initParams.argc = argc;
	initParams.argv = argv;

	int r = GameMain(&initParams);

	sceKernelExitGame();
	return r;
}

#else

int main(int argc, const char *argv[])
{
	MFInitParams initParams;
	MFZeroMemory(&initParams, sizeof(MFInitParams));
	initParams.argc = argc;
	initParams.argv = argv;

	return GameMain(&initParams);
}

#endif
