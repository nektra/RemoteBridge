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

#include "StdAfx.h"
#include "Clients.h"

//-----------------------------------------------------------

#define LIST_GROW_SIZE                                    16

//-----------------------------------------------------------

CClientMgr::CClientMgr() : cConn(this)
{
  memset(&sClients, 0, sizeof(sClients));
  return;
}

CClientMgr::~CClientMgr()
{
  RemoveAllClients();
  return;
}

HRESULT CClientMgr::AddClient(__in DWORD dwPid, __deref_out CClient **lplpClient,
                              __out CNktAutoRemoteHandle &cSlavePipe)
{
  CNktAutoFastMutex cLock(&cMtx);
  TNktComPtr<CClient> cClient;
  SIZE_T nIndex, nMin, nMax, nInsPoint[2];
  CLIENT *lpNewList[2];
  HANDLE hConn;
  HRESULT hRes;

  cSlavePipe.Reset();
  if (lplpClient == NULL)
    return E_POINTER;
  *lplpClient = NULL;
  if (dwPid == 0)
    return E_INVALIDARG;
  if (IsRunning() == FALSE)
  {
    if (Start() == FALSE)
      return E_OUTOFMEMORY;
  }
  //find insertion point or check if already exists
  nMin = 1; //shifted by one to avoid problems with negative indexes
  nMax = sClients.nCount; //if count == 0, loop will not enter
  while (nMin <= nMax)
  {
    nIndex = nMin + (nMax - nMin) / 2;
    if (dwPid == sClients.lpListSortByPid[nIndex-1].dwPid)
      return NKT_E_AlreadyExists;
    if (dwPid < sClients.lpListSortByPid[nIndex-1].dwPid)
      nMax = nIndex - 1;
    else
      nMin = nIndex + 1;
  }
  nInsPoint[0] = nMin-1;
  //initialize for connection
  hRes = cConn.SetupMaster(hConn, cSlavePipe, dwPid);
  if (FAILED(hRes))
    return hRes;
  //create new client
  cClient.Attach(new CClient(this, dwPid, hConn));
  if (cClient == NULL)
  {
    cConn.Close(hConn);
    cSlavePipe.Reset();
    return E_OUTOFMEMORY;
  }
  //find secondary insertion point
  nMin = 1; //shifted by one to avoid problems with negative indexes
  nMax = sClients.nCount; //if count == 0, loop will not enter
  while (nMin <= nMax)
  {
    nIndex = nMin + (nMax - nMin) / 2;
    _ASSERT((SIZE_T)hConn != (SIZE_T)(sClients.lpListSortByConn[nIndex-1].hConn));
    if ((SIZE_T)hConn < (SIZE_T)(sClients.lpListSortByConn[nIndex-1].hConn))
      nMax = nIndex - 1;
    else
      nMin = nIndex + 1;
  }
  nInsPoint[1] = nMin-1;
  //grow list if necessary
  if (sClients.nCount >= sClients.nSize)
  {
    lpNewList[0] = (CLIENT*)NKT_MALLOC((sClients.nSize+LIST_GROW_SIZE)*sizeof(CLIENT));
    lpNewList[1] = (CLIENT*)NKT_MALLOC((sClients.nSize+LIST_GROW_SIZE)*sizeof(CLIENT));
    if (lpNewList[0] == NULL || lpNewList[1] == NULL)
    {
      NKT_FREE(lpNewList[0]);
      NKT_FREE(lpNewList[1]);
      cConn.Close(hConn);
      cSlavePipe.Reset();
      return E_OUTOFMEMORY;
    }
    memcpy(lpNewList[0], sClients.lpListSortByPid, sClients.nCount*sizeof(CLIENT));
    memcpy(lpNewList[1], sClients.lpListSortByConn, sClients.nCount*sizeof(CLIENT));
    sClients.nSize += LIST_GROW_SIZE;
    NKT_FREE(sClients.lpListSortByPid);
    sClients.lpListSortByPid = lpNewList[0];
    NKT_FREE(sClients.lpListSortByConn);
    sClients.lpListSortByConn = lpNewList[1];
  }
  //insert new item
  memmove(sClients.lpListSortByPid+(nInsPoint[0]+1), sClients.lpListSortByPid+nInsPoint[0],
          (sClients.nCount-nInsPoint[0])*sizeof(CLIENT));
  memmove(sClients.lpListSortByConn+(nInsPoint[1]+1), sClients.lpListSortByConn+nInsPoint[1],
          (sClients.nCount-nInsPoint[1])*sizeof(CLIENT));
  sClients.lpListSortByPid[nInsPoint[0]].dwPid = sClients.lpListSortByConn[nInsPoint[1]].dwPid = dwPid;
  sClients.lpListSortByPid[nInsPoint[0]].hConn = sClients.lpListSortByConn[nInsPoint[1]].hConn = hConn;
  sClients.lpListSortByPid[nInsPoint[0]].lpClient = sClients.lpListSortByConn[nInsPoint[1]].lpClient = cClient;
  (sClients.nCount)++;
  *lplpClient = cClient.Detach();
  (*lplpClient)->AddRef();
  return S_OK;
}

BOOL CClientMgr::RemoveClient(__in DWORD dwPid)
{
  CNktAutoFastMutex cLock(&cMtx);
  SIZE_T nIdx, nIdx2;

  nIdx = FindClientByPid(dwPid);
  if (nIdx == NKT_SIZE_T_MAX)
    return FALSE;
  nIdx2 = FindClientByConn(sClients.lpListSortByPid[nIdx].hConn);
  _ASSERT(nIdx2 != NKT_SIZE_T_MAX);
  sClients.lpListSortByPid[nIdx].lpClient->Shutdown();
  sClients.lpListSortByPid[nIdx].lpClient->Release();
  memmove(sClients.lpListSortByPid+nIdx, sClients.lpListSortByPid+(nIdx+1),
          (sClients.nCount-nIdx-1)*sizeof(CLIENT));
  memmove(sClients.lpListSortByConn+nIdx2, sClients.lpListSortByConn+(nIdx2+1),
          (sClients.nCount-nIdx2-1)*sizeof(CLIENT));
  sClients.nCount--;
  return TRUE;
}

VOID CClientMgr::RemoveAllClients()
{
  {
    CNktAutoFastMutex cLock(&cMtx);
    SIZE_T i;

    if (sClients.lpListSortByPid != NULL)
    {
      for (i=0; i<sClients.nCount; i++)
      {
        sClients.lpListSortByPid[i].lpClient->Shutdown();
        sClients.lpListSortByPid[i].lpClient->Release();
      }
      NKT_FREE(sClients.lpListSortByPid);
      sClients.lpListSortByPid = NULL;
    }
    NKT_FREE(sClients.lpListSortByConn);
    sClients.lpListSortByConn = NULL;
    sClients.nCount = sClients.nSize = 0;
  }
  Stop();
  return;
}

HRESULT CClientMgr::SendMsg(__in CClient *lpClient, __in LPVOID lpData, __in SIZE_T nDataSize,
                            __inout_opt LPVOID *lplpReply, __inout_opt SIZE_T *lpnReplySize)
{
  HRESULT hRes;

  hRes = 
  hRes = cConn.SendMsg(lpClient->GetConn(), lpData, nDataSize, lplpReply, lpnReplySize);
  if (FAILED(hRes))
  {
    if (cConn.IsConnected(lpClient->GetConn()) == S_FALSE)
    {
      if (RemoveClient(lpClient->GetPid()) != FALSE)
        OnClientDisconnected(lpClient);
    }
  }
  return hRes;
}

HRESULT CClientMgr::GetClientByPid(__in DWORD dwPid, __deref_out CClient **lplpClient)
{
  CNktAutoFastMutex cLock(&cMtx);
  SIZE_T nIdx;

  if (lplpClient == NULL)
    return E_POINTER;
  nIdx = FindClientByPid(dwPid);
  if (nIdx == NKT_SIZE_T_MAX)
  {
    *lplpClient = NULL;
    return NKT_E_NotFound;
  }
  *lplpClient = sClients.lpListSortByPid[nIdx].lpClient;
  (*lplpClient)->AddRef();
  return S_OK;
}

HRESULT CClientMgr::GetClientByConn(__in HANDLE hConn, __deref_out CClient **lplpClient)
{
  CNktAutoFastMutex cLock(&cMtx);
  SIZE_T nIdx;

  if (lplpClient == NULL)
    return E_POINTER;
  nIdx = FindClientByConn(hConn);
  if (nIdx == NKT_SIZE_T_MAX)
  {
    *lplpClient = NULL;
    return NKT_E_NotFound;
  }
  *lplpClient = sClients.lpListSortByConn[nIdx].lpClient;
  (*lplpClient)->AddRef();
  return S_OK;
}

VOID CClientMgr::ThreadProc()
{
  TNktComPtr<CClient> cClient;
  HANDLE hEvents[32];
  DWORD dwPids[32];
  SIZE_T nCount, nCurr, nRet;
  HRESULT hResCoInit;

  SetThreadName("RemoteBridge/ClientMgr");
  hResCoInit = ::CoInitializeEx(0, COINIT_MULTITHREADED);
  //----
  nCurr = 0;
  while (CheckForAbort(100) == FALSE)
  {
    {
      CNktAutoFastMutex cLock(&cMtx);
      SIZE_T nProcessed;

      if (nCurr >= sClients.nCount)
        nCurr = 0;
      for (nCount=nProcessed=0; nCount<NKT_ARRAYLEN(hEvents) && nProcessed<sClients.nCount; nProcessed++)
      {
        hEvents[nCount] = sClients.lpListSortByConn[nCurr].lpClient->GetProcVmHandle();
        if (hEvents[nCount] != NULL)
        {
          dwPids[nCount++] = sClients.lpListSortByConn[nCurr].lpClient->GetPid();
        }
        if ((++nCurr) >= sClients.nCount)
          nCurr = 0;
      }
    }
    while (nCount > 0 && CheckForAbort(0) == FALSE)
    {
      nRet = (SIZE_T)::WaitForMultipleObjects((DWORD)nCount, hEvents, FALSE, 0);
      if (nRet >= WAIT_OBJECT_0+32)
        break;
      nRet -= WAIT_OBJECT_0;
      cClient.Release();
      if (SUCCEEDED(GetClientByPid(dwPids[nRet], &cClient)))
      {
        if (RemoveClient(cClient->GetPid()) != FALSE)
          OnClientDisconnected(cClient);
        cClient.Release();
      }
      memmove(hEvents+nRet, hEvents+(nRet+1), (nCount-nRet-1)*sizeof(HANDLE));
      memmove(dwPids+nRet, dwPids+(nRet+1), (nCount-nRet-1)*sizeof(DWORD));
      nCount--;
    }
  }
  if (SUCCEEDED(hResCoInit))
    ::CoUninitialize();
  return;
}

SIZE_T CClientMgr::FindClientByPid(__in DWORD dwPid)
{
  CLIENT *lpItem;

  lpItem = (CLIENT*)bsearch_s(&dwPid, sClients.lpListSortByPid, sClients.nCount, sizeof(CLIENT),
                              &CClientMgr::CLIENT_ITEM_Compare, (LPVOID)1);
  return (lpItem != NULL) ? (lpItem-sClients.lpListSortByPid) : NKT_SIZE_T_MAX;
}

SIZE_T CClientMgr::FindClientByConn(__in HANDLE hConn)
{
  CLIENT *lpItem;

  lpItem = (CLIENT*)bsearch_s(&hConn, sClients.lpListSortByConn, sClients.nCount, sizeof(CLIENT),
                              &CClientMgr::CLIENT_ITEM_Compare, (LPVOID)2);
  return (lpItem != NULL) ? (lpItem-sClients.lpListSortByConn) : NKT_SIZE_T_MAX;
}

int CClientMgr::CLIENT_ITEM_Compare(void *lpCtx, const void *lpKey, const void *lpItem)
{
  switch ((SIZE_T)lpCtx)
  {
    case 1:
      if ((*((LPDWORD)lpKey)) < ((CLIENT*)lpItem)->dwPid)
        return -1;
      if ((*((LPDWORD)lpKey)) > ((CLIENT*)lpItem)->dwPid)
        return 1;
      break;

    case 2:
      if ((*((SIZE_T*)lpKey)) < (SIZE_T)(((CLIENT*)lpItem)->hConn))
        return -1;
      if ((*((SIZE_T*)lpKey)) > (SIZE_T)(((CLIENT*)lpItem)->hConn))
        return 1;
      break;
  }
  return 0;
}

LPBYTE CClientMgr::AllocIpcBuffer(__in SIZE_T nSize)
{
  return cConn.AllocBuffer(nSize);
}

VOID CClientMgr::FreeIpcBuffer(__in LPVOID lpBuffer)
{
  cConn.FreeBuffer(lpBuffer);
  return;
}

//-----------------------------------------------------------

CClientMgr::CClient::CClient(__in CClientMgr *_lpMgr, __in DWORD _dwPid, __in HANDLE _hConn)
{
  NktInterlockedExchange(&nRefCount, 1);
  dwPid = _dwPid;
  lpMgr = _lpMgr;
  hProc = NULL;
  hConn = _hConn;
  nHookFlags = 0;
  return;
}

CClientMgr::CClient::~CClient()
{
  Shutdown();
  //----
  if (hProc != NULL)
    ::CloseHandle(hProc);
  return;
}

ULONG CClientMgr::CClient::AddRef()
{
  return (ULONG)NktInterlockedIncrement(&nRefCount);
}

ULONG CClientMgr::CClient::Release()
{
  ULONG nRet;

  nRet = (ULONG)NktInterlockedDecrement(&nRefCount);
  if (nRet == 0)
    delete this;
  return nRet;
}

VOID CClientMgr::CClient::Shutdown()
{
  CNktAutoFastMutex cLock(&cMtx);

  if (hConn != NULL)
  {
    lpMgr->cConn.Close(hConn);
    hConn = NULL;
  }
  lpMgr = NULL;
  return;
}

HANDLE CClientMgr::CClient::GetProcVmHandle()
{
  if (hProc == NULL)
  {
    CNktAutoFastMutex cLock(&cMtx);

    if (hProc == NULL)
      hProc = ::OpenProcess(SYNCHRONIZE|PROCESS_VM_OPERATION|PROCESS_VM_READ|PROCESS_VM_WRITE, FALSE, dwPid);
  }
  return hProc;
}
