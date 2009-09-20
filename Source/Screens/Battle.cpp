#include "Warlords.h"
#include "Battle.h"

#include "MFInput.h"
#include "MFSystem.h"
#include "MFPrimitive.h"
#include "MFFont.h"
#include "MFMaterial.h"
#include "MFView.h"

#include "Path.h"

Battle::Battle(Group *pGroup1, Group *pGroup2, const char *_pForeground, const char *_pBackground, const char *_pCastle)
{
  pForeground = MFMaterial_Create(MFStr("Battle/fg-%s", _pForeground));
  pBackground = MFMaterial_Create(MFStr("Battle/bg-%s", _pBackground));
  pCastle = _pCastle ? MFMaterial_Create(MFStr("Battle/castle-%s", _pCastle)) : NULL;
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

  // render units
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
