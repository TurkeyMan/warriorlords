#if !defined(_INPUTHANDLER_H)
#define _INPUTHANDLER_H

#include "MFInput.h"

class InputHandler
{
public:
	virtual int UpdateInput() = 0;

	static void HandleInput()
	{
		if(pExclusive)
			pExclusive->UpdateInput();
		else if(pCurrent)
			pCurrent->UpdateInput();
	}

	static InputHandler *SetCurrentInputHandler(InputHandler *pHandler)
	{
		InputHandler *pOld = pCurrent;
		pCurrent = pHandler;
		return pOld;
	}

protected:
	InputHandler *SetExclusive()
	{
		InputHandler *pOld = pExclusive;
		pExclusive = this;
		return pOld;
	}

	void ReleaseExclusive()
	{
		if(pExclusive == this)
			pExclusive = NULL;
	}

	static InputHandler *pCurrent;
	static InputHandler *pExclusive;
};

enum InputEvent
{
	// screen/mouse
	IE_Hover,
	IE_Tap,
	IE_Drag,
	IE_Down,
	IE_Up,
	IE_Pinch,
	IE_Spin,

	// buttons and keys
	IE_ButtonTriggered,
	IE_ButtonDown,
	IE_ButtonUp,
};

struct InputInfo
{
	InputEvent ev;			// event
	MFInputDevice device;	// device
	int deviceID;			// device id
	int contact;			// for multiple-contact devices (multiple mice/multi-touch screens)
	int buttonID;			// button ID (always 0 for touch screens)

	union
	{
		struct
		{
			float x, y;
			float deltaX, deltaY;
		} hover;
		struct
		{
			float x, y;
			float holdLength;
		} tap;
		struct
		{
			float x, y;
			float deltaX, deltaY;
			float startX, startY;
		} drag;
		struct
		{
			float x, y;
		} down;
		struct
		{
			float x, y;
			float downX, downY;
			float holdLength;
		} up;
		struct
		{
			float centerX, centerY;
			float deltaScale;
			int contact2;
		} pinch;
		struct
		{
			float centerX, centerY;
			float deltaAngle;
			int contact2;
		} spin;
	};
};

class InputReceiver
{
	friend class InputManager;
public:
	InputReceiver() { pNext = NULL; bSpacial = false; }
	InputReceiver(MFRect &rect) { pNext = NULL; bSpacial = true; this->rect = rect; }
	~InputReceiver() { }

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info) = 0;

protected:
	bool bSpacial;
	MFRect rect;

	InputReceiver *pNext;
};

class InputManager
{
public:
	InputManager();
	~InputManager();

	void Update();

	void PushReceiver(InputReceiver *pReceiver);
	void PopReceiver(InputReceiver *pReceiver);
	void RemoveReceiver(InputReceiver *pReceiver);
	void ClearReceivers();

	float SetDragThreshold(float threshold);

	InputReceiver *SetExclusiveContactReceiver(int contact, InputReceiver *pReceiver);

protected:
	bool Dispatch(InputInfo &info, InputReceiver *pExplicitReceiver);

	static const int MAX_CONTACTS = 16;

	struct Contact
	{
		void Init(MFInputDevice device, int deviceID, int buttonID = -1, float x = 0.f, float y = 0.f);

		MFInputDevice device;
		int deviceID;
		int buttonID;

		float x, y;			// current position of contact
		float downX, downY;	// position each button was pressed down
		float downTime;		// length of down time for each button
		bool bState;		// bits represent the button pressed state
		bool bDrag;			// bits represent weather the interaction is a tap or a drag

		InputReceiver *pExclusiveReceiver;
	};

	Contact contacts[MAX_CONTACTS];
	bool bCurrentContacts[MAX_CONTACTS];
	int mouseContacts[16];
	int mouseButtonContacts[16][Mouse_MaxButtons];

	float dragThreshold;

	InputReceiver *pInputStack;

	void InitEvent(InputInfo &info, InputEvent ev, int contact);
};

extern InputManager *pInputManager;

#endif
