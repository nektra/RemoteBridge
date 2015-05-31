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

#include "ComInstance.h"
#include "ComManager.h"
#include "..\AutoTls.h"
#include "..\..\Common\MallocMacros.h"
#include "..\..\Common\AutoPtr.h"

//-----------------------------------------------------------

namespace COM {

CInstance::CInstance(__in CInterface *_lpInterface, __in REFIID _sIid, __in IUnknown *_lpUnkRef) :
           TNktLnkLstNode<CInstance>(), CBaseRefCounter()
{
  lpInterface = _lpInterface;
  lpMgr = _lpInterface->lpMgr;
  sIid = _sIid;
  lpUnkRef = _lpUnkRef;
  dwTid = ::GetCurrentThreadId();
  return;
}

CInstance::~CInstance()
{
  Invalidate(TRUE, TRUE);
  return;
}

HRESULT CInstance::Initialize(__in IUnknown *lpUnk)
{
  CAutoTls cAutoTls(lpMgr->GetTlsIndex(), (LPVOID)-1); //disable extra release hand.
  TNktComPtr<CMainManager::CThreadEvent_AddInstance> cEvent;
  ULONG nRef[2];
  HRESULT hRes;

  _ASSERT(lpUnk != NULL);
  cEvent.Attach(new CMainManager::CThreadEvent_AddInstance());
  if (cEvent == NULL)
    return E_OUTOFMEMORY;
  hRes = ::CreateStreamOnHGlobal(NULL, TRUE, &cStream);
  if (SUCCEEDED(hRes))
  {
    lpUnk->AddRef();
    nRef[0] = lpUnk->Release();
    hRes = ::CoMarshalInterface(cStream, sIid, lpUnk, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    if (SUCCEEDED(hRes))
    {
      lpUnk->AddRef();
      nRef[1] = lpUnk->Release();
    }
  }
  if (SUCCEEDED(hRes))
  {
    nRefAddedByCoMarshal = nRef[1]-nRef[0];
    cEvent->cInstance = this;
    hRes = lpMgr->ExecuteThreadEvent(cEvent, TRUE);
    if (FAILED(hRes))
      ::CoReleaseMarshalData(cStream);
  }
  if (FAILED(hRes))
    cStream.Release();
  return hRes;
}

VOID CInstance::Invalidate(__in BOOL bReleaseUnk, __in BOOL bReleaseMarshal)
{
  TNktComPtr<IUnknown> _cUnk;
  TNktComPtr<IStream> _cStream;

  {
    CNktAutoFastMutex cLock(this);

    if (bReleaseUnk != FALSE)
      _cUnk.Attach(cUnk.Detach());
    else
      cUnk.Detach();
    _cStream.Attach(cStream.Detach());
    lpInterface = NULL;
    lpUnkRef = NULL;
  }
  if (_cStream != NULL)
  {
    if (bReleaseMarshal != FALSE)
      ::CoReleaseMarshalData(_cStream);
  }
  return;
}

HRESULT CInstance::Unmarshal()
{
  CNktAutoFastMutex cLock(this);
  LARGE_INTEGER liZero;
  ULARGE_INTEGER liCurr;
  HRESULT hRes;

  if (cStream == NULL)
    return (cUnk != NULL) ? S_OK : E_FAIL; //may be previously unmarshaled by force
  liZero.QuadPart = liCurr.QuadPart = 0;
  hRes = cStream->Seek(liZero, STREAM_SEEK_SET, &liCurr);
  if (SUCCEEDED(hRes))
    hRes = ::CoUnmarshalInterface(cStream, GUID_NULL, (LPVOID*)&cUnk);
  if (SUCCEEDED(hRes))
    cStream.Release();
  return hRes;
}

CInterface* CInstance::GetInterface()
{
  CNktAutoFastMutex cLock(this);

  return (lpInterface != NULL) ? lpInterface : NULL;
}

ULONG CInstance::GetInterfaceFlags()
{
  CNktAutoFastMutex cLock(this);

  return (lpInterface != NULL) ? (lpInterface->GetFlags()) : 0;
}

IUnknown* CInstance::GetUnkRef()
{
  return lpUnkRef;
}

BOOL CInstance::CheckSupportedIid(__in REFIID sIid)
{
  return (lpInterface != NULL) ? (lpInterface->cSupportedIids.Check(sIid)) : FALSE;
}

HRESULT CInstance::BuildResponsePacket(__in REFIID sIid, __in SIZE_T nPrefixBytes, __out LPVOID *lplpData,
                                       __out SIZE_T *lpnDataSize)
{
  _ASSERT(lplpData != NULL);
  _ASSERT(lpnDataSize != NULL);
  return lpMgr->BuildInterprocMarshalPacket(cUnk, sIid, nPrefixBytes, lplpData, lpnDataSize);
}

} //COM
