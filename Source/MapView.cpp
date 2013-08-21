#include "Warlords.h"
#include "MapView.h"
#include "Map.h"
#include "Group.h"
#include "Display.h"

#include "Haku/UI/HKUI.h"

#include "Fuji/MFRenderer.h"
#include "Fuji/MFView.h"
#include "Fuji/MFMaterial.h"
#include "Fuji/Materials/MFMat_Standard.h"
#include "Fuji/MFPrimitive.h"
#include "Fuji/MFTexture.h"

#include "stdio.h"

MapView::MapView(Game *pGame, Map &map)
: pGame(pGame)
, map(map)
{
	pCloud = MFMaterial_Create("Cloud");

	xOffset = yOffset = 0.f;

	zoom = 1.f;

	CreateRenderTarget();

	// add some clouds
	TileSize size = map.Tileset().GetTileSize();

	for(int a=0; a<numClouds; ++a)
	{
		clouds[a].x = MFRand_Range(-64.f, (float)(map.width * size.width));
		clouds[a].y = MFRand_Range(-64.f, (float)(map.height * size.height));
	}

	moveButton = 0;

	isDragging = false;
	dragContact = 0;

	lastX = lastY = 0.f;
	xVelocity = yVelocity = 0.f;
}

MapView::~MapView()
{
	MFMaterial_Release(pCloud);
}

MFMaterial *MapView::GetMinimap(int *pMapWidth, int *pMapHeight)
{
	int mapWidth, mapHeight;
	map.GetMapSize(&mapWidth, &mapHeight);

	uint32 *pImage = pMiniMapImage;
	int texWidth = mapWidth*2;
	int texHeight = mapHeight*2;

	for(int y=0; y<mapHeight; ++y)
	{
		for(int a=0; a<2; ++a)
		{
			uint32 *pLine = pImage;

			for(int x=0; x<mapWidth; ++x)
			{
				MapTile *pTile = map.GetTile(x, y);
				uint32 terrain = pTile->GetTerrain();
				if(a & 1)
					terrain >>= 16;

				MFVector colour = MFVector::black;
				bool bTerrainColour = false;

				int player = pTile->GetPlayer();
				if(player > -2)
					colour = pGame->GetPlayerColour(player);
				else if(pTile->GetType() == OT_Place)
					colour = MFVector::white;
				else if(pTile->IsRoad())
					colour = MakeVector(.75f, .5f, .0f, 1.f);
				else
					bTerrainColour = true;

				for(int b=0; b<2; ++b)
				{
					if(bTerrainColour)
					{
						int t = (b & 1) ? (terrain >> 8) & 0xFF : terrain & 0xFF;
						colour = map.Tileset().TerrainColour(t);
					}

					pLine[b] = colour.ToPackedColour();
				}

				pLine += 2;
			}

			pImage += texWidth;
		}
	}

	MFTexture *pTex = MFTexture_ScaleFromRawData("MiniMap", pMiniMapImage, texWidth, texHeight, texWidth * minimapPixelScale, texHeight * minimapPixelScale, ImgFmt_A8R8G8B8, minimapPixelScale > 1 ? SA_AdvMAME : SA_None);

	pMinimapMaterial = MFMaterial_Create("MiniMap");
	MFTexture_Release(pTex);

	if(pMapWidth)
		*pMapWidth = texWidth * minimapPixelScale;
	if(pMapHeight)
		*pMapHeight = texHeight * minimapPixelScale;

	return pMinimapMaterial;
}

void MapView::CreateRenderTarget()
{
	int mapWidth, mapHeight;
	map.GetMapSize(&mapWidth, &mapHeight);

	maxZoom = 2.f;

	MFRect screen;
	TileSize tileSize = map.Tileset().GetTileSize();
	GetDisplayRect(&screen);

try_again:
	int rtWidth = (int)(screen.width*maxZoom)+tileSize.width*2-1;
	int rtHeight = (int)(screen.height*maxZoom)+tileSize.height*2-1;
	rtWidth -= rtWidth%tileSize.width;
	rtHeight -= rtHeight%tileSize.height;
	rtWidth = MFUtil_NextPowerOf2(rtWidth);
	rtHeight = MFUtil_NextPowerOf2(rtHeight);
	pRenderTarget = MFTexture_CreateRenderTarget("MapSurface", rtWidth, rtHeight, ImgFmt_SelectFastest_NoAlpha);

	if(!pRenderTarget && maxZoom > 1.f)
	{
		maxZoom *= 0.5f;
		goto try_again;
	}

	pRenderTargetMaterial = MFMaterial_Create("MapSurface");
	MFMaterial_SetParameterI(pRenderTargetMaterial, MFMatStandard_Blend, 0, MFMatStandard_Blend_None);

	screen.width = MFMin(screen.width, 480.f);
	screen.height = MFMin(screen.height, 320.f);

	int sx = (int)screen.width / (mapWidth*2);
	int sy = (int)screen.height / (mapHeight*2);
	minimapPixelScale = MFClamp(1, MFMin(sx, sy), 4);

	pMiniMapImage = (uint32*)MFHeap_AllocAndZero((mapWidth*2) * (mapHeight*2) * sizeof(uint32));
	pMinimapMaterial = NULL;
}

void MapView::GetVisibleTileSize(float *pWidth, float *pHeight)
{
	TileSize tileSize = map.Tileset().GetTileSize();

	if(pWidth)
		*pWidth = tileSize.width*zoom;
	if(pHeight)
		*pHeight = tileSize.height*zoom;
}

void MapView::GetCursor(float x, float y, float *pX, float *pY)
{
	float tileWidth, tileHeight;
	GetVisibleTileSize(&tileWidth, &tileHeight);

	if(pX)
		*pX = xOffset + x / tileWidth;
	if(pY)
		*pY = yOffset + y / tileHeight;
}


bool MapView::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	float tileWidth, tileHeight;

	switch(ev)
	{
		case IE_Down:
			if(info.buttonID == moveButton)
			{
				pInputManager->SetExclusiveContactReceiver(info.contact, this);
				xVelocity = yVelocity = 0.f;
				return true;
			}
			break;
		case IE_Up:
			if(info.buttonID == moveButton)
				isDragging = false;
			break;
		case IE_Drag:
			if(info.buttonID == moveButton)
			{
				GetVisibleTileSize(&tileWidth, &tileHeight);
				SetOffset(xOffset + -info.drag.deltaX/tileWidth, yOffset + -info.drag.deltaY/tileHeight);
				isDragging = true;
				return true;
			}
			break;
		case IE_Pinch:
		{
			float newZoom = (info.device == IDD_Mouse) ? zoom + (info.pinch.deltaScale < 1.f ? -.25f : .25f) : zoom * info.pinch.deltaScale;
			GetVisibleTileSize(&tileWidth, &tileHeight);
			SetZoom(newZoom, info.pinch.centerX/tileWidth, info.pinch.centerY/tileHeight);
			return true;
		}
	}

	return false;
}

bool MapView::ReceiveInputEvent(HKInputManager &manager, const HKInputManager::EventInfo &ev)
{
	float tileWidth, tileHeight;

	switch(ev.ev)
	{
		case HKInputManager::IE_Down:
			if(ev.buttonID == moveButton)
			{
				HKUserInterface::Get().SetFocus(ev.pSource, pGame->GetUI()->GetMap());
				xVelocity = yVelocity = 0.f;
				return true;
			}
			break;
		case HKInputManager::IE_Up:
		case HKInputManager::IE_Cancel:
			if(ev.buttonID == moveButton)
			{
				isDragging = false;
				HKUserInterface::Get().SetFocus(ev.pSource, NULL);
			}
			break;
		case HKInputManager::IE_Drag:
			if(ev.buttonID == moveButton)
			{
				GetVisibleTileSize(&tileWidth, &tileHeight);
				SetOffset(xOffset + -ev.drag.deltaX/tileWidth, yOffset + -ev.drag.deltaY/tileHeight);
				isDragging = true;
				return true;
			}
			break;
		case HKInputManager::IE_Pinch:
		{
			float newZoom = (ev.pSource->device == IDD_Mouse) ? zoom + (ev.pinch.deltaScale < 1.f ? -.25f : .25f) : zoom * ev.pinch.deltaScale;
			GetVisibleTileSize(&tileWidth, &tileHeight);
			SetZoom(newZoom, ev.pinch.centerX/tileWidth, ev.pinch.centerY/tileHeight);
			return true;
		}
	}

	return false;
}

void MapView::Update()
{
	int mapWidth, mapHeight;
	map.GetMapSize(&mapWidth, &mapHeight);

	TileSize tileSize = map.Tileset().GetTileSize();

	for(int a=0; a<numClouds; ++a)
	{
		clouds[a].x -= MFSystem_TimeDelta() * 5.f;

		if(clouds[a].x < -64)
		{
			clouds[a].x = (float)(mapWidth * tileSize.width);
			clouds[a].y = MFRand_Range(-64.f, (float)(mapHeight * tileSize.height));
		}
	}

#if defined(MF_IPHONE)
	if(isDragging)
	{
		// track drag velocity
		xVelocity = xVelocity*0.5f + ((xOffset - lastX) / MFSystem_TimeDelta())*0.5f;
		yVelocity = yVelocity*0.5f + ((yOffset - lastY) / MFSystem_TimeDelta())*0.5f;
		lastX = xOffset;
		lastY = yOffset;

		if(MakeVector(xVelocity, yVelocity).Magnitude2() < 5.f)
			xVelocity = yVelocity = 0.f;
	}
	else
	{
		// apply release velocity
		SetOffset(xOffset + xVelocity*MFSystem_TimeDelta(), yOffset + yVelocity*MFSystem_TimeDelta());
		float deceleration = 1.f - 2.0f*MFSystem_TimeDelta();
		xVelocity *= deceleration;
		yVelocity *= deceleration;

		if(MakeVector(xVelocity, yVelocity).Magnitude2() < 0.2f)
			xVelocity = yVelocity = 0.f;
	}
#endif
}

void MapView::Draw()
{
	const Tileset &tileset = map.Tileset();
	UnitDefinitions *pUnits = map.UnitDefs();

	int mapWidth, mapHeight;
	map.GetMapSize(&mapWidth, &mapHeight);

	MFRenderLayer *pLayer = GetRenderLayer(RL_Map);
	MFRenderLayer_SetLayerRenderTarget(pLayer, 0, pRenderTarget);
	MFRenderLayer_SetLayerDepthTarget(pLayer, NULL);
	BeginLayer(RL_Map);

	MFView_Push();

//	MFRenderer_SetRenderTarget(pRenderTarget, NULL);

	float texelCenter = MFRenderer_GetTexelCenterOffset();
	int xStart = (int)xOffset;
	int yStart = (int)yOffset;

	int xTiles, yTiles;
	SetRTOrtho(&xTiles, &yTiles);

	// HACK: make sure it doesn't attempt to draw too many
	if(xStart + xTiles > mapWidth)
		xTiles = mapWidth - xStart;
	if(yStart + yTiles > mapHeight)
		yTiles = mapHeight - yStart;

	MapTile *pStart = map.pMap + yStart*mapWidth + xStart;

	// blit map portion to a render target
	tileset.DrawMap(xTiles, yTiles, &pStart->terrain, sizeof(MapTile), mapWidth, texelCenter);

	// now we should, like, render all the extra stuff
	struct RenderTile
	{
		uint8 x, y, flags;
		int8 i;
	} renderTiles[1024];

	int numTiles = 0;
	int numRoads = 0;
	int numCastleTiles = 0;
	int numMiscTiles = 0;

	Group *pSelection = NULL;

	renderUnits.clear();

	for(int y=0; y<yTiles; ++y)
	{
		for(int x=xTiles-1; x>=0; --x)
		{
			MapTile *pTile = pStart + x;

			if(pTile->GetNumGroups())
			{
				Unit *pVehicle = pTile->FindVehicle();
				if(pVehicle)
					renderUnits.push() = pVehicle->Render((float)x, (float)y);

				Group *pGroup = pTile->GetGroup(0);
				if(pGroup->IsSelected())
					pSelection = pGroup;

				Unit *pUnit = pGroup->GetFeatureUnit();
				if(pUnit && (!pVehicle || pGroup->GetVehicle() != pVehicle))
					renderUnits.push() = pUnit->Render((float)x, (float)y);
			}

			if(pTile->type == OT_None)
				continue;

			RenderTile &drawTile = renderTiles[numTiles++];
			drawTile.x = x;
			drawTile.y = y;

			if(pTile->objectX > 0 || pTile->objectY > 0)
			{
				--numTiles;
				continue;
			}

			switch(pTile->type)
			{
				case OT_Road:
					drawTile.i = tileset.FindRoad(pTile->index, map.GetTerrainAt(xStart+x, yStart+y));
					MFDebug_Assert(drawTile.i >= 0, "Invalid road!");

					drawTile.flags = 8;
					++numRoads;
					break;

				case OT_Castle:
				{
					Castle *pCastle = map.GetCastle(pTile->index);
					drawTile.i = (int8)pCastle->player;
					drawTile.flags = 0;
					++numCastleTiles;
					break;
				}

				case OT_Flag:
					drawTile.i = (int8)pTile->index;
					drawTile.flags = 1;
					++numCastleTiles;
					break;

				case OT_Place:
//					drawTile.i = map.bEditable ? (int8)pTile->index : (int8)map.pPlaces[pTile->index].GetRenderID();
					drawTile.i = (int8)map.places[pTile->index].GetRenderID();
					drawTile.flags = 4;
					++numMiscTiles;
					break;
			}
		}

		pStart += mapWidth;
	}

	// draw the roads
	if(numRoads)
	{
		MFMaterial_SetMaterial(tileset.GetRoadMaterial());

		MFPrimitive(PT_QuadList);
		MFBegin(numRoads*2);

		for(int a=0; a<numTiles; ++a)
		{
			RenderTile &drawTile = renderTiles[a];

			if(drawTile.flags & 8)
			{
				MFRect uvs = tileset.GetRoadUVs(drawTile.i, texelCenter);
				MFSetTexCoord1(uvs.x, uvs.y);
				MFSetPosition((float)renderTiles[a].x, (float)renderTiles[a].y, 0);
				MFSetTexCoord1(uvs.x + uvs.width, uvs.y + uvs.height);
				MFSetPosition((float)renderTiles[a].x + 1.f, (float)renderTiles[a].y + 1.f, 0);
			}
		}

		MFEnd();
	}

	// draw the castle stuff
	if(numCastleTiles)
	{
		MFMaterial_SetMaterial(pUnits->GetCastleMaterial());

		MFPrimitive(PT_QuadList);
		MFBegin(numCastleTiles*2);
		MFRect uvs;

		for(int a=0; a<numTiles; ++a)
		{
			RenderTile &drawTile = renderTiles[a];

			if(drawTile.flags < 4)
			{
				MFVector colour = MFVector::one;
				float width = 1.f;

				switch(drawTile.flags)
				{
					case 0:
						uvs = pUnits->GetCastleUVs(pGame->GetPlayerRace(drawTile.i), texelCenter);
						colour = pGame->GetPlayerColour(drawTile.i);
						width = 2.f;
						break;
					case 1:
						uvs = pUnits->GetFlagUVs(pGame->GetPlayerRace(drawTile.i), texelCenter);
						colour = pGame->GetPlayerColour(drawTile.i);
						break;
				}

				MFSetColourV(colour);
				MFSetTexCoord1(uvs.x, uvs.y);
				MFSetPosition((float)renderTiles[a].x, (float)renderTiles[a].y, 0);
				MFSetTexCoord1(uvs.x + uvs.width, uvs.y + uvs.height);
				MFSetPosition((float)renderTiles[a].x + width, (float)renderTiles[a].y + width, 0);
			}
		}

		MFEnd();
	}

	// draw misc stuff
	if(numMiscTiles)
	{
		MFMaterial_SetMaterial(pUnits->GetMiscMaterial());

		MFPrimitive(PT_QuadList);
		MFBegin(numMiscTiles*2);
		MFRect uvs;

		for(int a=0; a<numTiles; ++a)
		{
			RenderTile &drawTile = renderTiles[a];

			if(drawTile.flags == 4)
			{
				MFVector colour = MFVector::one;

				int width, height;
				uvs = pUnits->GetSpecialUVs(drawTile.i, texelCenter, &width, &height);

				MFSetColourV(colour);
				MFSetTexCoord1(uvs.x, uvs.y);
				MFSetPosition((float)renderTiles[a].x, (float)renderTiles[a].y, 0);
				MFSetTexCoord1(uvs.x + uvs.width, uvs.y + uvs.height);
				MFSetPosition((float)renderTiles[a].x + (float)width, (float)renderTiles[a].y + (float)height, 0);
			}
		}

		MFEnd();
	}

	// draw the selection
	if(pSelection)
	{
		int x = pSelection->GetTile()->GetX() - xStart;
		int y = pSelection->GetTile()->GetY() - yStart;
		MFPrimitive_DrawUntexturedQuad((float)x, (float)y, 1, 1, MakeVector(0.f, 0.f, 0.8f, 0.4f));
	}

	// draw the units
	pGame->DrawUnits(renderUnits, 1.f, texelCenter);

	TileSize tileSize = tileset.GetTileSize();
	float tileXScale = 1.f/(float)tileSize.width;
	float tileYScale = 1.f/(float)tileSize.height;

	// draw clouds
	MFMaterial_SetMaterial(pCloud);

	MFPrimitive(PT_QuadList);
	MFBegin(numClouds*2);

	for(int a=0; a<numClouds; ++a)
	{
		float x = clouds[a].x * tileXScale - (float)xStart;
		float y = clouds[a].y * tileYScale - (float)yStart;

		MFSetTexCoord1(0.f, 0.f);
		MFSetPosition(x, y, 0);
		MFSetTexCoord1(1.f, 1.f);
		MFSetPosition(x + 1.f, y + 2.f, 0);
	}

	MFEnd();

	// reset the render target
	BeginLayer(RL_Scene);
//	MFRenderer_SetDeviceRenderTarget();

	// render the map to the screen
	MFMatrix orthoMat;
	GetOrthoMatrix(&orthoMat, true);
	MFView_SetCustomProjection(orthoMat, false);

	MFRect uvs;
	int targetWidth, targetHeight;

	GetDisplayRect(&uvs);
	MFTexture_GetTextureDimensions(pRenderTarget, &targetWidth, &targetHeight);

	float texelOffset = zoom <= 0.5f ? 0.f : texelCenter;
	uvs.x = (xOffset - (float)(int)xOffset) * (tileSize.width / targetWidth) + (texelOffset/targetWidth);
	uvs.y = (yOffset - (float)(int)yOffset) * (tileSize.height / targetHeight) + (texelOffset/targetHeight);
	uvs.width = uvs.width / targetWidth / zoom;
#if defined(MF_WINDOWS)
	uvs.height = uvs.height / targetHeight / zoom;
#else
	uvs.height = -uvs.height / targetHeight / zoom;
#endif

	MFMaterial_SetMaterial(pRenderTargetMaterial);
	MFPrimitive_DrawQuad(0, 0, 1, 1, MFVector::one, uvs.x, uvs.y, uvs.x + uvs.width, uvs.y + uvs.height);

	MFView_Pop();
}

void MapView::DrawDebug()
{
	MFView_Push();

	MFRect screenRect;
	GetDisplayRect(&screenRect);
	screenRect.width /= zoom;
	screenRect.height /= zoom;

	TileSize tileSize = map.Tileset().GetTileSize();

	float screenWidth = screenRect.width / tileSize.width;
	float screenHeight = screenRect.height / tileSize.height;

	xOffset = (int)(xOffset*tileSize.width) / (float)tileSize.width;
	yOffset = (int)(yOffset*tileSize.height) / (float)tileSize.height;

	MFRect rect;
	rect.x = xOffset - (float)(int)xOffset;
	rect.y = yOffset - (float)(int)yOffset;
	rect.width = screenWidth;
	rect.height = screenHeight;
	MFView_SetOrtho(&rect);

	int xTiles = (int)MFCeil(screenWidth + 1.f);
	int yTiles = (int)MFCeil(screenHeight + 1.f);

	float tileWidth, tileHeight;
	GetVisibleTileSize(&tileWidth, &tileHeight);

	int cursorX = (int)(xOffset + MFInput_Read(Mouse_XPos, IDD_Mouse) / tileWidth);
	int cursorY = (int)(yOffset + MFInput_Read(Mouse_YPos, IDD_Mouse) / tileHeight);

	int xOffsetI = (int)xOffset;
	int yOffsetI = (int)yOffset;

	MFPrimitive(PT_TriStrip|PT_Untextured);
	MFBegin(4);
	MFSetColour(0,0,0,.2f);
	MFSetPosition((float)(cursorX - xOffsetI), (float)(cursorY - yOffsetI), 0);
	MFSetPosition((float)(cursorX - xOffsetI + 1), (float)(cursorY - yOffsetI), 0);
	MFSetPosition((float)(cursorX - xOffsetI), (float)(cursorY - yOffsetI + 1), 0);
	MFSetPosition((float)(cursorX - xOffsetI + 1), (float)(cursorY - yOffsetI + 1), 0);
	MFEnd();

	MFPrimitive(PT_LineList|PT_Untextured);
	MFBegin(xTiles*2 + yTiles*2 + 4);
	MFSetColour(0.f, 0.4f, 1.f, 1.f);

	for(int x=0; x<=xTiles; ++x)
	{
		MFSetPosition((float)x, 0, 0);
		MFSetPosition((float)x, (float)yTiles, 0);
	}
	for(int y=0; y<=yTiles; ++y)
	{
		MFSetPosition(0, (float)y, 0);
		MFSetPosition((float)xTiles, (float)y, 0);
	}

	MFEnd();

	for(int y=0; y<yTiles; ++y)
	{
		for(int x=0; x<xTiles; ++x)
		{
			MapTile *pTile = map.GetTile(xOffsetI + x, yOffsetI + y);
			char text[16] = "X";
			if(pTile->region != 15)
				sprintf(text, "%d", pTile->region + 1);
			float w = MFFont_GetStringWidth(MFFont_GetDebugFont(), text, 0.5f);
			MFFont_DrawText(MFFont_GetDebugFont(), MakeVector((float)x + 0.5f - w*0.5f + .02f, (float)y + 0.25f + .02f), 0.5f, MFVector::black, text);
			MFFont_DrawText(MFFont_GetDebugFont(), MakeVector((float)x + 0.5f - w*0.5f, (float)y + 0.25f), 0.5f, MFVector::white, text);
		}
	}

	MFView_Pop();

	MFFont_DrawTextf(MFFont_GetDebugFont(), MakeVector(8, 8), 24, MFVector::yellow, "%d, %d", cursorX, cursorY);
}

void MapView::SetRTOrtho(int *pXTiles, int *pYTiles)
{
	int targetWidth, targetHeight;
	MFTexture_GetTextureDimensions(pRenderTarget, &targetWidth, &targetHeight);
	TileSize tileSize = map.Tileset().GetTileSize();

	float screenWidth = (float)targetWidth / (float)tileSize.width;
	float screenHeight = (float)targetHeight / (float)tileSize.height;

	xOffset = (int)(xOffset*tileSize.width) / (float)tileSize.width;
	yOffset = (int)(yOffset*tileSize.height) / (float)tileSize.height;

	MFRect rect;
	rect.x = xOffset - (float)(int)xOffset;
	rect.y = yOffset - (float)(int)yOffset;
	rect.width = screenWidth;
	rect.height = screenHeight;
	MFView_SetOrtho(&rect);

	MFRect screenRect;
	GetDisplayRect(&screenRect);

	if(pXTiles)
		*pXTiles = (int)MFCeil((screenRect.width / tileSize.width) / zoom + 1.f);
	if(pYTiles)
		*pYTiles = (int)MFCeil((screenRect.height / tileSize.height) / zoom + 1.f);
}

void MapView::SetMapOrtho(int *pXTiles, int *pYTiles)
{
	float tileWidth, tileHeight;
	GetVisibleTileSize(&tileWidth, &tileHeight);

	MFRect screenRect;
	GetDisplayRect(&screenRect);
	float screenWidth = screenRect.width / tileWidth;
	float screenHeight = screenRect.height / tileHeight;
	float screenHScale = 1.f / screenWidth;
	float screenVScale = 1.f / screenHeight;

	MFMatrix proj, mapTrans;
	mapTrans.SetXAxis3(MFVector::right * screenHScale);
	mapTrans.SetYAxis3(MFVector::up * screenVScale);
	mapTrans.SetZAxis3(MFVector::forward);
	mapTrans.SetTrans3(MakeVector(-xOffset * screenHScale, -yOffset * screenVScale, 0));

	GetOrthoMatrix(&proj, true);
	mapTrans.Multiply4x4(proj);

	MFView_SetCustomProjection(mapTrans, false);

	if(pXTiles)
		*pXTiles = (int)MFCeil(screenWidth) + 1;
	if(pYTiles)
		*pYTiles = (int)MFCeil(screenHeight) + 1;
}

void MapView::SetOffset(float x, float y)
{
	int mapWidth, mapHeight;
	map.GetMapSize(&mapWidth, &mapHeight);

	float tileWidth, tileHeight;
	GetVisibleTileSize(&tileWidth, &tileHeight);

	MFRect display;
	GetDisplayRect(&display);
	float maxX = (float)mapWidth - display.width / tileWidth;
	float maxY = (float)mapHeight - display.height / tileHeight;

	xOffset = MFClamp(0.f, x, maxX);
	yOffset = MFClamp(0.f, y, maxY);
}

void MapView::GetOffset(float *pX, float *pY)
{
	if(pX)
		*pX = xOffset;
	if(pY)
		*pY = yOffset;
}

void MapView::CenterView(float x, float y)
{
	int mapWidth, mapHeight;
	map.GetMapSize(&mapWidth, &mapHeight);

	float tileWidth, tileHeight;
	GetVisibleTileSize(&tileWidth, &tileHeight);

	MFRect display;
	GetDisplayRect(&display);
	float screenWidth = display.width / tileWidth;
	float screenHeight = display.height / tileHeight;

	xOffset = x + 0.5f - screenWidth*0.5f;
	yOffset = y + 0.5f - screenHeight*0.5f;

	xOffset = MFClamp(0.f, xOffset, (float)mapWidth - screenWidth);
	yOffset = MFClamp(0.f, yOffset, (float)mapHeight - screenHeight);
}

void MapView::SetZoom(float _zoom, float pointX, float pointY)
{
	int mapWidth, mapHeight;
	map.GetMapSize(&mapWidth, &mapHeight);

	float tileWidth, tileHeight;
	GetVisibleTileSize(&tileWidth, &tileHeight);

	MFRect display;
	GetDisplayRect(&display);

	if(pointX < 0.f)
		pointX = display.width / tileWidth * 0.5f;
	if(pointY < 0.f)
		pointY = display.height / tileHeight * 0.5f;

	float newZoom = MFClamp(1.f/maxZoom, _zoom, 1.f);
	float zoomDiff = zoom / newZoom;
	zoom = newZoom;

	// calculate new position
	xOffset += pointX - pointX*zoomDiff;
	yOffset += pointY - pointY*zoomDiff;

	// calculate clamps
	GetVisibleTileSize(&tileWidth, &tileHeight);
	float maxX = (float)mapWidth - display.width / tileWidth;
	float maxY = (float)mapHeight - display.height / tileHeight;

	// clamp the new position
	xOffset = MFClamp(0.f, xOffset, maxX);
	yOffset = MFClamp(0.f, yOffset, maxY);
}
