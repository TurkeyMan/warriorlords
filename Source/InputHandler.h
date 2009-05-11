#if !defined(_INPUTHANDLER_H)
#define _INPUTHANDLER_H

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

#endif
