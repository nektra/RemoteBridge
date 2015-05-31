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

#include "..\RemoteBridgeHelper.h"
#include "JavaManager.h"
#include "..\..\Common\AutoPtr.h"
#include "..\..\Common\LinkedList.h"
#include "..\..\Common\StringLiteW.h"
#include "..\..\Common\WaitableObjects.h"

//-----------------------------------------------------------

namespace JAVA {

class CJavaClassCacheManager;
class CClassDef;
class CMethodOrFieldDef;

class CJValueDef : public TNktLnkLstNode<CJValueDef>
{
public:
  typedef enum {
    typVoid=0,
    typBoolean=1,
    typByte, //byte type is signed
    typChar, //unsigned short
    typShort,
    typInt,
    typLong,
    typFloat,
    typDouble,
    typString,
    typBigDecimal,
    typDate,
    typCalendar,
    typObject,
    typBooleanObj,
    typByteObj,
    typCharObj,
    typShortObj,
    typIntObj,
    typLongObj,
    typFloatObj,
    typDoubleObj,
    typAtomicIntObj,
    typAtomicLongObj,
    typMAX=typAtomicLongObj
  } eType;

public:
  CJValueDef();

  CMethodOrFieldDef* GetParentMethodOrFieldDef()
    {
    return lpParentMethodOrFieldDef;
    };

public:
  eType nType;
  ULONG nArrayDims; //0=single value
  CNktStringW cStrJObjectClassNameW;
  CClassDef *lpClassDef;

private:
  friend class CMethodOrFieldDef;

  HRESULT ParseType(__in_z LPCWSTR szTypeName);
  HRESULT InitReferences(__in CJavaClassCacheManager *lpMgr, __in JNIEnv *env);
  HRESULT GetSignatureW(__inout CNktStringW &cStrW);

private:
  CMethodOrFieldDef *lpParentMethodOrFieldDef;
};

typedef TNktLnkLst<CJValueDef> CJValueDefList;

//-----------------------------------------------------------

class CMethodOrFieldDef : public TNktLnkLstNode<CMethodOrFieldDef>
{
public:
  typedef enum {
    typMethod=1,
    typStaticMethod,
    typField,
    typStaticField,
    typMAX=typStaticField
  } eType;

public:
  CMethodOrFieldDef();
  ~CMethodOrFieldDef();

  CClassDef* GetParentClassDef()
    {
    return lpParentClassDef;
    };

public:
  eType nType;
  BOOL bIsNative, bIsSuperClass;
  CNktStringW cStrNameW;
  CJValueDefList cJValueDefParams;
  SIZE_T nJValueDefParamsCount;
  CJValueDef cJValueDefReturn;
  union {
    jmethodID nMethodId;
    jfieldID nFieldId;
  };

private:
  friend class CClassDef;

  HRESULT AddParameters(__in JNIEnv *env, __in jobject obj);
  HRESULT AddReturnResult(__in JNIEnv *env, __in jobject obj);
  HRESULT InitReferences(__in CJavaClassCacheManager *lpMgr, __in JNIEnv *env);
  LPSTR BuildSignatureA();

  HRESULT OnNativeRaiseCallback(__in LPVOID lpData);
  static VOID __stdcall Native_raiseCallbackStub(__in CMethodOrFieldDef *lpCtx, __in LPVOID lpData);

private:
  CClassDef *lpParentClassDef;
};

typedef TNktLnkLst<CMethodOrFieldDef> CMethodOrFieldDefList;

//-----------------------------------------------------------

class CClassDef : public IUnknown
{
protected:
  CClassDef();
public:
  ~CClassDef();

public:
  STDMETHOD(QueryInterface)(__in REFIID riid, __RPC__deref_out  void **ppvObject);
  STDMETHOD_(ULONG, AddRef)();
  STDMETHOD_(ULONG, Release)();

  CMethodOrFieldDef* FindMethod(__in LPCWSTR szMethodNameW);
  CMethodOrFieldDef* FindField(__in LPCWSTR szFieldNameW);

  HRESULT RegisterNatives(__in JNIEnv *env, __in HANDLE hHeap);

public:
  CNktStringW cStrNameW;
  BOOL bIsSuperClass;
  CMethodOrFieldDefList cConstructors;
  CMethodOrFieldDefList cMethods;
  CMethodOrFieldDefList cFields;
  jclass globalJClass;

private:
  friend class CJavaClassCacheManager;

  HRESULT AddConstructors(__in JNIEnv *env, __in jobject clsObj, __in jmethodID methodId);
  HRESULT AddMethods(__in JNIEnv *env, __in jobject clsObj, __in jmethodID methodId, __in BOOL bIsSuperClass);
  HRESULT AddFields(__in JNIEnv *env, __in jobject clsObj, __in jmethodID methodId, __in BOOL bIsSuperClass);
  HRESULT InitReferences(__in CJavaClassCacheManager *lpMgr, __in JNIEnv *env);

  VOID DestroyInternals(__in JNIEnv *env, __in BOOL bInExitChain);

private:
  friend class CMethodOrFieldDef;

  typedef struct {
    LPVOID lpStub;
    CMethodOrFieldDef *lpMethod;
  } NATIVEMETHOD_INFO;

  LONG volatile nRefCount;
  CJavaClassCacheManager *lpCacheMgr;
  struct {
    SIZE_T nCount;
    HANDLE hHeap;
    TNktAutoFreePtr<NATIVEMETHOD_INFO> aInfo;
  } sNatives;
  BOOL bBeingCreated;
};

//-----------------------------------------------------------

class CJavaClassCacheManager
{
public:
  CJavaClassCacheManager(__in CMainManager *lpMgr);
  ~CJavaClassCacheManager();

  HRESULT FindOrCreateClass(__in JNIEnv *env, __in jclass cls, __deref_out CClassDef **lplpClassDef);
  VOID RemoveAll(__in JNIEnv *env, __in BOOL bInExitChain);

private:
  friend class CClassDef;
  friend class CMethodOrFieldDef;
  friend class CJValueDef;

  HRESULT InternalFindOrCreateClass(__in JNIEnv *env, __in jclass cls, __deref_out CClassDef **lplpClassDef);

private:
  CNktFastMutex cMtx;
  TNktArrayListWithRelease<CClassDef*> aList;
  CMainManager *lpMainMgr;
};

} //JAVA
