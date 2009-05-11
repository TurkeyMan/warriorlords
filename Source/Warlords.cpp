#include "Warlords.h"
#include "Editor.h"

#include "MFFileSystem.h"
#include "FileSystem/MFFileSystemNative.h"
#include "MFModel.h"
#include "MFView.h"
#include "MFRenderer.h"
#include "MFSystem.h"

/*** Global Stuff ***/
Editor *pEditor;

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

	pEditor = new Editor();//"DefaultMap");

	Screen::SetNext(pEditor);
}

void Game_Update()
{
	MFCALLSTACKc;

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

	delete pEditor;
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

#if defined(MF_WINDOWS) || defined(_WINDOWS)
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

int main(int argc, char *argv[])
{
	MFInitParams initParams;
	MFZeroMemory(&initParams, sizeof(MFInitParams));
	initParams.argc = argc;
	initParams.argv = argv;

	return GameMain(&initParams);
}

#endif
