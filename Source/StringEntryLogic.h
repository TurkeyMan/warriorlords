#if !defined(_STRING_ENTRY_LOGIC)
#define _STRING_ENTRY_LOGIC

typedef FastDelegate1<const char *> StringChangeCallback;

class OnScreenKeyboard
{
	OnScreenKeyboard();
	virtual ~OnScreenKeyboard() = 0;

	virtual void Show() = 0;
	virtual void Hide() = 0;
	virtual void Update() = 0;
	virtual void Draw() = 0;

	virtual const char *GetString() = 0;
};

class StringEntryLogic
{
public:
	enum StringType
	{
		ST_Unknown = -1,

		ST_Regular,
		ST_MultiLine,
		ST_Numeric,
		ST_Password
	};

	StringEntryLogic();
	~StringEntryLogic() { }

	void Update();
	void Draw() {}

	MFString GetString() { return buffer; }
	MFString GetRenderString();
	void SetString(MFString string);

	void SetMaxLength(int maxLength) { maxLen = maxLength; }
	void SetType(StringType type) { this->type = type; }

	void SetChangeCallback(StringChangeCallback callback) { changeCallback = callback; }

	int GetCursorPos() { return cursorPos; }
	void SetCursorPos(int position, bool bUpdateSelection = false);

	void GetSelection(int *pSelStart, int *pSelEnd);
	void SetSelection(int start, int end);

	void SetAcceptableCharacters(const char *pCharList) { include = pCharList; }
	void SetExcludedCharacters(const char *pCharList) { exclude = pCharList; }

	static void SetRepeatParams(float repeatDelay, float repeatRate) { gRepeatDelay = repeatDelay; gRepeatRate = repeatRate; }
	static void RegisterOnScreenKeyboard(OnScreenKeyboard *pOnScreenKeyboard) { pOSK = pOnScreenKeyboard; }

private:
	void StringCopyOverlap(char *pDest, const char *pSrc);
	void ClearSelection();

	MFString buffer;
	MFString include;
	MFString exclude;

	int maxLen;
	int cursorPos;
	int selectionStart, selectionEnd;
	int holdKey;
	float repeatDelay;

	StringType type;

	StringChangeCallback changeCallback;

	static OnScreenKeyboard *pOSK;

	static float gRepeatDelay;
	static float gRepeatRate;
};

#endif
