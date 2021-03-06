#include "Warlords.h"
#include "Display.h"
#include "Game.h"
#include "Editor.h"

#include "Fuji/MFFileSystem.h"
#include "Fuji/FileSystem/MFFileSystemNative.h"
#include "Fuji/MFModel.h"
#include "Fuji/MFView.h"
#include "Fuji/MFRenderer.h"
#include "Fuji/MFSystem.h"
#include "Fuji/MFDisplay.h"

#include "Menu/Menu.h"
#include "Menu/Game/GameUI.h"

#include "Haku/UI/HKUI.h"
#include "Menu/Widgets/UnitButton.h"

#if defined(MF_WINDOWS)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
//	#include "../Project/resource.h"
#endif

#define TEST_ONLINE


/*** Global Stuff ***/

MFRenderer *pRenderer = NULL;

InputManager *pInputManager = NULL;

FrontMenu *pFrontMenu;

Editor *pEditor = NULL;

MFSystemCallbackFunction pInitFujiFS;


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

static void NullHandler(HKWidget &, const HKWidgetEventInfo &)
{
	int x = 0;
}

MFRenderLayer* GetRenderLayer(RenderLayer layer)
{
	return MFRenderer_GetLayer(pRenderer, layer);
}

void BeginLayer(RenderLayer layer)
{
	MFRenderLayerSet layerSet;
	MFZeroMemory(&layerSet, sizeof(layerSet));
	layerSet.pSolidLayer = GetRenderLayer(layer);
	MFRenderer_SetRenderLayerSet(pRenderer, &layerSet);
}

void Init_Renderer()
{
	// create the renderer with a single layer that clears before rendering
	MFRenderLayerDescription layers[] = { { "Map" }, { "Scene" }, { "UI" } };
	const int numLayers = sizeof(layers) / sizeof(layers[0]);
	pRenderer = MFRenderer_Create(layers, numLayers, NULL, NULL);
	MFRenderer_SetCurrent(pRenderer);

	for(int a=0; a<numLayers; ++a)
		MFRenderLayer_SetLayerSortMode(MFRenderer_GetLayer(pRenderer, a), MFRL_SM_None);

	MFRenderLayer_SetClear(MFRenderer_GetLayer(pRenderer, 0), MFRCF_All);
}

void Destroy_Renderer()
{
	MFRenderer_Destroy(pRenderer);
}

void Game_Init()
{
	MFCALLSTACK;

	MapTemplate::Init();
	Unit::Init();

	Init_Renderer();

	pInputManager = new InputManager;

	GameState::Init();
	GameData::Init();

	// check if we want to run a battle test...
	if(MFFileSystem_Exists("battle_test.ini"))
	{
		void BattleTest();
		BattleTest();
		return;
	}

	// load the UI
	HKUserInterface::Init();

	HKWidgetFactory::FactoryType *pButtonType = HKUserInterface::FindWidgetType(HKWidgetButton::TypeName());
	HKUserInterface::RegisterWidget<UnitButton>(pButtonType);
	HKUserInterface::RegisterWidgetRenderer(UnitButton::TypeName(), RendererUnitButton::Create, NULL);

	HKUserInterface::RegisterEventHandler("null", NullHandler);

	HKUserInterface *pUI = new HKUserInterface();
	HKUserInterface::SetActiveUI(pUI);

	pFrontMenu = new FrontMenu();

	pFrontMenu->ShowMainMenu();
	pFrontMenu->Show();
}

void Game_Update()
{
	MFCALLSTACKc;

	MFHeap_ValidateHeap();

	HTTPRequest::UpdateHTTPEvents();

	pInputManager->Update();

	Screen::UpdateScreen();

	GameData::Get()->Update(); // old ui in here
	HKUserInterface::Get().Update();

	if(MFInput_WasPressed(Key_F1, IDD_Keyboard))
		MFString_Dump();

	if(MFInput_WasPressed(Key_F2, IDD_Keyboard))
	{
		MFHeap *pMainHeap = MFHeap_GetHeap(MFHT_Active);
		MFDebug_Log(2, MFStr("%d - Allocated: %d bytes (%d bytes waste)", MFHeap_GetNumAllocations(pMainHeap), MFHeap_GetTotalAllocated(pMainHeap), MFHeap_GetTotalWaste(pMainHeap)));
	}
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

//	MFRenderer_ClearScreen();

	BeginLayer(RL_Scene);
	Screen::DrawScreen();

	BeginLayer(RL_UI);
	GameData::Get()->Draw(); // old ui in here
	HKUserInterface::Get().Draw();

//	MFFont_DrawTextf(MFFont_GetDebugFont(), 10, 420, 30, MFVector::black, "Mem: %db (%d allocs)", MFHeap_GetTotalAllocated(), MFHeap_GetNumAllocations());

	MFView_Pop();
}

void Game_Deinit()
{
	MFCALLSTACK;

	if(pEditor)
		delete pEditor;

	HKUserInterface::Deinit();

	GameData::Deinit();
	GameState::Deinit();

	Destroy_Renderer();
}

void Game_FocusGained()
{
}

void Game_FocusLost()
{
}

int GameMain(MFInitParams *pInitParams)
{
	MFRand_Seed((uint32)MFSystem_ReadRTC());

	MFDisplay_GetDefaultRes(&pInitParams->display.displayRect);

	if(pInitParams->display.displayRect.height > pInitParams->display.displayRect.width)
		SetDisplayOrientation(DO_90CW);

#if defined(_DEBUG)
	pInitParams->display.displayRect.width = 1280;
	pInitParams->display.displayRect.height = 720;
#endif

//	gDefaults.input.useDirectInputKeyboard = false;
	gDefaults.input.useXInput = false;
//	gDefaults.system.threadPriority = MFPriority_AboveNormal;
	gDefaults.display.pWindowTitle = "Warrior Lords";
#if defined(MF_WINDOWS)
//	gDefaults.display.pIcon = MAKEINTRESOURCE(IDI_ICON1);
#endif

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

#elif !defined(MF_NACL) && !defined(MF_ANDROID)

int main(int argc, const char *argv[])
{
	MFInitParams initParams;
	MFZeroMemory(&initParams, sizeof(MFInitParams));
	initParams.argc = argc;
	initParams.argv = argv;

	return GameMain(&initParams);
}

#endif
/*
#include <xmmintrin.h>

__m128 poojabber( __m128 &t )
{
	__m128 x = _mm_add_ps(t, t);

	HKWidget_GetVectorFromString( NULL );

	return x;
}
*/
