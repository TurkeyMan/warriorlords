#include "Warlords.h"
#include "CastleConfig.h"

#include "MFFont.h"
#include "MFRenderer.h"

#include "stdio.h"

CastleConfig::CastleConfig()
: Window(true)
{
	MFRect body, upperBody;
	DivideRect_Vert(window, MFFont_GetFontHeight(pFont) + margin*2, 0.f, &title, &body, true);
	DivideRect_Vert(body, 64*2 + margin*6, 0.f, &upperBody, &lower, true);
	DivideRect_Horiz(upperBody, 64*2 + margin*6, 0.f, &units, &right, true);
	AdjustRect_Margin(&title, margin);
	AdjustRect_Margin(&units, margin);
	AdjustRect_Margin(&right, margin);
	AdjustRect_Margin(&lower, margin);

	MFRect button, uvs = { 0, 0, 1, 1 };
	button.width = 64.f; button.height = 64.f;

	button.x = units.x + 5.f; button.y = units.y + 5.f;
	pBuildUnits[0] = Button::Create(NULL, &button, &uvs, MFVector::one, 0);
	pBuildUnits[0]->SetClickCallback(MakeDelegate(this, &CastleConfig::SelectUnit));
	button.x = units.x + units.width - 64.f - 5.f; button.y = units.y + 5.f;
	pBuildUnits[1] = Button::Create(NULL, &button, &uvs, MFVector::one, 1);
	pBuildUnits[1]->SetClickCallback(MakeDelegate(this, &CastleConfig::SelectUnit));
	button.x = units.x + 5.f; button.y = units.y + units.height - 64.f - 5.f;
	pBuildUnits[2] = Button::Create(NULL, &button, &uvs, MFVector::one, 2);
	pBuildUnits[2]->SetClickCallback(MakeDelegate(this, &CastleConfig::SelectUnit));
	button.x = units.x + units.width - 64.f - 5.f; button.y = units.y + units.height - 64.f - 5.f;
	pBuildUnits[3] = Button::Create(NULL, &button, &uvs, MFVector::one, 3);
	pBuildUnits[3]->SetClickCallback(MakeDelegate(this, &CastleConfig::SelectUnit));
}

CastleConfig::~CastleConfig()
{
}

bool CastleConfig::DrawContent()
{
	Game *pGame = Game::GetCurrent();
	pGame->DrawLine(lower.x + 5.f, lower.y-4.f, lower.x + lower.width - 5.f, lower.y-4.f);
	pGame->DrawLine(right.x - 4.f, right.y + 5.f, right.x - 4.f, right.y + right.height - 5.f);

	int height = (int)MFFont_GetFontHeight(pFont);

	MFFont_BlitTextf(pFont, (int)title.x + 5, (int)title.y, MFVector::yellow, pCastle->details.name);

	int building = pCastle->GetBuildUnit();
	if(building > -1)
	{
		UnitDefinitions *pUnitDefs = pGame->GetUnitDefs();
		UnitDetails *pDetails = pUnitDefs->GetUnitDetails(building);

		MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 5, MFVector::white, pDetails->pName);
		if(pDetails->type == UT_Vehicle)
		{
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 9 + height, MFVector::white, "Mov: %d%s", pDetails->movement, pDetails->movementClass > 0 ? MFStr(" (%s)", pUnitDefs->GetMovementClassName(pDetails->movementClass)) : "");
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 8 + height*2, MFVector::white, "Turns: %d", pCastle->GetBuildTime());
		}
		else
		{
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 9 + height, MFVector::white, "Type: %s", pUnitDefs->GetArmourClassName(pDetails->armour));
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 8 + height*2, MFVector::white, "Atk: %d - %d (%s%s %s)", pDetails->attackMin, pDetails->attackMax, pDetails->AttackSpeedDescription(), pUnitDefs->GetWeaponClassName(pDetails->attack), pUnitDefs->GetAttackTypeName(pDetails->atkType));
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 7 + height*3, MFVector::white, "Mov: %d%s", pDetails->movement, pDetails->movementClass > 0 ? MFStr(" (%s)", pUnitDefs->GetMovementClassName(pDetails->movementClass)) : "");
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 6 + height*4, MFVector::white, "Turns: %d", pCastle->GetBuildTime());
		}
	}

	for(int a=0; a<numBuildUnits; ++a)
		pBuildUnits[a]->Draw();

	return true;
}

bool CastleConfig::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	// HACK: right click returns
	if(info.buttonID == 1 && ev == IE_Tap)
	{
		Hide();
		return true;
	}

	return Window::HandleInputEvent(ev, info);
}

void CastleConfig::Show(Castle *pCastle)
{
	Window::Show();

	this->pCastle = pCastle;

	UnitDefinitions *pUnitDefs = pCastle->pUnitDefs;
	Game *pGame = pUnitDefs->GetGame();

	MFMaterial *pUnitMat = pUnitDefs->GetUnitMaterial();
	float texelCenter = MFRenderer_GetTexelCenterOffset();

	numBuildUnits = pCastle->details.numBuildUnits;

	int a = 0;
	for(; a<numBuildUnits; ++a)
	{
		MFRect uvs;
		int unit = pCastle->details.buildUnits[a].unit;
		pUnitDefs->GetUnitUVs(pCastle->details.buildUnits[a].unit, false, &uvs, texelCenter);
		pBuildUnits[a]->SetImage(pUnitMat, &uvs, pGame->GetPlayerColour(pCastle->player));

		if(a == 2)
		{
			MFRect pos;
			pos.x = units.x + 5.f;
			pos.y = units.y + units.height - 64.f - 5.f;
			pos.width = pos.height = 64.f;

			if(numBuildUnits == 3)
			{
				float ratio = uvs.width/uvs.height;
				float width = 64.f * ratio;
				pos.x = MFFloor(units.x + units.width*0.5f - width*0.5f);
				pos.width = MFFloor(pos.width * ratio);
			}

			pBuildUnits[a]->SetPos(&pos);
		}

		pBuildUnits[a]->SetOutline(true, pCastle->nextBuild == a ? MFVector::blue : MFVector::white);

		pInputManager->PushReceiver(pBuildUnits[a]);
	}

	Unit *pHero = pGame->GetPlayerHero(pCastle->GetPlayer());
	if(a < 4 && pHero->IsDead())
	{
		int unit = Game::GetCurrent()->GetPlayerRace(pCastle->GetPlayer())-1;
		MFRect uvs;
		pUnitDefs->GetUnitUVs(unit, false, &uvs, texelCenter);
		pBuildUnits[a]->SetImage(pUnitMat, &uvs, pGame->GetPlayerColour(pCastle->player));

		UnitDetails *pDetails = pHero->GetDetails();
		pCastle->details.buildUnits[a].unit = unit;
		pCastle->details.buildUnits[a].buildTime = pDetails->buildTime;

		if(a == 2)
		{
			float ratio = uvs.width/uvs.height;
			float width = 64.f * ratio;

			MFRect pos;
			pos.x = MFFloor(units.x + units.width*0.5f - width*0.5f);
			pos.y = units.y + units.height - 64.f - 5.f;
			pos.width = pos.height = 64.f;

			pBuildUnits[a]->SetPos(&pos);
		}

		pBuildUnits[a]->SetOutline(true, pCastle->nextBuild == a ? MFVector::blue : MFVector::white);

		pInputManager->PushReceiver(pBuildUnits[a]);

		++numBuildUnits;
	}
}

void CastleConfig::Hide()
{
	Window::Hide();
}

void CastleConfig::SelectUnit(int button, int buttonID)
{
	for(int a=0; a<numBuildUnits; ++a)
		pBuildUnits[a]->SetOutline(true, buttonID == a ? MFVector::blue : MFVector::white);

	pCastle->SetBuildUnit(buttonID);
}
