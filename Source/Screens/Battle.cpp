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

struct Group
{
  int race;

  Unit *pForwardUnits[5];
  Unit *pRearUnits[5];

  int numForwardUnits;
  int numRearUnits;
};

static Group groups[8];
static int races[2] = { 0, 1 };

Battle::Battle(GameData *pGameData)
{
  pUnitDefs = pGameData->GetUnitDefs();

  // load lots of battle related stuff
  pCooldownHead = pCooldownTail = NULL;
  pActionHead = pActionTail = NULL;
  cooldownCount = 0;
}

Battle::~Battle()
{
}

void Battle::Begin(Group *pGroup1, Group *pGroup2, const char *_pForeground, const char *_pBackground, const char *_pCastle)
{
  pIcons = MFMaterial_Create("BattleIcons");

  pForeground = MFMaterial_Create(MFStr("Battle/fg-%s", _pForeground));
  pBackground = MFMaterial_Create(MFStr("Battle/bg-%s", _pBackground));
  pCastle = _pCastle ? MFMaterial_Create(MFStr("Battle/castle-%s", _pCastle)) : NULL;

  // humans
  groups[0].race = 1;
  groups[0].pForwardUnits[groups[0].numForwardUnits++] = pUnitDefs->CreateUnit(0, 1);
  groups[0].pForwardUnits[groups[0].numForwardUnits++] = pUnitDefs->CreateUnit(8, 1);
  groups[0].pForwardUnits[groups[0].numForwardUnits++] = pUnitDefs->CreateUnit(9, 1);
  groups[0].pForwardUnits[groups[0].numForwardUnits++] = pUnitDefs->CreateUnit(12, 1);

  groups[0].pRearUnits[groups[0].numRearUnits++] = pUnitDefs->CreateUnit(24, 1);
  groups[0].pRearUnits[groups[0].numRearUnits++] = pUnitDefs->CreateUnit(24, 1);
  groups[0].pRearUnits[groups[0].numRearUnits++] = pUnitDefs->CreateUnit(24, 1);

  // elves
  groups[1].race = 2;
  groups[1].pForwardUnits[groups[1].numForwardUnits++] = pUnitDefs->CreateUnit(1, 2);
  groups[1].pForwardUnits[groups[1].numForwardUnits++] = pUnitDefs->CreateUnit(11, 2);

  groups[1].pRearUnits[groups[1].numRearUnits++] = pUnitDefs->CreateUnit(10, 2);
  groups[1].pRearUnits[groups[1].numRearUnits++] = pUnitDefs->CreateUnit(10, 2);
  groups[1].pRearUnits[groups[1].numRearUnits++] = pUnitDefs->CreateUnit(10, 2);
  groups[1].pRearUnits[groups[1].numRearUnits++] = pUnitDefs->CreateUnit(10, 2);
  groups[1].pRearUnits[groups[1].numRearUnits++] = pUnitDefs->CreateUnit(10, 2);

  // pirates
  groups[2].race = 3;
  groups[2].pForwardUnits[groups[2].numForwardUnits++] = pUnitDefs->CreateUnit(2, 3);
  groups[2].pForwardUnits[groups[2].numForwardUnits++] = pUnitDefs->CreateUnit(12, 3);
  groups[2].pForwardUnits[groups[2].numForwardUnits++] = pUnitDefs->CreateUnit(12, 3);

  groups[2].pRearUnits[groups[2].numRearUnits++] = pUnitDefs->CreateUnit(13, 3);
  groups[2].pRearUnits[groups[2].numRearUnits++] = pUnitDefs->CreateUnit(13, 3);

  // pharos
  groups[3].race = 4;
  groups[3].pForwardUnits[groups[3].numForwardUnits++] = pUnitDefs->CreateUnit(3, 4);
  groups[3].pForwardUnits[groups[3].numForwardUnits++] = pUnitDefs->CreateUnit(14, 4);
  groups[3].pForwardUnits[groups[3].numForwardUnits++] = pUnitDefs->CreateUnit(14, 4);

  groups[3].pRearUnits[groups[3].numRearUnits++] = pUnitDefs->CreateUnit(15, 4);
  groups[3].pRearUnits[groups[3].numRearUnits++] = pUnitDefs->CreateUnit(15, 4);

  // vikings
  groups[4].race = 5;
  groups[4].pForwardUnits[groups[4].numForwardUnits++] = pUnitDefs->CreateUnit(4, 5);
  groups[4].pForwardUnits[groups[4].numForwardUnits++] = pUnitDefs->CreateUnit(16, 5);

  groups[4].pRearUnits[groups[4].numRearUnits++] = pUnitDefs->CreateUnit(17, 5);
  groups[4].pRearUnits[groups[4].numRearUnits++] = pUnitDefs->CreateUnit(17, 5);
  groups[4].pRearUnits[groups[4].numRearUnits++] = pUnitDefs->CreateUnit(17, 5);

  // vikings
  groups[5].race = 6;
  groups[5].pForwardUnits[groups[5].numForwardUnits++] = pUnitDefs->CreateUnit(5, 6);
  groups[5].pForwardUnits[groups[5].numForwardUnits++] = pUnitDefs->CreateUnit(18, 6);
  groups[5].pForwardUnits[groups[5].numForwardUnits++] = pUnitDefs->CreateUnit(18, 6);
  groups[5].pForwardUnits[groups[5].numForwardUnits++] = pUnitDefs->CreateUnit(18, 6);
  groups[5].pForwardUnits[groups[5].numForwardUnits++] = pUnitDefs->CreateUnit(18, 6);

  groups[5].pRearUnits[groups[5].numRearUnits++] = pUnitDefs->CreateUnit(19, 6);
  groups[5].pRearUnits[groups[5].numRearUnits++] = pUnitDefs->CreateUnit(19, 6);
  groups[5].pRearUnits[groups[5].numRearUnits++] = pUnitDefs->CreateUnit(19, 6);


  // initialise the battle units
  armies[0].pGroup = &groups[races[0]];
  armies[0].numUnits = 0;
  armies[1].pGroup = &groups[races[1]];
  armies[1].numUnits = 0;

  MFRect rect;
  MFDisplay_GetDisplayRect(&rect);

  int centerLine = (int)(rect.height / 2);
  int centerField = centerLine + centerLine / 2;
  int horizSpacing = (int)(rect.width / 32);

  int rowSeparation[2][2];
  rowSeparation[0][0] = (centerLine + 64) / (groups[races[0]].numForwardUnits + 1);
  rowSeparation[0][1] = (centerLine + 64) / (groups[races[0]].numRearUnits + 1);
  rowSeparation[1][0] = (centerLine + 64) / (groups[races[1]].numForwardUnits + 1);
  rowSeparation[1][1] = (centerLine + 64) / (groups[races[1]].numRearUnits + 1);

  for(int a=0; a<2; ++a)
  {
    for(int b=0; b<5; ++b)
    {
      if(b < groups[races[a]].numForwardUnits)
      {
        BattleUnit *pUnit = &armies[a].units[armies[a].numUnits++];
        pUnit->Init(groups[races[a]].pForwardUnits[b]);
        pUnit->army = a;
        pUnit->row = 0;

        int y = centerLine - 32 + (b+1)*rowSeparation[a][0];
        int offset = (a == 0 ? centerField - y : y - centerField);
        pUnit->SetPos(horizSpacing*(a == 0 ? 11 : 21) + int(offset * 0.1f) - 32, y - 64 + 4);

        StartCooldown(pUnit);
      }
      if(b < groups[races[a]].numRearUnits)
      {
        BattleUnit *pUnit = &armies[a].units[armies[a].numUnits++];
        pUnit->Init(groups[races[a]].pRearUnits[b]);
        pUnit->army = a;
        pUnit->row = 1;

        int y = centerLine - 32 + (b+1)*rowSeparation[a][1];
        int offset = (a == 0 ? centerField - y : y - centerField);
        pUnit->SetPos(horizSpacing*(a == 0 ? 4 : 28) + int(offset * 0.3f) - 32, y - 64 + 4);

        StartCooldown(pUnit);
      }
    }
  }

  SetNext(this);
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

  for(int a=0; a<2; ++a)
  {
    for(int b=0; b<armies[a].numUnits; ++b)
    {
      BattleUnit &unit = armies[a].units[b];
      if(unit.stateTime)
      {
        unit.stateTime -= MFSystem_TimeDelta();

        if(unit.state == US_Engaging)
        {
          if(unit.stateTime <= 0.f)
          {
            unit.curX = unit.posX;
            unit.curY = unit.posY;

            UnitDetails *pDetails = unit.pUnit->GetDetails();
            if(unit.pTarget->pUnit->Damage((MFRand() % (pDetails->attackMax - pDetails->attackMin + 1)) + pDetails->attackMin) == 0)
            {
              if(unit.pTarget->state == US_Waiting)
                EndWaiting(unit.pTarget);
              else if(unit.pTarget->state == US_Cooldown)
                StopCooldown(unit.pTarget);

              unit.pTarget->state = US_Dying;
            }

            unit.pTarget->bEngaged = false;
            unit.pTarget = NULL;

            StartCooldown(&unit);
          }
          else
          {
            float speed = unit.pUnit->GetDetails()->attackSpeed;
            float phaseFactor = speed * 0.25f;
            int phase = (int)((speed - unit.stateTime) / phaseFactor);
            float phaseTime = fmodf(speed - unit.stateTime, phaseFactor) / phaseFactor;

            if(1) // melee
            {
              switch(phase)
              {
                case 0:
                  unit.curX = unit.posX;
                  unit.curY = unit.posY + (int)MFClamp(-10.f, sinf(phaseTime * 4.f * MFPI) * -10.f, 0.f);
                  break;
                case 1:
                  unit.curX = unit.posX + (int)((unit.pTarget->posX + (unit.army == 0 ? -64 : 64 ) - unit.posX)*phaseTime);
                  unit.curY = unit.posY + (int)((unit.pTarget->posY - unit.posY)*phaseTime);
                  break;
                case 2:
                  unit.curX = unit.pTarget->posX + (unit.army == 0 ? -64 : 64 );
                  unit.curY = unit.pTarget->posY;
                  break;
                case 3:
                  unit.curX = unit.posX + (int)((unit.pTarget->posX + (unit.army == 0 ? -64 : 64 ) - unit.posX)*(1.f-phaseTime));
                  unit.curY = unit.posY + (int)((unit.pTarget->posY - unit.posY)*(1.f-phaseTime));
                  break;
              }
            }
            else if(0) // ranged
            {
              // shoot a projectile
            }
          }
        }
      }
    }
  }

  BattleUnit *pUnit;
  while(pUnit = CheckCooldown())
    AddWaiting(pUnit);

  pUnit = pActionHead;
  while(pUnit)
  {
    if(!pUnit->bEngaged && pUnit->damageIndicatorTime == 0.f)
    {
      // pick target...
      BattleUnit *pTarget = NULL;
      Army &army = armies[1-pUnit->army];
      for(int a=0; a<army.numUnits; ++a)
      {
        BattleUnit *pT = &army.units[a];
        if(!pT->bEngaged && !pT->damageIndicatorTime && pT->pUnit->GetHealth() && (pT->state == US_Cooldown || pT->state == US_Waiting))
        {
          pTarget = pT;
          break;
        }
      }

      if(pTarget)
      {
        // attack!
        pUnit->pTarget = pTarget;
        pTarget->bEngaged = true;
        pUnit->state = US_Engaging;
        pUnit->stateTime = pUnit->pUnit->GetDetails()->attackSpeed;

        // unlink
        EndWaiting(pUnit);
      }
    }

    pUnit = pUnit->pNext;
  }

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

  for(int a=0; a<2; ++a)
  {
    for(int b=0; b<armies[a].numUnits; ++b)
    {
      BattleUnit &unit = armies[a].units[b];
      unit.pUnit->Draw((float)unit.curX, (float)unit.curY, a == 1);
    }
  }

  pUnitDefs->DrawUnits();

  // health bars + damage indicators
  for(int a=0; a<2; ++a)
  {
    for(int b=0; b<armies[a].numUnits; ++b)
    {
      BattleUnit &unit = armies[a].units[b];
      float healthSize = MFClamp(12, unit.pUnit->GetDetails()->life, 32) * 2.f;
      float health = unit.pUnit->GetHealth() * healthSize;
      MFPrimitive_DrawUntexturedQuad(float(unit.curX + 31 - healthSize*0.5f), float(unit.curY - 8), healthSize + 2.f, 4.f, MFVector::black);
      MFPrimitive_DrawUntexturedQuad(float(unit.curX + 32 - healthSize*0.5f), float(unit.curY - 7), health, 2.f, MFVector::red);

      if(unit.damageIndicatorTime)
      {
        // draw damage indicator with a little bouncey thing...
        //...
      }
    }
  }

  // timeline
  MFMaterial_SetMaterial(pIcons);

  int numQuads = 3 + cooldownCount;
  MFPrimitive_BeginBlitter(numQuads);

  int timelineY = (int)(rect.height / 6) - 16;

  int left = (int)(rect.width * 0.06f);
  int right = (int)rect.width - left;
  int width = (int)rect.width - left*2;

  MFPrimitive_Blit(left, timelineY, 0, 0, 32, 32);
  MFPrimitive_StretchBlit(left + 32, timelineY, width - 64, 32, 32, 0, 32, 32);
  MFPrimitive_Blit(right - 32, timelineY, 64, 0, 32, 32);

  // plot each unit on the timeline
  BattleUnit *pUnit = pCooldownTail;
  while(pUnit)
  {
    int offset = pUnit->pUnit->GetRace() == armies[0].pGroup->race ? -1 : 1;
    int x = left + (int)(width*(pUnit->stateTime/7.0f));
    int y = timelineY + pUnit->cooldownOffset + offset*20;

    MFSetColour(MakeVector(pUnit->colour, MFClamp(0.f, (-pUnit->stateTime * 3.f) + 21, 1.f)));
    MFPrimitive_Blit(x, y, 0, 32, 32, 32);
    pUnit->pUnit->Draw(x-16.f, y-16.f);

    // draw the unit head here for correct overlap behaviour
//    pUnitDefs->DrawUnits(0.5f, true);

    pUnit = pUnit->pPrev;
  }

  MFPrimitive_EndBlitter();

  // draw deferred unit heads
  pUnitDefs->DrawUnits(0.5f, true);

  MFView_Pop();
}

void Battle::Deselect()
{
  MFMaterial_Destroy(pForeground);
  MFMaterial_Destroy(pBackground);
  if(pCastle)
    MFMaterial_Destroy(pCastle);
}

int Battle::UpdateInput()
{
  return 0;
}

void Battle::StartCooldown(BattleUnit *pUnit)
{
  pUnit->state = US_Cooldown;
  pUnit->stateTime = pUnit->pUnit->GetDetails()->cooldown;

  pUnit->cooldownOffset = 0; // y offset

  BattleUnit *pU = pCooldownTail;
  while(pU)
  {
    if(pU->state == US_Cooldown && pU->stateTime <= pUnit->stateTime)
    {
      pUnit->pPrev = pU;
      pUnit->pNext = pU->pNext;
      if(pU->pNext)
        pU->pNext->pPrev = pUnit;
      else
        pCooldownTail = pUnit;
      pU->pNext = pUnit;
      goto done;
    }

    pU = pU->pPrev;
  }

  // we must be the first unit in the list
  pUnit->pPrev = NULL;
  pUnit->pNext = pCooldownHead;
  if(pCooldownHead)
    pCooldownHead->pPrev = pUnit;
  else
    pCooldownTail = pUnit;
  pCooldownHead = pUnit;

done:
  ++cooldownCount;
}

void Battle::StopCooldown(BattleUnit *pUnit)
{
  if(pCooldownHead == pUnit)
    pCooldownHead = pUnit->pNext;
  if(pCooldownTail == pUnit)
    pCooldownTail = pUnit->pPrev;
  if(pUnit->pNext)
    pUnit->pNext->pPrev = pUnit->pPrev;
  if(pUnit->pPrev)
    pUnit->pPrev->pNext = pUnit->pNext;
  pUnit->pPrev = pUnit->pNext = NULL;
  --cooldownCount;
}

Battle::BattleUnit *Battle::CheckCooldown()
{
  BattleUnit *pGet = NULL;

  if(pCooldownHead && pCooldownHead->stateTime <= 0.f)
  {
    pGet = pCooldownHead;

    pCooldownHead = pCooldownHead->pNext;
    if(pCooldownHead)
      pCooldownHead->pPrev = NULL;
    else
      pCooldownTail = NULL;

    pGet->pNext = pGet->pPrev = NULL;
    pGet->stateTime = 0.f;

    --cooldownCount;
  }

  return pGet;
}

void Battle::AddWaiting(BattleUnit *pUnit)
{
  pUnit->state = US_Waiting;
  pUnit->stateTime = 0.f;

  pUnit->pPrev = pActionTail;
  pUnit->pNext = NULL;
  if(pActionTail)
    pActionTail->pNext = pUnit;
  else
    pActionHead = pUnit;
  pActionTail = pUnit;
}

void Battle::EndWaiting(BattleUnit *pUnit)
{
  if(pActionHead == pUnit)
    pActionHead = pUnit->pNext;
  if(pActionTail == pUnit)
    pActionTail = pUnit->pPrev;
  if(pUnit->pNext)
    pUnit->pNext->pPrev = pUnit->pPrev;
  if(pUnit->pPrev)
    pUnit->pPrev->pNext = pUnit->pNext;
  pUnit->pPrev = pUnit->pNext = NULL;
}

void Battle::BattleUnit::Init(Unit *_pUnit)
{
  pUnit = _pUnit;

  state = US_Cooldown;
  stateTime = pUnit->GetDetails()->cooldown;

  pTarget = NULL;
  bEngaged = false;
  damageIndicatorTime = 0.f;

  posX = posY = 0;
  curX = curY = 0;
  cooldownOffset = 0;

  colour = pUnit->GetColour();

  pPrev = pNext = NULL;
}

void Battle::BattleUnit::SetPos(int x, int y)
{
  posX = curX = x;
  posY = curY = y;
}
