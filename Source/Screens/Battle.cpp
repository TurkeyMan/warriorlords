#include "Warlords.h"
#include "Battle.h"
#include "Unit.h"
#include "Path.h"

#include "Castle.h"

#include "MFInput.h"
#include "MFSystem.h"
#include "MFPrimitive.h"
#include "MFFont.h"
#include "MFMaterial.h"
#include "MFView.h"

#include "MFRenderer.h"
#include "MFInput.h"
#include "Display.h"

UnitDefinitions *pDefs;

struct Group
{
  Unit *pForwardUnits[5];
  Unit *pRearUnits[5];

  int numForwardUnits;
  int numRearUnits;
};

static Group groups[8];
static int races[2] = { 0, 1 };

Battle::Battle(Group *pGroup1, Group *pGroup2, const char *_pForeground, const char *_pBackground, const char *_pCastle)
{
  pForeground = MFMaterial_Create(MFStr("Battle/fg-%s", _pForeground));
  pBackground = MFMaterial_Create(MFStr("Battle/bg-%s", _pBackground));
  pCastle = _pCastle ? MFMaterial_Create(MFStr("Battle/castle-%s", _pCastle)) : NULL;

  pDefs = UnitDefinitions::Load("Units");

  // humans
  groups[0].pForwardUnits[groups[0].numForwardUnits++] = pDefs->CreateUnit(0, 1);
  groups[0].pForwardUnits[groups[0].numForwardUnits++] = pDefs->CreateUnit(8, 1);
  groups[0].pForwardUnits[groups[0].numForwardUnits++] = pDefs->CreateUnit(9, 1);
  groups[0].pForwardUnits[groups[0].numForwardUnits++] = pDefs->CreateUnit(12, 1);

  groups[0].pRearUnits[groups[0].numRearUnits++] = pDefs->CreateUnit(24, 1);
  groups[0].pRearUnits[groups[0].numRearUnits++] = pDefs->CreateUnit(24, 1);
  groups[0].pRearUnits[groups[0].numRearUnits++] = pDefs->CreateUnit(24, 1);

  // elves
  groups[1].pForwardUnits[groups[1].numForwardUnits++] = pDefs->CreateUnit(1, 2);
  groups[1].pForwardUnits[groups[1].numForwardUnits++] = pDefs->CreateUnit(11, 2);

  groups[1].pRearUnits[groups[1].numRearUnits++] = pDefs->CreateUnit(10, 2);
  groups[1].pRearUnits[groups[1].numRearUnits++] = pDefs->CreateUnit(10, 2);
  groups[1].pRearUnits[groups[1].numRearUnits++] = pDefs->CreateUnit(10, 2);
  groups[1].pRearUnits[groups[1].numRearUnits++] = pDefs->CreateUnit(10, 2);

  // pirates
  groups[2].pForwardUnits[groups[2].numForwardUnits++] = pDefs->CreateUnit(2, 3);
  groups[2].pForwardUnits[groups[2].numForwardUnits++] = pDefs->CreateUnit(12, 3);
  groups[2].pForwardUnits[groups[2].numForwardUnits++] = pDefs->CreateUnit(12, 3);

  groups[2].pRearUnits[groups[2].numRearUnits++] = pDefs->CreateUnit(13, 3);
  groups[2].pRearUnits[groups[2].numRearUnits++] = pDefs->CreateUnit(13, 3);

  // pharos
  groups[3].pForwardUnits[groups[3].numForwardUnits++] = pDefs->CreateUnit(3, 4);
  groups[3].pForwardUnits[groups[3].numForwardUnits++] = pDefs->CreateUnit(14, 4);
  groups[3].pForwardUnits[groups[3].numForwardUnits++] = pDefs->CreateUnit(14, 4);

  groups[3].pRearUnits[groups[3].numRearUnits++] = pDefs->CreateUnit(15, 4);
  groups[3].pRearUnits[groups[3].numRearUnits++] = pDefs->CreateUnit(15, 4);

  // vikings
  groups[4].pForwardUnits[groups[4].numForwardUnits++] = pDefs->CreateUnit(4, 5);
  groups[4].pForwardUnits[groups[4].numForwardUnits++] = pDefs->CreateUnit(16, 5);

  groups[4].pRearUnits[groups[4].numRearUnits++] = pDefs->CreateUnit(17, 5);
  groups[4].pRearUnits[groups[4].numRearUnits++] = pDefs->CreateUnit(17, 5);
  groups[4].pRearUnits[groups[4].numRearUnits++] = pDefs->CreateUnit(17, 5);

  // vikings
  groups[5].pForwardUnits[groups[5].numForwardUnits++] = pDefs->CreateUnit(5, 6);
  groups[5].pForwardUnits[groups[5].numForwardUnits++] = pDefs->CreateUnit(18, 6);
  groups[5].pForwardUnits[groups[5].numForwardUnits++] = pDefs->CreateUnit(18, 6);
  groups[5].pForwardUnits[groups[5].numForwardUnits++] = pDefs->CreateUnit(18, 6);
  groups[5].pForwardUnits[groups[5].numForwardUnits++] = pDefs->CreateUnit(18, 6);

  groups[5].pRearUnits[groups[5].numRearUnits++] = pDefs->CreateUnit(19, 6);
  groups[5].pRearUnits[groups[5].numRearUnits++] = pDefs->CreateUnit(19, 6);
  groups[5].pRearUnits[groups[5].numRearUnits++] = pDefs->CreateUnit(19, 6);
}

Battle::~Battle()
{
  MFMaterial_Destroy(pForeground);
  MFMaterial_Destroy(pBackground);
  if(pCastle)
    MFMaterial_Destroy(pCastle);
}

void Battle::Select()
{

}

int Battle::Update()
{
  if(MFInput_WasPressed(Key_1, IDD_Keyboard))
    races[0] = 0;
  if(MFInput_WasPressed(Key_2, IDD_Keyboard))
    races[0] = 1;
  if(MFInput_WasPressed(Key_3, IDD_Keyboard))
    races[0] = 2;
  if(MFInput_WasPressed(Key_4, IDD_Keyboard))
    races[0] = 3;
  if(MFInput_WasPressed(Key_5, IDD_Keyboard))
    races[0] = 4;
  if(MFInput_WasPressed(Key_6, IDD_Keyboard))
    races[0] = 5;

  if(MFInput_WasPressed(Key_Q, IDD_Keyboard))
    races[1] = 0;
  if(MFInput_WasPressed(Key_W, IDD_Keyboard))
    races[1] = 1;
  if(MFInput_WasPressed(Key_E, IDD_Keyboard))
    races[1] = 2;
  if(MFInput_WasPressed(Key_R, IDD_Keyboard))
    races[1] = 3;
  if(MFInput_WasPressed(Key_T, IDD_Keyboard))
    races[1] = 4;
  if(MFInput_WasPressed(Key_Y, IDD_Keyboard))
    races[1] = 5;

  return 0;
}

void Battle::Draw()
{
	MFView_Push();
	MFRect rect = { 0, 0, 1, 1 };
	MFView_SetOrtho(&rect);

  MFMaterial_SetMaterial(pBackground);
	MFPrimitive_DrawQuad(0, 0, 1, 0.5, MFVector::one, 0, 0, 1, 1);

  if(pCastle)
  {
 	  MFMaterial_SetMaterial(pCastle);
	  MFPrimitive_DrawQuad(0, 0, 1, 0.5, MFVector::one, 0, 0, 1, 1);
  }

  MFMaterial_SetMaterial(pForeground);
	MFPrimitive_DrawQuad(0, 0, 1, 1, MFVector::one, 0, 0, 1, 1);

  MFRenderer_ClearScreen(CS_ZBuffer | CS_Stencil);

  // render units
  MFDisplay_GetDisplayRect(&rect);
	MFView_SetOrtho(&rect);

  int rowSeparation[4];
  int centerLine = (int)(rect.height / 2);

  rowSeparation[0] = centerLine / (groups[races[0]].numForwardUnits + 1);
  rowSeparation[1] = centerLine / (groups[races[0]].numRearUnits + 1);
  rowSeparation[2] = centerLine / (groups[races[1]].numForwardUnits + 1);
  rowSeparation[3] = centerLine / (groups[races[1]].numRearUnits + 1);

  int horizCenter = (int)(rect.width / 2);

  for(int a=0; a<5; ++a)
  {
    if(a < groups[races[0]].numForwardUnits)
    {
      groups[races[0]].pForwardUnits[a]->Draw(horizCenter - 80 - 64, centerLine + (a+1)*rowSeparation[0] - 64);
    }

    if(a < groups[races[0]].numRearUnits)
    {
      groups[races[0]].pRearUnits[a]->Draw(horizCenter - 200 - 64, centerLine + (a+1)*rowSeparation[1] - 64);
    }

    if(a < groups[races[1]].numForwardUnits)
    {
      groups[races[1]].pForwardUnits[a]->Draw(horizCenter + 80, centerLine + (a+1)*rowSeparation[2] - 64, true);
    }

    if(a < groups[races[1]].numRearUnits)
    {
      groups[races[1]].pRearUnits[a]->Draw(horizCenter + 200, centerLine + (a+1)*rowSeparation[3] - 64, true);
    }
  }

  pDefs->DrawUnits();

  // time meters
  // damage indicators

  MFView_Pop();
}

void Battle::Deselect()
{

}

int Battle::UpdateInput()
{
  return 0;
}
