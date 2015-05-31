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

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
using mshtml;

namespace CSharpIE
{
    class HTMLDocument2Events2Sink : IDisposable
    {
        public delegate void DocumentCompleteEventHandler(int procId, IHTMLDocument2 doc);

        private int procId;
        private IHTMLDocument2 doc;
        private DocumentCompleteEventHandler docCompletedHandler;

        private int cookie = -1;
        private IConnectionPoint connPoint;
        private IConnectionPointContainer connPointContainer;
        private static Guid IID_HTMLDocumentEvents2 = typeof(HTMLDocumentEvents2).GUID;

        private Handler handler;

        //------------------------------------------------------------

        public HTMLDocument2Events2Sink(int procId, IHTMLDocument2 doc, DocumentCompleteEventHandler h)
        {
            this.procId = procId;
            this.doc = doc;
            docCompletedHandler = h;
            handler = new Handler(this);
            connPointContainer = (IConnectionPointContainer)doc;
            connPointContainer.FindConnectionPoint(ref IID_HTMLDocumentEvents2, out connPoint);
            connPoint.Advise(handler, out cookie);
        }

        ~HTMLDocument2Events2Sink()
        {

        }

        public void Dispose()
        {
            if (cookie != -1)
            {
                connPoint.Unadvise(cookie);
            }
            cookie = -1;
            handler = null;
        }

        public void OnDocReadyStateChanged()
        {
            if (doc.readyState == "complete")
            {
                docCompletedHandler(procId, doc);
            }
        }
    }

    [ComVisible(true), ComImport()]
    [TypeLibType((short)4160)]
    [InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIDispatch)]
    [Guid("3050f613-98b5-11cf-bb82-00aa00bdce0b")]
    internal interface HTMLDocumentEvents2
    {
        [DispId(-609)]
        void onreadystatechange([In, MarshalAs(UnmanagedType.Interface)] IHTMLEventObj pEvtObj);
    }

    internal class Handler : HTMLDocumentEvents2
    {
        private HTMLDocument2Events2Sink parent;

        public Handler(HTMLDocument2Events2Sink parent)
        {
            this.parent = parent;
        }

        public void onreadystatechange(IHTMLEventObj pEvtObj)
        {
            parent.OnDocReadyStateChanged();
        }
    }
}
