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

#include "..\Common\SimplePipesIPC.h"
#include "..\Common\TransportMessages.h"

//-----------------------------------------------------------

class CClientMgr : private CNktThread
{
public:
  class CClient
  {
  public:
    CClient(__in CClientMgr *lpMgr, __in DWORD dwPid, __in HANDLE hConn);
    ~CClient();

    ULONG AddRef();
    ULONG Release();

    DWORD GetPid()
      {
      return dwPid;
      };

    HANDLE GetConn()
      {
      return hConn;
      };

    VOID Shutdown();

    HANDLE GetProcVmHandle();

  private:
    friend class CClientMgr;
    friend class CNktRemoteBridgeImpl;

    CNktFastMutex cMtx;
    CNktEvent cShutdownEv;
    LONG volatile nRefCount;
    CClientMgr *lpMgr;
    DWORD dwPid;
    HANDLE hProc;
    HANDLE volatile hConn;
    ULONG nHookFlags;
  };

  CClientMgr();
  ~CClientMgr();

  HRESULT AddClient(__in DWORD dwPid, __deref_out CClient **lplpClient, __out CNktAutoRemoteHandle &cSlavePipe);
  BOOL RemoveClient(__in DWORD dwPid);
  VOID RemoveAllClients();

  HRESULT SendMsg(__in CClient *lpClient, __in LPVOID lpData, __in SIZE_T nDataSize,
                  __inout_opt LPVOID *lplpReply=NULL, __inout_opt SIZE_T *lpnReplySize=NULL);

  HRESULT GetClientByPid(__in DWORD dwPid, __deref_out CClient **lplpClient);
  HRESULT GetClientByConn(__in HANDLE hConn, __deref_out CClient **lplpClient);

  VOID ThreadProc();

  virtual HRESULT OnClientMessage(__in CClient *lpClient, __in LPBYTE lpData, __in SIZE_T nDataSize,
                                  __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize) = 0;
  virtual VOID OnClientDisconnected(__in CClient *lpClient) = 0;

  LPBYTE AllocIpcBuffer(__in SIZE_T nSize);
  VOID FreeIpcBuffer(__in LPVOID lpBuffer);

private:
  friend class CClient;
  friend class CNktRemoteBridgeImpl;

  class CMySimpleIPC : public CNktSimplePipesIPC
  {
  public:
    CMySimpleIPC(__in CClientMgr *_lpMgr) : CNktSimplePipesIPC(CNktSimplePipesIPC::atMTA)
      {
      lpMgr = _lpMgr;
      return;
      };

  private:
    HRESULT OnMessage(__in HANDLE hConn, __in LPBYTE lpData, __in SIZE_T nDataSize,
                      __out LPVOID *lplpReplyData, __out SIZE_T *lpnReplyDataSize)
      {
      TNktComPtr<CClient> cClient;
      HRESULT hRes;

      hRes = lpMgr->GetClientByConn(hConn, &cClient);
      if (SUCCEEDED(hRes))
        hRes = lpMgr->OnClientMessage(cClient, lpData, nDataSize, lplpReplyData, lpnReplyDataSize);
      return hRes;
      };

  private:
    CClientMgr *lpMgr;
  };

  SIZE_T FindClientByPid(__in DWORD dwPid);
  SIZE_T FindClientByConn(__in HANDLE hConn);

  static int CLIENT_ITEM_Compare(void *, const void *, const void *);

  CNktFastMutex cMtx;
  CMySimpleIPC cConn;

  typedef struct tagClient {
    DWORD dwPid;
    HANDLE hConn;
    CClient *lpClient;
  } CLIENT;

  struct tagClients {
    CLIENT *lpListSortByPid;
    CLIENT *lpListSortByConn;
    SIZE_T nCount, nSize;
  } sClients;
};
