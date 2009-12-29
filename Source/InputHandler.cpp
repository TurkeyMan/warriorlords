#include "Warlords.h"
#include "InputHandler.h"

#include "MFSystem.h"

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

	dragThreshold = 5.f;
	pInputStack = NULL;
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
					contacts[c].Init(IDD_Mouse, a, -1, MFInput_Read(Mouse_XPos, IDD_Mouse, a), MFInput_Read(Mouse_YPos, IDD_Mouse, a));
					contacts[c].downX = contacts[c].downY = 0.f;
					break;
				}
			}
		}

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
						contacts[c].Init(IDD_TouchPanel, 0, a, (float)pState->contacts[a].x, (float)pState->contacts[a].y);
						contacts[c].bState = true;

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
				float x = MFInput_Read(Mouse_XPos, contacts[a].device, contacts[a].deviceID);
				float y = MFInput_Read(Mouse_YPos, contacts[a].device, contacts[a].deviceID);

				if(x != contacts[a].x || y != contacts[a].y)
				{
					InputInfo info;
					InitEvent(info, IE_Hover, a);
					info.hover.x = x;
					info.hover.y = y;
					info.hover.deltaX = x - contacts[a].x;
					info.hover.deltaY = y - contacts[a].y;

					Dispatch(info, contacts[a].pExclusiveReceiver);

					float distX = x - contacts[a].downX;
					float distY = y - contacts[a].downY;
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

					contacts[a].x = x;
					contacts[a].y = y;
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
			float x = MFInput_Read(Touch_XPos(contacts[a].buttonID), IDD_TouchPanel);
			float y = MFInput_Read(Touch_YPos(contacts[a].buttonID), IDD_TouchPanel);
			
			if(x != contacts[a].x || y != contacts[a].y)
			{
				InputInfo info;
				InitEvent(info, IE_Hover, a);
				info.hover.x = x;
				info.hover.y = y;
				info.hover.deltaX = x - contacts[a].x;
				info.hover.deltaY = y - contacts[a].y;
				
				Dispatch(info, contacts[a].pExclusiveReceiver);
				
				float distX = x - contacts[a].downX;
				float distY = y - contacts[a].downY;
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
				
				contacts[a].x = x;
				contacts[a].y = y;
			}
		}
	}
}

bool InputManager::Dispatch(InputInfo &info, InputReceiver *pExplicitReceiver)
{
	if(pExplicitReceiver)
	{
		if(pExplicitReceiver->HandleInputEvent(info.ev, info))
			return true;
	}

	InputReceiver *pR = pInputStack;
	while(pR)
	{
		if(!pR->bSpacial || /*info.device != IDD_Mouse ||*/ (info.hover.x >= pR->rect.x && info.hover.x < pR->rect.x + pR->rect.width &&
														 info.hover.y >= pR->rect.y && info.hover.y < pR->rect.y + pR->rect.height))
		{
			if(pR->HandleInputEvent(info.ev, info))
				return true;
		}

		pR = pR->pNext;
	}

	return false;
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
