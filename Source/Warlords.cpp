#include "Warlords.h"
#include "Display.h"
#include "Screens/LoginScreen.h"
#include "Screens/HomeScreen.h"
#include "Screens/MenuScreen.h"
#include "Screens/LobbyScreen.h"
#include "Screens/JoinGameScreen.h"
#include "Game.h"
#include "Editor.h"

#include "MFFileSystem.h"
#include "FileSystem/MFFileSystemNative.h"
#include "MFModel.h"
#include "MFView.h"
#include "MFRenderer.h"
#include "MFSystem.h"
#include "MFDisplay.h"

#define TEST_ONLINE

/*** Global Stuff ***/
InputManager *pInputManager = NULL;

Game *pGame = NULL;
Editor *pEditor = NULL;

LoginScreen *pLogin = NULL;
HomeScreen *pHome = NULL;
MenuScreen *pMenu = NULL;
LobbyScreen *pLobby = NULL;
JoinGameScreen *pJoinGame = NULL;

MFSystemCallbackFunction pInitFujiFS;

bool gAppHasFocus = true;

/*** Game Functions ***/

void Game_InitFilesystem()
{
	// mount the game directory
	MFFileSystemHandle hNative = MFFileSystem_GetInternalFileSystemHandle(MFFSH_NativeFileSystem);
	MFMountDataNative mountData;
	mountData.cbSize = sizeof(MFMountDataNative);
	mountData.priority = MFMP_Normal;
	mountData.flags = MFMF_FlattenDirectoryStructure | MFMF_Recursive;
	mountData.pMountpoint = "game";
#if defined(MF_IPHONE)
	mountData.pPath = MFFile_SystemPath();
#else
	mountData.pPath = MFFile_SystemPath("Data/");
#endif
	MFFileSystem_Mount(hNative, &mountData);

	if(pInitFujiFS)
		pInitFujiFS();
}

void Game_Init()
{
	MFCALLSTACK;

	pInputManager = new InputManager;

	pLogin = new LoginScreen;
	pHome = new HomeScreen;
	pMenu = new MenuScreen;
	pLobby = new LobbyScreen;
	pJoinGame = new JoinGameScreen;

#if defined(TEST_ONLINE)
	Screen::SetNext(pLogin);
#else
	Screen::SetNext(pMenu);
#endif
}

void Game_Update()
{
	MFCALLSTACKc;

	HTTPRequest::UpdateHTTPEvents();

	pInputManager->Update();

	Session::Update();
	Screen::UpdateScreen();
}

void Game_Draw()
{
	MFCALLSTACKc;

	// Set identity camera (no camera)
	MFView_Push();
	MFView_ConfigureProjection(MFDEGREES(60.0f), 0.1f, 1000.0f);

	MFMatrix ortho;
	GetOrthoMatrix(&ortho, false);
	MFView_SetCustomProjection(ortho, false);

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

	if(pMenu)
		delete pMenu;

	if(pLogin)
		delete pLogin;
}

void Game_FocusGained()
{
	gAppHasFocus = true;
}

void Game_FocusLost()
{
	gAppHasFocus = false;
}

int GameMain(MFInitParams *pInitParams)
{
	MFRand_Seed((uint32)MFSystem_ReadRTC());

#if defined(MF_IPHONE) || 0
#if !defined(MF_IPHONE)
	gDefaults.display.displayWidth = 320;
	gDefaults.display.displayHeight = 480;
#endif
	SetDisplayOrientation(DO_90CW);
#else
#if defined(_DEBUG)
	gDefaults.display.displayWidth = 1280;
	gDefaults.display.displayHeight = 720;
#else
	gDefaults.display.displayWidth = 800;
	gDefaults.display.displayHeight = 480;
#endif
#endif

//	gDefaults.input.useDirectInputKeyboard = false;
	gDefaults.input.useXInput = false;
//	gDefaults.system.threadPriority = MFPriority_AboveNormal;
	gDefaults.display.pWindowTitle = "Warlords";

	pInitFujiFS = MFSystem_RegisterSystemCallback(MFCB_FileSystemInit, Game_InitFilesystem);
	MFSystem_RegisterSystemCallback(MFCB_InitDone, Game_Init);
	MFSystem_RegisterSystemCallback(MFCB_Update, Game_Update);
	MFSystem_RegisterSystemCallback(MFCB_Draw, Game_Draw);
	MFSystem_RegisterSystemCallback(MFCB_Deinit, Game_Deinit);

	MFSystem_RegisterSystemCallback(MFCB_GainedFocus, Game_FocusGained);
	MFSystem_RegisterSystemCallback(MFCB_LostFocus, Game_FocusLost);

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

void DrawTicker(float x, float y)
{
	uint64 t = MFSystem_ReadRTC();
	uint64 freq = MFSystem_GetRTCFrequency();
	freq /= 5;
	int numDots = (int)(t / freq) % 6;

	MFFont_DrawText(MFFont_GetDebugFont(), MakeVector(x, y), 32.f, MFVector::white, ".....", numDots);
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
