#include "Warlords.h"
#include "LayoutProp.h"
#include "Action.h"
#include "../Tools/ResourceCache.h"

void uiLayoutProp::RegisterEntity()
{
	FactoryType *pType = uiEntityManager::RegisterEntityType("Layout", Create, "Entity");

	uiActionManager::RegisterProperty("title", NULL, SetTitle, pType);
	uiActionManager::RegisterProperty("layout", NULL, SetLayout, pType);

	uiActionManager::RegisterInstantAction("setlayout", SetLayout, pType);
}

uiLayoutProp::uiLayoutProp()
{
	pLayoutDesc = NULL;
}

uiLayoutProp::~uiLayoutProp()
{
	if(pLayoutDesc)
	{
		// destroy intities
		for(int a=0; a<children.size(); ++a)
			GameData::Get()->GetEntityManager()->Destroy(children[a]);

		// destroy actions
		while(pActions)
		{
			GameData::Get()->GetActionManager()->DestroyAction(pActions->GetString(0));
			pActions = pActions->Next();
		}

		// destroy metrics
		while(pMetrics)
		{
			GameData::Get()->GetActionManager()->DestroyMetric(pMetrics->GetString(0));
			pMetrics = pMetrics->Next();
		}

		// destroy resources
		if(pResources)
			GameData::Get()->GetResourceCache()->UnloadResources(pResources);

		// destroy the layout file
		MFIni::Destroy(pLayoutDesc);
		pLayoutDesc = NULL;
	}
}

uiLayoutProp *uiLayoutProp::CreateLayout(const char *pName, const char *pLayoutDescription, uiEntity *pParent)
{
	uiLayoutProp *pLayout = (uiLayoutProp*)GameData::Get()->GetEntityManager()->Create("Layout", pName, pParent);
	pLayout->LoadLayout(pLayoutDescription);
	return pLayout;
}

/*
//**** THIS NEEDS TO BE INTEGRATED INTO THE PROPERTY LOOKUP CODE ****
const char *uiLayoutProp::GetProperty(const char *pPropertyName)
{
	const char *pDot = MFString_Chr(pPropertyName, '.');
	if(pDot)
	{
		const char *pChild = MFStrN(pPropertyName, pDot - pPropertyName);
		++pDot;

		int numChildren = children.size();
		for(int a=0; a<numChildren; ++a)
		{
			if(children[a]->GetName() == pChild)
			{

				children[a]->GetProperty(pDot);
			}
		}
	}

	return uiEntity::GetProperty(pPropertyName);
}
*/

void uiLayoutProp::SetTitle(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiLayoutProp *pLayout = (uiLayoutProp*)pEntity;
	pLayout->title = pArguments->GetString(0);
}

void uiLayoutProp::SetLayout(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiLayoutProp *pLayout = (uiLayoutProp*)pEntity;
	pLayout->LoadLayout(pArguments->GetString(0).CStr());
}

bool SplitLine(MFIniLine *pLine, MFString &left, MFString &right)
{
	MFString line = pLine->GetLine();

	int offset = line.FindChar(':');
	if(offset < 0)
		return false;

	left = line.SubStr(0, offset).Trim();
	right = line.SubStr(offset + 1).Trim();

	return true;
}

void uiLayoutProp::LoadLayout(const char *pLayoutDescriptor)
{
	pLayoutDesc = MFIni::Create(pLayoutDescriptor);
	MFDebug_Assert(pLayoutDesc, MFStr("Couldn't load layout '%s'", pLayoutDescriptor));

	MFIniLine *pLine = pLayoutDesc->GetFirstLine();
	while(pLine)
	{
		if(pLine->IsSection("Layout"))
		{
			MFIniLine *pSLine = pLine->Sub();
			while(pSLine)
			{
				if(pSLine->IsSection("Resources"))
				{
					pResources = pSLine->Sub();
					GameData::Get()->GetResourceCache()->LoadResources(pResources);
				}
				else if(pSLine->IsSection("Metrics"))
				{
					pMetrics = pSLine->Sub();

					MFIniLine *pSub = pMetrics;
					while(pSub)
					{
						GameData::Get()->GetActionManager()->CreateMetric(pSub->GetString(0), pSub->GetLineData().CStr());

						pSub = pSub->Next();
					}
				}
				else if(pSLine->IsSection("Actions"))
				{
					pActions = pSLine->Sub();

					MFIniLine *pSub = pActions;
					while(pSub)
					{
						MFString left, right;
						if(SplitLine(pSub, left, right))
							GameData::Get()->GetActionManager()->CreateAction(left.CStr(), right.CStr());

						pSub = pSub->Next();
					}
				}
				else if(pSLine->IsSection("Entities"))
				{
					MFIniLine *pSub = pSLine->Sub();
					while(pSub)
					{
						if(pSub->IsString(0, "section"))
						{
							MFIniLine *pEntity = pSub->Sub();
							while(pEntity)
							{
								if(pEntity->IsString(0, "name"))
								{
									uiEntity *pNewEntity = GetEntityManager()->Create(pSub->GetString(1), pEntity->GetString(1), this);
									pNewEntity->Init(pSub->Sub());
									break;
								}

								pEntity = pEntity->Next();
							}
						}

						pSub = pSub->Next();
					}
				}
				else
				{
					// must be a property
					GetActionManager()->SetEntityProperty(this, pSLine->GetString(0), pSLine->GetLineData().CStr());
				}

				pSLine = pSLine->Next();
			}
		}

		pLine = pLine->Next();
	}

	// init all the entities
	int numChildren = children.size();
	for(int a=0; a<numChildren; ++a)
	{
		children[a]->SignalEvent("init", NULL);
	}
}
