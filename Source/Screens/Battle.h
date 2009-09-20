#if !defined(_BATTLE_H)
#define _BATTLE_H

#include "MFMaterial.h"

#include "Screen.h"
#include "Unit.h"

struct Group;

class Battle : public Screen
{
public:
	Battle(Group *pGroup1, Group *pGroup2, const char *pForeground, const char *pBackground, const char *pCastle);
	virtual ~Battle();

	virtual void Select();
	virtual int Update();
	virtual void Draw();
	virtual void Deselect();

	virtual int UpdateInput();

protected:
  MFMaterial *pForeground;
  MFMaterial *pBackground;
  MFMaterial *pCastle;
};

#endif
