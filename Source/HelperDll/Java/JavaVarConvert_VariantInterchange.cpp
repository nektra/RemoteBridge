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
#include "JavaVarConvert.h"

//-----------------------------------------------------------

#define CalendarField_DAY_OF_MONTH  5
#define CalendarField_MONTH         2
#define CalendarField_YEAR          1
#define CalendarField_HOUR_OF_DAY  11
#define CalendarField_MINUTE       12
#define CalendarField_SECOND       13
#define CalendarField_MILLISECOND  14
#define CalendarField_ZONE_OFFSET  15

//-----------------------------------------------------------

namespace JAVA {

CJavaVariantInterchange::CJavaVariantInterchange()
{
  NktInterlockedExchange(&nMutex, 0);
  globalBigIntCls = globalBigDecCls = NULL;
  globalCalendarCls = globalDateCls = globalTimeZoneCls = NULL;
  globalBooleanCls = globalByteCls = globalCharacterCls = globalShortCls = globalIntegerCls =
    globalAtomicIntegerCls = globalLongCls = globalAtomicLongCls = globalFloatCls = globalDoubleCls = NULL;
  //----
  bigDecConst_MethodId = bigDecUnscaledValue_MethodId = bigDecSignum_MethodId = bigDecNegate_MethodId =
    bigDecScale_MethodId = bigDecCompareTo_MethodId = NULL;
  //----
  bigIntConst_MethodId = bigIntConst2_MethodId = bigIntBitLength_MethodId = bigIntIntValue_MethodId =
    bigIntShiftRight_MethodId = bigIntNegate_MethodId = NULL;
  //----
  calGetInstance_MethodId = calSet2_MethodId = calSet6_MethodId = calGetTime_MethodId =
    calGet1_MethodId = calSetTime_MethodId = NULL;
  //----
  booleanConst_MethodId = byteConst_MethodId = characterConst_MethodId = shortConst_MethodId =
    integerConst_MethodId = atomicIntegerConst_MethodId = longConst_MethodId = atomicLongConst_MethodId =
      floatConst_MethodId = doubleConst_MethodId = NULL;
  booleanGetValue_MethodId = byteGetValue_MethodId = characterGetValue_MethodId = shortGetValue_MethodId =
    integerGetValue_MethodId = atomicIntegerGetValue_MethodId = longGetValue_MethodId =
      atomicLongGetValue_MethodId = floatGetValue_MethodId = doubleGetValue_MethodId = NULL;
  //----
  globalLargeBigDec = globalSmallBigDec = NULL;
  return;
}

CJavaVariantInterchange::~CJavaVariantInterchange()
{
  return;
}

VOID CJavaVariantInterchange::Reset(__in JNIEnv *env)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);

  if (env != NULL)
  {
    if (globalBigIntCls != NULL)
      env->DeleteGlobalRef(globalBigIntCls);
    if (globalBigDecCls != NULL)
      env->DeleteGlobalRef(globalBigDecCls);
    //----
    if (globalCalendarCls != NULL)
      env->DeleteGlobalRef(globalCalendarCls);
    if (globalDateCls != NULL)
      env->DeleteGlobalRef(globalDateCls);
    if (globalTimeZoneCls != NULL)
      env->DeleteGlobalRef(globalTimeZoneCls);
    //----
    if (globalBooleanCls != NULL)
      env->DeleteGlobalRef(globalBooleanCls);
    if (globalByteCls != NULL)
      env->DeleteGlobalRef(globalByteCls);
    if (globalCharacterCls != NULL)
      env->DeleteGlobalRef(globalCharacterCls);
    if (globalShortCls != NULL)
      env->DeleteGlobalRef(globalShortCls);
    if (globalIntegerCls != NULL)
      env->DeleteGlobalRef(globalIntegerCls);
    if (globalAtomicIntegerCls != NULL && globalAtomicIntegerCls != (jclass)-1)
      env->DeleteGlobalRef(globalAtomicIntegerCls);
    if (globalLongCls != NULL)
      env->DeleteGlobalRef(globalLongCls);
    if (globalAtomicLongCls != NULL && globalAtomicLongCls != (jclass)-1)
      env->DeleteGlobalRef(globalAtomicLongCls);
    if (globalFloatCls != NULL)
      env->DeleteGlobalRef(globalFloatCls);
    if (globalDoubleCls != NULL)
      env->DeleteGlobalRef(globalDoubleCls);
  }
  globalBigIntCls = globalBigDecCls = NULL;
  globalCalendarCls = globalDateCls = globalTimeZoneCls = NULL;
  globalBooleanCls = globalByteCls = globalCharacterCls = globalShortCls = globalIntegerCls =
    globalAtomicIntegerCls = globalLongCls = globalAtomicLongCls = globalFloatCls = globalDoubleCls = NULL;
  //----
  bigDecConst_MethodId = bigDecUnscaledValue_MethodId = bigDecSignum_MethodId = bigDecNegate_MethodId =
    bigDecScale_MethodId = bigDecCompareTo_MethodId = NULL;
  //----
  bigIntConst_MethodId = bigIntConst2_MethodId = bigIntBitLength_MethodId = bigIntIntValue_MethodId =
    bigIntShiftRight_MethodId = bigIntNegate_MethodId = NULL;
  //----
  calGetInstance_MethodId = calSet2_MethodId = calSet6_MethodId = calGetTime_MethodId =
    calGet1_MethodId = calSetTime_MethodId = NULL;
  //----
  booleanConst_MethodId = byteConst_MethodId = characterConst_MethodId = shortConst_MethodId =
    integerConst_MethodId = atomicIntegerConst_MethodId = longConst_MethodId = atomicLongConst_MethodId =
      floatConst_MethodId = doubleConst_MethodId = NULL;
  booleanGetValue_MethodId = byteGetValue_MethodId = characterGetValue_MethodId = shortGetValue_MethodId =
    integerGetValue_MethodId = atomicIntegerGetValue_MethodId = longGetValue_MethodId =
      atomicLongGetValue_MethodId = floatGetValue_MethodId = doubleGetValue_MethodId = NULL;
  //----
  if (env != NULL)
  {
    if (globalLargeBigDec != NULL)
      env->DeleteGlobalRef(globalLargeBigDec);
    if (globalSmallBigDec != NULL)
      env->DeleteGlobalRef(globalSmallBigDec);
  }
  globalLargeBigDec = globalSmallBigDec = NULL;
  return;
}

HRESULT CJavaVariantInterchange::DecimalToJObject(__in JNIEnv *env, __in DECIMAL *lpDec, __out jobject *lpJObject)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  TJavaAutoPtr<jbyteArray> cTempByteArray;
  TJavaAutoPtr<jobject> cTempBigInt;
  jbyte buf[12];
  HRESULT hRes;

  *lpJObject = NULL;
  //init
  hRes = InitializeBigDecimal(env);
  if (FAILED(hRes))
    return hRes;
  //create a temp array
  cTempByteArray.Attach(env, env->NewByteArray(12));
  if (cTempByteArray == NULL)
    return E_OUTOFMEMORY;
  buf[0] =  (jbyte)((lpDec->Hi32  >> 24) & 255);
  buf[1] =  (jbyte)((lpDec->Hi32  >> 16) & 255);
  buf[2] =  (jbyte)((lpDec->Hi32  >>  8) & 255);
  buf[3] =  (jbyte)((lpDec->Hi32       ) & 255);
  buf[4] =  (jbyte)((lpDec->Mid32 >> 24) & 255);
  buf[5] =  (jbyte)((lpDec->Mid32 >> 16) & 255);
  buf[6] =  (jbyte)((lpDec->Mid32 >>  8) & 255);
  buf[7] =  (jbyte)((lpDec->Mid32      ) & 255);
  buf[8] =  (jbyte)((lpDec->Lo32  >> 24) & 255);
  buf[9] =  (jbyte)((lpDec->Lo32  >> 16) & 255);
  buf[10] = (jbyte)((lpDec->Lo32  >>  8) & 255);
  buf[11] = (jbyte)( lpDec->Lo32         & 255);
  env->SetByteArrayRegion(cTempByteArray, 0, 12, buf);
  hRes = JAVA::CheckJavaException(env);
  if (FAILED(hRes))
    return hRes;
  //create BigInteger
  cTempBigInt.Attach(env, env->NewObject(globalBigIntCls, bigIntConst_MethodId,
                                         (jint)((lpDec->sign != 0x80) ? 1 : (-1)), cTempByteArray.Get()));
  if (cTempBigInt == NULL)
    return E_OUTOFMEMORY;
  //create BigDecimal
  *lpJObject = env->NewObject(globalBigDecCls, bigDecConst_MethodId, cTempBigInt.Get(), (jint)(lpDec->scale));
  return ((*lpJObject) != NULL) ? S_OK : E_OUTOFMEMORY;
}

HRESULT CJavaVariantInterchange::JObjectToDecimal(__in JNIEnv *env, __in jobject javaobj, __out DECIMAL *lpDec)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  TJavaAutoPtr<jobject> cTempBigDec, cTempBigInt, cTempBigInt2;
  jint i;
  HRESULT hRes;

  //init
  hRes = InitializeBigDecimal(env);
  if (FAILED(hRes))
    return hRes;
  //check upper bound
  cTempBigDec.Attach(env, env->NewLocalRef(globalLargeBigDec));
  if (cTempBigDec == NULL)
    return E_OUTOFMEMORY;
  i = env->CallIntMethod(cTempBigDec.Get(), bigDecCompareTo_MethodId, javaobj);
  if (i < 0)
    return NKT_E_ArithmeticOverflow;
  hRes = CheckJavaException(env);
  if (FAILED(hRes))
    return hRes;
  //check lower bound
  cTempBigDec.Release();
  cTempBigDec.Attach(env, env->NewLocalRef(globalSmallBigDec));
  if (cTempBigDec == NULL)
    return E_OUTOFMEMORY;
  i = env->CallIntMethod(cTempBigDec.Get(), bigDecCompareTo_MethodId, javaobj);
  if (i > 0)
    return NKT_E_ArithmeticOverflow;
  hRes = CheckJavaException(env);
  if (FAILED(hRes))
    return hRes;
  //get scale
  i = env->CallIntMethod(javaobj, bigDecScale_MethodId);
  if (i < 0 || i > 28) //VT_DECIMAL supports a scale between 0 and 28
    return NKT_E_ArithmeticOverflow;
  hRes = CheckJavaException(env);
  if (FAILED(hRes))
    return hRes;
  //get unscaled
  cTempBigInt.Attach(env, env->CallObjectMethod(javaobj, bigDecUnscaledValue_MethodId));
  if (cTempBigInt == NULL)
    return E_OUTOFMEMORY;
  //check bit length
  i = env->CallIntMethod(cTempBigInt, bigIntBitLength_MethodId);
  if (i > 96) //VT_DECIMAL supports a maximum of 96 bits
    return NKT_E_ArithmeticOverflow;
  hRes = CheckJavaException(env);
  if (FAILED(hRes))
    return hRes;
  //get sign
  i = env->CallIntMethod(javaobj, bigDecSignum_MethodId);
  hRes = CheckJavaException(env);
  if (FAILED(hRes))
    return hRes;
  if (i < 0)
  {
    lpDec->sign = 0x80;
    //negate bigdec
    cTempBigDec.Release();
    cTempBigDec.Attach(env, env->CallObjectMethod(javaobj, bigDecNegate_MethodId));
    if (cTempBigDec == NULL)
      return E_OUTOFMEMORY;
    javaobj = cTempBigDec.Get();
  }
  else
  {
    lpDec->sign = 0;
  }
  //get scale... again
  i = env->CallIntMethod(javaobj, bigDecScale_MethodId);
  hRes = CheckJavaException(env);
  if (FAILED(hRes))
    return hRes;
  lpDec->scale = (BYTE)i;
  //get unscaled... again
  cTempBigInt.Release();
  cTempBigInt.Attach(env, env->CallObjectMethod(javaobj, bigDecUnscaledValue_MethodId));
  if (cTempBigInt == NULL)
    return E_OUTOFMEMORY;
  //get lower 32 bits
  lpDec->Lo32 = (ULONG)(env->CallIntMethod(cTempBigInt, bigIntIntValue_MethodId));
  hRes = CheckJavaException(env);
  if (FAILED(hRes))
    return hRes;
  //get middle 32 bits
  cTempBigInt2.Attach(env, env->CallObjectMethod(cTempBigInt, bigIntShiftRight_MethodId, 32));
  if (cTempBigInt2 == NULL)
    return E_OUTOFMEMORY;
  lpDec->Mid32 = (ULONG)(env->CallIntMethod(cTempBigInt2, bigIntIntValue_MethodId));
  hRes = CheckJavaException(env);
  if (FAILED(hRes))
    return hRes;
  //get higher 32 bits
  cTempBigInt2.Release();
  cTempBigInt2.Attach(env, env->CallObjectMethod(cTempBigInt, bigIntShiftRight_MethodId, 64));
  if (cTempBigInt2 == NULL)
    return E_OUTOFMEMORY;
  lpDec->Hi32 = (ULONG)(env->CallIntMethod(cTempBigInt2, bigIntIntValue_MethodId));
  hRes = CheckJavaException(env);
  if (FAILED(hRes))
    return hRes;
  //done
  return S_OK;
}

HRESULT CJavaVariantInterchange::SystemTimeToJObject(__in JNIEnv *env, __in SYSTEMTIME *lpSysTime,
                                                     __in BOOL bReturnDate, __out jobject *lpJObject)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  JAVA::TJavaAutoPtr<jobject> cTempTimeZone, cTempCalendar;
  TJavaAutoPtr<jstring> cTempStr;
  HRESULT hRes;

  *lpJObject = NULL;
  //init
  hRes = InitializeCalendar(env);
  if (FAILED(hRes))
    return hRes;
  //get GMT timezone
  cTempStr.Attach(env, env->NewString((jchar*)L"GMT", 3));
  if (cTempStr == NULL)
    return E_OUTOFMEMORY;
  cTempTimeZone.Attach(env, env->CallStaticObjectMethod(globalTimeZoneCls, tzoneGetTimeZone_MethodId, cTempStr.Get()));
  if (cTempTimeZone == NULL)
    return E_OUTOFMEMORY;
  //get instance
  cTempCalendar.Attach(env, env->CallStaticObjectMethod(globalCalendarCls, calGetInstance_MethodId,
                                                        cTempTimeZone.Get()));
  if (cTempCalendar == NULL)
    return E_OUTOFMEMORY;
  //set date
  env->CallVoidMethod(cTempCalendar, calSet6_MethodId, (jint)(lpSysTime->wYear), (jint)(lpSysTime->wMonth-1),
                        (jint)(lpSysTime->wDay), (jint)(lpSysTime->wHour), (jint)(lpSysTime->wMinute),
                        (jint)(lpSysTime->wSecond));
  hRes = JAVA::CheckJavaException(env);
  if (SUCCEEDED(hRes))
  {
    env->CallVoidMethod(cTempCalendar, calSet2_MethodId, (jint)CalendarField_MILLISECOND,
                        (jint)(lpSysTime->wMilliseconds));
    hRes = JAVA::CheckJavaException(env);
  }
  if (SUCCEEDED(hRes))
  {
    if (bReturnDate != FALSE)
      *lpJObject = env->CallObjectMethod(cTempCalendar, calGetTime_MethodId);
    else
      *lpJObject = cTempCalendar.Detach();
    hRes = ((*lpJObject) != NULL) ? S_OK : E_OUTOFMEMORY;
  }
  return hRes;
}

HRESULT CJavaVariantInterchange::JObjectToSystemTime(__in JNIEnv *env, __in jobject javaobj,
                                                     __out SYSTEMTIME *lpSysTime)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  JAVA::TJavaAutoPtr<jobject> cTempCalendar;
  HRESULT hRes;

  //init
  hRes = InitializeCalendar(env);
  if (FAILED(hRes))
    return hRes;
  if (env->IsInstanceOf(javaobj, globalDateCls) != JNI_FALSE)
  {
    //if date, convert to calendar first
    JAVA::TJavaAutoPtr<jobject> cTempTimeZone;
    JAVA::TJavaAutoPtr<jstring> cTempStr;

    //get GMT timezone
    cTempStr.Attach(env, env->NewString((jchar*)L"GMT", 3));
    if (cTempStr == NULL)
      return E_OUTOFMEMORY;
    cTempTimeZone.Attach(env, env->CallStaticObjectMethod(globalTimeZoneCls, tzoneGetTimeZone_MethodId,
                                                          cTempStr.Get()));
    if (cTempTimeZone == NULL)
      return E_OUTOFMEMORY;
    //get instance
    cTempCalendar.Attach(env, env->CallStaticObjectMethod(globalCalendarCls, calGetInstance_MethodId,
                         cTempTimeZone.Get()));
    if (cTempCalendar == NULL)
      return E_OUTOFMEMORY;
    env->CallVoidMethod(cTempCalendar, calSetTime_MethodId, javaobj);
    hRes = JAVA::CheckJavaException(env);
    if (FAILED(hRes))
      return hRes;
    javaobj = cTempCalendar.Get();
  }
  //check if calendar
  if (env->IsInstanceOf(javaobj, globalCalendarCls) == JNI_FALSE)
    return E_INVALIDARG;
  //get date and time
  lpSysTime->wDay = (WORD)(env->CallIntMethod(cTempCalendar, calGet1_MethodId, (jint)CalendarField_DAY_OF_MONTH));
  hRes = JAVA::CheckJavaException(env);
  if (SUCCEEDED(hRes))
  {
    lpSysTime->wMonth = (WORD)(env->CallIntMethod(cTempCalendar, calGet1_MethodId, (jint)CalendarField_MONTH)) + 1;
    hRes = JAVA::CheckJavaException(env);
  }
  if (SUCCEEDED(hRes))
  {
    lpSysTime->wYear = (WORD)(env->CallIntMethod(cTempCalendar, calGet1_MethodId, (jint)CalendarField_YEAR));
    hRes = JAVA::CheckJavaException(env);
  }
  if (SUCCEEDED(hRes))
  {
    lpSysTime->wHour = (WORD)(env->CallIntMethod(cTempCalendar, calGet1_MethodId, (jint)CalendarField_HOUR_OF_DAY));
    hRes = JAVA::CheckJavaException(env);
  }
  if (SUCCEEDED(hRes))
  {
    lpSysTime->wMinute = (WORD)(env->CallIntMethod(cTempCalendar, calGet1_MethodId, (jint)CalendarField_MINUTE));
    hRes = JAVA::CheckJavaException(env);
  }
  if (SUCCEEDED(hRes))
  {
    lpSysTime->wSecond = (WORD)(env->CallIntMethod(cTempCalendar, calGet1_MethodId, (jint)CalendarField_SECOND));
    hRes = JAVA::CheckJavaException(env);
  }
  if (SUCCEEDED(hRes))
  {
    lpSysTime->wMilliseconds = (WORD)(env->CallIntMethod(cTempCalendar, calGet1_MethodId,
                                      (jint)CalendarField_MILLISECOND));
    hRes = JAVA::CheckJavaException(env);
  }
  return hRes;
}

HRESULT CJavaVariantInterchange::BooleanToJavaLangBoolean(__in JNIEnv *env, __in jboolean value,
                                                          __out jobject *javaobj)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  HRESULT hRes;

  if (javaobj == NULL)
    return E_POINTER;
  *javaobj = NULL;
  //init
  hRes = InitializeBasicTypes(env);
  //get object
  if (SUCCEEDED(hRes))
  {
    *javaobj = env->CallStaticObjectMethod(globalBooleanCls, booleanConst_MethodId, value);
    if ((*javaobj) == NULL)
    {
      hRes = CheckJavaException(env);
      if (SUCCEEDED(hRes))
        hRes = E_FAIL;
    }
  }
  return hRes;
}

HRESULT CJavaVariantInterchange::JavaLangBooleanToBoolean(__in JNIEnv *env, __in jobject javaobj,
                                                          __out jboolean *value)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  HRESULT hRes;

  if (value == NULL)
    return E_POINTER;
  *value = JNI_FALSE;
  //init
  hRes = InitializeBasicTypes(env);
  //get value
  if (SUCCEEDED(hRes))
  {
    *value = env->CallBooleanMethod(javaobj, booleanGetValue_MethodId);
    hRes = JAVA::CheckJavaException(env);
  }
  return hRes;
}

HRESULT CJavaVariantInterchange::ByteToJavaLangByte(__in JNIEnv *env, __in jbyte value, __out jobject *javaobj)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  HRESULT hRes;

  if (javaobj == NULL)
    return E_POINTER;
  *javaobj = NULL;
  //init
  hRes = InitializeBasicTypes(env);
  //get object
  if (SUCCEEDED(hRes))
  {
    *javaobj = env->CallStaticObjectMethod(globalByteCls, byteConst_MethodId, value);
    if ((*javaobj) == NULL)
    {
      hRes = CheckJavaException(env);
      if (SUCCEEDED(hRes))
        hRes = E_FAIL;
    }
  }
  return hRes;
}

HRESULT CJavaVariantInterchange::JavaLangByteToByte(__in JNIEnv *env, __in jobject javaobj, __out jbyte *value)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  HRESULT hRes;

  if (value == NULL)
    return E_POINTER;
  *value = 0;
  //init
  hRes = InitializeBasicTypes(env);
  //get value
  if (SUCCEEDED(hRes))
  {
    *value = env->CallByteMethod(javaobj, byteGetValue_MethodId);
    hRes = JAVA::CheckJavaException(env);
  }
  return hRes;
}

HRESULT CJavaVariantInterchange::CharToJavaLangCharacter(__in JNIEnv *env, __in jchar value, __out jobject *javaobj)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  HRESULT hRes;

  if (javaobj == NULL)
    return E_POINTER;
  *javaobj = NULL;
  //init
  hRes = InitializeBasicTypes(env);
  //get object
  if (SUCCEEDED(hRes))
  {
    *javaobj = env->CallStaticObjectMethod(globalCharacterCls, characterConst_MethodId, value);
    if ((*javaobj) == NULL)
    {
      hRes = CheckJavaException(env);
      if (SUCCEEDED(hRes))
        hRes = E_FAIL;
    }
  }
  return hRes;
}

HRESULT CJavaVariantInterchange::JavaLangCharacterToChar(__in JNIEnv *env, __in jobject javaobj, __out jchar *value)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  HRESULT hRes;

  if (value == NULL)
    return E_POINTER;
  *value = 0;
  //init
  hRes = InitializeBasicTypes(env);
  //get value
  if (SUCCEEDED(hRes))
  {
    *value = env->CallCharMethod(javaobj, characterGetValue_MethodId);
    hRes = JAVA::CheckJavaException(env);
  }
  return hRes;
}

HRESULT CJavaVariantInterchange::ShortToJavaLangShort(__in JNIEnv *env, __in jshort value, __out jobject *javaobj)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  HRESULT hRes;

  if (javaobj == NULL)
    return E_POINTER;
  *javaobj = NULL;
  //init
  hRes = InitializeBasicTypes(env);
  //get object
  if (SUCCEEDED(hRes))
  {
    *javaobj = env->CallStaticObjectMethod(globalShortCls, shortConst_MethodId, value);
    if ((*javaobj) == NULL)
    {
      hRes = CheckJavaException(env);
      if (SUCCEEDED(hRes))
        hRes = E_FAIL;
    }
  }
  return hRes;
}

HRESULT CJavaVariantInterchange::JavaLangShortToShort(__in JNIEnv *env, __in jobject javaobj, __out jshort *value)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  HRESULT hRes;

  if (value == NULL)
    return E_POINTER;
  *value = 0;
  //init
  hRes = InitializeBasicTypes(env);
  //get value
  if (SUCCEEDED(hRes))
  {
    *value = env->CallShortMethod(javaobj, shortGetValue_MethodId);
    hRes = JAVA::CheckJavaException(env);
  }
  return hRes;
}

HRESULT CJavaVariantInterchange::IntegerToJavaLangInteger(__in JNIEnv *env, __in jint value, __out jobject *javaobj)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  HRESULT hRes;

  if (javaobj == NULL)
    return E_POINTER;
  *javaobj = NULL;
  //init
  hRes = InitializeBasicTypes(env);
  //get object
  if (SUCCEEDED(hRes))
  {
    *javaobj = env->CallStaticObjectMethod(globalIntegerCls, integerConst_MethodId, value);
    if ((*javaobj) == NULL)
    {
      hRes = CheckJavaException(env);
      if (SUCCEEDED(hRes))
        hRes = E_FAIL;
    }
  }
  return hRes;
}

HRESULT CJavaVariantInterchange::JavaLangIntegerToInteger(__in JNIEnv *env, __in jobject javaobj, __out jint *value)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  HRESULT hRes;

  if (value == NULL)
    return E_POINTER;
  *value = 0;
  //init
  hRes = InitializeBasicTypes(env);
  //get value
  if (SUCCEEDED(hRes))
  {
    *value = env->CallIntMethod(javaobj, integerGetValue_MethodId);
    hRes = JAVA::CheckJavaException(env);
  }
  return hRes;
}

HRESULT CJavaVariantInterchange::IntegerToJavaUtilConcurrentAtomicInteger(__in JNIEnv *env, __in jint value,
                                                                          __out jobject *javaobj)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  HRESULT hRes;

  if (javaobj == NULL)
    return E_POINTER;
  *javaobj = NULL;
  //init
  hRes = InitializeBasicTypes(env);
  //get object
  if (SUCCEEDED(hRes))
  {
    if (globalAtomicIntegerCls != NULL)
    {
      *javaobj = env->CallStaticObjectMethod(globalAtomicIntegerCls, atomicIntegerConst_MethodId, value);
      if ((*javaobj) == NULL)
      {
        hRes = CheckJavaException(env);
        if (SUCCEEDED(hRes))
          hRes = E_FAIL;
      }
    }
    else
    {
      _ASSERT(FALSE);
      hRes = E_NOTIMPL;
    }
  }
  return hRes;
}

HRESULT CJavaVariantInterchange::JavaUtilConcurrentAtomicIntegerToInteger(__in JNIEnv *env, __in jobject javaobj,
                                                                          __out jint *value)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  HRESULT hRes;

  if (value == NULL)
    return E_POINTER;
  *value = 0;
  //init
  hRes = InitializeBasicTypes(env);
  //get value
  if (SUCCEEDED(hRes))
  {
    if (atomicIntegerGetValue_MethodId != NULL)
    {
      *value = env->CallIntMethod(javaobj, atomicIntegerGetValue_MethodId);
      hRes = JAVA::CheckJavaException(env);
    }
    else
    {
      _ASSERT(FALSE);
      hRes = E_NOTIMPL;
    }
  }
  return hRes;
}

HRESULT CJavaVariantInterchange::LongToJavaLangLong(__in JNIEnv *env, __in jlong value, __out jobject *javaobj)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  HRESULT hRes;

  if (javaobj == NULL)
    return E_POINTER;
  *javaobj = NULL;
  //init
  hRes = InitializeBasicTypes(env);
  //get object
  if (SUCCEEDED(hRes))
  {
    *javaobj = env->CallStaticObjectMethod(globalLongCls, longConst_MethodId, value);
    if ((*javaobj) == NULL)
    {
      hRes = CheckJavaException(env);
      if (SUCCEEDED(hRes))
        hRes = E_FAIL;
    }
  }
  return hRes;
}

HRESULT CJavaVariantInterchange::JavaLangLongToLong(__in JNIEnv *env, __in jobject javaobj, __out jlong *value)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  HRESULT hRes;

  if (value == NULL)
    return E_POINTER;
  *value = 0;
  //init
  hRes = InitializeBasicTypes(env);
  //get value
  if (SUCCEEDED(hRes))
  {
    *value = env->CallLongMethod(javaobj, longGetValue_MethodId);
    hRes = JAVA::CheckJavaException(env);
  }
  return hRes;
}

HRESULT CJavaVariantInterchange::LongToJavaUtilConcurrentAtomicLong(__in JNIEnv *env, __in jlong value,
                                                                    __out jobject *javaobj)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  HRESULT hRes;

  if (javaobj == NULL)
    return E_POINTER;
  *javaobj = NULL;
  //init
  hRes = InitializeBasicTypes(env);
  //get object
  if (SUCCEEDED(hRes))
  {
    if (globalAtomicLongCls != NULL)
    {
      *javaobj = env->CallStaticObjectMethod(globalAtomicLongCls, atomicLongConst_MethodId, value);
      if ((*javaobj) == NULL)
      {
        hRes = CheckJavaException(env);
        if (SUCCEEDED(hRes))
          hRes = E_FAIL;
      }
    }
    else
    {
      _ASSERT(FALSE);
      hRes = E_NOTIMPL;
    }
  }
  return hRes;
}

HRESULT CJavaVariantInterchange::JavaUtilConcurrentAtomicLongToLong(__in JNIEnv *env, __in jobject javaobj,
                                                                    __out jlong *value)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  HRESULT hRes;

  if (value == NULL)
    return E_POINTER;
  *value = 0;
  //init
  hRes = InitializeBasicTypes(env);
  //get value
  if (SUCCEEDED(hRes))
  {
    if (atomicLongGetValue_MethodId != NULL)
    {
      *value = env->CallLongMethod(javaobj, atomicLongGetValue_MethodId);
      hRes = JAVA::CheckJavaException(env);
    }
    else
    {
      _ASSERT(FALSE);
      hRes = E_NOTIMPL;
    }
  }
  return hRes;
}

HRESULT CJavaVariantInterchange::FloatToJavaLangFloat(__in JNIEnv *env, __in jfloat value, __out jobject *javaobj)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  HRESULT hRes;

  if (javaobj == NULL)
    return E_POINTER;
  *javaobj = NULL;
  //init
  hRes = InitializeBasicTypes(env);
  //get object
  if (SUCCEEDED(hRes))
  {
    *javaobj = env->CallStaticObjectMethod(globalFloatCls, floatConst_MethodId, value);
    if ((*javaobj) == NULL)
    {
      hRes = CheckJavaException(env);
      if (SUCCEEDED(hRes))
        hRes = E_FAIL;
    }
  }
  return hRes;
}

HRESULT CJavaVariantInterchange::JavaLangFloatToFloat(__in JNIEnv *env, __in jobject javaobj, __out jfloat *value)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  HRESULT hRes;

  if (value == NULL)
    return E_POINTER;
  *value = 0;
  //init
  hRes = InitializeBasicTypes(env);
  //get value
  if (SUCCEEDED(hRes))
  {
    *value = env->CallFloatMethod(javaobj, floatGetValue_MethodId);
    hRes = JAVA::CheckJavaException(env);
  }
  return hRes;
}

HRESULT CJavaVariantInterchange::DoubleToJavaLangDouble(__in JNIEnv *env, __in jdouble value, __out jobject *javaobj)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  HRESULT hRes;

  if (javaobj == NULL)
    return E_POINTER;
  *javaobj = NULL;
  //init
  hRes = InitializeBasicTypes(env);
  //get object
  if (SUCCEEDED(hRes))
  {
    *javaobj = env->CallStaticObjectMethod(globalDoubleCls, doubleConst_MethodId, value);
    if ((*javaobj) == NULL)
    {
      hRes = CheckJavaException(env);
      if (SUCCEEDED(hRes))
        hRes = E_FAIL;
    }
  }
  return hRes;
}

HRESULT CJavaVariantInterchange::JavaLangDoubleToDouble(__in JNIEnv *env, __in jobject javaobj, __out jdouble *value)
{
  CNktSimpleLockNonReentrant cLock(&nMutex);
  HRESULT hRes;

  if (value == NULL)
    return E_POINTER;
  *value = 0;
  //init
  hRes = InitializeBasicTypes(env);
  //get value
  if (SUCCEEDED(hRes))
  {
    *value = env->CallDoubleMethod(javaobj, doubleGetValue_MethodId);
    hRes = JAVA::CheckJavaException(env);
  }
  return hRes;
}

HRESULT CJavaVariantInterchange::InitializeBasicTypes(__in JNIEnv *env)
{
  TJavaAutoPtr<jclass> cCls;

  if (globalBooleanCls == NULL)
  {
    cCls.Attach(env, env->FindClass("java/lang/Boolean"));
    if (cCls == NULL)
      return NKT_E_NotFound;
    globalBooleanCls = (jclass)(env->NewGlobalRef((jobject)cCls.Get()));
    if (globalBooleanCls == NULL)
      return E_OUTOFMEMORY;
  }
  //----
  if (globalByteCls == NULL)
  {
    cCls.Attach(env, env->FindClass("java/lang/Byte"));
    if (cCls == NULL)
      return NKT_E_NotFound;
    globalByteCls = (jclass)(env->NewGlobalRef((jobject)cCls.Get()));
    if (globalByteCls == NULL)
      return E_OUTOFMEMORY;
  }
  //----
  if (globalCharacterCls == NULL)
  {
    cCls.Release();
    cCls.Attach(env, env->FindClass("java/lang/Character"));
    if (cCls == NULL)
      return NKT_E_NotFound;
    globalCharacterCls = (jclass)(env->NewGlobalRef((jobject)cCls.Get()));
    if (globalCharacterCls == NULL)
      return E_OUTOFMEMORY;
  }
  //----
  if (globalShortCls == NULL)
  {
    cCls.Release();
    cCls.Attach(env, env->FindClass("java/lang/Short"));
    if (cCls == NULL)
      return NKT_E_NotFound;
    globalShortCls = (jclass)(env->NewGlobalRef((jobject)cCls.Get()));
    if (globalShortCls == NULL)
      return E_OUTOFMEMORY;
  }
  //----
  if (globalIntegerCls == NULL)
  {
    cCls.Release();
    cCls.Attach(env, env->FindClass("java/lang/Integer"));
    if (cCls == NULL)
      return NKT_E_NotFound;
    globalIntegerCls = (jclass)(env->NewGlobalRef((jobject)cCls.Get()));
    if (globalIntegerCls == NULL)
      return E_OUTOFMEMORY;
  }
  //----
  if (globalAtomicIntegerCls == NULL && globalAtomicIntegerCls != (jclass)-1)
  {
    cCls.Release();
    cCls.Attach(env, env->FindClass("java/util/concurrent/atomic/AtomicInteger"));
    if (cCls != NULL)
    {
      globalAtomicIntegerCls = (jclass)(env->NewGlobalRef((jobject)cCls.Get()));
      if (globalAtomicIntegerCls == NULL)
        return E_OUTOFMEMORY;
    }
    else
    {
      env->ExceptionClear(); //clear exception in order to let next calls to work as expected (jni rulez)
      globalAtomicIntegerCls = (jclass)-1;
    }
  }
  //----
  if (globalLongCls == NULL)
  {
    cCls.Release();
    cCls.Attach(env, env->FindClass("java/lang/Long"));
    if (cCls == NULL)
      return NKT_E_NotFound;
    globalLongCls = (jclass)(env->NewGlobalRef((jobject)cCls.Get()));
    if (globalLongCls == NULL)
      return E_OUTOFMEMORY;
  }
  //----
  if (globalAtomicLongCls == NULL && globalAtomicLongCls != (jclass)-1)
  {
    cCls.Release();
    cCls.Attach(env, env->FindClass("java/util/concurrent/atomic/AtomicLong"));
    if (cCls != NULL)
    {
      globalAtomicLongCls = (jclass)(env->NewGlobalRef((jobject)cCls.Get()));
      if (globalAtomicLongCls == NULL)
        return E_OUTOFMEMORY;
    }
    else
    {
      env->ExceptionClear(); //clear exception in order to let next calls to work as expected (jni rulez)
      globalAtomicLongCls = (jclass)-1;
    }
  }
  //----
  if (globalFloatCls == NULL)
  {
    cCls.Release();
    cCls.Attach(env, env->FindClass("java/lang/Float"));
    if (cCls == NULL)
      return NKT_E_NotFound;
    globalFloatCls = (jclass)(env->NewGlobalRef((jobject)cCls.Get()));
    if (globalFloatCls == NULL)
      return E_OUTOFMEMORY;
  }
  //----
  if (globalDoubleCls == NULL)
  {
    cCls.Release();
    cCls.Attach(env, env->FindClass("java/lang/Double"));
    if (cCls == NULL)
      return NKT_E_NotFound;
    globalDoubleCls = (jclass)(env->NewGlobalRef((jobject)cCls.Get()));
    if (globalDoubleCls == NULL)
      return E_OUTOFMEMORY;
  }
  //----
  if (booleanConst_MethodId == NULL)
  {
    booleanConst_MethodId = env->GetMethodID(globalBooleanCls, "<init>", "(Z)V");
    byteConst_MethodId = env->GetMethodID(globalByteCls, "<init>", "(B)V");
    characterConst_MethodId = env->GetMethodID(globalCharacterCls, "<init>", "(C)V");
    shortConst_MethodId = env->GetMethodID(globalShortCls, "<init>", "(S)V");
    integerConst_MethodId = env->GetMethodID(globalIntegerCls, "<init>", "(I)V");
    longConst_MethodId = env->GetMethodID(globalLongCls, "<init>", "(J)V");
    floatConst_MethodId = env->GetMethodID(globalFloatCls, "<init>", "(F)V");
    doubleConst_MethodId = env->GetMethodID(globalDoubleCls, "<init>", "(D)V");
    //----
    booleanGetValue_MethodId = env->GetMethodID(globalBooleanCls, "booleanValue", "()Z");
    byteGetValue_MethodId = env->GetMethodID(globalByteCls, "byteValue", "()B");
    characterGetValue_MethodId = env->GetMethodID(globalCharacterCls, "charValue", "()C");
    shortGetValue_MethodId = env->GetMethodID(globalShortCls, "shortValue", "()S");
    integerGetValue_MethodId = env->GetMethodID(globalIntegerCls, "intValue", "()I");
    longGetValue_MethodId = env->GetMethodID(globalLongCls, "longValue", "()J");
    floatGetValue_MethodId = env->GetMethodID(globalFloatCls, "floatValue", "()F");
    doubleGetValue_MethodId = env->GetMethodID(globalDoubleCls, "doubleValue", "()D");
    //----
    if (booleanConst_MethodId == NULL || byteConst_MethodId == NULL || characterConst_MethodId == NULL ||
          shortConst_MethodId == NULL || integerConst_MethodId == NULL || longConst_MethodId == NULL ||
          floatConst_MethodId == NULL || doubleConst_MethodId == NULL || 
        booleanGetValue_MethodId == NULL || byteGetValue_MethodId == NULL || characterGetValue_MethodId == NULL ||
          shortGetValue_MethodId == NULL || integerGetValue_MethodId == NULL || longGetValue_MethodId == NULL ||
          floatGetValue_MethodId == NULL || doubleGetValue_MethodId == NULL)
    {
method_notimpl:
      booleanConst_MethodId = byteConst_MethodId = characterConst_MethodId = shortConst_MethodId =
        integerConst_MethodId = atomicIntegerConst_MethodId = longConst_MethodId = atomicLongConst_MethodId =
          floatConst_MethodId = doubleConst_MethodId = NULL;
      booleanGetValue_MethodId = byteGetValue_MethodId = characterGetValue_MethodId = shortGetValue_MethodId =
        integerGetValue_MethodId = atomicIntegerGetValue_MethodId = longGetValue_MethodId =
          atomicLongGetValue_MethodId = floatGetValue_MethodId = doubleGetValue_MethodId = NULL;
      _ASSERT(FALSE);
      return E_NOTIMPL;
    }
    if (globalAtomicIntegerCls != (jclass)-1)
    {
      atomicIntegerConst_MethodId = env->GetMethodID(globalAtomicIntegerCls, "<init>", "(I)V");
      atomicIntegerGetValue_MethodId = env->GetMethodID(globalAtomicIntegerCls, "intValue", "()I");
      if (atomicIntegerConst_MethodId == NULL || atomicIntegerGetValue_MethodId == NULL)
        goto method_notimpl;
    }
    if (globalAtomicLongCls != (jclass)-1)
    {
      atomicLongConst_MethodId = env->GetMethodID(globalAtomicLongCls, "<init>", "(J)V");
      atomicLongGetValue_MethodId = env->GetMethodID(globalAtomicLongCls, "longValue", "()J");
      if (atomicLongConst_MethodId == NULL || atomicLongGetValue_MethodId == NULL)
        goto method_notimpl;
    }
  }
  //done
  return S_OK;
}

HRESULT CJavaVariantInterchange::InitializeBigDecimal(__in JNIEnv *env)
{
  if (globalBigDecCls == NULL)
  {
    TJavaAutoPtr<jclass> cCls;

    cCls.Attach(env, env->FindClass("java/math/BigDecimal"));
    if (cCls == NULL)
      return NKT_E_NotFound;
    globalBigDecCls = (jclass)(env->NewGlobalRef((jobject)cCls.Get()));
    if (globalBigDecCls == NULL)
      return E_OUTOFMEMORY;
  }
  //----
  if (globalBigIntCls == NULL)
  {
    TJavaAutoPtr<jclass> cCls;

    cCls.Attach(env, env->FindClass("java/math/BigInteger"));
    if (cCls == NULL)
      return NKT_E_NotFound;
    globalBigIntCls = (jclass)(env->NewGlobalRef((jobject)cCls.Get()));
    if (globalBigIntCls == NULL)
      return E_OUTOFMEMORY;
  }
  //----
  if (bigDecConst_MethodId == NULL)
  {
    bigDecConst_MethodId = env->GetMethodID(globalBigDecCls, "<init>", "(Ljava/math/BigInteger;I)V");
    bigDecUnscaledValue_MethodId = env->GetMethodID(globalBigDecCls, "unscaledValue", "()Ljava/math/BigInteger;");
    bigDecSignum_MethodId = env->GetMethodID(globalBigDecCls, "signum", "()I");
    bigDecNegate_MethodId = env->GetMethodID(globalBigDecCls, "negate", "()Ljava/math/BigDecimal;");
    bigDecScale_MethodId = env->GetMethodID(globalBigDecCls, "scale", "()I");
    bigDecCompareTo_MethodId = env->GetMethodID(globalBigDecCls, "compareTo", "(Ljava/math/BigDecimal;)I");
    //----
    bigIntConst_MethodId = env->GetMethodID(globalBigIntCls, "<init>", "(I[B)V");
    bigIntConst2_MethodId = env->GetMethodID(globalBigIntCls, "<init>", "(Ljava/lang/String;I)V");
    bigIntBitLength_MethodId = env->GetMethodID(globalBigIntCls, "bitLength", "()I");
    bigIntIntValue_MethodId = env->GetMethodID(globalBigIntCls, "intValue", "()I");
    bigIntShiftRight_MethodId = env->GetMethodID(globalBigIntCls, "shiftRight", "(I)Ljava/math/BigInteger;");
    bigIntNegate_MethodId = env->GetMethodID(globalBigIntCls, "negate", "()Ljava/math/BigInteger;");
    //----
    if (bigDecConst_MethodId == NULL || bigDecUnscaledValue_MethodId == NULL ||
          bigDecSignum_MethodId == NULL || bigDecNegate_MethodId == NULL || bigDecScale_MethodId == NULL ||
          bigDecCompareTo_MethodId == NULL ||
        bigIntConst_MethodId == NULL || bigIntConst2_MethodId == NULL || bigIntBitLength_MethodId == NULL ||
          bigIntIntValue_MethodId == NULL || bigIntShiftRight_MethodId == NULL || bigIntNegate_MethodId == NULL)
    {
      bigDecConst_MethodId = bigDecUnscaledValue_MethodId = bigDecSignum_MethodId = bigDecNegate_MethodId =
        bigDecScale_MethodId = bigDecCompareTo_MethodId = NULL;
      bigIntConst_MethodId = bigIntConst2_MethodId = bigIntBitLength_MethodId = bigIntIntValue_MethodId =
        bigIntShiftRight_MethodId = bigIntNegate_MethodId = NULL;
      _ASSERT(FALSE);
      return E_NOTIMPL;
    }
  }
  //create BigDecimal helpers
  if (globalLargeBigDec == NULL)
  {
    TJavaAutoPtr<jobject> cTempBigInt, cTempBigIntNeg;
    TJavaAutoPtr<jobject> cTempBigDec, cTempBigDecNeg;
    TJavaAutoPtr<jstring> cTempStr;

    cTempStr.Attach(env, env->NewString((jchar*)L"ffffffffffffffffffffffff", 24));
    if (cTempStr == NULL)
      return E_OUTOFMEMORY;
    cTempBigInt.Attach(env, env->NewObject(globalBigIntCls, bigIntConst2_MethodId, cTempStr.Get(), 16));
    if (cTempBigInt == NULL)
      return E_OUTOFMEMORY;
    cTempBigIntNeg.Attach(env, env->CallObjectMethod(cTempBigInt, bigIntNegate_MethodId, cTempBigInt.Get()));
    if (cTempBigIntNeg == NULL)
      return E_OUTOFMEMORY;
    cTempBigDec.Attach(env, env->NewObject(globalBigDecCls, bigDecConst_MethodId, cTempBigInt.Get(), 0));
    if (cTempBigDec == NULL)
      return E_OUTOFMEMORY;
    cTempBigDecNeg.Attach(env, env->NewObject(globalBigDecCls, bigDecConst_MethodId, cTempBigIntNeg.Get(), 0));
    if (cTempBigDecNeg == NULL)
      return E_OUTOFMEMORY;
    globalLargeBigDec = env->NewGlobalRef(cTempBigDec.Get());
    if (globalLargeBigDec == NULL)
      return E_OUTOFMEMORY;
    globalSmallBigDec = env->NewGlobalRef(cTempBigDecNeg.Get());
    if (globalSmallBigDec == NULL)
    {
      env->DeleteGlobalRef(globalLargeBigDec);
      globalLargeBigDec = NULL;
      return E_OUTOFMEMORY;
    }
  }
  //done
  return S_OK;
}

HRESULT CJavaVariantInterchange::InitializeCalendar(__in JNIEnv *env)
{
  if (globalCalendarCls == NULL)
  {
    TJavaAutoPtr<jclass> cCls;

    cCls.Attach(env, env->FindClass("java/util/Calendar"));
    if (cCls == NULL)
      return NKT_E_NotFound;
    globalCalendarCls = (jclass)(env->NewGlobalRef((jobject)cCls.Get()));
    if (globalCalendarCls == NULL)
      return E_OUTOFMEMORY;
  }
  //----
  if (globalDateCls == NULL)
  {
    TJavaAutoPtr<jclass> cCls;

    cCls.Attach(env, env->FindClass("java/util/Date"));
    if (cCls == NULL)
      return NKT_E_NotFound;
    globalDateCls = (jclass)(env->NewGlobalRef((jobject)cCls.Get()));
    if (globalDateCls == NULL)
      return E_OUTOFMEMORY;
  }
  //----
  if (globalTimeZoneCls == NULL)
  {
    TJavaAutoPtr<jclass> cCls;

    cCls.Attach(env, env->FindClass("java/util/TimeZone"));
    if (cCls == NULL)
      return NKT_E_NotFound;
    globalTimeZoneCls = (jclass)(env->NewGlobalRef((jobject)cCls.Get()));
    if (globalTimeZoneCls == NULL)
      return E_OUTOFMEMORY;
  }
  //----
  if (calGetInstance_MethodId == NULL)
  {
    calGetInstance_MethodId = env->GetStaticMethodID(globalCalendarCls, "getInstance",
                                                     "(Ljava/util/TimeZone;)Ljava/util/Calendar;");
    calSet2_MethodId = env->GetMethodID(globalCalendarCls, "set", "(II)V");
    calSet6_MethodId = env->GetMethodID(globalCalendarCls, "set", "(IIIIII)V");
    calGetTime_MethodId = env->GetMethodID(globalCalendarCls, "getTime", "()Ljava/util/Date;");
    calGet1_MethodId = env->GetMethodID(globalCalendarCls, "get", "(I)I");
    calSetTime_MethodId = env->GetMethodID(globalCalendarCls, "setTime", "(Ljava/util/Date;)V");
    if (calGetInstance_MethodId == NULL || calSet6_MethodId == NULL || calSet2_MethodId == NULL ||
        calGetTime_MethodId == NULL || calGet1_MethodId == NULL || calSetTime_MethodId == NULL)
    {
      calGetInstance_MethodId = calSet2_MethodId = calSet6_MethodId = calGetTime_MethodId =
        calGet1_MethodId = calSetTime_MethodId = NULL;
      _ASSERT(FALSE);
      return E_NOTIMPL;
    }
  }
  //----
  if (tzoneGetTimeZone_MethodId == NULL)
  {
    tzoneGetTimeZone_MethodId = env->GetStaticMethodID(globalTimeZoneCls, "getTimeZone",
                                                       "(Ljava/lang/String;)Ljava/util/TimeZone;");
    if (tzoneGetTimeZone_MethodId == NULL)
    {
      tzoneGetTimeZone_MethodId = NULL;
      _ASSERT(FALSE);
      return E_NOTIMPL;
    }
  }
  //done
  return S_OK;
}

} //JAVA
