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

typedef [v1_enum] enum eNktDispIds {
  dispidNktRemoteBridgeHook = 1,
  dispidNktRemoteBridgeUnhook,
  dispidNktRemoteBridgeUnhookAll,
  dispidNktRemoteBridgeWatchComInterface,
  dispidNktRemoteBridgeUnwatchComInterface,
  dispidNktRemoteBridgeUnwatchAllComInterfaces,
  dispidNktRemoteBridgeGetComInterfaceFromHWnd,
  dispidNktRemoteBridgeGetComInterfaceFromIndex,
  dispidNktRemoteBridgeHookComInterfaceMethod,
  dispidNktRemoteBridgeFindWindow,
  dispidNktRemoteBridgeWindowText,
  dispidNktRemoteBridgeSendMessage,
  dispidNktRemoteBridgeSendMessageTimeout,
  dispidNktRemoteBridgePostMessage,
  dispidNktRemoteBridgeGetChildWindow,
  dispidNktRemoteBridgeGetChildWindowFromId,
  dispidNktRemoteBridgeProcessMemory,
  dispidNktRemoteBridgeCreateProcess,
  dispidNktRemoteBridgeCreateProcessWithLogon,
  dispidNktRemoteBridgeResumeProcess,
  dispidNktRemoteBridgeTerminateProcess,
  dispidNktRemoteBridgeCloseHandle,
  dispidNktRemoteBridgeInvokeComMethod,
  dispidNktRemoteBridgeSetGetComProperty,
  dispidNktRemoteBridgeSetGetComPropertyWithParams,
  //----
  dispidNktRemoteBridgeDefineJavaClass,
  dispidNktRemoteBridgeCreateJavaObject,
  dispidNktRemoteBridgeInvokeJavaStaticMethod,
  dispidNktRemoteBridgeSetGetStaticJavaField,
  dispidNktRemoteBridgeIsJVMAttached,
  //----
  dispidNktRemoteBridgeEventsOnWndProc = 1,
  dispidNktRemoteBridgeEventsOnCreateProcessCall,
  dispidNktRemoteBridgeEventsOnProcessUnhooked,
  dispidNktRemoteBridgeEventsOnComInterfaceCreated,
  dispidNktRemoteBridgeEventsOnJavaCustomNativeCall,
  //----
  dispidNktProcessMemoryGetProtection = 1,
  dispidNktProcessMemoryReadMem,
  dispidNktProcessMemoryWriteMem,
  dispidNktProcessMemoryWrite,
  dispidNktProcessMemoryReadString,
  dispidNktProcessMemoryReadStringN,
  dispidNktProcessMemoryWriteString,
  dispidNktProcessMemoryStringLength,
  dispidNktProcessMemoryCharVal,
  dispidNktProcessMemoryByteVal,
  dispidNktProcessMemoryShortVal,
  dispidNktProcessMemoryUShortVal,
  dispidNktProcessMemoryLongVal,
  dispidNktProcessMemoryULongVal,
  dispidNktProcessMemoryLongLongVal,
  dispidNktProcessMemoryULongLongVal,
  dispidNktProcessMemorySSizeTVal,
  dispidNktProcessMemorySizeTVal,
  dispidNktProcessMemoryFloatVal,
  dispidNktProcessMemoryDoubleVal,
  dispidNktProcessMemoryAllocMem,
  dispidNktProcessMemoryFreeMem,
  //----
  dispidNktJavaObjectInvokeMethod = 1,
  dispidNktJavaObjectInvokeSuperclassMethod,
  dispidNktJavaObjectSetGetField,
  dispidNktJavaObjectSetGetSuperclassField,
  dispidNktJavaObjectIsSameObject,
  dispidNktJavaObjectGetClassName,
  dispidNktJavaObjectIsInstanceOf
} eNktDispIds;
