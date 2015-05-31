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

#ifndef _NKT_SIMPLEPIPESIPC_H
#define _NKT_SIMPLEPIPESIPC_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include "WaitableObjects.h"
#include "LinkedList.h"
#include "Threads.h"
#include "AutoHandle.h"

//-----------------------------------------------------------

#define E_NKT_PIPECOMM_SHUTDOWN   MAKE_HRESULT(1, 0x811, 0x00000001)

//-----------------------------------------------------------

class CNktSimplePipesIPC : private CNktThread
{
public:
  typedef enum {
    atNone=0, atSTA, atMTA, atOLE
  } eApartmentType;

  CNktSimplePipesIPC(__in eApartmentType nComApartmentType);
  virtual ~CNktSimplePipesIPC();

  VOID Shutdown();

  HRESULT SetupMaster(__out HANDLE &hConn, __out CNktAutoRemoteHandle &cSlavePipe, __in DWORD dwTargetPid);
  HRESULT SetupSlave(__out HANDLE &hConn, __in DWORD dwMasterPid, __in HANDLE hPipe, __in HANDLE hMasterProc);
  VOID Close(__in HANDLE hConn);

  HRESULT IsConnected(__in HANDLE hConn);

  HRESULT SendMsg(__in HANDLE hConn, __in LPVOID lpData, __in SIZE_T nDataSize, __inout_opt LPVOID *lplpReply=NULL,
                  __inout_opt SIZE_T *lpnReplySize=NULL);

  virtual HRESULT OnMessage(__in HANDLE hConn, __in LPBYTE lpData, __in SIZE_T nDataSize,
                            __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize) = 0;

  virtual LPBYTE AllocBuffer(__in SIZE_T nSize);
  virtual VOID FreeBuffer(__in LPVOID lpBuffer);

private:
  friend DWORD WINAPI InternalWorkItemThreadProc(__in LPVOID);

  HRESULT Initialize();
  VOID ThreadProc();
  DWORD WorkItemThreadProc(__in LPVOID lpContext);

  VOID CheckAliveConnections();

  VOID OnTaskTerminated(__in LPVOID lpContext, __in HRESULT hReturnValue);
  VOID OnTaskCancelled(__in LPVOID lpContext);
  VOID OnTaskExceptionError(__in LPVOID lpContext, __in DWORD dwException, __in struct _EXCEPTION_POINTERS *excPtr);

private:
  friend class CInternalWorkItemThreadProcData;
  class CConnection;

  class CPacket : public TNktLnkLstNode<CPacket>
  {
  public:
    CPacket(__in CConnection *lpConn);

    VOID Reset();

    static CPacket* ClassFromOverlapped(__in LPOVERLAPPED lpOvr)
      {
      return (CPacket*)((char*)lpOvr - (char*)&(((CPacket*)0)->sOvr));
      };

  public:
#pragma pack(1)
    typedef struct {
      ULONG nTotalSize;
      ULONG nId;
      ULONG nChainIndex;
      ULONG nSize;
    } HEADER;
#pragma pack()

    typedef enum {
      typDiscard, typDoInitialSetup, typDoRead, typWriteRequest, typDoWrite
    } eType;

    CConnection *lpConn;
    CNktEvent cEv;
    ULONG nOrder;
    OVERLAPPED sOvr;
    struct {
      HEADER sHdr;
      BYTE aBuffer[4096-sizeof(HEADER)];
    } sData;
    eType nType;
  };

  //--------

  class CPacketList : private CNktFastMutex
  {
  public:
    CPacketList();
    ~CPacketList();

    VOID DiscardAll();

    VOID QueueLast(__inout CPacket *lpPacket);
    CPacket* DequeueFirst();

    VOID QueueSorted(__inout CPacket *lpPacket);
    CPacket* Dequeue(__in LONG volatile nOrder);

    VOID Remove(__inout CPacket *lpPacket);

    SIZE_T GetCount();

  private:
    TNktLnkLst<CPacket> cList;
    SIZE_T nCount;
  };

  //--------

  class CConnection : TNktLnkLstNode<CConnection>, public CNktFastMutex
  {
    class CReadedMessage;

  public:
    CConnection(__in CNktSimplePipesIPC *lpMgr);
  protected:
    ~CConnection();

  public:
    ULONG AddRef();
    ULONG Release();

    BOOL IsConnected(__in BOOL DoPeek=FALSE);

    VOID TerminateConnection();

  private:
    HRESULT InitializeMaster(__in DWORD dwSlavePid, __in HANDLE hIOCP, __out CNktAutoRemoteHandle &cSlavePipe);
    HRESULT InitializeSlave(__in DWORD dwMasterPid, __in HANDLE hMasterProc, __in HANDLE hPipe, __in HANDLE hIOCP);

    CNktSimplePipesIPC::CPacket* GetPacket();
    VOID FreePacket(__inout CNktSimplePipesIPC::CPacket *lpPacket);

    HRESULT SendRead();
    HRESULT SendWrite(__in LPVOID lpData, __in ULONG nDataSize, __in ULONG nMsgId, __deref_out CReadedMessage **lplpAnswer);

    HRESULT ProcessReadedPacket(__inout CNktSimplePipesIPC::CPacket *lpPacket);
    HRESULT ProcessReadedMessage(__in CNktSimplePipesIPC::CConnection::CReadedMessage *lpMsg);

    VOID RemoveReadedMessage(__in CReadedMessage *lpMsg);

    HRESULT SendInitialReadAhead();

  private:
    friend class CInternalWorkItemThreadProcData;
    friend class CNktSimplePipesIPC;

    class CReadedMessage : public TNktLnkLstNode<CReadedMessage>, private CNktFastMutex
    {
    public:
      CReadedMessage();
    protected:
      ~CReadedMessage();

    public:
      ULONG AddRef();
      ULONG Release();

      HRESULT Initialize(__in ULONG nId, __in ULONG nTotalSize);

      HRESULT Add(__in CPacket *lpPacket);

      HRESULT BuildReply(__in CNktSimplePipesIPC *lpThis, __inout LPVOID *lplpReply, __inout SIZE_T *lpnReplySize);

    public:
      CNktEvent cDoneEv;
      TNktLnkLst<CPacket> cMsgList;
      ULONG nId, nLastChain;
      ULONG nCurrSize, nTotalSize;
      LONG volatile nRefCount;
    };

    CNktEvent cTerminateEv;
    HANDLE hIOCP;
    //----
    struct {
      DWORD dwPid;
      CNktAutoHandle cProc;
    } sOtherParty;
    CNktAutoHandle cPipe;
    //----
    CPacketList cReadedList;
    LONG volatile nNextReadOrder;
    LONG volatile nNextReadOrderToProcess;
    CPacketList cToWriteList;
    LONG volatile nNextWriteOrder;
    LONG volatile nNextWriteOrderToProcess;
    CPacketList cFreePacketList;
    CPacketList cActiveRwPacketList;
    LONG volatile nNextMsgId;
    //----
    struct {
      CNktFastMutex cMtx;
      TNktLnkLst<CReadedMessage> cList;
    } sReadedMsgs;
    //----
    CNktSimplePipesIPC *lpMgr;
    LONG volatile nRefCount;
  };

  class CMyThreadPool : public CNktThreadPool
  {
  public:
    CMyThreadPool(__in CNktSimplePipesIPC *_lpMgr)
      {
      lpMgr = _lpMgr;
      return;
      };

    LPVOID OnThreadStarted()
      {
      HRESULT hRes;

      switch (lpMgr->nComApartmentType)
      {
        case CNktSimplePipesIPC::atSTA:
          hRes = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
          return (LPVOID)((SUCCEEDED(hRes) && hRes != S_FALSE) ? 1 : 0);
        case CNktSimplePipesIPC::atMTA:
          hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
          return (LPVOID)((SUCCEEDED(hRes) && hRes != S_FALSE) ? 1 : 0);
          break;
        case CNktSimplePipesIPC::atOLE:
          hRes = ::OleInitialize(NULL);
          return (LPVOID)((SUCCEEDED(hRes) && hRes != S_FALSE) ? 2 : 0);
          break;
      }
      return (LPVOID)0;
      };

    VOID OnThreadTerminated(__in LPVOID lpContext)
      {
      switch ((SIZE_T)lpContext)
      {
        case 1:
          ::CoUninitialize();
          break;
        case 2:
          ::OleUninitialize();
          break;
      }
      return;
      };

    VOID OnTaskTerminated(__in LPVOID lpContext, __in HRESULT hReturnValue)
      {
      lpMgr->OnTaskTerminated(lpContext, hReturnValue);
      return;
      };
    VOID OnTaskCancelled(__in LPVOID lpContext)
      {
      lpMgr->OnTaskCancelled(lpContext);
      return;
      };
    VOID OnTaskExceptionError(__in LPVOID lpContext, __in DWORD dwException, __in struct _EXCEPTION_POINTERS *excPtr)
      {
      lpMgr->OnTaskExceptionError(lpContext, dwException, excPtr);
      return;
      };

  private:
    CNktSimplePipesIPC *lpMgr;
  };

private:
  CNktFastMutex cMtx;
  struct {
    CNktFastMutex cMtx;
    TNktLnkLst<CConnection> cList;
  } sConnections;
  eApartmentType nComApartmentType;
  HANDLE hIOCP;
  CMyThreadPool cThreadPool;
};

//-----------------------------------------------------------

#endif //_NKT_SIMPLEIPC_H
