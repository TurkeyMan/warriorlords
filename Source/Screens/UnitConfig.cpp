#include "Warlords.h"
#include "UnitConfig.h"

#include "Fuji/MFFont.h"
#include "Fuji/MFRenderer.h"

void DrawHealthBar(int x, int y, int maxHealth, float currentHealth);

UnitConfig::UnitConfig(Game *pGame)
: Window(pGame, true)
, inventory(pGame)
{
	DivideRect_Vert(window, 128+10, margin, &top, &bottom, true);

	float texelCenter = MFRenderer_GetTexelCenterOffset() / 256.f;
	MFRect pos = { top.x + top.width - 64.f, top.y + top.height - 64.f, 64.f, 64.f };
	MFRect uvs = { 0.f + texelCenter, 0.25f + texelCenter, 0.25f, 0.25f };
	pInventory = Button::Create(pIcons, &pos, &uvs, MFVector::white);
	pInventory->SetClickCallback(MakeDelegate(this, &UnitConfig::ShowInventory));

	float height = 23.f;
	MFRect cbRect = { bottom.x + 40, bottom.y + 10, 200.f, height };
	pStrategySelect[0] = CheckBox::Create(&cbRect, "Attack Strongest", MFVector::yellow, 1, 0);
	pStrategySelect[0]->SetChangeCallback(MakeDelegate(this, &UnitConfig::SelectStrat));
	cbRect.y += height;
	pStrategySelect[1] = CheckBox::Create(&cbRect, "Attack Weakest", MFVector::yellow, 0, 1);
	pStrategySelect[1]->SetChangeCallback(MakeDelegate(this, &UnitConfig::SelectStrat));
	cbRect.y += height + 10;
	pStrategySelect[2] = CheckBox::Create(&cbRect, "Attack First Available", MFVector::yellow, 1, 2);
	pStrategySelect[2]->SetChangeCallback(MakeDelegate(this, &UnitConfig::SelectStrat));
	cbRect.x = bottom.x + 260;
	cbRect.y = bottom.y + 15;
	pStrategySelect[3] = CheckBox::Create(&cbRect, "Attack Any", MFVector::yellow, 1, 3);
	pStrategySelect[3]->SetChangeCallback(MakeDelegate(this, &UnitConfig::SelectStrat));
	cbRect.y += height;
	pStrategySelect[4] = CheckBox::Create(&cbRect, "Attack Melee", MFVector::yellow, 0, 4);
	pStrategySelect[4]->SetChangeCallback(MakeDelegate(this, &UnitConfig::SelectStrat));
	cbRect.y += height;
	pStrategySelect[5] = CheckBox::Create(&cbRect, "Attack Ranged", MFVector::yellow, 0, 5);
	pStrategySelect[5]->SetChangeCallback(MakeDelegate(this, &UnitConfig::SelectStrat));
}

UnitConfig::~UnitConfig()
{
}

bool UnitConfig::DrawContent()
{
	if(!bVisible)
		return false;

	if(inventory.Draw())
		return true;

	pGame->DrawLine(window.x + 16, bottom.y - 5, window.x + window.width - 16, bottom.y - 5);

	const UnitDefinitions *pDefs = pUnit->UnitDefs();
	const UnitDetails &details = pUnit->GetDetails();

	
	MFArray<UnitRender> units;
	units.push() = pUnit->Render(top.x + 32.f, top.y + 32.f);
	pGame->DrawUnits(units, 64.f, MFRenderer_GetTexelCenterOffset(), false, true);

	// do we want to see this?
//	DrawHealthBar((int)(unit.x + 32.f), (int)(unit.y + 32.f), pUnit->GetMaxHP(), pUnit->GetHealth());

	const char *attackIcon[] = { "¥", "¤" };

	MFFont *pFont = pGame->GetTextFont();
	int height = (int)MFFont_GetFontHeight(pFont);
	float tWidth = MFFont_GetStringWidth(pFont, pUnit->GetName().CStr(), (float)height);
	MFFont_BlitText(pFont, (int)top.x + ((int)top.width / 2) - (int)(tWidth*0.5f), (int)top.y + 5, MFVector::yellow, pUnit->GetName().CStr());
	MFFont_BlitTextf(pFont, (int)top.x + 133, (int)top.y + 5 + height, MFVector::white, "Type: %s", pDefs->GetArmourClassName(details.armour).CStr());
	MFFont_BlitTextf(pFont, (int)top.x + 133, (int)top.y + 5 + height*2, MFVector::white, "HP: %d/%d", (int)(pUnit->GetMaxHP() * pUnit->GetHealth()), pUnit->GetMaxHP());
	MFFont_BlitTextf(pFont, (int)top.x + 133, (int)top.y + 5 + height*3, MFVector::white, "Atk: %d - %d %s (%s%s)", (int)pUnit->GetMinDamage(), (int)pUnit->GetMaxDamage(), attackIcon[details.atkType], pUnit->AttackSpeedDescription(), pDefs->GetWeaponClassName(details.attack).CStr());
	MFFont_BlitTextf(pFont, (int)top.x + 133, (int)top.y + 5 + height*4, MFVector::white, "Mov: %g/%d%s%s", pUnit->MoveRemaining(), pUnit->GetMaxMovement(), details.movementClass == 2 ? " £" : "", details.movementClass > 0 ? MFStr(" (%s)", pDefs->GetMovementClassName(details.movementClass).CStr()) : "");

	MFFont_BlitTextf(pFont, (int)top.x + 320, (int)top.y + 5 + height, MFVector::white, "Victories: %d", pUnit->GetVictories());
	MFFont_BlitTextf(pFont, (int)top.x + 320, (int)top.y + 5 + height*2, MFVector::white, "Kills: %d", pUnit->GetKills());

	if(pUnit->IsHero() && pUnit->GetNumItems() > 0)
		pInventory->Draw();

	for(int a=0; a<6; ++a)
		pStrategySelect[a]->Draw();

	return true;
}

bool UnitConfig::HandleInputEvent(InputEvent ev, InputInfo &info)
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
			break;
	}

	return Window::HandleInputEvent(ev, info);
}

void UnitConfig::Show(Game *_pGame, Unit *_pUnit)
{
	Window::Show();

	pGame = _pGame;
	pUnit = _pUnit;

	BattlePlan *pPlan = pUnit->GetBattlePlan();
	pStrategySelect[0]->SetValue(pPlan->strength == TS_Strongest ? 1 : 0);
	pStrategySelect[1]->SetValue(pPlan->strength == TS_Weakest ? 1 : 0);
	pStrategySelect[2]->SetValue(pPlan->bAttackAvailable ? 1 : 0);
	pStrategySelect[3]->SetValue(pPlan->type == TT_Any ? 1 : 0);
	pStrategySelect[4]->SetValue(pPlan->type == TT_Melee ? 1 : 0);
	pStrategySelect[5]->SetValue(pPlan->type == TT_Ranged ? 1 : 0);

	if(pUnit->IsHero() && pUnit->GetNumItems() > 0)
		pInputManager->PushReceiver(pInventory);

	for(int a=0; a<6; ++a)
		pInputManager->PushReceiver(pStrategySelect[a]);
}

void UnitConfig::Hide()
{
	Window::Hide();
}

void UnitConfig::ShowInventory(int button, int buttonID)
{
	inventory.Show(pGame, pUnit);
}

void UnitConfig::SelectStrat(int value, int buttonID)
{
	BattlePlan *pPlan = pUnit->GetBattlePlan();

	switch(buttonID)
	{
		case 0:
		case 1:
			pPlan->strength = (TargetStrength)buttonID;

			if(value == 0)
				pStrategySelect[buttonID]->SetValue(1);
			else
				pStrategySelect[1 - buttonID]->SetValue(0);
			break;

		case 2:
			pPlan->bAttackAvailable = !!value;
			break;

		case 3:
		case 4:
		case 5:
			pPlan->type = (TargetType)(buttonID - 3);

			if(value == 0)
			{
				pStrategySelect[buttonID]->SetValue(1);
			}
			else
			{
				for(int a=3; a<6; ++a)
				{
					if(buttonID != a)
						pStrategySelect[a]->SetValue(0);
				}
			}
			break;
	}
}
