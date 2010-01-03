#include "Warlords.h"
#include "Display.h"

#include "MFDisplay.h"

static DisplayOrientation gDisplayOrientation = DO_Normal;

void SetDisplayOrientation(DisplayOrientation displayOrientation)
{
	gDisplayOrientation = displayOrientation;
}

void GetInputMatrix(MFMatrix *pMatrix)
{
	MFRect dispRect;
	MFDisplay_GetDisplayRect(&dispRect);

	pMatrix->SetIdentity();

	switch(gDisplayOrientation)
	{
		case DO_90CW:
			pMatrix->SetXAxis3(-MFVector::up);
			pMatrix->SetYAxis3(MFVector::right);
			pMatrix->SetTrans3(MakeVector(0, dispRect.width, 0));
			break;
		case DO_90CCW:
			pMatrix->SetXAxis3(MFVector::up);
			pMatrix->SetYAxis3(-MFVector::right);
			pMatrix->SetTrans3(MakeVector(dispRect.height, 0, 0));
			break;
		case DO_180:
			pMatrix->SetXAxis3(-MFVector::right);
			pMatrix->SetYAxis3(-MFVector::up);
			pMatrix->SetTrans3(MakeVector(dispRect.width, dispRect.height, 0));
			break;
		case DO_HFlip:
			pMatrix->SetXAxis3(-MFVector::right);
			pMatrix->SetTrans3(MakeVector(dispRect.width, 0, 0));
			break;
		case DO_VFlip:
			pMatrix->SetYAxis3(-MFVector::up);
			pMatrix->SetTrans3(MakeVector(0, dispRect.height, 0));
			break;
		case DO_Normal:
		default:
			break;
	}
}

void GetOrthoMatrix(MFMatrix *pMatrix, bool bUnitScale)
{
	MFRect rect = { 0, 0, 1, 1 };

	if(!bUnitScale)
		MFDisplay_GetDisplayRect(&rect);

	pMatrix->SetIdentity();

	switch(gDisplayOrientation)
	{
		case DO_90CW:
			pMatrix->SetXAxis3(MFVector::up * (2.f/-rect.height));
			pMatrix->SetYAxis3(MFVector::right * (2.f/-rect.width));
			pMatrix->SetTrans3(MakeVector(1, 1, 0));
			break;
		case DO_90CCW:
			pMatrix->SetXAxis3(MFVector::up * (2.f/rect.height));
			pMatrix->SetYAxis3(MFVector::right * (2.f/rect.width));
			pMatrix->SetTrans3(MakeVector(-1, -1, 0));
			break;
		case DO_180:
			pMatrix->SetXAxis3(MFVector::right * (2.f/-rect.width));
			pMatrix->SetYAxis3(MFVector::up * (2.f/rect.height));
			pMatrix->SetTrans3(MakeVector(1, -1, 0));
			break;
		case DO_HFlip:
			pMatrix->SetXAxis3(MFVector::right * (2.f/-rect.width));
			pMatrix->SetYAxis3(MFVector::up * (2.f/-rect.height));
			pMatrix->SetTrans3(MakeVector(1, 1, 0));
			break;
		case DO_VFlip:
			pMatrix->SetXAxis3(MFVector::right * (2.f/rect.width));
			pMatrix->SetYAxis3(MFVector::up * (2.f/rect.height));
			pMatrix->SetTrans3(MakeVector(-1, -1, 0));
			break;
		case DO_Normal:
		default:
			pMatrix->SetXAxis3(MFVector::right * (2.f/rect.width));
			pMatrix->SetYAxis3(MFVector::up * (2.f/-rect.height));
			pMatrix->SetTrans3(MakeVector(-1, 1, 0));
			break;
	}
}

void GetDisplayRect(MFRect *pRect)
{
	MFDisplay_GetDisplayRect(pRect);

	if(gDisplayOrientation == DO_90CW || gDisplayOrientation == DO_90CCW)
	{
		float t = pRect->height;
		pRect->height = pRect->width;
		pRect->width = t;
	}
}

MFVector CorrectPosition(float x, float y)
{
	MFMatrix inputMat;
	GetInputMatrix(&inputMat);
	return inputMat.TransformVectorH(MakeVector(x, y, 0.f, 1.f));
}
