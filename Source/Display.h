#if !defined(_DISPLAY_H)
#define _DISPLAY_H

enum DisplayOrientation
{
	DO_Normal,
	DO_90CW,
	DO_90CCW,
	DO_180,
	DO_HFlip,
	DO_VFlip,

	DO_Max,
	DO_ForceInt = 0x7FFFFFFF
};

void SetDisplayOrientation(DisplayOrientation displayOrientation);

void GetInputMatrix(MFMatrix *pMatrix);
void GetOrthoMatrix(MFMatrix *pMatrix, bool bUnitScale = false);

void GetDisplayRect(MFRect *pRect);

MFVector CorrectPosition(float x, float y);

#endif
