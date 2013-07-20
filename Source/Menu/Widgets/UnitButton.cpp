#include "Warlords.h"
#include "Haku/UI/HKUI.h"

#include "UnitButton.h"

#include "Fuji/MFRenderer.h"
#include "Fuji/MFMaterial.h"
#include "Fuji/MFPrimitive.h"

HKWidget *UnitButton::Create(HKWidgetType *pType)
{
	return new UnitButton(pType);
}


UnitButton::UnitButton(HKWidgetType *pType)
: HKWidgetButton(pType)
{
	unit = -1;
}

UnitButton::~UnitButton()
{
}

void UnitButton::SetProperty(const char *pProperty, const char *pValue)
{
	if(!MFString_CaseCmp(pProperty, "unit"))
		SetUnit(MFString_AsciiToInteger(pValue));
	else
		HKWidgetButton::SetProperty(pProperty, pValue);
}

MFString UnitButton::GetProperty(const char *pProperty)
{
	if(!MFString_CaseCmp(pProperty, "unit"))
		return MFString::Format("%d", unit);
	return HKWidgetLabel::GetProperty(pProperty);
}

HKWidgetRenderer *RendererUnitButton::Create(HKWidgetRendererType *pType)
{
	return new RendererUnitButton();
}

void RendererUnitButton::Render(const HKWidget &widget, const MFMatrix &worldTransform)
{
	HKWidgetRenderer::Render(widget, worldTransform);

	UnitButton &button = (UnitButton&)widget;

	if(button.unit < 0)
		return;

	UnitDefinitions *pUnitDefs = Game::GetCurrent()->GetUnitDefs();

	MFMaterial *pUnitMat = pUnitDefs->GetUnitMaterial();
	float texelCenter = MFRenderer_GetTexelCenterOffset();

	MFRect uvs;
	pUnitDefs->GetUnitUVs(button.unit, false, &uvs, texelCenter);

	// draw the unit...
	MFVector widgetColour = widget.GetColour();
	MFVector size = widget.GetSize();
	size.x -= padding.x + padding.z;
	size.y -= padding.y + padding.w;

	MFMaterial_SetMaterial(pUnitMat);

	float offset = 0;
	float tc = MFRenderer_GetTexelCenterOffset();
	if(tc > 0.f)
	{
		if(size.x == texWidth && size.y == texHeight)
			offset = tc;
	}

	MFPrimitive_DrawQuad(padding.x - offset, padding.y - offset, size.x, size.y, button.unitColour*widgetColour, uvs.x, uvs.y, uvs.x + uvs.width, uvs.y + uvs.height, worldTransform);
}
