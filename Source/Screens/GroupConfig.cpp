#include "Warlords.h"
#include "GroupConfig.h"
#include "MapScreen.h"

#include "MFPrimitive.h"
#include "MFRenderer.h"
#include "MFFont.h"

#include "stdio.h"

void DrawHealthBar(int x, int y, int maxHealth, float currentHealth);

static float gPositions[5][5][2] =
{
	{ {.5f, .5f},   {0.f, 0.f},   {0.f, 0.f},   {0.f, 0.f},   {0.f, 0.f}  },
	{ {.32f, .5f},  {.67f, .5f},  {0.f, 0.f},   {0.f, 0.f},   {0.f, 0.f}  },
	{ {.32f, .25f}, {.67f, .5f},  {.32f, .75f}, {0.f, 0.f},   {0.f, 0.f}  },
	{ {.32f, .25f}, {.67f, .25f}, {.32f, .75f}, {.67f, .75f}, {0.f, 0.f}  },
	{ {.16f, .25f}, {.84f, .25f}, {.5f, .5f},   {.16f, .75f}, {.84f, .75f} }
};

GroupConfig::GroupConfig()
: Window(true)
{
	MFRect center, middle;
	DivideRect_Vert(window, window.height - 64 - margin*4, 0.f, &center, &lower, true);
	DivideRect_Vert(center, center.height - 64*2 - margin*8, 0.f, &top, &middle, true);
	DivideRect_Horiz(middle, 0.5f, 0.f, &rear, &front, false);
}

GroupConfig::~GroupConfig()
{
}

bool GroupConfig::Draw()
{
	if(!bVisible)
		return false;

	if(battleConfig.Draw())
		return true;

	if(!Window::Draw())
		return false;

	Game *pGame = Game::GetCurrent();
	pGame->DrawLine(rear.x + 5.f, rear.y+rear.height, front.x + front.width - 5.f, rear.y+rear.height);

	for(int a = 0; a < numExtraGroups; ++a)
		pGame->DrawLine(bottom[a].x + bottom[a].width, bottom[a].y + 5.f, bottom[a].x + bottom[a].width, bottom[a].y + bottom[a].height - 5.f);

	UnitDefinitions *pDefs = units[0].pUnit->GetDefs();

	for(int a = numUnits-1; a >= 0; --a)
	{
		GroupUnit &unit = units[a];
		unit.pUnit->Draw(unit.x - 32.f, unit.y - 32.f);
	}

	if(pDefs)
		pDefs->DrawUnits(64.f, MFRenderer_GetTexelCenterOffset(), false, true);

	for(int a = numUnits-1; a >= 0; --a)
	{
		GroupUnit &unit = units[a];

		if(unit.pGroup == units[0].pGroup && !unit.pUnit->IsVehicle())
			DrawHealthBar(unit.x - 32, unit.y - 32, unit.pUnit->GetMaxHP(), unit.pUnit->GetHealth());

		char move[8];
		sprintf(move, "%g", unit.pUnit->GetMovement() * 0.5f);

		MFFont *pFont = Game::GetCurrent()->GetSmallNumbersFont();
		float height = MFFont_GetFontHeight(pFont);
		float moveWidth = MFFont_GetStringWidth(pFont, move, height);

		MFFont_BlitText(pFont, unit.x + 30 - (int)moveWidth, unit.y + 30 - (int)height, MFVector::white, move);
	}

	return true;
}

bool GroupConfig::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	// HACK: right click returns
	if(info.buttonID == 1 && ev == IE_Tap)
		Hide();

	// only handle left clicks
	if(info.buttonID != 0)
		return true;

	switch(ev)
	{
		case IE_Tap:
		{
			// see if we have selected a unit
			int unit = GetUnitFromPoint(info.tap.x, info.tap.y);
			if(unit > -1)
			{
				if(!units[unit].pUnit->IsVehicle())
					battleConfig.Show(units[unit].pUnit);
			}

			// see if we have selected another group
			int group = GetGroupFromPoint(info.tap.x, info.tap.y);
			if(group > -1)
			{
				pTile->BringGroupToFront(pExtraGroups[group]);
				PositionUnits();
			}
			break;
		}
		case IE_Drag:
		{
			if(dragUnit == -1 && pDragGroup == NULL)
			{
				// check for unit drag
				int unit = GetUnitFromPoint(info.drag.startX, info.drag.startY);
				if(unit > -1)
				{
					dragUnit = unit;
					break;
				}

				// check for group drag
				int group = GetGroupFromPoint(info.drag.startX, info.drag.startY);
				if(group > -1)
				{
					pDragGroup = pExtraGroups[group];
					break;
				}
			}
			else
			{
				if(dragUnit > -1)
				{
					units[dragUnit].x = (int)info.drag.x;
					units[dragUnit].y = (int)info.drag.y;
				}
				else if(pDragGroup)
				{
					for(int a=0; a<numUnits; ++a)
					{
						if(units[a].pGroup == pDragGroup)
						{
							units[a].x += (int)info.drag.deltaX;
							units[a].y += (int)info.drag.deltaY;
						}
					}
				}
			}
			break;
		}
		case IE_Up:
		{
			// find where we dropped it...
			if(dragUnit != -1)
			{
				bool bFound = false;

				if(units[dragUnit].rank != 2)
				{
					// check if we're swapping with another unit
					int unit = GetUnitFromPoint(info.up.x, info.up.y);
					if(unit > -1 && units[unit].pGroup == units[dragUnit].pGroup && !units[unit].pUnit->IsVehicle())
					{
						units[unit].pGroup->SwapUnits(units[unit].pUnit, units[dragUnit].pUnit);
						bFound = true;
					}

					// check dropping in the other file
					if(!bFound)
					{
						int dropped = GetFileFromPoint(info.up.x, info.up.y);
						if(dropped > -1)
						{
							if(dropped != units[dragUnit].rank)
							{
								Unit *pUnit = units[dragUnit].pUnit;
								Group *pGroup = units[dragUnit].pGroup;

								// check there is room in the target file
								if(dropped == 0)
								{
									if(numFront < 5)
									{
										pGroup->RemoveUnit(pUnit);
										pGroup->AddForwardUnit(pUnit);
									}
								}
								else
								{
									if(numRear < 5)
									{
										pGroup->RemoveUnit(pUnit);
										pGroup->AddRearUnit(pUnit);
									}
								}
							}

							bFound = true;
						}
					}
				}

				// check for dropping in other groups
				if(!bFound)
				{
					int group = GetGroupFromPoint(info.up.x, info.up.y);
					if(group > -1)
					{
						if(units[dragUnit].pUnit->IsVehicle() && pExtraGroups[group]->GetNumUnits() == 0)
						{
							// swap vehicles
							Unit *pV1 = units[dragUnit].pGroup->GetVehicle();
							Unit *pV2 = pExtraGroups[group]->GetVehicle();

							units[dragUnit].pGroup->RemoveUnit(pV1);
							pExtraGroups[group]->RemoveUnit(pV2);

							units[dragUnit].pGroup->AddUnit(pV2);
							pExtraGroups[group]->AddUnit(pV1);
						}
						else if(pExtraGroups[group]->GetNumUnits() < 10 || units[dragUnit].rank == 2)
						{
							// move unit into group
							Unit *pUnit = units[dragUnit].pUnit;
							Group *pGroup = units[dragUnit].pGroup;

							if(pExtraGroups[group]->AddUnit(pUnit))
								pGroup->RemoveUnit(pUnit);

							// check if we have just emptied the group
							if(pGroup->GetNumUnits() == 0 && pGroup->GetVehicle() == NULL)
							{
								pTile->RemoveGroup(pGroup);
								pGroup->Destroy();
							}
						}

						bFound = true;
					}
				}

				// check for dropping into the 'new' group
				if(!bFound)
				{
					if(MFTypes_PointInRect(info.up.x, info.up.y, &empty))
					{
						Unit *pUnit = units[dragUnit].pUnit;
						Group *pGroup = units[dragUnit].pGroup;

						Group *pNewGroup = Group::Create(pGroup->GetPlayer());
						pTile->AddGroupToBack(pNewGroup);

						if(pNewGroup->AddUnit(pUnit))
							pGroup->RemoveUnit(pUnit);

						// check if we have just emptied the group
						if(pGroup->GetNumUnits() == 0 && pGroup->GetVehicle() == NULL)
						{
							pTile->RemoveGroup(pGroup);
							pGroup->Destroy();
						}

						bFound = true;
					}
				}

				if(bFound)
				{
					// clear the undo stack
					Game::GetCurrent()->ClearUndoStack();
				}

				dragUnit = -1;
			}
			else if(pDragGroup)
			{
				bool bClearUndo = false;

				// check if we're swapping vehicles
				int unit = GetUnitFromPoint(info.up.x, info.up.y);
				if(unit > -1 && units[unit].pUnit->IsVehicle() && pDragGroup->GetNumUnits() == 0 && pDragGroup->GetVehicle())
				{
					// swap vehicles
					Unit *pV1 = pDragGroup->GetVehicle();
					Unit *pV2 = units[0].pGroup->GetVehicle();

					pDragGroup->RemoveUnit(pV1);
					units[0].pGroup->RemoveUnit(pV2);

					pDragGroup->AddUnit(pV2);
					units[0].pGroup->AddUnit(pV1);

					bClearUndo = true;
				}
				else
				{
					// check if we dropped the group in one of the top files
					int dropped = GetFileFromPoint(info.up.x, info.up.y);
					if(dropped > -1)
					{
						// move as many into the chosen file as possible
						Group *pTarget = units[0].pGroup;

						Unit *pVehicle = pDragGroup->GetVehicle();
						if(pVehicle)
						{
							if(pTarget->AddUnit(pVehicle))
								pDragGroup->RemoveUnit(pVehicle);
						}

						while(pDragGroup->GetNumUnits() > 0 && pTarget->GetNumUnits() < 10)
						{
							Unit *pUnit = pDragGroup->GetUnit(0);
							if(pTarget->AddUnit(pUnit))
								pDragGroup->RemoveUnit(pUnit);
						}

						bClearUndo = true;
					}
					else
					{
						// check if we dropped the group on another group
						int group = GetGroupFromPoint(info.up.x, info.up.y);

						if(group > -1 && pExtraGroups[group] != pDragGroup)
						{
							// move as many into the target group as possible
							Unit *pVehicle = pDragGroup->GetVehicle();
							if(pVehicle)
							{
								if(pExtraGroups[group]->AddUnit(pVehicle))
									pDragGroup->RemoveUnit(pVehicle);
							}

							while(pDragGroup->GetNumUnits() > 0 && pExtraGroups[group]->GetNumUnits() < 10)
							{
								Unit *pUnit = pDragGroup->GetUnit(0);
								if(pExtraGroups[group]->AddUnit(pUnit))
									pDragGroup->RemoveUnit(pUnit);
							}

							bClearUndo = true;
						}
					}
				}

				if(pDragGroup->GetNumUnits() == 0 && pDragGroup->GetVehicle() == NULL)
				{
					pTile->RemoveGroup(pDragGroup);
					pDragGroup->Destroy();
				}

				if(bClearUndo)
				{
					// clear the undo stack
					Game::GetCurrent()->ClearUndoStack();
				}

				pDragGroup = NULL;
			}

			PositionUnits();
			break;
		}
	}

	return Window::HandleInputEvent(ev, info);
}

int GroupConfig::GetUnitFromPoint(float x, float y)
{
	int unit = -1;

	Group *pFirstGroup = units[0].pGroup;
	for(int a=0; a<numUnits && units[a].pGroup == pFirstGroup; ++a)
	{
		if(x >= units[a].x - 32 && x < units[a].x + 32 &&
			y >= units[a].y - 32 && y < units[a].y + 32)
		{
			if(a != dragUnit)
			{
				unit = a;
				break;
			}
		}
	}

	return unit;
}

int GroupConfig::GetFileFromPoint(float x, float y)
{
	int file = -1;

	if(MFTypes_PointInRect(x, y, &front))
		file = 0;
	else if(MFTypes_PointInRect(x, y, &rear))
		file = 1;

	return file;
}

int GroupConfig::GetGroupFromPoint(float x, float y)
{
	int group = -1;

	for(int a=0; a<numExtraGroups; ++a)
	{
		if(MFTypes_PointInRect(x, y, &bottom[a]))
		{
			group = a;
			break;
		}
	}

	return group;
}

void GroupConfig::Show(MapTile *_pTile)
{
	Window::Show();

	pTile = _pTile;

	dragUnit = -1;
	pDragGroup = NULL;

	PositionUnits();
}

void GroupConfig::PositionUnits()
{
	int numGroups = MFMin(pTile->GetNumGroups(), 5);
	numUnits = 0;
	numFront = numRear = -1;

	for(int a=0; a<numGroups; ++a)
	{
		Group *pG = pTile->GetGroup(a);

		int front = pG->GetNumForwardUnits();
		int rear = pG->GetNumRearUnits();

		if(numFront == -1)
		{
			numFront = front;
			numRear = rear;
		}

		for(int b=0; b<front; ++b)
		{
			GroupUnit &unit = units[numUnits++];
			unit.pGroup = pG;
			unit.pUnit = pG->GetForwardUnit(b);
			unit.rank = 0;
		}

		for(int b=0; b<rear; ++b)
		{
			GroupUnit &unit = units[numUnits++];
			unit.pGroup = pG;
			unit.pUnit = pG->GetRearUnit(b);
			unit.rank = 1;
		}

		Unit *pVehicle = pG->GetVehicle();
		if(pVehicle)
		{
			GroupUnit &unit = units[numUnits++];
			unit.pGroup = pG;
			unit.pUnit = pVehicle;
			unit.rank = 2;
		}
	}

	// reposition the lower groups
	float bottomUsed = 0.f;
	const float margin = 5.f;

	numExtraGroups = -1;
	Group *pCurrentGroup = units[0].pGroup;

	for(int a=0; a<numUnits; ++a)
	{
		if(units[a].pGroup != pCurrentGroup)
		{
			++numExtraGroups;
			pCurrentGroup = units[a].pGroup;

			pExtraGroups[numExtraGroups] = pCurrentGroup;

			bottom[numExtraGroups].x = lower.x + bottomUsed;
			bottom[numExtraGroups].y = lower.y;
			bottom[numExtraGroups].width = 32.f + margin*4.f;
			bottom[numExtraGroups].height = lower.height;
			bottomUsed += bottom[numExtraGroups].width;
		}

		if(numExtraGroups < 0)
		{
			// place the units
			if(units[a].rank == 0)
			{
				units[a].x = (int)(front.x + front.width*gPositions[numFront-1][a][0]);
				units[a].y = (int)(front.y + front.height*gPositions[numFront-1][a][1]) + 4;
			}
			else if(units[a].rank == 1)
			{
				units[a].x = (int)(rear.x + rear.width*gPositions[numRear-1][a - numFront][0]);
				units[a].y = (int)(rear.y + rear.height*gPositions[numRear-1][a - numFront][1]) + 4;
			}
			else
			{
				units[a].x = (int)((rear.x + rear.width + front.x)*0.5f - units[a].pUnit->GetDetails()->width*0.5f);
				units[a].y = (int)(front.y + front.height*0.5f - units[a].pUnit->GetDetails()->height*0.5f);
			}
		}
		else
		{
			bottom[numExtraGroups].width += 32.f;
			bottomUsed += 32.f;
			units[a].x = (int)bottom[numExtraGroups].x + (int)bottom[numExtraGroups].width - 32 - (int)margin*2;
			units[a].y = (int)bottom[numExtraGroups].y + 32 + (int)margin*2;
		}
	}

	++numExtraGroups;

	empty.x = lower.x + bottomUsed;
	empty.y = lower.y;
	empty.width = lower.width - bottomUsed;
	empty.height = lower.height;
}

void GroupConfig::Hide()
{
	Window::Hide();

	Game::GetCurrent()->GetMapScreen()->SelectGroup(pTile->GetGroup(0));
}
