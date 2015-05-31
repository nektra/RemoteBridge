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

#pragma once

#include "RemoteBridgeHelper.h"
#include "..\Common\LinkedList.h"

//-----------------------------------------------------------

template <class T>
class TCopyList
{
public:
  TCopyList()
    {
    lpList = NULL;
    nCount = nSize = 0;
    return;
    };

  ~TCopyList()
    {
    for (SIZE_T i=0; i<nCount; i++)
      lpList[i].lpItem->Release();
    if (lpList != NULL)
      NKT_FREE(lpList);
    return;
    };

  HRESULT Add(__in T *lpItem, __in SIZE_T nParam)
    {
    ITEM *lpNewList;

    if (lpItem == NULL)
      return E_POINTER;
    if (nCount >= nSize)
    {
      lpNewList = (ITEM*)NKT_MALLOC((nSize+32)*sizeof(ITEM));
      if (lpNewList == NULL)
        return E_OUTOFMEMORY;
      memcpy(lpNewList, lpList, nCount*sizeof(ITEM));
      if (lpList != NULL)
        NKT_FREE(lpList);
      lpList = lpNewList;
      nSize += 32;
    }
    lpList[nCount].lpItem = lpItem;
    lpList[nCount].nParam = nParam;
    lpItem->AddRef();
    nCount++;
    return S_OK;
    };

  __inline SIZE_T GetCount()
    {
    return nCount;
    };

  __inline T* GetItemAt(__in SIZE_T nIndex)
    {
    return lpList[nIndex].lpItem;
    };

  __inline SIZE_T GetParamAt(__in SIZE_T nIndex)
    {
    return lpList[nIndex].nParam;
    };

private:
  typedef struct {
    T *lpItem;
    SIZE_T nParam;
  } ITEM;
  ITEM *lpList;
  SIZE_T nCount, nSize;
};
