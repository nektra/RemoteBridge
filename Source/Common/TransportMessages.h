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

#ifndef _TRANSPORT_MESSAGES_H
#define _TRANSPORT_MESSAGES_H

#include <windows.h>

//-----------------------------------------------------------

#pragma pack(1)
typedef struct {
  ULONG nStructSize;
  ULONGLONG hControllerProc;
  ULONGLONG hShutdownEv;
  ULONGLONG hSlavePipe;
  ULONG nMasterPid;
  ULONG nFlags;
} INIT_DATA;
#pragma pack()

//-----------------------------------------------------------

#define TMSG_MAX_MESSAGE_SIZE                           4096

#define TMSG_CODE_WatchComInterface               0x00000001
#define TMSG_CODE_UnwatchComInterface             0x00000002
#define TMSG_CODE_UnwatchAllComInterfaces         0x00000003
#define TMSG_CODE_GetComInterfaceFromHWnd         0x00000004
#define TMSG_CODE_GetComInterfaceFromIndex        0x00000005
#define TMSG_CODE_InspectComInterfaceMethod       0x00000006
#define TMSG_CODE_FindWindow                      0x00000007
#define TMSG_CODE_GetWindowText                   0x00000008
#define TMSG_CODE_SetWindowText                   0x00000009
#define TMSG_CODE_PostSendMessage                 0x0000000A
#define TMSG_CODE_GetChildWindow                  0x0000000B
#define TMSG_CODE_SuspendChildProcess             0x0000000C
#define TMSG_CODE_DefineJavaClass                 0x0000000D
#define TMSG_CODE_CreateJavaObject                0x0000000E
#define TMSG_CODE_InvokeJavaMethod                0x0000000F
#define TMSG_CODE_PutJavaField                    0x00000010
#define TMSG_CODE_GetJavaField                    0x00000011
#define TMSG_CODE_IsSameJavaObject                0x00000012
#define TMSG_CODE_RemoveJObjectRef                0x00000013
#define TMSG_CODE_IsJVMAttached                   0x00000014
#define TMSG_CODE_GetJavaObjectClass              0x00000015
#define TMSG_CODE_IsJavaObjectInstanceOf          0x00000016

#define TMSG_CODE_RaiseOnCreateProcessEvent       0x10000001
#define TMSG_CODE_RaiseOnComInterfaceCreatedEvent 0x10000002
#define TMSG_CODE_RaiseOnJavaNativeMethodCalled   0x10010001

//-----------------------------------------------------------

#pragma pack(1)
typedef struct {
  DWORD dwMsgCode;
  DWORD dwCheckSum;
} TMSG_HEADER;

typedef struct {
  HRESULT hRes;
} TMSG_SIMPLE_REPLY;

//----

typedef struct {
  TMSG_HEADER sHeader;
  CLSID sClsId;
  IID sIid;
} TMSG_WATCHCOMINTERFACE, TMSG_UNWATCHCOMINTERFACE;

//----

typedef struct {
  TMSG_HEADER sHeader;
  ULONGLONG hWnd;
  IID sIid;
} TMSG_GETCOMINTERFACEFROMHWND;

typedef struct {
  TMSG_HEADER sHeader;
  IID sIid;
  ULONG nIndex;
} TMSG_GETCOMINTERFACEFROMINDEX;

//----

typedef struct {
  TMSG_HEADER sHeader;
  IID sIid;
  ULONG nMethodIndex;
  ULONG nIidIndex;
  IID sQueryIid;
  ULONG nReturnParameterIndex;
} TMSG_INSPECTCOMINTERFACEMETHOD;

//----

typedef struct {
  TMSG_HEADER sHeader;
  ULONGLONG hParentWnd;
  BOOL bRecurseChilds;
  ULONGLONG nNameLen;
  ULONGLONG nClassLen;
} TMSG_FINDWINDOW;

typedef struct {
  ULONGLONG hWnd;
} TMSG_FINDWINDOW_REPLY;

//----

typedef struct {
  TMSG_HEADER sHeader;
  ULONGLONG hWnd;
} TMSG_GETWINDOWTEXT;

typedef struct {
  ULONGLONG nTextLen;
  WCHAR szTextW[1];
} TMSG_GETWINDOWTEXT_REPLY;

//----

typedef struct {
  TMSG_HEADER sHeader;
  ULONGLONG hWnd;
} TMSG_SETWINDOWTEXT;

//----

typedef struct {
  TMSG_HEADER sHeader;
  ULONGLONG hWnd;
  ULONG nMsg;
  ULONGLONG wParam, lParam;
  ULONG nTimeout;
  BOOL bPost;
} TMSG_POSTSENDMESSAGE;

typedef struct {
  HRESULT hRes;
  ULONGLONG nResult;
} TMSG_POSTSENDMESSAGE_REPLY;

//----

typedef struct {
  TMSG_HEADER sHeader;
  ULONGLONG hParentWnd;
  BOOL bIsIndex;
  ULONG nIdOrIndex;
} TMSG_GETCHILDWINDOW;

typedef struct {
  HRESULT hRes;
  ULONGLONG hWnd;
} TMSG_GETCHILDWINDOW_REPLY;

//----

typedef struct {
  TMSG_HEADER sHeader;
  ULONGLONG hParentProc;
  ULONGLONG hProcess;
  ULONGLONG hThread;
} TMSG_SUSPENDCHILDPROCESS;

typedef struct {
  HRESULT hRes;
  ULONGLONG hReadyEvent;
  ULONGLONG hContinueEvent;
} TMSG_SUSPENDCHILDPROCESS_REPLY;

//----

typedef struct {
  TMSG_HEADER sHeader;
  ULONGLONG nByteCodeLen;
  ULONGLONG nClassLen;
} TMSG_DEFINEJAVACLASS;

//----

typedef struct {
  TMSG_HEADER sHeader;
  ULONGLONG nClassLen;
  ULONGLONG nParametersLen;
} TMSG_CREATEJAVAOBJECT;

typedef struct {
  HRESULT hRes;
  ULONGLONG javaobj;
} TMSG_CREATEJAVAOBJECT_REPLY;

//----

typedef struct {
  TMSG_HEADER sHeader;
  ULONGLONG javaobj;
  ULONGLONG nClassLen;
  ULONGLONG nMethodLen;
  ULONGLONG nParametersLen;
} TMSG_INVOKEJAVAMETHOD;

typedef struct {
  TMSG_HEADER sHeader;
  HRESULT hRes;
  ULONGLONG nResultLen;
} TMSG_INVOKEJAVAMETHOD_REPLY;

//----

typedef struct {
  TMSG_HEADER sHeader;
  ULONGLONG javaobj;
  ULONGLONG nClassLen;
  ULONGLONG nFieldLen;
  ULONGLONG nValueLen;
  ULONG nFlags;
} TMSG_PUTJAVAFIELD;

//----

typedef struct {
  TMSG_HEADER sHeader;
  ULONGLONG javaobj;
  ULONGLONG nClassLen;
  ULONGLONG nFieldLen;
  ULONG nFlags;
} TMSG_GETJAVAFIELD;

typedef struct {
  TMSG_HEADER sHeader;
  HRESULT hRes;
  ULONGLONG nResultLen;
} TMSG_GETJAVAFIELD_REPLY;

//----

typedef struct {
  TMSG_HEADER sHeader;
  ULONGLONG javaobj1;
  ULONGLONG javaobj2;
} TMSG_ISSAMEJAVAOBJECT;

//----

typedef struct {
  TMSG_HEADER sHeader;
  ULONGLONG javaobj;
} TMSG_REMOVEJOBJECTREF;

//----

typedef struct {
  TMSG_HEADER sHeader;
  ULONGLONG javaobj;
} TMSG_GETJAVAOBJECTCLASS;

typedef struct {
  TMSG_HEADER sHeader;
  HRESULT hRes;
  ULONGLONG nClassNameLen;
} TMSG_GETJAVAOBJECTCLASS_REPLY;

//----

typedef struct {
  TMSG_HEADER sHeader;
  HRESULT hRes;
  WORD wVersionMajor, wVersionMinor;
} TMSG_ISJVMATTACHED_REPLY;

//----

typedef struct {
  TMSG_HEADER sHeader;
  ULONGLONG javaobj;
  ULONGLONG nClassLen;
} TMSG_ISJAVAOBJECTINSTANCEOF;

//----

typedef struct {
  TMSG_HEADER sHeader;
  ULONG dwPid, dwTid, dwPlatformBits;
} TMSG_RAISEONCREATEPROCESSEVENT;

//----

typedef struct {
  TMSG_HEADER sHeader;
  ULONG dwTid;
  CLSID sClsId;
  IID sIid;
} TMSG_RAISEONCOMINTERFACECREATEDEVENT;

//----

typedef struct {
  TMSG_HEADER sHeader;
  ULONGLONG nClassNameLen;
  ULONGLONG nMethodNameLen;
  ULONGLONG javaobj_or_class;
  ULONGLONG nParamsCount;
} TMSG_RAISEONJAVANATIVEMETHODCALLED;

typedef struct {
  TMSG_HEADER sHeader;
  HRESULT hRes;
  ULONGLONG nResultLen;
} TMSG_RAISEONJAVANATIVEMETHODCALLED_REPLY;

#pragma pack()

//-----------------------------------------------------------

#endif //_TRANSPORT_MESSAGES_H
