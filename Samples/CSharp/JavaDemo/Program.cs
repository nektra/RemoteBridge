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
using System.Text;
using Microsoft.Win32;

namespace CSharpJavaDemo
{
    class Program
    {
        [DllImport("Shlwapi.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        static extern uint AssocQueryString(uint flags, uint str, string pszAssoc, string pszExtra, [Out] StringBuilder pszOut, [In][Out] ref uint pcchOut);

        [DllImport("shell32.dll")]
        static extern IntPtr FindExecutable(string lpFile, string lpDirectory, [Out] StringBuilder lpResult);

        static NktRemoteBridge remoteBridge;
        static Process procNotepad;

        static void Main(string[] args)
        {
            object continueEv;
            INktJavaObject topLevelFrame;
            int verMajor, verMinor;

            Console.WriteLine("This example launches the Java Development Kit Notepad demo application and...");
            Console.WriteLine("   a) Changes the main frame caption.");
            Console.WriteLine("   b) Creates a JTextArea with a text and inserts it at the bottom.");
            Console.WriteLine("   c) Sends System.exit() on exit to gracefully close Notepad if still running.");

            remoteBridge = new NktRemoteBridge();
            if (remoteBridge == null)
            {
                Console.Write("Error: NktRemoteBridge not registered.");
                return;
            }

            try
            {
                String s;
                int pid;

                Console.Write("Launching NOTEPAD.JAR... ");
                s = FindJavaLauncher(args);
                if (s == "")
                {
                    Console.WriteLine("Cannot locate a registered jarfile launcher.");
                    return;
                }
                //create command line
                s = "\"" + s + "\" ";
                if (DoVerboseJNI(args) != false)
                    s += "-Xcheck:jni -verbose:jni ";
                s += "-jar \"" + System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
                s += "\\..\\Samples\\Demos4Test\\Java\\Notepad\\notepad.jar\"";
                //create process
                pid = remoteBridge.CreateProcess(s, (DoDelayedHook(args) != false) ? false : true, out continueEv);
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

            //---------------- INJECT

            try
            {
                Console.Write("Injecting... ");
                remoteBridge.Hook(procNotepad.Id, eNktHookFlags.flgDebugPrintInterfaces);
                if (continueEv != null)
                    remoteBridge.ResumeProcess(procNotepad.Id, continueEv);
                Console.WriteLine("OK");
            }
            catch (Exception ex)
            {
                Console.WriteLine("failed.");
                Console.WriteLine(ex.ToString());
                return;
            }

            //---------------- WAIT UNTIL JVM INITIALIZATION

            Console.Write("Waiting for JVM initialization... ");
            try
            {
                while (remoteBridge.IsJVMAttached(procNotepad.Id, out verMajor, out verMinor) == false)
                {
                    if (procNotepad.HasExited != false)
                    {
                        Console.WriteLine("");
                        Console.WriteLine("Hooked process has ended. Quitting.");
                        return;
                    }
                    if (Console.KeyAvailable != false)
                    {
                        Char ch = Console.ReadKey(true).KeyChar;
                        if (ch == 27)
                        {
                            Console.WriteLine("");
                            Console.WriteLine("Exiting by user request.");
                            //close java notepad if still open
                            QuitJavaApp(true);
                            return;
                        }
                    }
                    else
                    {
                        System.Threading.Thread.Sleep(10);
                    }
                }
                Console.WriteLine("OK");
            }
            catch (Exception ex)
            {
                Console.WriteLine("failed.");
                Console.WriteLine(ex.ToString());
                //close java notepad if still open
                QuitJavaApp(true);
                return;
            }

            //---------------- PERFORM TASKS

            Console.Write("Obtaining main JFrame... ");
            topLevelFrame = null;
            while (topLevelFrame == null)
            {
                object frames;

                if (procNotepad.HasExited != false)
                {
                    Console.WriteLine("");
                    Console.WriteLine("Hooked process has ended. Quitting.");
                    return;
                }
                if (Console.KeyAvailable != false)
                {
                    Char ch = Console.ReadKey(true).KeyChar;
                    if (ch == 27)
                    {
                        Console.WriteLine("");
                        Console.WriteLine("Exiting by user request.");
                        //close java notepad if still open
                        QuitJavaApp(true);
                        return;
                    }
                }
                //get top level frame
                try
                {
                    frames = remoteBridge.InvokeJavaStaticMethod(procNotepad.Id, "javax.swing.JFrame", "getFrames", null);
                }
                catch (Exception ex)
                {
                    Console.WriteLine("failed.");
                    Console.WriteLine(ex.ToString());
                    QuitJavaApp(true);
                    return;
                }
                if (frames is Array)
                {
                    if (((object[])frames).Length > 0)
                        topLevelFrame = ((object[])frames)[0] as INktJavaObject;
                }
                if (topLevelFrame == null)
                    System.Threading.Thread.Sleep(100);
            }
            Console.WriteLine("OK");

            Console.Write("Waiting until visible... ");
            while ((bool)(topLevelFrame.InvokeMethod("isVisible", null)) == false)
            {
                System.Threading.Thread.Sleep(10);
            }
            Console.WriteLine("OK");

            //set new caption
            topLevelFrame.InvokeMethod("setTitle", "Nektra - Notepad");

            //get main frame content pane
            INktJavaObject cntPane = topLevelFrame.InvokeMethod("getContentPane", null) as INktJavaObject;

            //add a new JTextArea at the bottom
            INktJavaObject newTxArea = remoteBridge.CreateJavaObject(procNotepad.Id, "javax.swing.JTextArea", "Nektra Demo");
            cntPane.InvokeMethod("add", new object[] { "South", newTxArea });

            //resize window in order to force a re-layout
            topLevelFrame.InvokeMethod("setSize", new object[] { 500, 601 });

            //free resources
            Marshal.ReleaseComObject(cntPane);
            Marshal.ReleaseComObject(newTxArea);
            Marshal.ReleaseComObject(topLevelFrame);

            //---------------- WAIT FOR EXIT

            Console.WriteLine("Press 'ESC' key to quit");
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
            }

            //---------------- CLOSE JAVA APP IF STILL RUNNING

            QuitJavaApp(false);
        }

        static private string FindJavaLauncher(string[] args)
        {
            StringBuilder sb;
            uint pcchOut;
            string s;
            RegistryKey key;
            int i;

            for (i = 0; i < args.Length; i++)
            {
                s = args[i].Substring(0, 5).ToLower();
                if (s == "-jre:" || s == "/jre:")
                    return args[i].Substring(5);
            }
            //try to find 32/64 bit javaw.exe by gathering JavaWebInstaller
            key = Registry.ClassesRoot.OpenSubKey("TypeLib\\{5852F5E0-8BF4-11D4-A245-0080C6F74284}\\1.0\\0\\win" + (IntPtr.Size * 8).ToString());
            if (key != null)
            {
                s = (string)key.GetValue("");
                key.Close();
                if (s != "")
                {
                    s = s.Replace("\"", "");
                    int pos = s.LastIndexOf("\\") + 1;
                    s = s.Substring(0, pos) + "javaw.exe";
                    if (System.IO.File.Exists(s) != false)
                        return s;
                }
            }
            //if the above fails, find jarfile association
            pcchOut = 0;
            AssocQueryString(0x40/*Verify*/, 2/*Executable*/, "jarfile", null, null, ref pcchOut);
            if (pcchOut > 0)
            {
                sb = new StringBuilder((int)pcchOut);
                AssocQueryString(0x40/*Verify*/, 2/*Executable*/, "jarfile", null, sb, ref pcchOut);
                s = sb.ToString();
                if (System.IO.File.Exists(s) != false)
                    return s;
            }
            return "";
        }

        static private bool DoVerboseJNI(string[] args)
        {
            string s;
            int i;

            for (i = 0; i < args.Length; i++)
            {
                s = args[i].ToLower();
                if (s == "-verbose" || s == "/verbose")
                    return true;
            }
            return false;
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

        static private void QuitJavaApp(bool waitKey)
        {
            if (procNotepad != null && procNotepad.HasExited == false)
            {
                try
                {
                    //this call will throw an exception because the process being disconnected
                    remoteBridge.InvokeJavaStaticMethod(procNotepad.Id, "java.lang.System", "exit", 0);
                }
                catch (System.Exception)
                { }
            }
            //----
            if (waitKey != false)
            {
                Console.Write("Press any key to quit... ");
                while (Console.KeyAvailable == false)
                    System.Threading.Thread.Sleep(10);
                Char ch = Console.ReadKey(true).KeyChar;
            }
            return;
        }
    }
}
