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
using System.Runtime.InteropServices;
using Microsoft.WindowsAPICodePack.Dialogs;
using Microsoft.WindowsAPICodePack.Shell;

namespace CSharpDemo
{
    class Program
    {
        //static string IID_IUnknown = "{00000000-0000-0000-C000-000000000046}";

        static void Main(string[] args)
        {
            NktRemoteBridge remoteBridge;
            object continueEv;
            Process procNotepad;

            if (Environment.OSVersion.Version.Major < 6)
            {
                Console.Write("Error: This application requires Windows Vista or later to work.");
                return;
            }

            remoteBridge = new NktRemoteBridge();
            if (remoteBridge == null)
            {
                Console.Write("Error: NktRemoteBridge not registered.");
                return;
            }

            try
            {
                int pid;

                Console.Write("Launching NOTEPAD.EXE... ");
                pid = remoteBridge.CreateProcess("notepad.exe", (DoDelayedHook(args) != false) ? false : true, out continueEv);
                if (pid == 0)
                {
                    Console.WriteLine("failed.");
                    return;
                }
                procNotepad = Process.GetProcessById(pid);
                Console.WriteLine("OK");
            }
            catch (Exception ex)
            {
                Console.WriteLine("failed.");
                Console.WriteLine(ex.ToString());
                return;
            }

            //----------------

            try
            {
                Console.Write("Injecting... ");
                remoteBridge.Hook(procNotepad.Id, eNktHookFlags.flgDebugPrintInterfaces);
                if (continueEv != null)
                    remoteBridge.ResumeProcess(procNotepad.Id, continueEv);
                remoteBridge.WatchComInterface(procNotepad.Id, ShellCLSIDGuid.FileOpenDialog, ShellIIDGuid.IFileDialog);
                Console.WriteLine("OK");
            }
            catch (Exception ex)
            {
                Console.WriteLine("failed.");
                Console.WriteLine(ex.ToString());
                return;
            }

            //----------------

            Console.WriteLine("Ready.");
            Console.WriteLine("Usage:");
            Console.WriteLine("  This demo launches a Windows' Notepad application and scans 'File Open' dialog boxes.");
            Console.WriteLine("  When a file open dialog box is created, you can take the following actions:");
            Console.WriteLine("    1) Press the 'F' key to retrieve the typed file name in the dialog box.");
            Console.WriteLine("    2) Press the 'C' key to close the dialog box window using the OK button.");
            Console.WriteLine("    3) Press 'ESC' key to quit this demo!");
            while (true)
            {
                if (procNotepad.HasExited != false)
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
                switch (ch)
                {
                    case 'f':
                    case 'F':
                        DoGetOpenFileDialogFilename(remoteBridge, procNotepad);
                        break;

                    case 'c':
                    case 'C':
                        DoCloseOpenFileDialog(remoteBridge, procNotepad);
                        break;
                }
            }
            return;
        }

        static private bool DoDelayedHook(string[] args)
        {
            string s;
            int i;

            for (i = 0; i < args.Length; i++)
            {
                s = args[i].ToLower();
                if (s == "-delayedhook" || s == "/delayedhook")
                    return true;
            }
            return false;
        }

        private static void DoGetOpenFileDialogFilename(NktRemoteBridge remoteBridge, Process proc)
        {
            IFileOpenDialog dlg = null;
            object obj = null;
            IntPtr hWnd;
            string s;

            try
            {
                hWnd = remoteBridge.FindWindow(proc.Id, IntPtr.Zero, "Open", "#32770", false);
                if (hWnd == IntPtr.Zero)
                    hWnd = remoteBridge.FindWindow(proc.Id, IntPtr.Zero, "Abrir", "#32770", false);
                if (hWnd == IntPtr.Zero)
                {
                    Console.WriteLine("Cannot find OpenFileDialog window");
                    return;
                }
                obj = remoteBridge.GetComInterfaceFromHwnd(proc.Id, hWnd, ShellIIDGuid.IFileDialog);
                if (obj == null)
                {
                    Console.WriteLine("Cannot find retrieve IFileOpenDialog interface");
                    return;
                }
                dlg = obj as IFileOpenDialog;
                dlg.GetFileName(out s);
                Console.WriteLine("Filename: [" + s + "]");
            }
            catch (System.Exception ex)
            {
                Console.WriteLine(ex.ToString());
            }
            finally
            {
                //if (dlg != null)
                //    Marshal.ReleaseComObject(dlg);
                if (obj != null)
                    Marshal.ReleaseComObject(obj);
            }
            return;
        }

        private static void DoCloseOpenFileDialog(NktRemoteBridge remoteBridge, Process proc)
        {
            IntPtr hWnd;

            try
            {
                hWnd = remoteBridge.FindWindow(proc.Id, IntPtr.Zero, "Open", "#32770", false);
                if (hWnd == IntPtr.Zero)
                    hWnd = remoteBridge.FindWindow(proc.Id, IntPtr.Zero, "Abrir", "#32770", false);
                if (hWnd == IntPtr.Zero)
                {
                    Console.WriteLine("Cannot find OpenFileDialog window");
                    return;
                }
                //we can try sending a WM_COMMAND with IDOK but we will try to find the ok button and send
                //fake lbuttondown/up to test more methods instead.
                hWnd = remoteBridge.GetChildWindowFromId(proc.Id, hWnd, 1);
                if (hWnd == IntPtr.Zero)
                {
                    Console.WriteLine("Cannot find OpenFileDialog's OK button");
                    return;
                }
                remoteBridge.PostMessage(proc.Id, hWnd, 0x0201, IntPtr.Zero, IntPtr.Zero); //WM_LBUTTONDOWN
                remoteBridge.PostMessage(proc.Id, hWnd, 0x0202, IntPtr.Zero, IntPtr.Zero); //WM_LBUTTONUP
            }
            catch (System.Exception ex)
            {
                Console.WriteLine(ex.ToString());
            }
            return;
        }
    }
}
