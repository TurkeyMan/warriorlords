#pragma once
#if !defined(_MAPVIEW_H)
#define _MAPVIEW_H

#include "Tileset.h"
#include "Unit.h"
#include "InputHandler.h"
#include "Path.h"

#include "Haku/UI/HKWidget.h"

struct MFTexture;
class Game;
class Map;

class MapView : public InputReceiver
{
public:
	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);
	bool ReceiveInputEvent(HKInputManager &manager, const HKInputManager::EventInfo &ev);

	MapView(Game *pGame, Map &map);
	~MapView();

	void Update();
	void Draw();

	void DrawDebug();

	void GetCursor(float x, float y, float *pX, float *pY);
	void GetVisibleTileSize(float *pWidth, float *pHeight);

	void SetOffset(float x, float y);
	void GetOffset(float *pX, float *pY);
	void SetZoom(float zoom, float centerX = -1.f, float centerY = -1.f);
	void CenterView(float x, float y);

	void SetMapOrtho(int *pXTiles = NULL, int *pYTiles = NULL);

	MFMaterial *GetMinimap(int *pMapWidth, int *pMapHeight);

	// editor stuff
	void SetMoveKey(bool bAlternate)					{ moveButton = bAlternate ? 1 : 0; }

protected:
	void CreateRenderTarget();

	Game *pGame;
	Map &map;

	float xOffset, yOffset;
	float zoom, maxZoom;

	int dragContact;
	float lastX, lastY;
	float xVelocity, yVelocity;
	bool isDragging;

	MFArray<UnitRender> renderUnits;

	MFTexture *pRenderTarget;
	MFMaterial *pRenderTargetMaterial;
	MFMaterial *pCloud;

	MFMaterial *pMinimapMaterial;
	int minimapPixelScale;
	uint32 *pMiniMapImage;

	// details
	struct Cloud
	{
		float x, y;
	};

	static const int numClouds = 16;

	Cloud clouds[numClouds];

	void SetRTOrtho(int *pXTiles = NULL, int *pYTiles = NULL);

	// editor stuff
	int moveButton;
};

#endif
