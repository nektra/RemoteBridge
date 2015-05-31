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

#include "..\MainManager.h"
#include "JavaManager.h"
#include "JavaClassCacheMgr.h"
#include "JavaUtils.h"
#include "..\..\Common\AutoPtr.h"
#include "..\..\Common\ComPtr.h"
#include "..\HookTrampoline.h"
#define X_BYTE_ENC(x, y) (x)
#if defined _M_IX86
  static
  #include "Asm\Native_RaiseCallback_x86.inl"
#elif defined _M_X64
  static
  #include "Asm\Native_RaiseCallback_x64.inl"
#else
  #error Platform not supported
#endif

//-----------------------------------------------------------

#define MODIFIER_Static                                    8
#define MODIFIER_Native                                  256
#define MODIFIER_INTERNAL_NOT_EXPOSED_VarArgs            128
#define MODIFIER_INTERNAL_NOT_EXPOSED_Synthetic         4096
#define MODIFIER_INTERNAL_NOT_EXPOSED_Enum             16384

//-----------------------------------------------------------

#if defined _M_IX86

typedef struct {
  JNIEnv *env;
  jobject obj_or_class;
  LPVOID lp3rdParameter;
  DWORD __unnamed1;
  union {
    ULONGLONG retValue;
    jvalue varRet;
  };
} NATIVECALLED_DATA;

typedef struct {
  BYTE returnType;
  BYTE __unnamed1;
  WORD retJumpSize;
} NATIVECALLED_EXTRAINFO;

#elif defined _M_X64

typedef struct {
  JNIEnv *env;
  jobject obj_or_class;
  ULONGLONG _R8;
  ULONGLONG _R9;
  double _XMM2;
  double _XMM3;
  LPVOID lp5thParameter;
  union {
    ULONGLONG retValue;
    jvalue varRet;
  };
} NATIVECALLED_DATA;

typedef struct {
  BYTE returnType;
} NATIVECALLED_EXTRAINFO;

#endif

//-----------------------------------------------------------

static int JavaClass_Search(__in void *, __in LPCWSTR lpElem1, __in JAVA::CClassDef **lpElem2);
static int JavaClass_Compare(__in void *, __in JAVA::CClassDef **lpElem1, __in JAVA::CClassDef **lpElem2);

//-----------------------------------------------------------

namespace JAVA {

CJValueDef::CJValueDef() : TNktLnkLstNode<CJValueDef>()
{
  nType = typVoid;
  nArrayDims = 0;
  lpClassDef = NULL;
  lpParentMethodOrFieldDef = NULL;
  return;
}

HRESULT CJValueDef::ParseType(__in_z LPCWSTR szTypeName)
{
  static struct {
    LPWSTR szLongNameW;
    WCHAR chShortNameW;
    CJValueDef::eType nType;
  } aBasicTypes[] = {
    { L"void",    L'V', CJValueDef::typVoid },
    { L"boolean", L'Z', CJValueDef::typBoolean },
    { L"char",    L'C', CJValueDef::typChar },
    { L"byte",    L'B', CJValueDef::typByte },
    { L"short",   L'S', CJValueDef::typShort },
    { L"int",     L'I', CJValueDef::typInt },
    { L"long",    L'J', CJValueDef::typLong },
    { L"float",   L'F', CJValueDef::typFloat },
    { L"double",  L'D', CJValueDef::typDouble },
    { NULL,       0,    CJValueDef::typVoid }
  };
  SIZE_T i, nLen;

  if (szTypeName == NULL)
    return E_POINTER;
  nArrayDims = 0;
  for (i=0; aBasicTypes[i].szLongNameW != NULL; i++)
  {
    if (wcscmp(szTypeName, aBasicTypes[i].szLongNameW) == 0)
    {
      nType = aBasicTypes[i].nType;
      return S_OK;
    }
  }
  //get dimensions count
  while (*szTypeName == L'[' && nArrayDims <= 128)
  {
    nArrayDims++;
    szTypeName++;
  }
  if (nArrayDims > 32)
    return E_NOTIMPL;
  //get type
  if (*szTypeName == L'L')
  {
    szTypeName++;
    for (nLen=0; szTypeName[nLen]!=0 && szTypeName[nLen]!=L';'; nLen++);
  }
  else
  {
    nLen = wcslen(szTypeName);
    if (nLen == 1)
    {
      for (i=0; aBasicTypes[i].szLongNameW != NULL; i++)
      {
        if (szTypeName[0] == aBasicTypes[i].chShortNameW)
        {
          nType = aBasicTypes[i].nType;
          return S_OK;
        }
      }
    }
  }
  if (nLen == 0)
    return E_NOTIMPL;
  switch (nLen)
  {
    case 14:
      if (wcsncmp(szTypeName, L"java/util/Date", nLen) == 0)
      {
        nType = CJValueDef::typDate;
        return S_OK;
      }
      if (wcsncmp(szTypeName, L"java/lang/Byte", nLen) == 0)
      {
        nType = CJValueDef::typByteObj;
        return S_OK;
      }
      if (wcsncmp(szTypeName, L"java/lang/Long", nLen) == 0)
      {
        nType = CJValueDef::typLongObj;
        return S_OK;
      }
      break;

    case 15:
      if (wcsncmp(szTypeName, L"java/lang/Short", nLen) == 0)
      {
        nType = CJValueDef::typShortObj;
        return S_OK;
      }
      if (wcsncmp(szTypeName, L"java/lang/Float", nLen) == 0)
      {
        nType = CJValueDef::typFloatObj;
        return S_OK;
      }
      break;

    case 16:
      if (wcsncmp(szTypeName, L"java/lang/String", nLen) == 0)
      {
        nType = CJValueDef::typString;
        return S_OK;
      }
      if (wcsncmp(szTypeName, L"java/lang/Double", nLen) == 0)
      {
        nType = CJValueDef::typDoubleObj;
        return S_OK;
      }
      break;

    case 17:
      if (wcsncmp(szTypeName, L"java/lang/Integer", nLen) == 0)
      {
        nType = CJValueDef::typIntObj;
        return S_OK;
      }
      if (wcsncmp(szTypeName, L"java/lang/Boolean", nLen) == 0)
      {
        nType = CJValueDef::typBooleanObj;
        return S_OK;
      }
      break;

    case 18:
      if (wcsncmp(szTypeName, L"java/util/Calendar", nLen) == 0)
      {
        nType = CJValueDef::typCalendar;
        return S_OK;
      }
      break;

    case 19:
      if (wcsncmp(szTypeName, L"java/lang/Character", nLen) == 0)
      {
        nType = CJValueDef::typCharObj;
        return S_OK;
      }
      break;

    case 20:
      if (wcsncmp(szTypeName, L"java/math/BigDecimal", nLen) == 0)
      {
        nType = CJValueDef::typBigDecimal;
        return S_OK;
      }
      break;

    case 38:
      if (wcsncmp(szTypeName, L"java/util/concurrent/atomic/AtomicLong", nLen) == 0)
      {
        nType = CJValueDef::typAtomicLongObj;
        return S_OK;
      }
      break;

    case 41:
      if (wcsncmp(szTypeName, L"java/util/concurrent/atomic/AtomicInteger", nLen) == 0)
      {
        nType = CJValueDef::typAtomicIntObj;
        return S_OK;
      }
      break;
  }
  nType = CJValueDef::typObject;
  if (cStrJObjectClassNameW.CopyN(szTypeName, nLen) == FALSE)
    return E_OUTOFMEMORY;
  return S_OK;
}

HRESULT CJValueDef::InitReferences(__in CJavaClassCacheManager *lpMgr, __in JNIEnv *env)
{
  LPSTR szClassNameA;
  TJavaAutoPtr<jclass> cCls;
  TNktComPtr<CClassDef> cClassDef;
  HRESULT hRes;

  if (nType == CJValueDef::typObject)
  {
    _ASSERT(lpParentMethodOrFieldDef != NULL);
    _ASSERT(lpParentMethodOrFieldDef->GetParentClassDef() != NULL);
    if (wcscmp((LPWSTR)cStrJObjectClassNameW, (LPWSTR)(lpParentMethodOrFieldDef->GetParentClassDef()->cStrNameW)) == 0)
    {
      //value is referencing parent class (like a copy operator)
      lpClassDef = lpParentMethodOrFieldDef->GetParentClassDef();
    }
    else
    {
      szClassNameA = CNktStringW::Wide2Ansi(cStrJObjectClassNameW);
      if (szClassNameA == NULL)
        return E_OUTOFMEMORY;
      cCls.Attach(env, env->FindClass(szClassNameA));
      NKT_FREE(szClassNameA);
      if (cCls == NULL)
        return NKT_E_NotFound;
      hRes = lpMgr->InternalFindOrCreateClass(env, cCls, &cClassDef);
      if (FAILED(hRes))
        return hRes;
      lpClassDef = (CClassDef*)cClassDef;
    }
  }
  return S_OK;
}

HRESULT CJValueDef::GetSignatureW(__inout CNktStringW &cStrW)
{
  SIZE_T i;

  cStrW.Empty();
  for (i=(SIZE_T)nArrayDims; i>=8; i++)
  {
    if (cStrW.Concat(L"[[[[[[[[") == FALSE)
      return E_OUTOFMEMORY;
  }
  for (; i>0; i--)
  {
    if (cStrW.Concat(L"[") == FALSE)
      return E_OUTOFMEMORY;
  }
  switch (nType)
  {
    case typVoid:
      if (cStrW.Concat(L"V") == FALSE)
        return E_OUTOFMEMORY;
      return S_OK;
    case typBoolean:
      if (cStrW.Concat(L"Z") == FALSE)
        return E_OUTOFMEMORY;
      return S_OK;
    case typByte:
      if (cStrW.Concat(L"B") == FALSE)
        return E_OUTOFMEMORY;
      return S_OK;
    case typChar:
      if (cStrW.Concat(L"C") == FALSE)
        return E_OUTOFMEMORY;
      return S_OK;
    case typShort:
      if (cStrW.Concat(L"S") == FALSE)
        return E_OUTOFMEMORY;
      return S_OK;
    case typInt:
      if (cStrW.Concat(L"I") == FALSE)
        return E_OUTOFMEMORY;
      return S_OK;
    case typLong:
      if (cStrW.Concat(L"J") == FALSE)
        return E_OUTOFMEMORY;
      return S_OK;
    case typFloat:
      if (cStrW.Concat(L"F") == FALSE)
        return E_OUTOFMEMORY;
      return S_OK;
    case typDouble:
      if (cStrW.Concat(L"D") == FALSE)
        return E_OUTOFMEMORY;
      return S_OK;
    case typString:
      if (cStrW.Concat(L"Ljava/lang/String;") == FALSE)
        return E_OUTOFMEMORY;
      return S_OK;
    case typBigDecimal:
      if (cStrW.Concat(L"Ljava/math/BigDecimal;") == FALSE)
        return E_OUTOFMEMORY;
      return S_OK;
    case typDate:
      if (cStrW.Concat(L"Ljava/util/Date;") == FALSE)
        return E_OUTOFMEMORY;
      return S_OK;
    case typCalendar:
      if (cStrW.Concat(L"Ljava/util/Calendar;") == FALSE)
        return E_OUTOFMEMORY;
      return S_OK;
    case typObject:
      if (cStrW.Concat(L"L") == FALSE ||
          cStrW.Concat((LPWSTR)cStrJObjectClassNameW) == FALSE ||
          cStrW.Concat(L";") == FALSE)
        return E_OUTOFMEMORY;
      return S_OK;
    case typBooleanObj:
      if (cStrW.Concat(L"Ljava/lang/Boolean;") == FALSE)
        return E_OUTOFMEMORY;
      return S_OK;
    case typByteObj:
      if (cStrW.Concat(L"Ljava/lang/Byte;") == FALSE)
        return E_OUTOFMEMORY;
      return S_OK;
    case typCharObj:
      if (cStrW.Concat(L"Ljava/lang/Character;") == FALSE)
        return E_OUTOFMEMORY;
      return S_OK;
    case typShortObj:
      if (cStrW.Concat(L"Ljava/lang/Short;") == FALSE)
        return E_OUTOFMEMORY;
      return S_OK;
    case typIntObj:
      if (cStrW.Concat(L"Ljava/lang/Integer;") == FALSE)
        return E_OUTOFMEMORY;
      return S_OK;
    case typLongObj:
      if (cStrW.Concat(L"Ljava/lang/Long;") == FALSE)
        return E_OUTOFMEMORY;
      return S_OK;
    case typFloatObj:
      if (cStrW.Concat(L"Ljava/lang/Float;") == FALSE)
        return E_OUTOFMEMORY;
      return S_OK;
    case typDoubleObj:
      if (cStrW.Concat(L"Ljava/lang/Double;") == FALSE)
        return E_OUTOFMEMORY;
      return S_OK;
    case typAtomicIntObj:
      if (cStrW.Concat(L"Ljava/util/concurrent/atomic/AtomicInteger;") == FALSE)
        return E_OUTOFMEMORY;
      return S_OK;
    case typAtomicLongObj:
      if (cStrW.Concat(L"Ljava/util/concurrent/atomic/AtomicLong;") == FALSE)
        return E_OUTOFMEMORY;
      return S_OK;
  }
  _ASSERT(FALSE);
  return E_FAIL;
}

//-----------------------------------------------------------

CMethodOrFieldDef::CMethodOrFieldDef() : TNktLnkLstNode<CMethodOrFieldDef>()
{
  nType = (eType)0;
  bIsNative = FALSE;
  nJValueDefParamsCount = 0;
  nMethodId = NULL;
  nFieldId = NULL;
  lpParentClassDef = NULL;
  cJValueDefReturn.lpParentMethodOrFieldDef = this;
  return;
}

CMethodOrFieldDef::~CMethodOrFieldDef()
{
  CJValueDef *lpJValueDef;

  while ((lpJValueDef = cJValueDefParams.PopHead()) != NULL)
    delete lpJValueDef;
  return;
}

HRESULT CMethodOrFieldDef::AddParameters(__in JNIEnv *env, __in jobject obj)
{
  TNktAutoDeletePtr<CJValueDef> cJValueDef;
  TJavaAutoPtr<jclass> cClsDescr;
  jmethodID getParameterTypes_MethodId;
  TJavaAutoPtr<jobjectArray> cParamsArrayObj;
  TJavaAutoPtr<jobject> cParamObj;
  jsize i, nCount;
  CNktStringW cStrNameW;
  HRESULT hRes;

  cClsDescr.Attach(env, env->GetObjectClass(obj));
  if (cClsDescr == NULL)
    return E_FAIL;
  getParameterTypes_MethodId = env->GetMethodID(cClsDescr, "getParameterTypes", "()[Ljava/lang/Class;");
  if (getParameterTypes_MethodId == NULL)
    return E_FAIL;
  cParamsArrayObj.Attach(env, (jobjectArray)(env->CallObjectMethod(obj, getParameterTypes_MethodId)));
  if (cParamsArrayObj == NULL)
    return E_OUTOFMEMORY;
  nCount = env->GetArrayLength(cParamsArrayObj);
  for (i=0; i<nCount; i++)
  {
    cParamObj.Release();
    cParamObj.Attach(env, env->GetObjectArrayElement(cParamsArrayObj, i));
    if (cParamObj == NULL)
      return E_FAIL;
    //create new parameter
    cJValueDef.Attach(new CJValueDef());
    if (cJValueDef == NULL)
      return E_OUTOFMEMORY;
    cJValueDef->lpParentMethodOrFieldDef = this;
    //get type
    hRes = GetJObjectName(cStrNameW, env, cParamObj);
    if (SUCCEEDED(hRes))
      hRes = cJValueDef->ParseType((LPWSTR)cStrNameW);
    //error?
    if (FAILED(hRes))
      return hRes;
    //add to list
    cJValueDefParams.PushTail(cJValueDef.Detach());
    nJValueDefParamsCount++;
  }
  return S_OK;
}

HRESULT CMethodOrFieldDef::AddReturnResult(__in JNIEnv *env, __in jobject obj)
{
  TJavaAutoPtr<jclass> cClsDescr;
  jmethodID methodId;
  TJavaAutoPtr<jobject> cResultObj;
  CNktStringW cStrNameW;
  HRESULT hRes;

  cClsDescr.Attach(env, env->GetObjectClass(obj));
  if (cClsDescr == NULL)
    return E_FAIL;
  switch (nType)
  {
    case CMethodOrFieldDef::typField:
    case CMethodOrFieldDef::typStaticField:
      methodId = env->GetMethodID(cClsDescr, "getType", "()Ljava/lang/Class;");
      break;
    default:
      methodId = env->GetMethodID(cClsDescr, "getReturnType", "()Ljava/lang/Class;");
      break;
  }
  if (methodId == NULL)
    return E_FAIL;
  cResultObj.Attach(env, env->CallObjectMethod(obj, methodId));
  if (cResultObj == NULL)
    return E_OUTOFMEMORY;
  //get type
  hRes = GetJObjectName(cStrNameW, env, cResultObj);
  if (SUCCEEDED(hRes))
    hRes = cJValueDefReturn.ParseType((LPWSTR)cStrNameW);
  //done
  if (FAILED(hRes))
    return hRes;
  return S_OK;
}

HRESULT CMethodOrFieldDef::InitReferences(__in CJavaClassCacheManager *lpMgr, __in JNIEnv *env)
{
  TNktLnkLst<CJValueDef>::Iterator it;
  CJValueDef *lpJValueDef;
  HRESULT hRes;

  hRes = S_OK;
  for (lpJValueDef=it.Begin(cJValueDefParams); SUCCEEDED(hRes) && lpJValueDef!=NULL; lpJValueDef=it.Next())
    hRes = lpJValueDef->InitReferences(lpMgr, env);
  if (SUCCEEDED(hRes))
    hRes = cJValueDefReturn.InitReferences(lpMgr, env);
  return hRes;
}

LPSTR CMethodOrFieldDef::BuildSignatureA()
{
  CNktStringW cStrTempW, cStrSigW;
  TNktLnkLst<CJValueDef>::Iterator it;
  CJValueDef *lpJValueDef;

  if (nType == typMethod || nType == typStaticMethod)
  {
    if (cStrTempW.Concat(L"(") == FALSE)
      return NULL;
    for (lpJValueDef=it.Begin(cJValueDefParams); lpJValueDef!=NULL; lpJValueDef=it.Next())
    {
      if (FAILED(lpJValueDef->GetSignatureW(cStrSigW)) ||
          cStrTempW.Concat((LPWSTR)cStrSigW) == FALSE)
        return NULL;
    }
    if (cStrTempW.Concat(L")") == FALSE)
      return NULL;
  }
  if (FAILED(cJValueDefReturn.GetSignatureW(cStrSigW)) ||
      cStrTempW.Concat((LPWSTR)cStrSigW) == FALSE)
    return NULL;
  return CNktStringW::Wide2Ansi(cStrTempW);
}

HRESULT CMethodOrFieldDef::OnNativeRaiseCallback(__in LPVOID _lpData)
{
  NATIVECALLED_DATA *lpData = (NATIVECALLED_DATA*)_lpData;
  TNktLnkLst<CJValueDef>::Iterator it;
  CJValueDef *lpJValueDef;
  TNktAutoFreePtr<jvalue> aJValues;
  jvalue aJValuesTemp[16], *lpJValues;
  LPBYTE lpSrc;
  SIZE_T i;

  lpData->retValue = 0;
  if (nJValueDefParamsCount > NKT_ARRAYLEN(aJValuesTemp))
  {
    aJValues.Attach((jvalue*)NKT_MALLOC(nJValueDefParamsCount * sizeof(jvalue)));
    if (aJValues == NULL)
      return E_OUTOFMEMORY;
    lpJValues = aJValues.Get();
  }
  else if (nJValueDefParamsCount > 0)
  {
    lpJValues = aJValuesTemp;
  }
  else
  {
    lpJValues = NULL;
  }
#if defined _M_IX86
  lpSrc = (LPBYTE)(lpData->lp3rdParameter);
  for (i=0,lpJValueDef=it.Begin(cJValueDefParams); lpJValueDef!=NULL; lpJValueDef=it.Next(),i++)
  {
    if (lpJValueDef->nArrayDims > 0)
    {
      lpJValues[i].l = *((jobject*)lpSrc);
      lpSrc += 4;
    }
    else
    {
      switch (lpJValueDef->nType)
      {
        case CJValueDef::typBoolean:
          lpJValues[i].z = *((jboolean*)lpSrc);
          lpSrc += 4;
          break;
        case CJValueDef::typByte:
          lpJValues[i].b = *((jbyte*)lpSrc);
          lpSrc += 4;
          break;
        case CJValueDef::typChar:
          lpJValues[i].c = *((jchar*)lpSrc);
          lpSrc += 4;
          break;
        case CJValueDef::typShort:
          lpJValues[i].s = *((jshort*)lpSrc);
          lpSrc += 4;
          break;
        case CJValueDef::typInt:
          lpJValues[i].i = *((jint*)lpSrc);
          lpSrc += 4;
          break;
        case CJValueDef::typLong:
          lpJValues[i].j = *((jlong*)lpSrc);
          lpSrc += 8;
          break;
        case CJValueDef::typFloat:
          lpJValues[i].f = *((jfloat*)lpSrc);
          lpSrc += 4;
          break;
        case CJValueDef::typDouble:
          lpJValues[i].d = *((jdouble*)lpSrc);
          lpSrc += 8;
          break;
        case CJValueDef::typVoid:
          return E_NOTIMPL;
        default:
          lpJValues[i].l = *((jobject*)lpSrc);
          lpSrc += 4;
          break;
      }
    }
  }
#elif defined _M_X64
  for (i=0,lpJValueDef=it.Begin(cJValueDefParams); lpJValueDef!=NULL; lpJValueDef=it.Next(),i++)
  {
    switch (i)
    {
      case 0:
        if (lpJValueDef->nType == CJValueDef::typFloat || lpJValueDef->nType == CJValueDef::typDouble)
          lpSrc = (LPBYTE)&(lpData->_XMM2);
        else
          lpSrc = (LPBYTE)&(lpData->_R8);
        break;
      case 1:
        if (lpJValueDef->nType == CJValueDef::typFloat || lpJValueDef->nType == CJValueDef::typDouble)
          lpSrc = (LPBYTE)&(lpData->_XMM3);
        else
          lpSrc = (LPBYTE)&(lpData->_R9);
        break;
      case 2:
        lpSrc = (LPBYTE)(lpData->lp5thParameter);
        break;
      default:
        lpSrc += 8;
        break;
    }
    if (lpJValueDef->nArrayDims > 0)
    {
      lpJValues[i].l = *((jobject*)lpSrc);
    }
    else
    {
      switch (lpJValueDef->nType)
      {
        case CJValueDef::typBoolean:
          lpJValues[i].z = *((jboolean*)lpSrc);
          break;
        case CJValueDef::typByte:
          lpJValues[i].b = *((jbyte*)lpSrc);
          break;
        case CJValueDef::typChar:
          lpJValues[i].c = *((jchar*)lpSrc);
          break;
        case CJValueDef::typShort:
          lpJValues[i].s = *((jshort*)lpSrc);
          break;
        case CJValueDef::typInt:
          lpJValues[i].i = *((jint*)lpSrc);
          break;
        case CJValueDef::typLong:
          lpJValues[i].j = *((jlong*)lpSrc);
          break;
        case CJValueDef::typFloat:
          lpJValues[i].f = *((jfloat*)lpSrc);
          break;
        case CJValueDef::typDouble:
          lpJValues[i].d = *((jdouble*)lpSrc);
          break;
        case CJValueDef::typVoid:
          return E_NOTIMPL;
        default:
          lpJValues[i].l = *((jobject*)lpSrc);
          break;
      }
    }
  }
#endif
  return lpParentClassDef->lpCacheMgr->lpMainMgr->OnNativeRaiseCallback(lpData->env,
                (LPWSTR)(lpParentClassDef->cStrNameW), (LPWSTR)cStrNameW, lpData->obj_or_class,
                lpJValues, this, &(lpData->varRet));
}

VOID __stdcall CMethodOrFieldDef::Native_raiseCallbackStub(__in CMethodOrFieldDef *lpCtx, __in LPVOID lpData)
{
  HRESULT hRes;

  hRes = lpCtx->OnNativeRaiseCallback(lpData);
  if (FAILED(hRes))
    ThrowJavaException(((NATIVECALLED_DATA*)lpData)->env, hRes);
  return;
}

//-----------------------------------------------------------

CClassDef::CClassDef()
{
  NktInterlockedExchange(&nRefCount, 1);
  sNatives.nCount = 0;
  sNatives.hHeap = NULL;
  bBeingCreated = TRUE;
  globalJClass = NULL;
  return;
}

CClassDef::~CClassDef()
{
  CMethodOrFieldDef *lpMethodOrFieldDef;

  while ((lpMethodOrFieldDef = cConstructors.PopHead()) != NULL)
    delete lpMethodOrFieldDef;
  while ((lpMethodOrFieldDef = cMethods.PopHead()) != NULL)
    delete lpMethodOrFieldDef;
  while ((lpMethodOrFieldDef = cFields.PopHead()) != NULL)
    delete lpMethodOrFieldDef;
  return;
}

STDMETHODIMP CClassDef::QueryInterface(__in REFIID riid, __RPC__deref_out  void **ppvObject)
{
  return E_NOTIMPL;
}

STDMETHODIMP_(ULONG) CClassDef::AddRef()
{
  return (ULONG)NktInterlockedIncrement(&nRefCount);
}

STDMETHODIMP_(ULONG) CClassDef::Release()
{
  ULONG nRef;

  nRef = NktInterlockedDecrement(&nRefCount);
  if (nRef == 0)
  {
    NktInterlockedExchange(&nRefCount, -1000);
    delete this;
  }
  return nRef;
}

CMethodOrFieldDef* CClassDef::FindMethod(__in LPCWSTR szMethodNameW)
{
  TNktLnkLst<CMethodOrFieldDef>::Iterator it;
  CMethodOrFieldDef *lpMethodOrFieldDef;

  if (szMethodNameW == NULL || szMethodNameW[0] == 0)
    return NULL;
  for (lpMethodOrFieldDef=it.Begin(cMethods); lpMethodOrFieldDef!=NULL; lpMethodOrFieldDef=it.Next())
  {
    if (wcscmp((LPWSTR)(lpMethodOrFieldDef->cStrNameW), szMethodNameW) == 0)
      return lpMethodOrFieldDef;
  }
  return NULL;
}

CMethodOrFieldDef* CClassDef::FindField(__in LPCWSTR szFieldNameW)
{
  TNktLnkLst<CMethodOrFieldDef>::Iterator it;
  CMethodOrFieldDef *lpMethodOrFieldDef;

  if (szFieldNameW == NULL || szFieldNameW[0] == 0)
    return NULL;
  for (lpMethodOrFieldDef=it.Begin(cFields); lpMethodOrFieldDef!=NULL; lpMethodOrFieldDef=it.Next())
  {
    if (wcscmp((LPWSTR)(lpMethodOrFieldDef->cStrNameW), szFieldNameW) == 0)
      return lpMethodOrFieldDef;
  }
  return NULL;
}

HRESULT CClassDef::RegisterNatives(__in JNIEnv *env, __in HANDLE hHeap)
{
  TNktLnkLst<CMethodOrFieldDef>::Iterator it;
  CMethodOrFieldDef *lpCurr;
  TNktAutoFreePtr<JNINativeMethod> aMethods;
  TNktArrayListWithFree<LPSTR> aNamesList;
  TNktArrayListWithFree<LPSTR> aSignaturesList;
  LPSTR szTempA;
  SIZE_T nCurr, nNativesCount;
  jint nJniErr;
  HRESULT hRes;

  nNativesCount = 0;
  for (lpCurr=it.Begin(cMethods); lpCurr!=NULL; lpCurr=it.Next())
  {
    if (lpCurr->bIsNative != FALSE && lpCurr->bIsSuperClass == FALSE)
      nNativesCount++;
  }
  hRes = S_OK;
  if (nNativesCount > 0)
  {
    aMethods.Attach((JNINativeMethod*)NKT_MALLOC(nNativesCount*sizeof(JNINativeMethod)));
    if (aMethods == NULL)
      return E_OUTOFMEMORY;
    memset(aMethods.Get(), 0, nNativesCount*sizeof(JNINativeMethod));
    sNatives.aInfo.Attach((NATIVEMETHOD_INFO*)NKT_MALLOC(nNativesCount*sizeof(NATIVEMETHOD_INFO)));
    if (sNatives.aInfo == NULL)
      return E_OUTOFMEMORY;
    memset(sNatives.aInfo.Get(), 0, nNativesCount*sizeof(NATIVEMETHOD_INFO));
    nCurr = 0;
    for (lpCurr=it.Begin(cMethods); SUCCEEDED(hRes) && lpCurr!=NULL; lpCurr=it.Next())
    {
      if (lpCurr->bIsNative == FALSE || lpCurr->bIsSuperClass != FALSE)
        continue;
      //----
      szTempA = CNktStringW::Wide2Ansi(lpCurr->cStrNameW);
      if (szTempA != NULL)
      {
        if (aNamesList.AddElement(szTempA) == FALSE)
        {
          NKT_FREE(szTempA);
          hRes = E_OUTOFMEMORY;
        }
        (aMethods.Get())[nCurr].name = szTempA;
      }
      else
      {
        hRes = E_OUTOFMEMORY;
      }
      //----
      if (SUCCEEDED(hRes))
      {
        szTempA = lpCurr->BuildSignatureA();
        if (szTempA != NULL)
        {
          if (aSignaturesList.AddElement(szTempA) == FALSE)
          {
            NKT_FREE(szTempA);
            hRes = E_OUTOFMEMORY;
          }
          (aMethods.Get())[nCurr].signature = szTempA;
        }
        else
        {
          hRes = E_OUTOFMEMORY;
        }
      }
      //----
      if (SUCCEEDED(hRes))
      {
        LPBYTE lpTrampoline;

        lpTrampoline = HOOKTRAMPOLINE2CREATE_WITHOBJ(hHeap, aNativeRaiseCallback, lpCurr,
                                                     &CMethodOrFieldDef::Native_raiseCallbackStub, TRUE);
        if (lpTrampoline != NULL)
        {
          union {
            NATIVECALLED_EXTRAINFO sExtraInfo;
            LPVOID lpData;
          } u;
#if defined _M_IX86
          TNktLnkLst<CJValueDef>::Iterator itJValues;
          CJValueDef *lpJValueDef;
#endif //_M_IX86

          u.lpData = NULL;
#if defined _M_IX86
          //if return value a float or double...
          switch (lpCurr->cJValueDefReturn.nType)
          {
            case CJValueDef::typLong:
              u.sExtraInfo.returnType = 1;
              break;
            case CJValueDef::typFloat:
            case CJValueDef::typDouble:
              u.sExtraInfo.returnType = 2;
              break;
            default:
              u.sExtraInfo.returnType = 0;
              break;
          }
          //calculate size of parameters
          u.sExtraInfo.retJumpSize = 0;
          for (lpJValueDef=itJValues.Begin(lpCurr->cJValueDefParams); lpJValueDef!=NULL; lpJValueDef=itJValues.Next())
          {
            switch (lpJValueDef->nType)
            {
              case CJValueDef::typDouble:
              case CJValueDef::typLong:
                u.sExtraInfo.retJumpSize += 10; //Twice "DoReturn" MACRO size in bytes in Native_RaiseCallback_x86
                break;
              default:
                u.sExtraInfo.retJumpSize += 5; //"DoReturn" MACRO size in bytes in Native_RaiseCallback_x86
                break;
            }
          }
#elif defined _M_X64
          //if return value a float or double...
          u.sExtraInfo.returnType = (lpCurr->cJValueDefReturn.nType == CJValueDef::typFloat ||
                                     lpCurr->cJValueDefReturn.nType == CJValueDef::typDouble) ? 1 : 0;
#endif
          HookTrampoline2_SetCallOriginalProc(lpTrampoline, u.lpData);
          (aMethods.Get())[nCurr].fnPtr = lpTrampoline;
          (sNatives.aInfo.Get())[nCurr].lpStub = lpTrampoline;
          (sNatives.aInfo.Get())[nCurr].lpMethod = lpCurr;
          nCurr++;
        }
        else
        {
          hRes = E_OUTOFMEMORY;
        }
      }
    }
    //do registration
    if (SUCCEEDED(hRes))
    {
      nJniErr = env->RegisterNatives(globalJClass, aMethods.Get(), (jint)nNativesCount);
      switch (nJniErr)
      {
        case JNI_ENOMEM:
          hRes = E_OUTOFMEMORY;
          break;
        case JNI_EINVAL:
          hRes = E_INVALIDARG;
          break;
        default:
          if (nJniErr < 0)
            hRes = E_FAIL;
          break;
      }
    }
    //done
    if (SUCCEEDED(hRes))
    {
      sNatives.nCount = nNativesCount;
      sNatives.hHeap = hHeap;
    }
    else
    {
      NATIVEMETHOD_INFO *lpInfo;

      lpInfo = sNatives.aInfo.Get();
      if (lpInfo != NULL)
      {
        for (nCurr=0; nCurr<nNativesCount; nCurr++)
        {
          if (lpInfo[nCurr].lpStub != NULL)
            HookTrampoline2_Delete(hHeap, lpInfo[nCurr].lpStub);
        }
        sNatives.aInfo.Reset();
      }
    }
  }
  return hRes;
}

HRESULT CClassDef::AddConstructors(__in JNIEnv *env, __in jobject clsObj, __in jmethodID methodId)
{
  TNktAutoDeletePtr<CMethodOrFieldDef> cMethodDef;
  TJavaAutoPtr<jobjectArray> cItemsArrayObj;
  TJavaAutoPtr<jobject> cItemObj;
  jsize i, nCount;
  HRESULT hRes;

  cItemsArrayObj.Attach(env, (jobjectArray)(env->CallObjectMethod(clsObj, methodId)));
  if (cItemsArrayObj == NULL)
    return E_OUTOFMEMORY;
  nCount = env->GetArrayLength(cItemsArrayObj);
  for (i=0; i<nCount; i++)
  {
    cItemObj.Release();
    cItemObj.Attach(env, env->GetObjectArrayElement(cItemsArrayObj, i));
    if (cItemObj == NULL)
      return E_FAIL;
    //create new method
    cMethodDef.Attach(new CMethodOrFieldDef());
    if (cMethodDef == NULL)
      return E_OUTOFMEMORY;
    cMethodDef->lpParentClassDef = this;
    //get method type
    cMethodDef->nType = CMethodOrFieldDef::typMethod;
    //get id
    cMethodDef->nMethodId = env->FromReflectedMethod(cItemObj);
    hRes = CheckJavaException(env);
    if (FAILED(hRes))
      return hRes;
    //add parameters
    hRes = cMethodDef->AddParameters(env, cItemObj);
    if (FAILED(hRes))
      return hRes;
    //add to list
    cConstructors.PushTail(cMethodDef.Detach());
  }
  return S_OK;
}

HRESULT CClassDef::AddMethods(__in JNIEnv *env, __in jobject clsObj, __in jmethodID methodId, __in BOOL bIsSuperClass)
{
  TNktAutoDeletePtr<CMethodOrFieldDef> cMethodDef;
  TJavaAutoPtr<jclass> cClsDescr;
  jmethodID getModifiers_MethodId, isSynthetic_MethodId, isVarArgs_MethodId;
  TJavaAutoPtr<jobjectArray> cItemsArrayObj;
  TJavaAutoPtr<jobject> cItemObj;
  jint modifiers;
  jsize i, nCount;
  HRESULT hRes;

  cClsDescr.Attach(env, env->FindClass("java/lang/reflect/Method"));
  if (cClsDescr == NULL)
    return E_FAIL;
  getModifiers_MethodId = env->GetMethodID(cClsDescr, "getModifiers", "()I");
  isSynthetic_MethodId = env->GetMethodID(cClsDescr, "isSynthetic", "()Z");
  isVarArgs_MethodId = env->GetMethodID(cClsDescr, "isVarArgs", "()Z");
  if (getModifiers_MethodId == NULL)
    return E_FAIL;
  //----
  cItemsArrayObj.Attach(env, (jobjectArray)(env->CallObjectMethod(clsObj, methodId)));
  if (cItemsArrayObj == NULL)
    return E_OUTOFMEMORY;
  nCount = env->GetArrayLength(cItemsArrayObj);
  for (i=0; i<nCount; i++)
  {
    cItemObj.Release();
    cItemObj.Attach(env, env->GetObjectArrayElement(cItemsArrayObj, i));
    if (cItemObj == NULL)
      return E_FAIL;
    //get modifiers
    modifiers = env->CallIntMethod(cItemObj, getModifiers_MethodId);
    hRes = CheckJavaException(env);
    if (FAILED(hRes))
      return hRes;
    //check if synthetic and ignore them
    if (isSynthetic_MethodId != NULL)
    {
      if (env->CallBooleanMethod(cItemObj, isSynthetic_MethodId) != JNI_FALSE)
        continue;
    }
    else
    {
      if ((modifiers & MODIFIER_INTERNAL_NOT_EXPOSED_Synthetic) != 0)
        continue;
    }
    //check if varargs and ignore them (not supported yet)
    if (isVarArgs_MethodId != NULL)
    {
      if (env->CallBooleanMethod(cItemObj, isVarArgs_MethodId) != JNI_FALSE)
        continue;
    }
    else
    {
      if ((modifiers & MODIFIER_INTERNAL_NOT_EXPOSED_VarArgs) != 0)
        continue;
    }
    //create new method
    cMethodDef.Attach(new CMethodOrFieldDef());
    if (cMethodDef == NULL)
      return E_OUTOFMEMORY;
    cMethodDef->lpParentClassDef = this;
    //set method type and others
    cMethodDef->nType = ((modifiers & MODIFIER_Static) == 0) ? CMethodOrFieldDef::typMethod :
                                                               CMethodOrFieldDef::typStaticMethod;
    cMethodDef->bIsNative = ((modifiers & MODIFIER_Native) != 0) ? TRUE : FALSE;
    cMethodDef->bIsSuperClass = bIsSuperClass;
    //get name
    hRes = GetJObjectName(cMethodDef->cStrNameW, env, cItemObj);
    if (FAILED(hRes))
      return hRes;
    //get id
    cMethodDef->nMethodId = env->FromReflectedMethod(cItemObj);
    hRes = CheckJavaException(env);
    if (FAILED(hRes))
      return hRes;
    //add parameters
    hRes = cMethodDef->AddParameters(env, cItemObj);
    if (FAILED(hRes))
      return hRes;
    //add result
    hRes = cMethodDef->AddReturnResult(env, cItemObj);
    if (FAILED(hRes))
      return hRes;
    //add to list
    cMethods.PushTail(cMethodDef.Detach());
  }
  return S_OK;
}

HRESULT CClassDef::AddFields(__in JNIEnv *env, __in jobject clsObj, __in jmethodID methodId, __in BOOL bIsSuperClass)
{
  TNktAutoDeletePtr<CMethodOrFieldDef> cFieldDef;
  TJavaAutoPtr<jclass> cClsDescr;
  jmethodID getModifiers_MethodId, isSynthetic_MethodId, isEnumConstant_MethodId;
  TJavaAutoPtr<jobjectArray> cItemsArrayObj;
  TJavaAutoPtr<jobject> cItemObj;
  jint modifiers;
  jsize i, nCount;
  HRESULT hRes;

  cClsDescr.Attach(env, env->FindClass("java/lang/reflect/Field"));
  if (cClsDescr == NULL)
    return E_FAIL;
  getModifiers_MethodId = env->GetMethodID(cClsDescr, "getModifiers", "()I");
  isSynthetic_MethodId = env->GetMethodID(cClsDescr, "isSynthetic", "()Z");
  isEnumConstant_MethodId = env->GetMethodID(cClsDescr, "isEnumConstant", "()Z");
  if (getModifiers_MethodId == NULL)
    return E_FAIL;
  //----
  cItemsArrayObj.Attach(env, (jobjectArray)(env->CallObjectMethod(clsObj, methodId)));
  if (cItemsArrayObj == NULL)
    return E_OUTOFMEMORY;
  nCount = env->GetArrayLength(cItemsArrayObj);
  for (i=0; i<nCount; i++)
  {
    cItemObj.Release();
    cItemObj.Attach(env, env->GetObjectArrayElement(cItemsArrayObj, i));
    if (cItemObj == NULL)
      return E_FAIL;
    //get modifiers
    modifiers = env->CallIntMethod(cItemObj, getModifiers_MethodId);
    hRes = CheckJavaException(env);
    if (FAILED(hRes))
      return hRes;
    //check if synthetic and ignore them
    if (isSynthetic_MethodId != NULL)
    {
      if (env->CallBooleanMethod(cItemObj, isSynthetic_MethodId) != JNI_FALSE)
        continue;
    }
    else
    {
      if ((modifiers & MODIFIER_INTERNAL_NOT_EXPOSED_Synthetic) != 0)
        continue;
    }
    //check if enumeration constant and ignore them
    if (isEnumConstant_MethodId != NULL)
    {
      if (env->CallBooleanMethod(cItemObj, isEnumConstant_MethodId) != JNI_FALSE)
        continue;
    }
    else
    {
      if ((modifiers & MODIFIER_INTERNAL_NOT_EXPOSED_Enum) != 0)
        continue;
    }
    //create new field
    cFieldDef.Attach(new CMethodOrFieldDef());
    if (cFieldDef == NULL)
      return E_OUTOFMEMORY;
    cFieldDef->lpParentClassDef = this;
    //set field type and others
    cFieldDef->nType = ((modifiers & MODIFIER_Static) == 0) ? CMethodOrFieldDef::typField :
                                                              CMethodOrFieldDef::typStaticField;
    cFieldDef->bIsSuperClass = bIsSuperClass;
    //get name
    hRes = GetJObjectName(cFieldDef->cStrNameW, env, cItemObj);
    if (FAILED(hRes))
      return hRes;
    //get id
    cFieldDef->nFieldId = env->FromReflectedField(cItemObj);
    hRes = CheckJavaException(env);
    if (FAILED(hRes))
      return hRes;
    //add type
    hRes = cFieldDef->AddReturnResult(env, cItemObj);
    if (FAILED(hRes))
      return hRes;
    //add to list
    cFields.PushTail(cFieldDef.Detach());
  }
  return S_OK;
}

HRESULT CClassDef::InitReferences(__in CJavaClassCacheManager *lpMgr, __in JNIEnv *env)
{
  TNktLnkLst<CMethodOrFieldDef>::Iterator it;
  CMethodOrFieldDef *lpMethodOrFieldDef;
  HRESULT hRes;

  hRes = S_OK;
  for (lpMethodOrFieldDef=it.Begin(cConstructors); SUCCEEDED(hRes) && lpMethodOrFieldDef!=NULL;
       lpMethodOrFieldDef=it.Next())
    hRes = lpMethodOrFieldDef->InitReferences(lpMgr, env);
  for (lpMethodOrFieldDef=it.Begin(cMethods); SUCCEEDED(hRes) && lpMethodOrFieldDef!=NULL;
       lpMethodOrFieldDef=it.Next())
    hRes = lpMethodOrFieldDef->InitReferences(lpMgr, env);
  for (lpMethodOrFieldDef=it.Begin(cFields); SUCCEEDED(hRes) && lpMethodOrFieldDef!=NULL;
       lpMethodOrFieldDef=it.Next())
    hRes = lpMethodOrFieldDef->InitReferences(lpMgr, env);
  return hRes;
}

VOID CClassDef::DestroyInternals(__in JNIEnv *env, __in BOOL bInExitChain)
{
  NATIVEMETHOD_INFO *lpInfo;
  SIZE_T nCurr;

  if (globalJClass != NULL)
  {
    if (env != NULL && sNatives.nCount > 0)
      env->UnregisterNatives(globalJClass);
    lpInfo = sNatives.aInfo.Get();
    if (lpInfo != NULL && bInExitChain == FALSE)
    {
      for (nCurr=0; nCurr<sNatives.nCount; nCurr++)
      {
        if (lpInfo[nCurr].lpStub != NULL)
        {
          HookTrampoline2_Disable(lpInfo[nCurr].lpStub, TRUE);
          //HOOKTRAMPOLINE2CREATE_Delete(sNatives.hHeap, lpInfo[nCurr].lpStub);
        }
      }
    }
    sNatives.aInfo.Reset();
    if (env != NULL)
      env->DeleteGlobalRef(globalJClass);
    globalJClass = NULL;
  }
  return;
}

//-----------------------------------------------------------

CJavaClassCacheManager::CJavaClassCacheManager(__in CMainManager *lpMgr)
{
  lpMainMgr = lpMgr;
  return;
}

CJavaClassCacheManager::~CJavaClassCacheManager()
{
  RemoveAll(NULL, TRUE);
  return;
}

HRESULT CJavaClassCacheManager::FindOrCreateClass(__in JNIEnv *env, __in jclass cls,
                                                  __deref_out CClassDef **lplpClassDef)
{
  CNktAutoFastMutex cLock(&cMtx);
  SIZE_T i, nCount;
  HRESULT hRes;

  hRes = InternalFindOrCreateClass(env, cls, lplpClassDef);
  if (SUCCEEDED(hRes))
  {
    nCount = aList.GetCount();
    for (i=0; i<nCount; i++)
      aList[i]->bBeingCreated = FALSE;
  }
  else
  {
    nCount = aList.GetCount();
    i = 0;
    while (i < nCount)
    {
      if (aList[i]->bBeingCreated != FALSE)
      {
        aList[i]->DestroyInternals(env, FALSE);
        aList.RemoveElementAt(i);
        nCount--;
      }
      else
      {
        i++;
      }
    }
  }
  return hRes;
}

VOID CJavaClassCacheManager::RemoveAll(__in JNIEnv *env, __in BOOL bInExitChain)
{
  CNktAutoFastMutex cLock(&cMtx);
  SIZE_T i, nCount;

  nCount = aList.GetCount();
  for (i=0; i<nCount; i++)
    aList[i]->DestroyInternals(env, bInExitChain);
  aList.RemoveAllElements();
  return;
}

HRESULT CJavaClassCacheManager::InternalFindOrCreateClass(__in JNIEnv *env, __in jclass cls,
                                                          __deref_out CClassDef **lplpClassDef)
{
  CNktStringW cStrClassNameW;
  TNktComPtr<CClassDef> cClassDef;
  TJavaAutoPtr<jclass> cCls, cClsDescr;
  CClassDef **lplpClassDefFound;
  jmethodID methodId;
  jclass currCls;
  BOOL bIsSuperClass;
  HRESULT hRes;

  _ASSERT(env != NULL);
  if (lplpClassDef != NULL)
    *lplpClassDef = NULL;
  if (cls == NULL)
    return E_POINTER;
  hRes = GetJClassName(cStrClassNameW, env, cls);
  if (FAILED(hRes))
    return hRes;
  //lookup for the class if already exists
  lplpClassDefFound = (CClassDef**)bsearch_s((LPWSTR)cStrClassNameW, aList.GetBuffer(), aList.GetCount(),
                                             sizeof(CClassDef*), reinterpret_cast<int (__cdecl *)(void *,const void *,
                                                                                  const void *)>(&JavaClass_Search),
                                             NULL);
  if (lplpClassDefFound != NULL)
  {
    *lplpClassDef = *lplpClassDefFound;
    (*lplpClassDef)->AddRef();
    return S_OK;
  }
  //does not exists so create a new one
  cClassDef.Attach(new CClassDef);
  hRes = (cClassDef != NULL) ? S_OK : E_OUTOFMEMORY;
  if (SUCCEEDED(hRes))
  {
    cClassDef->lpCacheMgr = this;
    if (cClassDef->cStrNameW.Copy((LPWSTR)cStrClassNameW) == FALSE)
      hRes = E_OUTOFMEMORY;
  }
  //add to list
  if (SUCCEEDED(hRes))
  {
    if (aList.SortedInsert((CClassDef*)cClassDef, &JavaClass_Compare, NULL) != FALSE)
      ((CClassDef*)cClassDef)->AddRef();
    else
      hRes = E_OUTOFMEMORY;
  }
  //find constructors, methods and fields
  if (SUCCEEDED(hRes))
  {
    currCls = cls;
    bIsSuperClass = FALSE;
    do
    {
      cClsDescr.Release();
      cClsDescr.Attach(env, env->GetObjectClass(currCls));
      if (cClsDescr == NULL)
        hRes = E_FAIL;
      if (SUCCEEDED(hRes) && bIsSuperClass == FALSE)
      {
        //From: http://docs.oracle.com/javase/tutorial/reflect/class/classMembers.html
        //constructors are NOT inherited
        methodId = env->GetMethodID(cClsDescr, "getDeclaredConstructors", "()[Ljava/lang/reflect/Constructor;");
        hRes = (methodId != NULL) ? cClassDef->AddConstructors(env, currCls, methodId) : E_FAIL;
      }
      //get methods
      if (SUCCEEDED(hRes))
      {
        methodId = env->GetMethodID(cClsDescr, "getDeclaredMethods", "()[Ljava/lang/reflect/Method;");
        hRes = (methodId != NULL) ? cClassDef->AddMethods(env, currCls, methodId, bIsSuperClass) : E_FAIL;
      }
      //get fields
      if (SUCCEEDED(hRes))
      {
        methodId = env->GetMethodID(cClsDescr, "getDeclaredFields", "()[Ljava/lang/reflect/Field;");
        hRes = (methodId != NULL) ? cClassDef->AddFields(env, currCls, methodId, bIsSuperClass) : E_FAIL;
      }
      //get superclass
      if (SUCCEEDED(hRes))
      {
        currCls = env->GetSuperclass(currCls);
        cCls.Release();
        if (currCls != NULL)
          cCls.Attach(env, currCls);
        else
          hRes = CheckJavaException(env);
      }
      //----
      bIsSuperClass = TRUE;
    }
    while (SUCCEEDED(hRes) && currCls != NULL);
  }
  //locate CJValueDef references to objects
  if (SUCCEEDED(hRes))
    hRes = cClassDef->InitReferences(this, env);
  //create global class reference
  if (SUCCEEDED(hRes))
  {
    cClassDef->globalJClass = (jclass)(env->NewGlobalRef((jobject)cls));
    if (cClassDef->globalJClass == NULL)
      hRes = E_OUTOFMEMORY;
  }
  //done
  if (SUCCEEDED(hRes))
  {
    cClassDef->bBeingCreated = FALSE;
    *lplpClassDef = cClassDef.Detach();
  }
  return hRes;
}

} //JAVA

//-----------------------------------------------------------
//-----------------------------------------------------------

static int JavaClass_Search(__in void *, __in LPCWSTR lpElem1, __in JAVA::CClassDef **lpElem2)
{
  return wcscmp(lpElem1, (LPWSTR)((*lpElem2)->cStrNameW));
}

static int JavaClass_Compare(__in void *, __in JAVA::CClassDef **lpElem1, __in JAVA::CClassDef **lpElem2)
{
  return wcscmp((LPWSTR)((*lpElem1)->cStrNameW), (LPWSTR)((*lpElem2)->cStrNameW));
}
