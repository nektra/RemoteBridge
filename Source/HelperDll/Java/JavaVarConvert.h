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
#include <Ole2.h>
#include "JavaManager.h"

//-----------------------------------------------------------

namespace JAVA {

class CUnmarshalledJValues
{
public:
  typedef struct {
    BYTE nType;
    BYTE nDimCount;
  } JVALUETYPE;

  CUnmarshalledJValues(__in JNIEnv *env);
  ~CUnmarshalledJValues();

  SIZE_T GetJValuesCount()
    {
    return nValuesCount;
    };

  jvalue* GetJValues()
    {
    return lpJValues;
    };

  JVALUETYPE* GetJValueTypes()
    {
    return lpJValueTypes;
    };

  HRESULT Add(__in jvalue value, __in BYTE nValueType, __in BYTE nDimCount);

  jvalue* DetachJValues();

private:
  JNIEnv *env;
  SIZE_T nValuesCount, nArraySizes;
  jvalue *lpJValues;
  JVALUETYPE *lpJValueTypes;
};

//----------------

class CJavaVariantInterchange
{
public:
  CJavaVariantInterchange();
  ~CJavaVariantInterchange();

  VOID Reset(__in JNIEnv *env);

  HRESULT DecimalToJObject(__in JNIEnv *env, __in DECIMAL *lpDec, __out jobject *lpJObject);
  HRESULT JObjectToDecimal(__in JNIEnv *env, __in jobject javaobj, __out DECIMAL *lpDec);

  HRESULT SystemTimeToJObject(__in JNIEnv *env, __in SYSTEMTIME *lpSysTime, __in BOOL bReturnDate,
                              __out jobject *lpJObject);
  HRESULT JObjectToSystemTime(__in JNIEnv *env, __in jobject javaobj, __out SYSTEMTIME *lpSysTime);

  HRESULT BooleanToJavaLangBoolean(__in JNIEnv *env, __in jboolean value, __out jobject *javaobj);
  HRESULT JavaLangBooleanToBoolean(__in JNIEnv *env, __in jobject javaobj, __out jboolean *value);

  HRESULT ByteToJavaLangByte(__in JNIEnv *env, __in jbyte value, __out jobject *javaobj);
  HRESULT JavaLangByteToByte(__in JNIEnv *env, __in jobject javaobj, __out jbyte *value);

  HRESULT CharToJavaLangCharacter(__in JNIEnv *env, __in jchar value, __out jobject *javaobj);
  HRESULT JavaLangCharacterToChar(__in JNIEnv *env, __in jobject javaobj, __out jchar *value);

  HRESULT ShortToJavaLangShort(__in JNIEnv *env, __in jshort value, __out jobject *javaobj);
  HRESULT JavaLangShortToShort(__in JNIEnv *env, __in jobject javaobj, __out jshort *value);

  HRESULT IntegerToJavaLangInteger(__in JNIEnv *env, __in jint value, __out jobject *javaobj);
  HRESULT JavaLangIntegerToInteger(__in JNIEnv *env, __in jobject javaobj, __out jint *value);

  HRESULT IntegerToJavaUtilConcurrentAtomicInteger(__in JNIEnv *env, __in jint value, __out jobject *javaobj);
  HRESULT JavaUtilConcurrentAtomicIntegerToInteger(__in JNIEnv *env, __in jobject javaobj, __out jint *value);

  HRESULT LongToJavaLangLong(__in JNIEnv *env, __in jlong value, __out jobject *javaobj);
  HRESULT JavaLangLongToLong(__in JNIEnv *env, __in jobject javaobj, __out jlong *value);

  HRESULT LongToJavaUtilConcurrentAtomicLong(__in JNIEnv *env, __in jlong value, __out jobject *javaobj);
  HRESULT JavaUtilConcurrentAtomicLongToLong(__in JNIEnv *env, __in jobject javaobj, __out jlong *value);

  HRESULT FloatToJavaLangFloat(__in JNIEnv *env, __in jfloat value, __out jobject *javaobj);
  HRESULT JavaLangFloatToFloat(__in JNIEnv *env, __in jobject javaobj, __out jfloat *value);

  HRESULT DoubleToJavaLangDouble(__in JNIEnv *env, __in jdouble value, __out jobject *javaobj);
  HRESULT JavaLangDoubleToDouble(__in JNIEnv *env, __in jobject javaobj, __out jdouble *value);

private:
  HRESULT InitializeBasicTypes(__in JNIEnv *env);
  HRESULT InitializeBigDecimal(__in JNIEnv *env);
  HRESULT InitializeCalendar(__in JNIEnv *env);

private:
  LONG volatile nMutex;
  //----
  jclass globalBigIntCls, globalBigDecCls;
  //----
  jclass globalCalendarCls, globalDateCls, globalTimeZoneCls;
  //----
  jclass globalBooleanCls, globalByteCls, globalCharacterCls, globalShortCls, globalIntegerCls, globalAtomicIntegerCls;
  jclass globalLongCls, globalAtomicLongCls, globalFloatCls, globalDoubleCls;
  //----
  jmethodID bigDecConst_MethodId, bigDecUnscaledValue_MethodId, bigDecSignum_MethodId;
  jmethodID bigDecNegate_MethodId, bigDecScale_MethodId, bigDecCompareTo_MethodId;
  //----
  jmethodID bigIntConst_MethodId, bigIntConst2_MethodId, bigIntBitLength_MethodId, bigIntIntValue_MethodId;
  jmethodID bigIntShiftRight_MethodId, bigIntNegate_MethodId;
  //----
  jmethodID calGetInstance_MethodId, calSet2_MethodId, calSet6_MethodId, calGetTime_MethodId;
  jmethodID calGet1_MethodId, calSetTime_MethodId;
  jmethodID tzoneGetTimeZone_MethodId;
  //----
  jmethodID booleanConst_MethodId, byteConst_MethodId, characterConst_MethodId, shortConst_MethodId;
  jmethodID integerConst_MethodId, atomicIntegerConst_MethodId, longConst_MethodId, atomicLongConst_MethodId;
  jmethodID floatConst_MethodId, doubleConst_MethodId;
  jmethodID booleanGetValue_MethodId, byteGetValue_MethodId, characterGetValue_MethodId, shortGetValue_MethodId;
  jmethodID integerGetValue_MethodId, atomicIntegerGetValue_MethodId, longGetValue_MethodId;
  jmethodID atomicLongGetValue_MethodId, floatGetValue_MethodId, doubleGetValue_MethodId;
  //----
  jobject globalLargeBigDec, globalSmallBigDec;
};

//----------------

//Convert a jvalue into a data stream
//* It can be a single value or an array of a defined type
//* If it is a jobject or an array of jobjects, we add the definition of that jobject (fields, methods, etc.)
HRESULT VarConvert_MarshalJValue(__in JNIEnv *env, __in jvalue val, __in CJValueDef *lpJValueDef,
                                 __in CJavaVariantInterchange *lpJavaVarInterchange,
                                 __in CJavaObjectManager *lpJObjectMgr, __in LPBYTE lpDest, __inout SIZE_T &nSize);

//Peeks the passed parameter types from a stream.
HRESULT VarConvert_UnmarshalJValues_PeekTypes(__in JNIEnv *env, __in LPBYTE lpSrc, __in SIZE_T nSrcBytes,
                                              __in CJavaObjectManager *lpJObjectMgr,
                                              __out CUnmarshalledJValues **lpValues);
//Converts a stream into an array of jvalues.
//* An array is basically a jobject and we only need the root to access or delete it
//* Also we return an array of bytes indicating the type of each jvalue
HRESULT VarConvert_UnmarshalJValues(__in JNIEnv *env, __in LPBYTE lpSrc, __in SIZE_T nSrcBytes,
                                    __in CJavaVariantInterchange *lpJavaVarInterchange,
                                    __in CJavaObjectManager *lpJObjectMgr, __in CMethodOrFieldDef *lpMethodDef,
                                    __in BOOL bParseResult, __out CUnmarshalledJValues **lpValues);

} //JAVA
