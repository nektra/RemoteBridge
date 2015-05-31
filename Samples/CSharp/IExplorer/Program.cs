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
using Nektra.RemoteBridge;
using System.Diagnostics;
using System.Management;
using System.Runtime.InteropServices;
using System.Threading;
using Microsoft.Win32;
using mshtml;

namespace CSharpIE
{
    class Program
    {
        static string CLSID_HTMLDocument = "{25336920-03F9-11CF-8FD0-00AA00686F13}";
        static string IID_IUnknown = "{00000000-0000-0000-C000-000000000046}";

        static NktRemoteBridge remoteBridge = null;
        static SortedList<Int32, string> processesList;
        static Int32 iePid = 0;

        static void Main(string[] args)
        {
            object continueEvent;

            //Console.Write("Press any key to continue... ");
            //Console.ReadKey(true);
            //Console.WriteLine("OK");

            processesList = new SortedList<Int32, string>();

            remoteBridge = new NktRemoteBridge();
            if (remoteBridge == null)
            {
                Console.Write("Error: NktRemoteBridge not registered.");
                return;
            }
            remoteBridge.OnCreateProcessCall += new DNktRemoteBridgeEvents_OnCreateProcessCallEventHandler(OnCreateProcessCall);
            remoteBridge.OnComInterfaceCreated += new DNktRemoteBridgeEvents_OnComInterfaceCreatedEventHandler(OnComInterfaceCreated);
            remoteBridge.OnProcessUnhooked += new DNktRemoteBridgeEvents_OnProcessUnhookedEventHandler(OnProcessUnhooked);

            try
            {
                string s;

                s = Registry.GetValue(@"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\IEXPLORE.EXE", "", null) as string;
                if (s == null)
                {
                    Console.WriteLine("failed.");
                    return;
                }
                Console.Write("Launching & hooking IEXPLORE.EXE... ");
                s = "\"" + s + "\" http://www.bbc.co.uk";
                iePid = remoteBridge.CreateProcess(s, true, out continueEvent);
                if (iePid == 0)
                {
                    Console.WriteLine("failed.");
                    return;
                }
                lock (processesList)
                {
                    processesList.Add(iePid, "");
                }

                remoteBridge.Hook(iePid, eNktHookFlags.flgDebugPrintInterfaces);
                remoteBridge.WatchComInterface(iePid, CLSID_HTMLDocument, IID_IUnknown);
                remoteBridge.ResumeProcess(iePid, continueEvent);
                Console.WriteLine("OK");
            }
            catch (Exception ex)
            {
                Console.WriteLine("ERROR");
                Console.WriteLine(ex.ToString());
                return;
            }

            while (true)
            {
                bool b;

                lock (processesList)
                {
                    b = (processesList.Count > 0);
                }
                if (b == false)
                {
                    Console.WriteLine("Hooked process has ended. Quitting.");
                    break;
                }
                if (Console.KeyAvailable == false)
                {
                    System.Threading.Thread.Sleep(10);
                    continue;
                }
                Char ch = Console.ReadKey(true).KeyChar;
                if (ch == 27)
                    break;
            }
            return;
        }

        private static void OnCreateProcessCall(int procId, int childPid, int mainThreadId, bool is64BitProcess)
        {
            if (procId == iePid)
            {
                //only process main IE child processes
                try
                {
                    Console.Write("Child IE process #" + childPid.ToString() + " launched. Hooking... ");
                    lock (processesList)
                    {
                        processesList.Add(childPid, "");
                    }
                    remoteBridge.Hook(childPid, eNktHookFlags.flgDebugPrintInterfaces);
                    remoteBridge.WatchComInterface(childPid, CLSID_HTMLDocument, IID_IUnknown);
                    Console.WriteLine("OK");
                }
                catch (Exception ex)
                {
                    Console.WriteLine("ERROR");
                    Console.WriteLine(ex.ToString());
                    return;
                }
            }
        }

        private static void OnComInterfaceCreated(int procId, int threadId, string clsid, string iid, object newObject)
        {
            System.Diagnostics.Debug.WriteLine("CLSID: " + clsid + " / " + CLSID_HTMLDocument);
            if (clsid.ToUpper() == CLSID_HTMLDocument)
            {
                IHTMLDocument2 doc;

                doc = newObject as IHTMLDocument2;
                Console.WriteLine(procId.ToString() + ") HTMLDocument2 created [NewObj:" + newObject.ToString() + " / Doc:" + doc.ToString() + "]");
                HTMLDocument2Events2Sink docSink = new HTMLDocument2Events2Sink(procId, doc, OnDocCompleted);
            }
        }

        private static void OnDocCompleted(int procId, IHTMLDocument2 doc)
        {
            Console.WriteLine(procId.ToString() + ") OnDocCompleted called [Doc:" + doc.ToString() + "]");

            //on document completion add our custom DIV
            //create sample element
            IHTMLElement elem = doc.createElement("div");
            elem.innerHTML = "Hello from RemoteCOM";
            elem.setAttribute("id", "RemoteCOM_SampleDiv", 0);
            //style it
            IHTMLStyle2 style2 = elem.style as IHTMLStyle2;
            style2.right = 0;
            elem.style.top = 0;
            style2.position = "absolute";
            elem.style.border = "2px solid #FFFF00";
            elem.style.background = "#FFFFC0";
            elem.style.zIndex = 10000;
            elem.style.font = "bold 12px Helvetica";
            elem.style.padding = "5px";
            //insert new element into body
            IHTMLDOMNode bodyNode = doc.body as IHTMLDOMNode;
            IHTMLDOMNode elemNode = elem as IHTMLDOMNode;
            bodyNode.appendChild(elemNode);
            //remember to force release com objects
            Marshal.ReleaseComObject(elem);
            Marshal.ReleaseComObject(style2);
            Marshal.ReleaseComObject(elemNode);
            Marshal.ReleaseComObject(bodyNode);
            return;
        }

        private static void OnProcessUnhooked(int procId)
        {
            bool b;

            try
            {
                lock (processesList)
                {
                    b = processesList.ContainsKey(procId);
                    if (b != false)
                        processesList.Remove(procId);
                }
                if (b != false)
                {
                    Console.WriteLine("Removed process #" + procId.ToString());
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.ToString());
            }
        }
    }
}
