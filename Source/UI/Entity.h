#if !defined(_ENTITY_H)
#define _ENTITY_H

#include "MFIni.h"
#include "MFPtrList.h"
#include "../Tools/Factory.h"
#include "../Tools/HashTable.h"
#include "Action.h"

class uiEntityManager;

struct uiDrawState
{
	MFMatrix mat;
	MFVector colour;
};

class uiAction_Move : public uiAction
{
public:
	static void *Create() { return new uiAction_Move; }
	virtual void Init(uiRuntimeArgs *pParameters);
	virtual bool Update();

private:
	float time, length;
	MFVector start, target;
	bool bTarget;
};

class uiAction_Fade : public uiAction
{
public:
	static void *Create() { return new uiAction_Fade; }
	virtual void Init(uiRuntimeArgs *pParameters);
	virtual bool Update();

private:
	float time, length;
	MFVector start, target;
	bool bTarget;
};

class uiAction_Scale : public uiAction
{
public:
	static void *Create() { return new uiAction_Scale; }
	virtual void Init(uiRuntimeArgs *pParameters);
	virtual bool Update();

private:
	float time, length;
	MFVector start, target;
	bool bTarget;
};

class uiAction_Rotate : public uiAction
{
public:
	static void *Create() { return new uiAction_Rotate; }
	virtual void Init(uiRuntimeArgs *pParameters);
	virtual bool Update();

private:
	float time, length;
	MFVector start, target;
	bool bTarget;
};

class uiEntity
{
	friend class uiEntityManager;
	friend class uiActionManager;
	friend class uiAction_Move;
	friend class uiAction_Fade;
	friend class uiAction_Scale;
	friend class uiAction_Rotate;
public:
	static void RegisterEntity();
	static void *Create() { return new uiEntity; }

	enum Anchor
	{
		TopLeft = 0,
		TopCenter,
		TopRight,
		CenterLeft,
		Center,
		CenterRight,
		BottomLeft,
		BottomCenter,
		BottomRight,

		AnchorMax
	};

	uiEntity();
	virtual ~uiEntity();

	virtual void Init(MFIniLine *pEntityData);
	void InitLine(MFIniLine *pEntityData);

	virtual void Update();
	virtual void Draw(const uiDrawState &state);

	virtual bool HandleInputEvent(InputEvent ev, const InputInfo &info);
	virtual bool ChangeFocus(bool bGainFocus) { return true; }
	virtual void OnEnable(bool bEnable) { }
	virtual void OnShow(bool bShow) { }

	void UpdateEntity();
	void DrawEntity(const uiDrawState &state = identity);

	bool CalculateDrawState(uiDrawState &output, const uiDrawState &parent);

	FactoryType *GetType() { return pType; }
	bool IsType(FactoryType *pType);

	MFVector GetContainerSize();

	uiEntity *Parent() const { return pParent; }

	void AddChild(uiEntity *pEntity);
	void RemoveChild(uiEntity *pEntity);

	int GetNumChildren() { return children.size(); }
	uiEntity *GetChild(int index) const { return children[index]; }
	uiEntity *FindChild(const char *pName) const;

	MFString GetName() { return name; }
	const MFVector &GetPos() { return pos; }
	const MFVector &GetSize() { return size; }
	const MFVector &GetRot() { return rot; }
	const MFVector &GetScale() { return scale; }
	const MFVector &GetColour() { return colour; }
	bool GetEnable() { return bEnabled; }
	bool GetVisible() { return bVisible; }

	void SetPos(const MFVector &position) { this->pos = position; }
	void SetSize(const MFVector &size) { this->size = size; }
	void SetRot(const MFVector &rotation) { this->rot = rotation; }
	void SetScale(const MFVector &scale) { this->scale = scale; }
	void SetColour(const MFVector &colour) { this->colour = colour; }
	void SetEnable(bool bEnabled);
	void SetVisible(bool bVisible);
	void SetAnchor(Anchor anchor) { this->anchor = anchor; }

	bool SignalEvent(const char *pEvent, const char *pParams = NULL);
	void GetPosition(MFVector *pPosition);
	void GetMatrix(MFMatrix *pMat);
	void GetWorldMatrix(MFMatrix *pMat);

	uiEntityManager *GetEntityManager();
	uiActionManager *GetActionManager();

	MFString GetProperty(const char *pProperty) { GetActionManager()->GetEntityProperty(this, pProperty); }

	void RegisterAction(const char *pName, uiActionManager::InstantActionHandler *pHandler);
	bool ExecuteAction(const char *pName, uiRuntimeArgs *pArguments);

protected:
	struct EntityAction
	{
		const char *pActionName;
		uiActionManager::InstantActionHandler *pHandler;
	};

	bool HandleInput(InputEvent ev, const InputInfo &info);
	bool TransformInputInfo(InputInfo &info, bool bCalculateOutside = false);
	bool FullTransformInputInfo(InputInfo &info, bool bCalculateOutside = false);

	FactoryType *pType;
	MFString name;

	MFVector pos, size, rot, scale, colour;

	Anchor anchor;

	bool bEnabled, bVisible, bHasFocus;

	uiEntity *pParent;
	MFArray<uiEntity*> children;

	MFArray<uiActionScript*> events;
	MFArray<EntityAction> actions;

	// script functions
	static MFString GetPos(uiEntity *pEntity);
	static MFString GetSize(uiEntity *pEntity);
	static MFString GetRot(uiEntity *pEntity);
	static MFString GetScale(uiEntity *pEntity);
	static MFString GetColour(uiEntity *pEntity);
	static MFString GetName(uiEntity *pEntity);
	static MFString GetVisible(uiEntity *pEntity);
	static MFString GetEnabled(uiEntity *pEntity);

	static void SetPos(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void SetSize(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void SetRot(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void SetScale(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void SetColour(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void SetEnable(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void SetVisible(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void SetAnchor(uiEntity *pEntity, uiRuntimeArgs *pArguments);

	static void SetFocus(uiEntity *pEntity, uiRuntimeArgs *pArguments);

	static void If(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void Select(uiEntity *pEntity, uiRuntimeArgs *pArguments);

	static uiDrawState identity;
	static const char *pAnchorNames[AnchorMax];
	static const MFVector anchorOffset[AnchorMax];
};

class uiEntityManager : public InputReceiver
{
public:
	static void InitManager();
	static void DeinitManager();
	static FactoryType *RegisterEntityType(const char *pEntityTypeName, Factory_CreateFunc *pCreateFunc, const char *pParentType = NULL);

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	void Init();
	void Deinit();

	void Update();
	void Draw();

	uiEntity *Create(const char *pType, const char *pName, uiEntity *pParent);
	void Destroy(uiEntity *pEntity);

	void LoadRootNode(const char *pRootLayout);

	uiEntity *Find(const char *pName);
	uiEntity *Iterate(uiEntity *pLast);

	uiEntity *GetRoot() { return pRoot; }

	uiEntity *GetFocus() { return pFocus; }
	uiEntity *SetFocus(uiEntity *pNewFocus);

	uiEntity *SetExclusiveReceiver(uiEntity *pReceiver);
	uiEntity *SetExclusiveContactReceiver(int contact, uiEntity *pReceiver);

	void SetHover(uiEntity *pEntity, const MFVector &pos);

private:
	void ClearContactCallback(int contact);

	uiEntity *pRoot;
	uiEntity *pFocus;

	uiEntity *pExclusiveReceiver;
	uiEntity *pContactReceivers[InputManager::MAX_CONTACTS];

	uiEntity *pHover;
	MFVector hoverPos;

	HashList<uiEntity> entityPool;

	static Factory<uiEntity> entityFactory;
};

#endif
