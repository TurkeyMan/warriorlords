#if !defined(_UNIT_BUTTON_H)
#define _UNIT_BUTTON_H

#include "UI/Widgets/HKWidgetButton.h"

class UnitButton : public HKWidgetButton
{
	friend class RendererUnitButton;
public:
	static HKWidget *Create(HKWidgetType *pType);
	static const char *TypeName() { return "UnitButton"; }

	UnitButton(HKWidgetType *pType);
	virtual	~UnitButton();

	void SetUnit(int unit) { this->unit = unit; }
	void SetUnitColour(const MFVector &colour) { unitColour = colour; }

	virtual void SetProperty(const char *pProperty, const char *pValue);
	virtual MFString GetProperty(const char *pProperty);

protected:
	MFVector unitColour;
	int unit;
};

class RendererUnitButton : public HKWidgetRenderer
{
public:
	static HKWidgetRenderer *Create(HKWidgetRendererType *pType);
protected:
	virtual void Render(const HKWidget &widget, const MFMatrix &worldTransform);
};

#endif