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

namespace JAVA {

CUnmarshalledJValues::CUnmarshalledJValues(__in JNIEnv *_env)
{
  env = _env;
  nValuesCount = nArraySizes = 0;
  lpJValues = NULL;
  lpJValueTypes = NULL;
  return;
}

CUnmarshalledJValues::~CUnmarshalledJValues()
{
  SIZE_T i;

  if (lpJValues != NULL)
  {
    for (i=0; i<nValuesCount; i++)
    {
      if (lpJValueTypes[i].nDimCount > 0)
      {
        //arrays are objects
        if (lpJValues[i].l != NULL)
          env->DeleteLocalRef(lpJValues[i].l);
      }
      else
      {
        switch (lpJValueTypes[i].nType)
        {
          case CJValueDef::typString:
          case CJValueDef::typBigDecimal:
          case CJValueDef::typDate:
          case CJValueDef::typCalendar:
          case CJValueDef::typObject:
          case CJValueDef::typBooleanObj:
          case CJValueDef::typByteObj:
          case CJValueDef::typCharObj:
          case CJValueDef::typShortObj:
          case CJValueDef::typIntObj:
          case CJValueDef::typLongObj:
          case CJValueDef::typFloatObj:
          case CJValueDef::typDoubleObj:
          case CJValueDef::typAtomicIntObj:
          case CJValueDef::typAtomicLongObj:
            if (lpJValues[i].l != NULL)
              env->DeleteLocalRef(lpJValues[i].l);
            break;
        }
      }
    }
    NKT_FREE(lpJValues);
  }
  NKT_FREE(lpJValueTypes);
  return;
}

HRESULT CUnmarshalledJValues::Add(__in jvalue value, __in BYTE nValueType, __in BYTE nDimCount)
{
  if (nValuesCount >= nArraySizes)
  {
    jvalue *lpNewJValues;
    JVALUETYPE *lpNewJValueTypes;

    lpNewJValues = (jvalue*)NKT_MALLOC((nArraySizes+32)*sizeof(jvalue));
    if (lpNewJValues == NULL)
      return E_OUTOFMEMORY;
    lpNewJValueTypes = (JVALUETYPE*)NKT_MALLOC((nArraySizes+32)*sizeof(JVALUETYPE));
    if (lpNewJValueTypes == NULL)
    {
      NKT_FREE(lpNewJValues);
      return E_OUTOFMEMORY;
    }
    nArraySizes += 32;
    memcpy(lpNewJValues, lpJValues, nValuesCount*sizeof(jvalue));
    NKT_FREE(lpJValues);
    lpJValues = lpNewJValues;
    memcpy(lpNewJValueTypes, lpJValueTypes, nValuesCount*sizeof(JVALUETYPE));
    NKT_FREE(lpJValueTypes);
    lpJValueTypes = lpNewJValueTypes;
  }
  lpJValues[nValuesCount] = value;
  lpJValueTypes[nValuesCount].nType = nValueType;
  lpJValueTypes[nValuesCount].nDimCount = nDimCount;
  nValuesCount++;
  return S_OK;
}

} //JAVA
