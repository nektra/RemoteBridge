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
class CNktRemoteBridgeImpl;

//-----------------------------------------------------------

typedef struct {
  CNktRemoteBridgeImpl *lpThis;
  HRESULT (CNktRemoteBridgeImpl::*lpfnFindOrCreateNktJavaObject)(__in DWORD dwPid, __in ULONGLONG jobject,
                                                                 __deref_out INktJavaObject **ppJavaObj);
  DWORD dwPid;
} VARCONVERT_UNMARSHALVARIANTS_DATA;

//-----------------------------------------------------------

namespace JAVA {

//Converts a variant or 1D array of variants into a data stream.
//* An element can be a multidimensional array of a supported type
//* If an element (or array) is an IDispatch, it must be an INktJavaObject in order to send its "jobject id"
HRESULT VarConvert_MarshalSafeArray(__in SAFEARRAY *pArray, __in DWORD dwPid, __in LPBYTE lpDest,
                                    __inout SIZE_T &nSize);

//Converts a stream into an array of variants.
//* If there is a jobject in the stream, we parse the definition and buuild new INktJavaObject's (or
//  reuse existing one (see (*env)->IsSameObject(env, obj1, obj2))
HRESULT VarConvert_UnmarshalVariants(__in LPBYTE lpSrc, __in SIZE_T nSrcBytes,
                                     __in VARCONVERT_UNMARSHALVARIANTS_DATA *lpUnmarshalData, __inout VARIANT &vt);

} //JAVA
