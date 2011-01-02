#include "Warlords.h"
#include "InputHandler.h"
#include "Display.h"

#include "MFSystem.h"

extern bool gAppHasFocus;

InputManager::InputManager()
{
	MFZeroMemory(contacts, sizeof(contacts));
	MFZeroMemory(bCurrentContacts, sizeof(bCurrentContacts));

	for(int a=0; a<16; ++a)
	{
		touchContacts[a] = -1;

		mouseContacts[a] = -1;
		for(int b=0; b<Mouse_MaxButtons; ++b)
			mouseButtonContacts[a][b] = -1;
	}

	dragThreshold = 16.f;
	pInputStack = NULL;
	pExclusiveReceiver = NULL;
}

InputManager::~InputManager()
{

}

float InputManager::SetDragThreshold(float threshold)
{
	float old = dragThreshold;
	dragThreshold = threshold;
	return old;
}

void InputManager::InitEvent(InputInfo &info, InputEvent ev, int contact)
{
	info.ev = ev;
	info.contact = contact;
	info.device = contacts[contact].device;
	info.deviceID = contacts[contact].deviceID;
	info.buttonID = (info.device == IDD_Mouse) ? contacts[contact].buttonID - Mouse_LeftButton : contacts[contact].buttonID;
}

void InputManager::PushReceiver(InputReceiver *pReceiver)
{
	pReceiver->pNext = pInputStack;
	pInputStack = pReceiver;
}

void InputManager::PopReceiver(InputReceiver *pReceiver)
{
	if(pExclusiveReceiver == pReceiver)
		pExclusiveReceiver = NULL;

	pReceiver = pReceiver->pNext;
	while(pInputStack != pReceiver)
	{
		InputReceiver *pNext = pInputStack->pNext;
		pInputStack->pNext = NULL;
		pInputStack = pNext;
	}
}

void InputManager::RemoveReceiver(InputReceiver *pReceiver)
{
	if(!pInputStack)
		return;

	if(pExclusiveReceiver == pReceiver)
		pExclusiveReceiver = NULL;

	if(pInputStack == pReceiver)
	{
		pInputStack = pReceiver->pNext;
		pReceiver->pNext = NULL;
	}
	else
	{
		InputReceiver *pR = pInputStack;
		while(pR->pNext)
		{
			if(pR->pNext == pReceiver)
			{
				pR->pNext = pReceiver->pNext;
				pReceiver->pNext = NULL;
			}
		}
	}
}

void InputManager::ClearReceivers()
{
	while(pInputStack)
	{
		InputReceiver *pR = pInputStack->pNext;
		pInputStack->pNext = NULL;
		pInputStack = pR;
	}
}

void InputManager::Update()
{
	// search for new mouse events
	int pointers = MFInput_GetNumPointers();
	for(int a=0; a<pointers; ++a)
	{
		if(mouseContacts[a] == -1 && MFInput_IsReady(IDD_Mouse, a))
		{
			for(int c=0; c<MAX_CONTACTS; ++c)
			{
				if(!bCurrentContacts[c])
				{
					bCurrentContacts[c] = true;
					mouseContacts[a] = c;

					// create the hover contact
					MFVector pos = CorrectPosition(MFInput_Read(Mouse_XPos, IDD_Mouse, a), MFInput_Read(Mouse_YPos, IDD_Mouse, a));
					contacts[c].Init(IDD_Mouse, a, -1, pos.x, pos.y);
					contacts[c].downX = contacts[c].downY = 0.f;

					pNewContactCallback(c);
					break;
				}
			}
		}

		if(gAppHasFocus)
		{
			for(int b=0; b<Mouse_MaxButtons; ++b)
			{
				if(mouseButtonContacts[a][b] == -1)
				{
					if(MFInput_Read(Mouse_LeftButton + b, IDD_Mouse, a))
					{
						for(int c=0; c<MAX_CONTACTS; ++c)
						{
							if(!bCurrentContacts[c])
							{
								bCurrentContacts[c] = true;
								mouseButtonContacts[a][b] = c;

								contacts[c].Init(IDD_Mouse, a, Mouse_LeftButton + b, contacts[mouseContacts[a]].x, contacts[mouseContacts[a]].y);
								contacts[c].bState = true;

								pNewContactCallback(c);

								// send the down event
								InputInfo info;
								InitEvent(info, IE_Down, c);
								info.down.x = contacts[c].x;
								info.down.y = contacts[c].y;

								Dispatch(info, contacts[c].pExclusiveReceiver);
								break;
							}
						}
					}
				}
			}
		}

		float wheel = MFInput_Read(Mouse_Wheel, IDD_Mouse, a);
		if(wheel)
		{
			// we'll simulate a pinch event on the mouses hover contact
			InputInfo info;
			InitEvent(info, IE_Pinch, a);
			info.pinch.centerX = contacts[a].x;
			info.pinch.centerY = contacts[a].y;
			info.pinch.deltaScale = wheel < 0.f ? 0.5f : 2.f;
			info.pinch.contact2 = -1;

			Dispatch(info, contacts[mouseContacts[a]].pExclusiveReceiver);
		}
	}

	// search for new touch screen interactions
	if(MFInput_IsReady(IDD_TouchPanel, 0))
	{
		MFTouchPanelState *pState = MFInput_GetContactInfo(0);

		for(int a=0; a<MAX_CONTACTS; ++a)
		{
			if(a < pState->numContacts && touchContacts[a] == -1 && pState->contacts[a].phase < 3)
			{
				for(int c=0; c<MAX_CONTACTS; ++c)
				{
					if(!bCurrentContacts[c])
					{
						bCurrentContacts[c] = true;
						touchContacts[a] = c;

						// create the hover contact
						MFVector pos = CorrectPosition((float)pState->contacts[a].x, (float)pState->contacts[a].y);
						contacts[c].Init(IDD_TouchPanel, 0, a, pos.x, pos.y);
						contacts[c].bState = true;

						pNewContactCallback(c);

						// send the down event
						InputInfo info;
						InitEvent(info, IE_Down, a);
						info.down.x = contacts[c].x;
						info.down.y = contacts[c].y;

						Dispatch(info, contacts[c].pExclusiveReceiver);
						break;
					}
				}
			}
			else if(touchContacts[a] != -1 && (a >= pState->numContacts || pState->contacts[a].phase >= 3))
			{
				int c = touchContacts[a];

				if(!contacts[c].bDrag)
				{
					// event classifies as a tap
					InputInfo info;
					InitEvent(info, IE_Tap, contacts[c].buttonID);
					info.tap.x = contacts[c].x;
					info.tap.y = contacts[c].y;
					info.tap.holdLength = contacts[c].downTime;

					Dispatch(info, contacts[c].pExclusiveReceiver);
				}

				// send the up event
				InputInfo info;
				InitEvent(info, IE_Up, contacts[c].buttonID);
				info.up.x = contacts[c].x;
				info.up.y = contacts[c].y;
				info.up.downX = contacts[c].downX;
				info.up.downY = contacts[c].downY;
				info.up.holdLength = contacts[c].downTime;

				Dispatch(info, contacts[c].pExclusiveReceiver);

				bCurrentContacts[c] = false;
				touchContacts[a] = -1;
			}
		}
	}

	// track moved contacts
	struct Moved
	{
		int contact;
		float x, y;
	} moved[16];
	int numMoved = 0;

	for(int a=0; a<MAX_CONTACTS; ++a)
	{
		if(!bCurrentContacts[a])
			continue;

		if(contacts[a].device != IDD_TouchPanel)
		{
			if(!MFInput_IsReady(contacts[a].device, contacts[a].deviceID))
			{
				bCurrentContacts[a] = false;

				if(contacts[a].device == IDD_Mouse)
					mouseContacts[contacts[a].deviceID] = -1;
				else if(contacts[a].device == IDD_TouchPanel)
					touchContacts[contacts[a].buttonID] = -1;
			}

			if(contacts[a].device == IDD_Mouse)
			{
				MFVector pos = CorrectPosition(MFInput_Read(Mouse_XPos, contacts[a].device, contacts[a].deviceID), MFInput_Read(Mouse_YPos, contacts[a].device, contacts[a].deviceID));

				if(pos.x != contacts[a].x || pos.y != contacts[a].y)
				{
					InputInfo info;
					InitEvent(info, IE_Hover, a);
					info.hover.x = pos.x;
					info.hover.y = pos.y;
					info.hover.deltaX = pos.x - contacts[a].x;
					info.hover.deltaY = pos.y - contacts[a].y;

					Dispatch(info, contacts[a].pExclusiveReceiver);

					float distX = pos.x - contacts[a].downX;
					float distY = pos.y - contacts[a].downY;
					if(MFSqrt(distX*distX + distY*distY) >= dragThreshold)
						contacts[a].bDrag = true;

					if(contacts[a].bDrag && contacts[a].bState)
					{
						// send the drag event
						info.ev = IE_Drag;
						info.drag.startX = contacts[a].downX;
						info.drag.startY = contacts[a].downY;

						Dispatch(info, contacts[a].pExclusiveReceiver);
					}

					contacts[a].x = pos.x;
					contacts[a].y = pos.y;
				}
			}

			if(contacts[a].bState)
			{
				if(!MFInput_Read(contacts[a].buttonID, contacts[a].device, contacts[a].deviceID))
				{
					if(!contacts[a].bDrag)
					{
						// event classifies as a tap
						InputInfo info;
						InitEvent(info, IE_Tap, a);
						info.tap.x = contacts[a].x;
						info.tap.y = contacts[a].y;
						info.tap.holdLength = contacts[a].downTime;

						Dispatch(info, contacts[a].pExclusiveReceiver);
					}

					// send the up event
					InputInfo info;
					InitEvent(info, IE_Up, a);
					info.up.x = contacts[a].x;
					info.up.y = contacts[a].y;
					info.up.downX = contacts[a].downX;
					info.up.downY = contacts[a].downY;
					info.up.holdLength = contacts[a].downTime;

					Dispatch(info, contacts[a].pExclusiveReceiver);

					bCurrentContacts[a] = false;

					// if it was a mouse, release the button to we can sense it again
					if(contacts[a].device == IDD_Mouse)
						mouseButtonContacts[contacts[a].deviceID][contacts[a].buttonID - Mouse_LeftButton] = -1;
				}
				else
				{
					contacts[a].downTime += MFSystem_TimeDelta();
				}
			}
		}
		else
		{
			MFVector pos = CorrectPosition(MFInput_Read(Touch_XPos(contacts[a].buttonID), IDD_TouchPanel), MFInput_Read(Touch_YPos(contacts[a].buttonID), IDD_TouchPanel));

			if(pos.x != contacts[a].x || pos.y != contacts[a].y)
			{
				InputInfo info;
				InitEvent(info, IE_Hover, a);
				info.hover.x = pos.x;
				info.hover.y = pos.y;
				info.hover.deltaX = pos.x - contacts[a].x;
				info.hover.deltaY = pos.y - contacts[a].y;

				Dispatch(info, contacts[a].pExclusiveReceiver);

				float distX = pos.x - contacts[a].downX;
				float distY = pos.y - contacts[a].downY;
				if(MFSqrt(distX*distX + distY*distY) >= dragThreshold)
					contacts[a].bDrag = true;

				if(contacts[a].bDrag && contacts[a].bState)
				{
					// send the drag event
					info.ev = IE_Drag;
					info.drag.startX = contacts[a].downX;
					info.drag.startY = contacts[a].downY;

					Dispatch(info, contacts[a].pExclusiveReceiver);
				}

				// store the old pos for further processing
				moved[numMoved].x = contacts[a].x;
				moved[numMoved].y = contacts[a].y;
				moved[numMoved++].contact = a;

				contacts[a].x = pos.x;
				contacts[a].y = pos.y;
			}
		}
	}

	if(numMoved > 1)
	{
		// calculate rotation and zoom for each pair of contacts
		for(int a=0; a<numMoved; ++a)
		{
			for(int b=a+1; b<numMoved; ++b)
			{
				MFVector center;
				MFVector newa = MakeVector(contacts[moved[a].contact].x, contacts[moved[a].contact].y);
				MFVector newDiff = MakeVector(contacts[moved[b].contact].x, contacts[moved[b].contact].y) - newa;
				MFVector oldDiff = MakeVector(moved[b].x - moved[a].x, moved[b].y - moved[a].y);
				center.Mad2(newDiff, 0.5f, newa);

				float oldLen = oldDiff.Magnitude2();
				float newLen = newDiff.Magnitude2();
				float scale = newLen / oldLen;

				InputInfo info;
				InitEvent(info, IE_Pinch, moved[a].contact);
				info.pinch.contact2 = moved[b].contact;
				info.pinch.centerX = center.x;
				info.pinch.centerY = center.y;
				info.pinch.deltaScale = scale;

				Dispatch(info, contacts[a].pExclusiveReceiver);

				oldDiff.Mul2(oldDiff, 1.f/oldLen);
				newDiff.Mul2(newDiff, 1.f/newLen);
				float angle = oldDiff.GetAngle(newDiff);

				info.ev = IE_Spin;
				info.spin.deltaAngle = angle;

				Dispatch(info, contacts[a].pExclusiveReceiver);
			}
		}
	}
}

bool InputManager::Dispatch(InputInfo &info, InputReceiver *pExplicitReceiver)
{
	float x = info.hover.x;
	float y = info.hover.y;

	if(pExplicitReceiver)
	{
		if(pExplicitReceiver->bSpacial)
		{
			info.hover.x -= pExplicitReceiver->rect.x;
			info.hover.y -= pExplicitReceiver->rect.y;
		}

		if(pExplicitReceiver->HandleInputEvent(info.ev, info))
			return true;
	}

	if(pExclusiveReceiver)
	{
		info.hover.x = x;
		info.hover.y = y;

		if(pExclusiveReceiver->bSpacial)
		{
			info.hover.x -= pExclusiveReceiver->rect.x;
			info.hover.y -= pExclusiveReceiver->rect.y;
		}

		if(pExclusiveReceiver->HandleInputEvent(info.ev, info))
			return true;
	}

	InputReceiver *pR = pInputStack;
	while(pR)
	{
		info.hover.x = x;
		info.hover.y = y;

		if(!pR->bSpacial || /*info.device != IDD_Mouse ||*/ (info.hover.x >= pR->rect.x && info.hover.x < pR->rect.x + pR->rect.width &&
														 info.hover.y >= pR->rect.y && info.hover.y < pR->rect.y + pR->rect.height))
		{
			if(pR->bSpacial)
			{
				info.hover.x -= pR->rect.x;
				info.hover.y -= pR->rect.y;
			}

			if(pR->HandleInputEvent(info.ev, info))
				return true;
		}

		pR = pR->pNext;
	}

	return false;
}

InputReceiver *InputManager::SetExclusiveReceiver(InputReceiver *pReceiver)
{
	InputReceiver *pOld = pExclusiveReceiver;
	pExclusiveReceiver = pReceiver;
	return pOld;
}

InputReceiver *InputManager::SetExclusiveContactReceiver(int contact, InputReceiver *pReceiver)
{
	InputReceiver *pOld = contacts[contact].pExclusiveReceiver;
	contacts[contact].pExclusiveReceiver = pReceiver;
	return pOld;
}

void InputManager::Contact::Init(MFInputDevice _device, int _deviceID, int _buttonID, float _x, float _y)
{
	device = _device;
	deviceID = _deviceID;
	buttonID = _buttonID;
	downTime = 0.f;
	bDrag = false;
	bState = false;
	x = downX = _x;
	y = downY = _y;
	pExclusiveReceiver = NULL;
}
