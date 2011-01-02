#if !defined(_DB_HASHTABLE_H)
#define _DB_HASHTABLE_H

#include "MFHeap.h"
#include "MFObjectPool.h"

//#define SUPPORT_FLEXIBLE_TABLE_SIZE

template <typename T>
class HashList
{
private:
	struct HashItem
	{
		const char *pName;
		T *pItem;
		HashItem *pNext;
	};

public:
	void Create(int _tableSize, int maxItems, int growItems = 0)
	{
#if !defined(SUPPORT_FLEXIBLE_TABLE_SIZE)
		_tableSize = 256;
#endif
		itemPool.Init(sizeof(HashItem), maxItems, growItems);
		ppItems = (HashItem**)MFHeap_AllocAndZero(sizeof(HashItem*)*_tableSize);
		tableSize = _tableSize;
		itemCount = 0;
	}

	void Destroy()
	{
		itemPool.Deinit();
		MFHeap_Free(ppItems);
		tableSize = 0;
		itemCount = 0;
	}

	void Add(T* pItem, const char *pItemName)
	{
		HashItem *pNew = (HashItem*)itemPool.Alloc();
		pNew->pItem = pItem;
		pNew->pName = pItemName;

		uint32 i = GetTableIndex(pItemName);

#if defined(_DEBUG)
		if(ppItems[i] && MFString_Compare(pItemName, ppItems[i]->pName))
			MFDebug_Warn(3, MFStr("Hash collision! 0x%02d: '%s' == '%s'", i, pItemName, ppItems[i]->pName));
#endif

		pNew->pNext = ppItems[i];
		ppItems[i] = pNew;

		++itemCount;
	}

	T* Remove(const char *pName)
	{
		uint32 i = GetTableIndex(pName);

		HashItem *pTemp = ppItems[i];
		HashItem *pDel = NULL;
		if(!MFString_Compare(pTemp->pName, pName))
		{
			pDel = pTemp;
			ppItems[i] = pTemp->pNext;
		}
		else
		{
			while(pTemp->pNext && MFString_Compare(pTemp->pName, pName))
				pTemp = pTemp->pNext;
			if(pTemp->pNext)
			{
				pDel = pTemp->pNext;
				pTemp->pNext = pTemp->pNext->pNext;
			}
		}

		T *pItem = pDel->pItem;
		if(pDel)
		{
			itemPool.Free(pDel);
			--itemCount;
		}

		return pItem;
	}

	void Remove(T* pItem)
	{
		HashItem *pDel = NULL;
		for(int a=0; a<tableSize; ++a)
		{
			HashItem *pTemp = ppItems[a];
			if(pTemp->pItem == pItem)
			{
				pDel = pTemp;
				ppItems[a] = pTemp->pNext;
				break;
			}
			else
			{
				while(pTemp->pNext && pTemp->pNext->pItem != pItem)
					pTemp = pTemp->pNext;
				if(pTemp->pNext)
				{
					pDel = pTemp->pNext;
					pTemp->pNext = pTemp->pNext->pNext;
					break;
				}
			}
		}

		if(pDel)
		{
			itemPool.Free(pDel);
			--itemCount;
		}
	}

	T* Find(const char *pItemName) const
	{
		uint32 i = GetTableIndex(pItemName);
		HashItem *pItem = ppItems[i];

		while(pItem && MFString_CaseCmp(pItem->pName, pItemName))
			pItem = pItem->pNext;

		return pItem ? pItem->pItem : NULL;
	}

	T* First() const
	{
		if(!itemCount)
			return NULL;

		for(int a=0; a<tableSize; ++a)
		{
			if(ppItems[a])
				return ppItems[a]->pItem;
		}
		return NULL;
	}

	T* Next(T *pItem) const
	{
		if(!pItem)
			return First();

		uint32 i = GetTableIndex(pItem->GetName().CStr());
		HashItem *pTemp = ppItems[i];

		while(pTemp && pTemp->pItem != pItem)
			pTemp = pTemp->pNext;

		if(!pTemp)
			return NULL;
		else if(pTemp->pNext)
			return pTemp->pNext->pItem;
		else
		{
			for(int a=i+1; a<tableSize; ++a)
			{
				if(ppItems[a])
					return ppItems[a]->pItem;
			}
		}
		return NULL;
	}

	T* NextMatch(T *pItem) const
	{
		if(!pItem)
			return NULL;

		uint32 i = GetTableIndex(pItem->GetName().CStr());
		HashItem *pTemp = ppItems[i];

		while(pTemp && pTemp->pItem != pItem)
			pTemp = pTemp->pNext;

		if(pTemp)
		{
			pTemp = pTemp->pNext;
			while(pTemp)
			{
				if(pTemp->pItem->GetName() == pItem->GetName())
					return pTemp->pItem;
				pTemp = pTemp->pNext;
			}
		}

		return NULL;
	}

protected:
	MFObjectPool itemPool;
	HashItem **ppItems;
	int tableSize;
	int itemCount;

	uint32 GetTableIndex(const char *pString) const
	{
		uint32 hash = MFUtil_HashString(pString);
#if !defined(SUPPORT_FLEXIBLE_TABLE_SIZE)
		// 8 bit hash folding... we'll just fold the bottom 16 bits :/
		return ((hash >> 8) ^ hash) & (0xFF);
#else
		return hash % tableSize;
#endif
	}
};

#endif
