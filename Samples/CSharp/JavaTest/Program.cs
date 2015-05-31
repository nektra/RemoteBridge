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

namespace CSharpJavaTest
{
    class Program
    {
        [DllImport("Shlwapi.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        static extern uint AssocQueryString(uint flags, uint str, string pszAssoc, string pszExtra, [Out] StringBuilder pszOut, [In][Out] ref uint pcchOut);

        [DllImport("shell32.dll")]
        static extern IntPtr FindExecutable(string lpFile, string lpDirectory, [Out] StringBuilder lpResult);

        static NktRemoteBridge remoteBridge = null;
        static Process procJavaTest = null;

        static void Main(string[] args)
        {
            object continueEv;
            INktJavaObject topLevelFrame;
            int verMajor, verMinor;

            Console.WriteLine("This example performs some tests on RemoteBridge's Java functionality.");

            remoteBridge = new NktRemoteBridge();
            if (remoteBridge == null)
            {
                Console.Write("Error: NktRemoteBridge not registered.");
                QuitJavaApp(true);
                return;
            }

            try
            {
                String s;
                int pid;

                Console.Write("Launching JAVATEST.JAR... ");
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
                s += "\\..\\Samples\\Demos4Test\\Java\\Test\\JavaTest.jar\"";
                //create process
                pid = remoteBridge.CreateProcess(s, (DoDelayedHook(args) != false) ? false : true, out continueEv);
                if (pid == 0)
                {
                    Console.WriteLine("failed.");
                    return;
                }
                procJavaTest = Process.GetProcessById(pid);
                Console.WriteLine("OK");
            }
            catch (Exception ex)
            {
                Console.WriteLine("failed.");
                Console.WriteLine(ex.ToString());
                QuitJavaApp(true);
                return;
            }

            //---------------- INJECT

            try
            {
                Console.Write("Injecting... ");
                remoteBridge.Hook(procJavaTest.Id, eNktHookFlags.flgDebugPrintInterfaces);
                if (continueEv != null)
                    remoteBridge.ResumeProcess(procJavaTest.Id, continueEv);
                Console.WriteLine("OK");
            }
            catch (Exception ex)
            {
                Console.WriteLine("failed.");
                Console.WriteLine(ex.ToString());
                QuitJavaApp(true);
                return;
            }

            //---------------- WAIT UNTIL JVM INITIALIZATION

            Console.Write("Waiting for JVM initialization... ");
            try
            {
                while (remoteBridge.IsJVMAttached(procJavaTest.Id, out verMajor, out verMinor) == false)
                {
                    if (procJavaTest.HasExited != false)
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
                            //close java test if still open
                            QuitJavaApp(false);
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
                QuitJavaApp(true);
                return;
            }

            //---------------- FIND TOPLEVEL WINDOW

            Console.Write("Obtaining main Window... ");
            topLevelFrame = null;
            while (topLevelFrame == null)
            {
                object frames;

                if (procJavaTest.HasExited != false)
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
                        //close java test if still open
                        QuitJavaApp(false);
                        return;
                    }
                }
                //get top level frame
                try
                {
                    frames = remoteBridge.InvokeJavaStaticMethod(procJavaTest.Id, "javax.swing.JFrame", "getFrames", null);
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
                    System.Threading.Thread.Sleep(10);
            }
            Console.WriteLine("OK");

            //---------------- RUN TESTS

            Console.Write("Running tests...");
            for (int testNo=1; testNo<=12; testNo++)
            {
                if (procJavaTest.HasExited != false)
                {
                    Console.WriteLine("Hooked process has ended. Quitting.");
                    return;
                }
                if (Console.KeyAvailable != false)
                {
                    Char ch = Console.ReadKey(true).KeyChar;
                    if (ch == 27)
                    {
                        Console.WriteLine("Exiting by user request.");
                        //close java test if still open
                        QuitJavaApp(false);
                        return;
                    }
                }
                //run test
                bool b = false;
                switch (testNo)
                {
                    case 1:
                        b = BooleanTests.RunTests(procJavaTest.Id, remoteBridge);
                        break;
                    case 2:
                        b = ByteTests.RunTests(procJavaTest.Id, remoteBridge);
                        break;
                    case 3:
                        b = ShortTests.RunTests(procJavaTest.Id, remoteBridge);
                        break;
                    case 4:
                        b = CharTests.RunTests(procJavaTest.Id, remoteBridge);
                        break;
                    case 5:
                        b = IntTests.RunTests(procJavaTest.Id, remoteBridge);
                        break;
                    case 6:
                        b = LongTests.RunTests(procJavaTest.Id, remoteBridge);
                        break;
                    case 7:
                        b = FloatTests.RunTests(procJavaTest.Id, remoteBridge);
                        break;
                    case 8:
                        b = DoubleTests.RunTests(procJavaTest.Id, remoteBridge);
                        break;
                    case 9:
                        b = DateTests.RunTests(procJavaTest.Id, remoteBridge);
                        break;
                    case 10:
                        b = BigDecimalTests.RunTests(procJavaTest.Id, remoteBridge);
                        break;
                    case 11:
                        b = StringTests.RunTests(procJavaTest.Id, remoteBridge);
                        break;
                    case 12:
                        remoteBridge.OnJavaCustomNativeCall += Test_OnJavaCustomNativeCall;
                        b = EventCallbackTests.RunTests(procJavaTest.Id, remoteBridge);
                        remoteBridge.OnJavaCustomNativeCall -= Test_OnJavaCustomNativeCall;
                        break;
                }
                if (b == false)
                {
                    QuitJavaApp(true);
                    return;
                }
            }

            //---------------- CLOSE JAVA APP IF STILL RUNNING

            QuitJavaApp(true);
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
            if (procJavaTest != null && procJavaTest.HasExited == false)
            {
                try
                {
                    //this call will throw an exception because the process being disconnected
                    remoteBridge.InvokeJavaStaticMethod(procJavaTest.Id, "java.lang.System", "exit", 0);
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

        static private void Test_OnJavaCustomNativeCall(int procId, string className, string methodName, NktJavaObject objectOrClass, ref object[] parameters, out object retValue)
        {
            retValue = null;
            if (className == "InjectTestWithCallbacks")
            {
                if (methodName == "noParametersReturningInt")
                {
                    if (parameters.Length == 0)
                    {
                        retValue = 10;
                    }
                }
                else if (methodName == "noParametersReturningJavaLangDoubleObject")
                {
                    if (parameters.Length == 0)
                    {
                        try
                        {
                            retValue = remoteBridge.CreateJavaObject(procJavaTest.Id, "java/lang/Double", 10.0);
                        }
                        catch (System.Exception /*ex*/)
                        {
                            retValue = null;
                        }
                    }
                }
                else if (methodName == "withParametersReturningInt")
                {
                    int res;

                    if (parameters.Length == 5)
                    {
                        try
                        {
                            if ((string)parameters[0] == "Test_WithParametersReturningInt")
                            {
                                NktJavaObject javaObj = parameters[1] as NktJavaObject;
                                res = (int)javaObj.get_Field("value"); //accessing private field

                                int[,] intArrayParam = parameters[2] as int[,];
                                for (int i=0; i<2; i++)
                                {
                                    for (int j=0; j<3; j++)
                                        res += intArrayParam[i,j];
                                }
                                res += (int)(double)parameters[3];
                                float[] flt = parameters[4] as float[];
                                for (int i=0; i<2; i++)
                                    res += (int)flt[i];
                                retValue = res;
                            }
                        }
                        catch (System.Exception /*ex*/)
                        {
                            retValue = null;
                        }
                    }
                }
                else if (methodName == "withParametersReturningJavaLangDoubleObject")
                {
                    double res;

                    if (parameters.Length == 5)
                    {
                        try
                        {
                            if ((string)parameters[0] == "Test_WithParametersReturningJavaLangDoubleObject")
                            {
                                NktJavaObject javaObj = parameters[1] as NktJavaObject;
                                res = (double)(int)javaObj.get_Field("value"); //accessing private field

                                int[,] intArrayParam = parameters[2] as int[,];
                                for (int i = 0; i < 2; i++)
                                {
                                    for (int j = 0; j < 3; j++)
                                        res += (double)intArrayParam[i, j];
                                }
                                res += (double)parameters[3];
                                float[] flt = parameters[4] as float[];
                                for (int i = 0; i < 2; i++)
                                    res += (double)flt[i];
                                retValue = remoteBridge.CreateJavaObject(procJavaTest.Id, "java/lang/Double", res);
                            }
                        }
                        catch (System.Exception /*ex*/)
                        {
                            retValue = null;
                        }
                    }
                }
            }
            return;
        }
    }
}
