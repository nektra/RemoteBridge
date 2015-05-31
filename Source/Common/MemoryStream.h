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

#ifndef _NKT_MEMORYSTREAM_H
#define _NKT_MEMORYSTREAM_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include <stdio.h>

//-----------------------------------------------------------

class CStaticMemoryStream : public IStream
{
public:
  CStaticMemoryStream(__in LPVOID lpData, __in SIZE_T nDataSize);
  ~CStaticMemoryStream();

  STDMETHOD(QueryInterface)(__RPC__in REFIID riid, __RPC__deref_out void **ppvObject);
  STDMETHOD_(ULONG, AddRef)();
  STDMETHOD_(ULONG, Release )();

  STDMETHOD(Read)(__out_bcount_part(cb, *pcbRead) void *pv, __in ULONG cb, __out_opt ULONG *pcbRead);
  STDMETHOD(Write)(__in_bcount(cb)  const void *pv, __in ULONG cb, __out_opt ULONG *pcbWritten);

  STDMETHOD(Seek)(__in LARGE_INTEGER dlibMove, __in DWORD dwOrigin, __out_opt ULARGE_INTEGER *plibNewPosition);

  STDMETHOD(SetSize)(__in ULARGE_INTEGER libNewSize);

  STDMETHOD(CopyTo)(__RPC__in IStream *pstm, __in ULARGE_INTEGER cb, __out_opt ULARGE_INTEGER *pcbRead,
                    __out_opt ULARGE_INTEGER *pcbWritten);

  STDMETHOD(Commit)(__in DWORD grfCommitFlags);

  STDMETHOD(Revert)();

  STDMETHOD(LockRegion)(__in ULARGE_INTEGER libOffset, __in ULARGE_INTEGER cb, __in DWORD dwLockType);
  STDMETHOD(UnlockRegion)(__in ULARGE_INTEGER libOffset, __in ULARGE_INTEGER cb, __in DWORD dwLockType);

  STDMETHOD(Stat)(__RPC__out STATSTG *pstatstg, __in DWORD grfStatFlag);

  STDMETHOD(Clone)(__RPC__deref_out_opt IStream **ppstm);

  VOID Invalidate();

private:
  LPBYTE lpData;
  SIZE_T nPos, nSize;
  LONG volatile nRefCount;
};

//-----------------------------------------------------------

#endif //_NKT_MEMORYSTREAM_H
