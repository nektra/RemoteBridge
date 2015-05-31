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

#include "SimplePipesIPC.h"
#include "ComPtr.h"
#include <stdio.h>
#include "MallocMacros.h"

//-----------------------------------------------------------

#define READAHEAD                                          5
#define BUFFERSIZE                                      4096
#define MAXFREEPACKETS                                    64
#define CHECK_ALIVE_CONNECTIONS_THRESHOLD               5000

#ifdef _DEBUG
  //#define HANDLE_ONMESSAGE_EXCEPTIONS
#else //_DEBUG
  #define HANDLE_ONMESSAGE_EXCEPTIONS
#endif //_DEBUG

//-----------------------------------------------------------

#pragma pack(1)
typedef struct {
  ULONGLONG nTotalSize;
  ULONG nId;
  ULONG nChainIndex;
  ULONG nSize;
} MSG_HEADER;
#pragma pack()

class CInternalWorkItemThreadProcData
{
public:
  CInternalWorkItemThreadProcData()
    {
    lpMsg = NULL;
    return;
    };
  ~CInternalWorkItemThreadProcData()
    {
    NKT_FREE(lpMsg);
    return;
    };

public:
  CNktSimplePipesIPC *lpMgr;
  TNktComPtr<CNktSimplePipesIPC::CConnection> cConn;
  ULONG nMsgId;
  LPVOID lpMsg;
  SIZE_T nMsgSize;
};

//-----------------------------------------------------------

static DWORD WINAPI InternalWorkItemThreadProc(__in LPVOID lpCtx);

//-----------------------------------------------------------

#pragma warning(disable : 4355)
CNktSimplePipesIPC::CNktSimplePipesIPC(__in eApartmentType _nComApartmentType) : CNktThread(), cThreadPool(this)
{
  nComApartmentType = _nComApartmentType;
  hIOCP = NULL;
  return;
}
#pragma warning(default : 4355)

CNktSimplePipesIPC::~CNktSimplePipesIPC()
{
  Shutdown();
  return;
}

VOID CNktSimplePipesIPC::Shutdown()
{
  CNktAutoFastMutex cLock(&cMtx);
  TNktLnkLst<CConnection>::Iterator itConn;
  CConnection *lpConn;

  if (hIOCP != NULL)
    ::PostQueuedCompletionStatus(hIOCP, 0, 0, NULL);
  //kill active connections
  for (lpConn=itConn.Begin(sConnections.cList); lpConn!=NULL; lpConn=itConn.Next())
    lpConn->TerminateConnection();
  //----
  {
    CNktAutoFastMutexReverse cLockRev(&cMtx);

    Stop();
    while (sConnections.cList.IsEmpty() == FALSE)
      Close((HANDLE)((CConnection*)sConnections.cList.GetHead()));
  }
  //----
  if (hIOCP != NULL)
  {
    ::CloseHandle(hIOCP);
    hIOCP = NULL;
  }
  //----
  cThreadPool.Finalize();
  return;
}

HRESULT CNktSimplePipesIPC::SetupMaster(__out HANDLE &hConn, __out CNktAutoRemoteHandle &cSlavePipe,
                                        __in DWORD dwTargetPid)
{
  TNktComPtr<CConnection> cNewConn;
  HRESULT hRes;

  hConn = NULL;
  cSlavePipe.Reset();
  //----
  hRes = Initialize();
  if (FAILED(hRes))
    return hRes;
  //----
  {
    CNktAutoFastMutex cConnLock(&(sConnections.cMtx));
    TNktLnkLst<CConnection>::Iterator itConn;
    CConnection *lpConn;

    for (lpConn=itConn.Begin(sConnections.cList); lpConn!=NULL; lpConn=itConn.Next())
    {
      if (lpConn->sOtherParty.dwPid == dwTargetPid)
        return NKT_E_AlreadyExists;
    }
    cNewConn.Attach(new CConnection(this));
    if (cNewConn == NULL)
      return E_OUTOFMEMORY;
    hRes = cNewConn->InitializeMaster(dwTargetPid, hIOCP, cSlavePipe);
    if (FAILED(hRes))
      return hRes;
    hConn = (HANDLE)((CConnection*)cNewConn);
    sConnections.cList.PushTail(cNewConn.Detach());
  }
  return S_OK;
}

HRESULT CNktSimplePipesIPC::SetupSlave(__out HANDLE &hConn, __in DWORD dwMasterPid, __in HANDLE hPipe,
                                       __in HANDLE hMasterProc)
{
  TNktComPtr<CConnection> cNewConn;
  HRESULT hRes;

  hConn = NULL;
  //----
  hRes = Initialize();
  if (FAILED(hRes))
    return hRes;
  //----
  {
    CNktAutoFastMutex cConnLock(&(sConnections.cMtx));
    TNktLnkLst<CConnection>::Iterator itConn;
    CConnection *lpConn;

    for (lpConn=itConn.Begin(sConnections.cList); lpConn!=NULL; lpConn=itConn.Next())
    {
      if (lpConn->sOtherParty.dwPid == dwMasterPid)
        return NKT_E_AlreadyExists;
    }
    cNewConn.Attach(new CConnection(this));
    if (cNewConn == NULL)
      return E_OUTOFMEMORY;
    hRes = cNewConn->InitializeSlave(dwMasterPid, hMasterProc, hPipe, hIOCP);
    if (FAILED(hRes))
      return hRes;
    hConn = (HANDLE)((CConnection*)cNewConn);
    sConnections.cList.PushTail(cNewConn.Detach());
  }
  return S_OK;
}

VOID CNktSimplePipesIPC::Close(__in HANDLE hConn)
{
  TNktComPtr<CConnection> cConn;

  {
    CNktAutoFastMutex cConnLock(&(sConnections.cMtx));
    TNktLnkLst<CConnection>::Iterator itConn;
    CConnection *lpConn;

    for (lpConn=itConn.Begin(sConnections.cList); lpConn!=NULL; lpConn=itConn.Next())
    {
      if (lpConn == (CConnection*)hConn)
      {
        cConn = lpConn;
        lpConn->RemoveNode();
        break;
      }
    }
  }
  if (cConn != NULL)
    cConn->TerminateConnection();
  return;
}

HRESULT CNktSimplePipesIPC::IsConnected(__in HANDLE hConn)
{
  CNktAutoFastMutex cConnLock(&(sConnections.cMtx));
  TNktLnkLst<CConnection>::Iterator itConn;
  CConnection *lpConn;

  for (lpConn=itConn.Begin(sConnections.cList); lpConn!=NULL; lpConn=itConn.Next())
  {
    if (lpConn == (CConnection*)hConn)
      return (lpConn->IsConnected() != FALSE) ? S_OK : S_FALSE;
  }
  return NKT_E_NotFound;
}

HRESULT CNktSimplePipesIPC::SendMsg(__in HANDLE hConn, __in LPVOID lpData, __in SIZE_T nDataSize,
                                    __inout_opt LPVOID *lplpReply, __inout_opt SIZE_T *lpnReplySize)
{
  TNktComPtr<CConnection> cConn;
  TNktComPtr<CConnection::CReadedMessage> cAnswer;
  HANDLE hEvents[3];
  DWORD dwRetCode;
  HRESULT hRes;

  if (lpData == NULL ||
      (lplpReply == NULL && lpnReplySize != NULL) ||
      (lplpReply != NULL && lpnReplySize == NULL))
    return E_POINTER;
  if (lplpReply != NULL)
    *lplpReply = NULL;
  if (lpnReplySize != NULL)
    *lpnReplySize = 0;
  if (nDataSize < 1 || nDataSize >= 0x7FFFFFFF)
    return E_INVALIDARG;
  {
    CNktAutoFastMutex cConnLock(&(sConnections.cMtx));
    TNktLnkLst<CConnection>::Iterator itConn;
    CConnection *lpConn;

    for (lpConn=itConn.Begin(sConnections.cList); lpConn!=NULL; lpConn=itConn.Next())
    {
      if (lpConn == (CConnection*)hConn)
      {
        cConn = lpConn;
        break;
      }
    }
  }
  if (cConn == NULL)
    return NKT_E_NotFound;
  //----
  hRes = cConn->SendWrite(lpData, (ULONG)nDataSize, 0, (lplpReply != NULL) ? &cAnswer : NULL);
  if (SUCCEEDED(hRes))
  {
    if (lplpReply != NULL)
    {
      hEvents[0] = cAnswer->cDoneEv.GetEventHandle();
      hEvents[1] = cConn->cTerminateEv.GetEventHandle();
      hEvents[2] = cConn->sOtherParty.cProc.Get();
      dwRetCode = CNktThread::CoWaitAndDispatchMessages(INFINITE, 3, hEvents);
      switch (dwRetCode)
      {
        case WAIT_OBJECT_0:
          hRes = cAnswer->BuildReply(this, lplpReply, lpnReplySize);
          break;
        case WAIT_OBJECT_0+1:
        case WAIT_OBJECT_0+2:
          hRes = NKT_E_Cancelled;
          break;
        default:
          hRes = E_FAIL;
          break;
      }
      cConn->RemoveReadedMessage(cAnswer);
    }
  }
  return hRes;
}

LPBYTE CNktSimplePipesIPC::AllocBuffer(__in SIZE_T nSize)
{
  return (LPBYTE)NKT_MALLOC((nSize > 0) ? nSize : 1);
}

VOID CNktSimplePipesIPC::FreeBuffer(__in LPVOID lpBuffer)
{
  NKT_FREE(lpBuffer);
  return;
}

HRESULT CNktSimplePipesIPC::Initialize()
{
  CNktAutoFastMutex cLock(&cMtx);

  if (cThreadPool.IsInitialized() == FALSE)
  {
    if (cThreadPool.Initialize(0, 0, 2000) == FALSE)
      return NKT_HRESULT_FROM_LASTERROR();
  }
  if (hIOCP == NULL)
  {
    hIOCP = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
    if (hIOCP == NULL || hIOCP == INVALID_HANDLE_VALUE)
    {
      hIOCP = NULL;
      return NKT_HRESULT_FROM_LASTERROR();
    }
  }
  //----
  if (IsRunning() == FALSE)
  {
    if (Start() == FALSE)
      return E_OUTOFMEMORY;
  }
  //----
  return S_OK;
}

VOID CNktSimplePipesIPC::ThreadProc()
{
  TNktComPtr<CConnection> cConn;
  CConnection *_lpConn;
  OVERLAPPED *_lpOvr;
  DWORD dwNumberOfBytes, dwWritten;
  CPacket *lpPacket;
  BOOL bReadPending;
  HRESULT hRes, hResCoInit;

  SetThreadName("NktSimplePipesIPC");
  switch (nComApartmentType)
  {
    case CNktSimplePipesIPC::atSTA:
      hResCoInit = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
      break;
    case CNktSimplePipesIPC::atMTA:
      hResCoInit = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
      break;
    case CNktSimplePipesIPC::atOLE:
      hResCoInit = ::OleInitialize(NULL);
      break;
    default:
      hResCoInit = S_FALSE;
      break;
  }
  //main loop
  while (1)
  {
    dwNumberOfBytes = 0;
    _lpConn = NULL;
    _lpOvr = NULL;
    hRes = S_OK;
    if (::GetQueuedCompletionStatus(hIOCP, &dwNumberOfBytes, (PULONG_PTR)&_lpConn, &_lpOvr,
                                    CHECK_ALIVE_CONNECTIONS_THRESHOLD) == FALSE)
    {
      hRes = NKT_HRESULT_FROM_LASTERROR();
      if (hRes == S_OK || hRes == NKT_E_Timeout) //yes... on timeout there is no "lasterror"
      {
        CheckAliveConnections();
        continue;
      }
    }
    if (_lpConn == NULL)
      break; //a completion key of 0 is posted to the iocp to request us to shut down...
    {
      cConn = _lpConn;
      lpPacket = CPacket::ClassFromOverlapped(_lpOvr);
      bReadPending = FALSE;
      if (lpPacket != NULL)
      {
        switch (lpPacket->nType)
        {
          case CPacket::typDoInitialSetup:
            if (SUCCEEDED(hRes))
            {
              CNktAutoFastMutex cConnLock(cConn);
              SIZE_T i;

              cConn->cActiveRwPacketList.Remove(lpPacket);
              cConn->FreePacket(lpPacket);
              for (i=0; i<READAHEAD && SUCCEEDED(hRes); i++)
                hRes =  cConn->SendRead(); //setup initial read-ahead
            }
            break;

          case CPacket::typDoRead:
            if (SUCCEEDED(hRes) &&
                dwNumberOfBytes < sizeof(lpPacket->sData.sHdr))
            {
              _ASSERT(FALSE);
              hRes = NKT_E_InvalidData;
            }
            //add to read queue
            if (SUCCEEDED(hRes))
            {
              cConn->cActiveRwPacketList.Remove(lpPacket);
              cConn->cReadedList.QueueSorted(lpPacket);
              hRes = cConn->SendRead(); //setup read-ahead
            }
            bReadPending = TRUE;
            break;

          case CPacket::typWriteRequest:
            if (SUCCEEDED(hRes))
            {
              CNktAutoFastMutex cConnLock(cConn);

              cConn->cActiveRwPacketList.Remove(lpPacket);
              cConn->cToWriteList.QueueSorted(lpPacket);
              while (SUCCEEDED(hRes) && cConn->cTerminateEv.Wait(0) == FALSE && CheckForAbort(0) == FALSE)
              {
                //get next sequenced block to write
                lpPacket = cConn->cToWriteList.Dequeue(cConn->nNextWriteOrderToProcess);
                if (lpPacket == NULL)
                  break;
                if (lpPacket->nType != CPacket::typDiscard)
                {
                  cConn->cActiveRwPacketList.QueueLast(lpPacket);
                  //do real write
                  lpPacket->nType = CPacket::typDoWrite;
                  if (::WriteFile(cConn->cPipe, &(lpPacket->sData), (DWORD)sizeof(lpPacket->sData.sHdr) +
                                  lpPacket->sData.sHdr.nSize, &dwWritten, &(lpPacket->sOvr)) == FALSE)
                  {
                    hRes = NKT_HRESULT_FROM_LASTERROR();
                    if (hRes == NKT_E_IoPending)
                      hRes = S_OK;
                  }
                  if (SUCCEEDED(hRes))
                  {
                    _lpConn->AddRef();
                  }
                  else
                  {
                    cConn->cActiveRwPacketList.Remove(lpPacket);
                    cConn->FreePacket(lpPacket);
                  }
                }
                else
                {
                  cConn->FreePacket(lpPacket);
                }
                _InterlockedIncrement(&(cConn->nNextWriteOrderToProcess));
              }
            }
            break;

          case CPacket::typDoWrite:
            if (SUCCEEDED(hRes) &&
                (DWORD)sizeof(lpPacket->sData.sHdr) + lpPacket->sData.sHdr.nSize != dwNumberOfBytes)
              hRes = NKT_E_WriteFault;
            cConn->cActiveRwPacketList.Remove(lpPacket);
            cConn->FreePacket(lpPacket);
            break;

          case CPacket::typDiscard:
            cConn->FreePacket(lpPacket);
            break;

          default:
            _ASSERT(FALSE);
            cConn->FreePacket(lpPacket);
            break;
        }
      }
      else
      {
        hRes = E_FAIL;
      }
      if (SUCCEEDED(hRes) && bReadPending != FALSE)
      {
        while (SUCCEEDED(hRes) && cConn->cTerminateEv.Wait(0) == FALSE && CheckForAbort(0) == FALSE)
        {
          //get next sequenced block readed
          lpPacket = _lpConn->cReadedList.Dequeue(cConn->nNextReadOrderToProcess);
          if (lpPacket == NULL)
            break;
          hRes = _lpConn->ProcessReadedPacket(lpPacket);
          _InterlockedIncrement(&(_lpConn->nNextReadOrderToProcess));
        }
      }
      if (FAILED(hRes))
      {
        cConn->TerminateConnection();
      }
      _lpConn->Release();
    }
  }
  if (SUCCEEDED(hResCoInit) && hResCoInit != S_FALSE)
  {
    switch (nComApartmentType)
    {
      case CNktSimplePipesIPC::atSTA:
      case CNktSimplePipesIPC::atMTA:
        ::CoUninitialize();
        break;
      case CNktSimplePipesIPC::atOLE:
        ::OleUninitialize();
        break;
    }
  }
  return;
}

DWORD CNktSimplePipesIPC::WorkItemThreadProc(__in LPVOID lpContext)
{
  CInternalWorkItemThreadProcData *lpData = (CInternalWorkItemThreadProcData*)lpContext;
  LPVOID lpReplyData;
  SIZE_T nReplyDataSize;
  HRESULT hRes;

  lpReplyData = NULL;
  nReplyDataSize = 0;
#ifdef HANDLE_ONMESSAGE_EXCEPTIONS
  try
  {
#endif //HANDLE_ONMESSAGE_EXCEPTIONS
    hRes = OnMessage((HANDLE)(CConnection*)(lpData->cConn), (LPBYTE)(lpData->lpMsg), lpData->nMsgSize, &lpReplyData,
                     &nReplyDataSize);
#ifdef HANDLE_ONMESSAGE_EXCEPTIONS
  }
  catch (...)
  {
    hRes = NKT_E_UnhandledException;
  }
#endif //HANDLE_ONMESSAGE_EXCEPTIONS
  if (SUCCEEDED(hRes) && lpReplyData == NULL && nReplyDataSize > 0)
    hRes = E_POINTER;
  //send answer
  if (SUCCEEDED(hRes) && nReplyDataSize > 0)
    hRes = lpData->cConn->SendWrite(lpReplyData, (ULONG)nReplyDataSize, lpData->nMsgId | 0x80000000, NULL);
  if (FAILED(hRes))
    lpData->cConn->TerminateConnection();
  //----
  if (lpReplyData != NULL)
    FreeBuffer(lpReplyData);
  return hRes;
}

VOID CNktSimplePipesIPC::CheckAliveConnections()
{
  CNktAutoFastMutex cLock(&cMtx);
  TNktLnkLst<CConnection>::Iterator itConn;
  CConnection *lpConn;

  //check if active connections are still alive
  for (lpConn=itConn.Begin(sConnections.cList); lpConn!=NULL; lpConn=itConn.Next())
  {
    if (lpConn->IsConnected(TRUE) == FALSE)
    {
      lpConn->TerminateConnection();
    }
  }
  return;
}

VOID CNktSimplePipesIPC::OnTaskTerminated(__in LPVOID lpContext, __in HRESULT hReturnValue)
{
  CInternalWorkItemThreadProcData *lpData = (CInternalWorkItemThreadProcData*)lpContext;

  if (FAILED(hReturnValue))
    lpData->cConn->TerminateConnection();
  delete lpData;
  return;
}

VOID CNktSimplePipesIPC::OnTaskCancelled(__in LPVOID lpContext)
{
  CInternalWorkItemThreadProcData *lpData = (CInternalWorkItemThreadProcData*)lpContext;

  delete lpData;
  return;
}

VOID CNktSimplePipesIPC::OnTaskExceptionError(__in LPVOID lpContext, __in DWORD dwException,
                                              __in struct _EXCEPTION_POINTERS *excPtr)
{
  CInternalWorkItemThreadProcData *lpData = (CInternalWorkItemThreadProcData*)lpContext;

  lpData->cConn->TerminateConnection();
  delete lpData;
  return;
}

//-----------------------------------------------------------
//-----------------------------------------------------------

CNktSimplePipesIPC::CPacket::CPacket(__in CConnection *_lpConn) : TNktLnkLstNode<CPacket>()
{
  lpConn = _lpConn;
  cEv.Create(TRUE, FALSE);
  memset(&sData, 0, sizeof(sData));
  memset(&sOvr, 0, sizeof(sOvr));
  nType = typDiscard;
  nOrder = 0;
  return;
}

VOID CNktSimplePipesIPC::CPacket::Reset()
{
  cEv.Reset();
  memset(&sOvr, 0, sizeof(sOvr));
  sOvr.hEvent = cEv.GetEventHandle();
  return;
}

//-----------------------------------------------------------
//-----------------------------------------------------------

CNktSimplePipesIPC::CPacketList::CPacketList() : CNktFastMutex()
{
  nCount = 0;
  return;
}

CNktSimplePipesIPC::CPacketList::~CPacketList()
{
  DiscardAll();
  return;
}

VOID CNktSimplePipesIPC::CPacketList::DiscardAll()
{
  CNktSimplePipesIPC::CPacket *lpPacket;

  while ((lpPacket=DequeueFirst()) != NULL)
    delete lpPacket;
  return;
}

VOID CNktSimplePipesIPC::CPacketList::QueueLast(__inout CNktSimplePipesIPC::CPacket *lpPacket)
{
  if (lpPacket != NULL)
  {
    CNktAutoFastMutex cLock(this);

    cList.PushTail(lpPacket);
    nCount++;
  }
  return;
}

CNktSimplePipesIPC::CPacket* CNktSimplePipesIPC::CPacketList::DequeueFirst()
{
  CNktAutoFastMutex cLock(this);
  CNktSimplePipesIPC::CPacket *lpPacket;

  lpPacket = cList.PopHead();
  if (lpPacket != NULL)
    nCount--;
  return lpPacket;
}

VOID CNktSimplePipesIPC::CPacketList::QueueSorted(__inout CNktSimplePipesIPC::CPacket *lpPacket)
{
  CNktAutoFastMutex cLock(this);
  TNktLnkLst<CPacket>::Iterator it;
  CPacket *lpCurrPacket;

  for (lpCurrPacket=it.Begin(cList); lpCurrPacket!=NULL; lpCurrPacket=it.Next())
  {
    if (lpPacket->nOrder < lpCurrPacket->nOrder)
      break;
  }
  if (lpCurrPacket != NULL)
    cList.PushBefore(lpPacket, lpCurrPacket);
  else
    cList.PushTail(lpPacket);
  nCount++;
  return;
}

CNktSimplePipesIPC::CPacket* CNktSimplePipesIPC::CPacketList::Dequeue(__in LONG volatile nOrder)
{
  CNktAutoFastMutex cLock(this);
  CPacket *lpPacket;

  lpPacket = cList.GetHead();
  if (lpPacket != NULL && lpPacket->nOrder == nOrder)
  {
    lpPacket->RemoveNode();
    return lpPacket;
  }
  return NULL;
}

VOID CNktSimplePipesIPC::CPacketList::Remove(__inout CNktSimplePipesIPC::CPacket *lpPacket)
{
  CNktAutoFastMutex cLock(this);

  _ASSERT(lpPacket->GetLinkedList() == &cList);
  lpPacket->RemoveNode();
  return;
}

SIZE_T CNktSimplePipesIPC::CPacketList::GetCount()
{
  CNktAutoFastMutex cLock(this);

  return nCount;
}

//-----------------------------------------------------------
//-----------------------------------------------------------

CNktSimplePipesIPC::CConnection::CConnection(__in CNktSimplePipesIPC *_lpMgr) : TNktLnkLstNode<CConnection>()
{
  lpMgr = _lpMgr;
  sOtherParty.dwPid = 0;
  _InterlockedExchange(&nNextReadOrder, 0);
  _InterlockedExchange(&nNextReadOrderToProcess, 1);
  _InterlockedExchange(&nNextWriteOrder, 0);
  _InterlockedExchange(&nNextWriteOrderToProcess, 1);
  _InterlockedExchange(&nNextMsgId, 0);
  _InterlockedExchange(&nRefCount, 1);
  return;
}

CNktSimplePipesIPC::CConnection::~CConnection()
{
  CReadedMessage *lpMsg;

  TerminateConnection();
  //----
  while ((lpMsg=sReadedMsgs.cList.PopHead()) != NULL)
    lpMsg->Release();
  return;
}

ULONG CNktSimplePipesIPC::CConnection::AddRef()
{
  return (ULONG)_InterlockedIncrement(&nRefCount);
}

ULONG CNktSimplePipesIPC::CConnection::Release()
{
  ULONG nRef = (ULONG)_InterlockedDecrement(&nRefCount);
  if (nRef == 0)
    delete this;
  return nRef;
}

BOOL CNktSimplePipesIPC::CConnection::IsConnected(__in BOOL DoPeek)
{
  CNktAutoFastMutex cLock(this);
  DWORD dw;

  if (cPipe == NULL)
    return FALSE;
  if (DoPeek != FALSE)
  {
    if (::PeekNamedPipe(cPipe, NULL, 0, NULL, &dw, NULL) == FALSE)
      return FALSE;
  }
  return TRUE;
}

VOID CNktSimplePipesIPC::CConnection::TerminateConnection()
{
  CNktAutoFastMutex cLock(this);

  cTerminateEv.Set();
  if (cPipe != NULL)
  {
    ::DisconnectNamedPipe(cPipe);
    cPipe.Reset();
  }
  return;
}

HRESULT CNktSimplePipesIPC::CConnection::InitializeMaster(__in DWORD dwSlavePid, __in HANDLE _hIOCP,
                                                          __out CNktAutoRemoteHandle &cSlavePipe)
{
  CNktAutoRemoteHandle cRemotePipe;
  WCHAR szBufW[256];
  SECURITY_ATTRIBUTES sSecAttrib;
  SECURITY_DESCRIPTOR sSecDesc;
  DWORD dwBytes, dwMode;
  CNktAutoHandle cTempSlavePipe;
  OVERLAPPED sConnOvr;
  CNktEvent cConnEv;
  BOOL bConnected;
  HRESULT hRes;

  if (dwSlavePid == 0 || dwSlavePid == ::GetCurrentProcessId())
    return E_INVALIDARG;
  //create shutdown event
  if (cTerminateEv.Create(TRUE, FALSE) == FALSE)
    return E_OUTOFMEMORY;
  sOtherParty.cProc.Attach(::OpenProcess(PROCESS_DUP_HANDLE|SYNCHRONIZE, FALSE, dwSlavePid));
  if (sOtherParty.cProc == NULL || sOtherParty.cProc == INVALID_HANDLE_VALUE)
    return NKT_HRESULT_FROM_LASTERROR();
  //create named pipe
  ::InitializeSecurityDescriptor(&sSecDesc, SECURITY_DESCRIPTOR_REVISION);
  ::SetSecurityDescriptorDacl(&sSecDesc, TRUE, NULL, FALSE);
  sSecAttrib.nLength = sizeof(SECURITY_ATTRIBUTES);
  sSecAttrib.bInheritHandle = FALSE;
  sSecAttrib.lpSecurityDescriptor = &sSecDesc;
  swprintf_s(szBufW, 256, L"\\\\.\\Pipe\\NktSimplePipes_%uX_%IX", ::GetCurrentProcessId(), (SIZE_T)this);
  cPipe.Attach(::CreateNamedPipeW(szBufW, PIPE_ACCESS_DUPLEX|FILE_FLAG_WRITE_THROUGH|FILE_FLAG_OVERLAPPED,
                                  PIPE_READMODE_MESSAGE|PIPE_TYPE_MESSAGE|PIPE_WAIT, 1, BUFFERSIZE, BUFFERSIZE,
                                  10000, &sSecAttrib));
  if (cPipe == NULL || cPipe == INVALID_HANDLE_VALUE)
    return NKT_HRESULT_FROM_LASTERROR();
  //initialize for connection wait
  if (cConnEv.Create(TRUE, FALSE) == FALSE)
    return NKT_HRESULT_FROM_LASTERROR();
  memset(&sConnOvr, 0, sizeof(sConnOvr));
  sConnOvr.hEvent = cConnEv.GetEventHandle();
  bConnected = ::ConnectNamedPipe(cPipe, &sConnOvr);
  //create other party pipe
  sOtherParty.dwPid = dwSlavePid;
  cTempSlavePipe.Attach(::CreateFileW(szBufW, GENERIC_READ|GENERIC_WRITE, 0, &sSecAttrib, OPEN_EXISTING,
                                      FILE_FLAG_WRITE_THROUGH|FILE_FLAG_OVERLAPPED, NULL));
  if (cTempSlavePipe == NULL || cTempSlavePipe == INVALID_HANDLE_VALUE)
    return NKT_HRESULT_FROM_LASTERROR();
  //now wait until connected (may be not necessary)
  if (bConnected == FALSE)
  {
    if (::WaitForSingleObject(sConnOvr.hEvent, 1000) != WAIT_OBJECT_0)
      return NKT_E_BrokenPipe;
    if (::GetOverlappedResult(cPipe, &sConnOvr, &dwBytes, FALSE) == FALSE)
    {
      hRes = NKT_HRESULT_FROM_LASTERROR();
      if (hRes != NKT_HRESULT_FROM_WIN32(ERROR_PIPE_CONNECTED))
        return hRes;
    }
  }
  //pipe connected; change to message-read mode
  dwMode = PIPE_READMODE_MESSAGE;
  if (::SetNamedPipeHandleState(cTempSlavePipe, &dwMode, NULL, NULL) == FALSE)
    return NKT_HRESULT_FROM_LASTERROR();
  hRes = cSlavePipe.Init((HANDLE)(sOtherParty.cProc), cTempSlavePipe);
  //associate the pipe with an I/O completion port
  if (SUCCEEDED(hRes))
  {
    hIOCP = ::CreateIoCompletionPort(cPipe, _hIOCP, (ULONG_PTR)this, 0);
    if (hIOCP == NULL)
      hRes = NKT_HRESULT_FROM_LASTERROR();
  }
  //send initial read(s)
  if (SUCCEEDED(hRes))
    hRes = SendInitialReadAhead();
  //done
  if (FAILED(hRes))
    cSlavePipe.Reset();
  return hRes;
}

HRESULT CNktSimplePipesIPC::CConnection::InitializeSlave(__in DWORD dwMasterPid, __in HANDLE hMasterProc,
                                                         __in HANDLE _hPipe, __in HANDLE _hIOCP)
{
  if (dwMasterPid == 0 || dwMasterPid == ::GetCurrentProcessId())
    return E_INVALIDARG;
  if (hMasterProc == NULL || hMasterProc == INVALID_HANDLE_VALUE ||
      _hPipe == NULL || _hPipe == INVALID_HANDLE_VALUE)
    return E_INVALIDARG;
  //setup shutdown event
  if (cTerminateEv.Create(TRUE, FALSE) == FALSE)
    return E_OUTOFMEMORY;
  //setup pipe
  sOtherParty.dwPid = dwMasterPid;
  cPipe.Attach(_hPipe);
  //setup other stuff
  sOtherParty.cProc.Attach(hMasterProc);
  //associate the pipe with an I/O completion port
  hIOCP = ::CreateIoCompletionPort(cPipe, _hIOCP, (ULONG_PTR)this, 0);
  if (hIOCP == NULL)
    return NKT_HRESULT_FROM_LASTERROR();
  //send initial read(s)
  return SendInitialReadAhead();
}

CNktSimplePipesIPC::CPacket* CNktSimplePipesIPC::CConnection::GetPacket()
{
  CNktSimplePipesIPC::CPacket *lpPacket;

  lpPacket = cFreePacketList.DequeueFirst();
  if (lpPacket == NULL)
  {
    lpPacket = new CNktSimplePipesIPC::CPacket(this); //create a new one if free list is empty
    if (lpPacket != NULL &&
        lpPacket->cEv.GetEventHandle() == NULL)
    {
      delete lpPacket;
      lpPacket = NULL;
    }
  }
  if (lpPacket != NULL)
    lpPacket->Reset();
  return lpPacket;
}

VOID CNktSimplePipesIPC::CConnection::FreePacket(__inout CNktSimplePipesIPC::CPacket *lpPacket)
{
  if (cFreePacketList.GetCount() < MAXFREEPACKETS)
  {
    lpPacket->nType = CNktSimplePipesIPC::CPacket::typDiscard;
    cFreePacketList.QueueLast(lpPacket);
  }
  else
  {
    delete lpPacket;
  }
  return;
}

HRESULT CNktSimplePipesIPC::CConnection::SendRead()
{
  CNktAutoFastMutex cLock(this);
  CNktSimplePipesIPC::CPacket *lpPacket;
  DWORD dwReaded;
  HRESULT hRes;

  if (cTerminateEv.Wait(0) != FALSE)
    return E_NKT_PIPECOMM_SHUTDOWN;
  //prepare buffer
  lpPacket = GetPacket();
  if (lpPacket == NULL)
    return E_OUTOFMEMORY;
  lpPacket->nType = CNktSimplePipesIPC::CPacket::typDoRead;
  lpPacket->nOrder = _InterlockedIncrement(&nNextReadOrder);
  cActiveRwPacketList.QueueLast(lpPacket);
  //do read
  if (::ReadFile(cPipe, &(lpPacket->sData), sizeof(lpPacket->sData), &dwReaded, &(lpPacket->sOvr)) == FALSE)
  {
    hRes = NKT_HRESULT_FROM_LASTERROR();
    if (FAILED(hRes) && hRes != NKT_E_IoPending)
    {
      cActiveRwPacketList.Remove(lpPacket);
      FreePacket(lpPacket);
      return hRes;
    }
  }
  AddRef();
  return S_OK;
}

HRESULT CNktSimplePipesIPC::CConnection::SendWrite(__in LPVOID lpData, __in ULONG nDataSize, __in ULONG nMsgId,
                                                   __deref_out CReadedMessage **lplpAnswer)
{
  CNktAutoFastMutex cLock(this);
  CNktSimplePipesIPC::CPacket *lpPacket;
  TNktComPtr<CReadedMessage> cNewAnswer;
  LPBYTE s;
  ULONG nChainIdx;
  HRESULT hRes;

  _ASSERT(lpData != NULL);
  _ASSERT(nDataSize > 0 && nDataSize < 0x7FFFFFFF);
  if (lplpAnswer != NULL)
    *lplpAnswer = NULL;
  if (cTerminateEv.Wait(0) != FALSE)
    return E_NKT_PIPECOMM_SHUTDOWN;
  if (nMsgId == 0)
  {
    do
    {
      nMsgId = (ULONG)_InterlockedIncrement(&nNextMsgId) & 0x7FFFFFFFUL;
    }
    while (nMsgId == 0);
  }
  //prepare readmessage for hold answer
  if (lplpAnswer != NULL)
  {
    cNewAnswer.Attach(new CReadedMessage());
    if (cNewAnswer != NULL)
      hRes = cNewAnswer->Initialize(nMsgId | 0x80000000, 0);
    else
      hRes = E_OUTOFMEMORY;
    if (FAILED(hRes))
      return hRes;
    {
      CNktAutoFastMutex cLock(&(sReadedMsgs.cMtx));

      sReadedMsgs.cList.PushHead(((CReadedMessage*)cNewAnswer));
      ((CReadedMessage*)cNewAnswer)->AddRef();
    }
  }
  //do write
  s = (LPBYTE)lpData;
  nChainIdx = 1;
  while (nDataSize > 0)
  {
    //get a free buffer
    lpPacket = GetPacket();
    if (lpPacket == NULL)
      return E_OUTOFMEMORY;
    //fill buffer
    lpPacket->sData.sHdr.nId = nMsgId;
    lpPacket->sData.sHdr.nChainIndex = nChainIdx++;
    lpPacket->sData.sHdr.nTotalSize = (ULONGLONG)nDataSize;
    lpPacket->sData.sHdr.nSize = (ULONG)((nDataSize > sizeof(lpPacket->sData.aBuffer)) ?
                                             sizeof(lpPacket->sData.aBuffer) : nDataSize);
    memcpy(lpPacket->sData.aBuffer, s, lpPacket->sData.sHdr.nSize);
    //do write prepare
    lpPacket->nType = CNktSimplePipesIPC::CPacket::typWriteRequest;
    lpPacket->nOrder = _InterlockedIncrement(&nNextWriteOrder);
    cActiveRwPacketList.QueueLast(lpPacket);
    hRes = S_OK;
    if (::PostQueuedCompletionStatus(hIOCP, 0, (ULONG_PTR)this, &(lpPacket->sOvr)) == FALSE)
    {
      hRes = NKT_HRESULT_FROM_LASTERROR();
      if (hRes != NKT_E_IoPending)
        return hRes;
    }
    AddRef();
    nDataSize -= (SIZE_T)(lpPacket->sData.sHdr.nSize);
    s += (SIZE_T)(lpPacket->sData.sHdr.nSize);
  }
  //done
  if (lplpAnswer != NULL)
    *lplpAnswer = cNewAnswer.Detach();
  return S_OK;
}

HRESULT CNktSimplePipesIPC::CConnection::ProcessReadedPacket(__inout CNktSimplePipesIPC::CPacket *lpPacket)
{
  TNktComPtr<CReadedMessage> cMsg;
  HRESULT hRes;

  {
    CNktAutoFastMutex cLock(&(sReadedMsgs.cMtx));
    TNktLnkLst<CReadedMessage>::Iterator it;
    CReadedMessage *lpReadedMsg;

    for (lpReadedMsg=it.Begin(sReadedMsgs.cList); lpReadedMsg!=NULL; lpReadedMsg=it.Next())
    {
      if (lpReadedMsg->nId == lpPacket->sData.sHdr.nId)
      {
        cMsg = lpReadedMsg;
        break;
      }
    }
    if (cMsg == NULL)
    {
      //if message was not found, check for new chain-index and non-answer packet
      if (lpPacket->sData.sHdr.nChainIndex != 1 || (lpPacket->sData.sHdr.nId & 0x80000000) != 0)
      {
        FreePacket(lpPacket);
        _ASSERT(FALSE);
        return NKT_E_InvalidData;
      }
      cMsg.Attach(new CReadedMessage());
      if (cMsg == NULL)
      {
        FreePacket(lpPacket);
        return E_OUTOFMEMORY;
      }
      hRes = cMsg->Initialize(lpPacket->sData.sHdr.nId, lpPacket->sData.sHdr.nTotalSize);
      if (FAILED(hRes))
      {
        FreePacket(lpPacket);
        return hRes;
      }
      sReadedMsgs.cList.PushHead(cMsg); //probably next packet will be the same id
      ((CReadedMessage*)cMsg)->AddRef();
    }
  }
  hRes = cMsg->Add(lpPacket);
  if (FAILED(hRes))
  {
    FreePacket(lpPacket);
    return hRes;
  }
  if (hRes == S_OK)
  {
    if ((cMsg->nId & 0x80000000) == 0)
    {
      //a message from the other party
      hRes = ProcessReadedMessage(cMsg);
      RemoveReadedMessage(cMsg);
      if (FAILED(hRes))
        return hRes;
    }
    //message completed. remove from the list, if this is an answer, the waiting SendMsg will
    //still hold a reference to it and process
  }
  return S_OK;
}

HRESULT CNktSimplePipesIPC::CConnection::ProcessReadedMessage(
                                                __in CNktSimplePipesIPC::CConnection::CReadedMessage *lpMsg)
{
  CInternalWorkItemThreadProcData *lpData;
  HRESULT hRes;

  lpData = new CInternalWorkItemThreadProcData;
  if (lpData == NULL)
    return E_OUTOFMEMORY;
  lpData->lpMgr = lpMgr;
  lpData->cConn = this;
  lpData->nMsgId = lpMsg->nId;
  hRes = lpMsg->BuildReply(lpMgr, &(lpData->lpMsg), &(lpData->nMsgSize));
  if (SUCCEEDED(hRes) &&
      lpMgr->cThreadPool.QueueTask(InternalWorkItemThreadProc, lpData) == FALSE)
    hRes = NKT_HRESULT_FROM_LASTERROR();
  if (FAILED(hRes))
    delete lpData;
  return S_OK;
}

VOID CNktSimplePipesIPC::CConnection::RemoveReadedMessage(__in CNktSimplePipesIPC::CConnection::CReadedMessage *lpMsg)
{
  CNktAutoFastMutex cLock(&(sReadedMsgs.cMtx));
  CPacket *lpPacket;

  if (lpMsg->GetLinkedList() == &(sReadedMsgs.cList))
  {
    lpMsg->RemoveNode();
    while ((lpPacket=lpMsg->cMsgList.PopTail()) != NULL)
      FreePacket(lpPacket);
    lpMsg->Release();
  }
  return;
}

HRESULT CNktSimplePipesIPC::CConnection::SendInitialReadAhead()
{
  CPacket *lpPacket;
  HRESULT hRes;

  lpPacket = GetPacket();
  if (lpPacket == NULL)
    return E_OUTOFMEMORY;
  lpPacket->nType = CNktSimplePipesIPC::CPacket::typDoInitialSetup;
  cActiveRwPacketList.QueueLast(lpPacket);
  if (::PostQueuedCompletionStatus(hIOCP, 0, (ULONG_PTR)this, &(lpPacket->sOvr)) == FALSE)
  {
    hRes = NKT_HRESULT_FROM_LASTERROR();
    if (hRes != NKT_E_IoPending)
      return hRes;
  }
  AddRef();
  return S_OK;
}

//-----------------------------------------------------------
//-----------------------------------------------------------

CNktSimplePipesIPC::CConnection::CReadedMessage::CReadedMessage() : TNktLnkLstNode<CReadedMessage>(), CNktFastMutex()
{
  _InterlockedExchange(&nRefCount, 1);
  nLastChain = 0;
  nCurrSize = 0;
  return;
}

CNktSimplePipesIPC::CConnection::CReadedMessage::~CReadedMessage()
{
  CPacket *lpPacket;

  while ((lpPacket=cMsgList.PopTail()) != NULL)
    delete lpPacket;
  return;
}

ULONG CNktSimplePipesIPC::CConnection::CReadedMessage::AddRef()
{
  return (ULONG)_InterlockedIncrement(&nRefCount);
}

ULONG CNktSimplePipesIPC::CConnection::CReadedMessage::Release()
{
  ULONG nRef = (ULONG)_InterlockedDecrement(&nRefCount);
  if (nRef == 0)
    delete this;
  return nRef;
}

HRESULT CNktSimplePipesIPC::CConnection::CReadedMessage::Initialize(__in ULONG _nId, __in ULONG _nTotalSize)
{
  if (cDoneEv.Create(TRUE, FALSE) == FALSE)
    return NKT_HRESULT_FROM_LASTERROR();
  nId = _nId;
  if ((nId & 0x80000000) == 0)
  {
    //a message from the other party
    nTotalSize = _nTotalSize;
    if (nTotalSize == 0)
    {
      _ASSERT(FALSE);
      return NKT_E_InvalidData;
    }
  }
  else
  {
    //an answer from the other party
    //preallocate some buffer
    nTotalSize = 0;
  }
  return S_OK;
}

HRESULT CNktSimplePipesIPC::CConnection::CReadedMessage::Add(__in CPacket *lpPacket)
{
  CNktAutoFastMutex cLock(this);

  if (nId == 0 || lpPacket == NULL)
    return E_FAIL;
  if ((nId & 0x80000000) == 0)
  {
    //a message from the other party
    if (nLastChain+1 != lpPacket->sData.sHdr.nChainIndex)
    {
      _ASSERT(FALSE);
      return NKT_E_InvalidData;
    }
  }
  else
  {
    //an answer from the other party
    if (nLastChain+1 != lpPacket->sData.sHdr.nChainIndex)
    {
      _ASSERT(FALSE);
      return NKT_E_InvalidData;
    }
    if (lpPacket->sData.sHdr.nChainIndex == 1)
      nTotalSize = lpPacket->sData.sHdr.nTotalSize;
  }
  //check size
  if (nCurrSize + lpPacket->sData.sHdr.nSize < nCurrSize ||
      nCurrSize + lpPacket->sData.sHdr.nSize > nTotalSize)
  {
    _ASSERT(FALSE);
    return NKT_E_InvalidData;
  }
  //increment last chain index
  nLastChain++;
  //add to queue
  if (lpPacket->sData.sHdr.nSize > 0)
  {
    cMsgList.PushTail(lpPacket);
    nCurrSize += lpPacket->sData.sHdr.nSize;
  }
  if (nCurrSize >= nTotalSize)
  {
    if ((nId & 0x80000000) != 0)
    {
      if (cDoneEv.Set() == FALSE)
        return E_OUTOFMEMORY;
    }
    return S_OK; //got a complete message
  }
  return S_FALSE;
}

HRESULT CNktSimplePipesIPC::CConnection::CReadedMessage::BuildReply(__in CNktSimplePipesIPC *lpThis,
                                                __inout LPVOID *lplpReply, __inout SIZE_T *lpnReplySize)
{
  CNktAutoFastMutex cLock(this);
  TNktLnkLst<CNktSimplePipesIPC::CPacket>::Iterator it;
  CNktSimplePipesIPC::CPacket *lpPacket;
  LPBYTE s;

  _ASSERT(lplpReply != NULL);
  _ASSERT(lpnReplySize != NULL);
  *lpnReplySize = 0;
  for (lpPacket=it.Begin(cMsgList); lpPacket!=NULL; lpPacket=it.Next())
    *lpnReplySize += lpPacket->sData.sHdr.nSize;
  *lplpReply = lpThis->AllocBuffer(*lpnReplySize);
  if ((*lplpReply) == NULL)
    return E_OUTOFMEMORY;
  s = (LPBYTE)(*lplpReply);
  for (lpPacket=it.Begin(cMsgList); lpPacket!=NULL; lpPacket=it.Next())
  {
    memcpy(s, lpPacket->sData.aBuffer, lpPacket->sData.sHdr.nSize);
    s += lpPacket->sData.sHdr.nSize;
  }
  return S_OK;
}

//-----------------------------------------------------------

static DWORD WINAPI InternalWorkItemThreadProc(__in LPVOID lpCtx)
{
  return ((CInternalWorkItemThreadProcData*)lpCtx)->lpMgr->WorkItemThreadProc(lpCtx);
}
