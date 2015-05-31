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

#ifndef _NKT_MALLOCMACROS_H
#define _NKT_MALLOCMACROS_H

//MallocMacros usage:
//~~~~~~~~~~~~~~~~~~
//  Add the following macro to the C/C++/Project macro list: NKT_MALLOCMACFROS_FILE=mymalloc.h
//  Where mymalloc.h is the name of the header file that will define NKT_MALLOC macros.
//  Do NOT enclose the header file inside quotes because Visual Studio will complain about that.
//  The stringizing operator below will do the job.

//-----------------------------------------------------------

#ifdef NKT_MALLOCMACFROS_FILE
  #define NKT_MALLOCMACFROS_STRINGIFY(x) #x
  #define NKT_MALLOCMACFROS_TOSTRING(x) NKT_MALLOCMACFROS_STRINGIFY(x)
  #include NKT_MALLOCMACFROS_TOSTRING( NKT_MALLOCMACFROS_FILE )
#endif //NKT_MALLOCMACFROS_FILE

#if (defined(NKT_MALLOC) || defined(NKT_FREE) || defined(NKT_REALLOC) || defined(NKT__MSIZE))
  #if ((!defined(NKT_MALLOC)) || (!defined(NKT_FREE)) || (!defined(NKT_REALLOC)) || (!defined(NKT__MSIZE)))
    #error You MUST define NKT_MALLOC, NKT_FREE, NKT_REALLOC and NKT__MSIZE
  #endif
#else //NKT_MALLOC || NKT_FREE || NKT_REALLOC || NKT__MSIZE
  #include <malloc.h>
  #define NKT_MALLOC(nSize) malloc((SIZE_T)(nSize))
  #define NKT_FREE(lpPtr) if ((LPVOID)(lpPtr) != NULL) free((LPVOID)(lpPtr))
  #define NKT_REALLOC(lpPtr,nNewSize) realloc((LPVOID)(lpPtr), (SIZE_T)(nNewSize))
  #define NKT__MSIZE(lpPtr) _msize((LPVOID)(lpPtr))
#endif //NKT_MALLOC || NKT_FREE || NKT_REALLOC || NKT__MSIZE

//-----------------------------------------------------------

#endif //_NKT_MALLOCMACROS_H
