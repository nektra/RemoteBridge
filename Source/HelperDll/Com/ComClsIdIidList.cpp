/*
 * Copyright (C) 2010-2015 Nektra S.A., Buenos Aires, Argentina.
 * All rights reserved. Contact: http://www.nektra.com
 *
 *
 * This file is part of Remote Bridge
 *
 *
 * Commercial License Usage
 * ------------------------
 * Licensees holding valid commercial Remote Bridge licenses may use this
 * file in accordance with the commercial license agreement provided with
 * the Software or, alternatively, in accordance with the terms contained
 * in a written agreement between you and Nektra.  For licensing terms and
 * conditions see http://www.nektra.com/licensing/. Use the contact form
 * at http://www.nektra.com/contact/ for further information.
 *
 *
 * GNU General Public License Usage
 * --------------------------------
 * Alternatively, this file may be used under the terms of the GNU General
 * Public License version 3.0 as published by the Free Software Foundation
 * and appearing in the file LICENSE.GPL included in the packaging of this
 * file.  Please visit http://www.gnu.org/copyleft/gpl.html and review the
 * information to ensure the GNU General Public License version 3.0
 * requirements will be met.
 *
 **/

#include "ComClsIdIidList.h"
#include "..\..\Common\MallocMacros.h"

//-----------------------------------------------------------

#define LIST_GROW_SIZE                                    32

//-----------------------------------------------------------

namespace COM {

CClsIdIidList::CClsIdIidList() : CNktFastMutex()
{
  lpList = NULL;
  nCount = nSize = 0;
  return;
}

CClsIdIidList::~CClsIdIidList()
{
  NKT_FREE(lpList);
  return;
}

HRESULT CClsIdIidList::Add(__in REFCLSID sClsId, __in REFIID sIid)
{
  CNktAutoFastMutex cLock(this);
  SIZE_T nIndex, nMin, nMax;
  ITEM *lpNewList;
  int res;

  //find insertion point or check if already exists
  nMin = 1; //shifted by one to avoid problems with negative indexes
  nMax = nCount; //if count == 0, loop will not enter
  while (nMin <= nMax)
  {
    nIndex = nMin + (nMax - nMin) / 2;
    res = memcmp(&sClsId, &(lpList[nIndex-1].sClsId), sizeof(CLSID));
    if (res == 0)
      res = memcmp(&sIid, &(lpList[nIndex-1].sIid), sizeof(IID));
    if (res == 0)
      return S_FALSE; //already exists
    if (res < 0)
      nMax = nIndex - 1;
    else
      nMin = nIndex + 1;
  }
  nMin--;
  //grow list if necessary
  if (nCount >= nSize)
  {
    lpNewList = (ITEM*)NKT_MALLOC((nSize+LIST_GROW_SIZE)*sizeof(ITEM));
    if (lpNewList == NULL)
      return E_OUTOFMEMORY;
    memcpy(lpNewList, lpList, nCount*sizeof(ITEM));
    nSize += LIST_GROW_SIZE;
    NKT_FREE(lpList);
    lpList = lpNewList;
  }
  //insert new item
  memmove(lpList+(nMin+1), lpList+nMin, (nCount-nMin)*sizeof(ITEM));
  lpList[nMin].sClsId = sClsId;
  lpList[nMin].sIid = sIid;
  nCount++;
  return S_OK;
}

HRESULT CClsIdIidList::Remove(__in REFCLSID sClsId, __in REFIID sIid)
{
  CNktAutoFastMutex cLock(this);
  SIZE_T nIndex;

  if ((nIndex = Find(sClsId, sIid)) != NKT_SIZE_T_MAX)
  {
    memmove(lpList+nIndex, lpList+(nIndex+1), (nCount-nIndex-1)*sizeof(ITEM));
    nCount--;
    return S_OK;
  }
  return S_FALSE;
}

VOID CClsIdIidList::RemoveAll()
{
  CNktAutoFastMutex cLock(this);

  NKT_FREE(lpList);
  lpList = NULL;
  nCount = nSize = 0;
  return;
}

BOOL CClsIdIidList::Check(__in REFCLSID sClsId, __in REFIID sIid)
{
  CNktAutoFastMutex cLock(this);

  return (Find(sClsId, sIid) != NKT_SIZE_T_MAX) ? TRUE : FALSE;
}

SIZE_T CClsIdIidList::Find(__in REFCLSID sClsId, __in REFIID sIid)
{
  SIZE_T nIndex, nMin, nMax;
  int res;

  nMin = 1; //shifted by one to avoid problems with negative indexes
  nMax = nCount; //if count == 0, loop will not enter
  while (nMin <= nMax)
  {
    nIndex = nMin + (nMax - nMin) / 2;
    res = memcmp(&sClsId, &(lpList[nIndex-1].sClsId), sizeof(CLSID));
    if (res == 0)
      res = memcmp(&sIid, &(lpList[nIndex-1].sIid), sizeof(IID));
    if (res == 0)
      return nIndex-1;
    if (res < 0)
      nMax = nIndex - 1;
    else
      nMin = nIndex + 1;
  }
  return NKT_SIZE_T_MAX;
}

} //COM
