using System;
using System.Collections.Generic;
using Nektra.RemoteBridge;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace CSharpJavaTest
{
    class EventCallbackTests
    {
        public static bool RunTests(int pid, NktRemoteBridge remoteBridge)
        {
            NktJavaObject testObj;

            Console.Write("  EventCallbackTests[DefineClass] test... ");
            try
            {
                byte[] byteCode;
                string s;

                s = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
                s += "\\..\\Samples\\Demos4Test\\Java\\Test\\InjectTestWithCallbacks.class";
                byteCode = System.IO.File.ReadAllBytes(s);
                remoteBridge.DefineJavaClass(pid, "InjectTestWithCallbacks", byteCode);
            }
            catch (System.Exception ex)
            {
                Console.WriteLine("failed.");
                Console.WriteLine(ex.ToString());
                return false;
            }
            Console.WriteLine("OK");

            Console.Write("  EventCallbackTests[Instatiate] test... ");
            try
            {
                testObj = remoteBridge.CreateJavaObject(pid, "InjectTestWithCallbacks", null);
                if (testObj  == null)
                {
                    Console.WriteLine("Error: Cannot create object");
                    return false;
                }
            }
            catch (System.Exception ex)
            {
                Console.WriteLine("failed.");
                Console.WriteLine(ex.ToString());
                return false;
            }
            Console.WriteLine("OK");

            Console.Write("  EventCallbackTests[NoParametersReturningInt] test... ");
            try
            {
                object res;

                res = testObj.InvokeMethod("Test_NoParametersReturningInt", null);
                if (res.GetType() != typeof(int))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((int)res != 10)
                {
                    Console.WriteLine("Error: Returned data mismatch");
                    return false;
                }
            }
            catch (System.Exception ex)
            {
                Console.WriteLine("failed.");
                Console.WriteLine(ex.ToString());
                return false;
            }
            Console.WriteLine("OK");

            Console.Write("  EventCallbackTests[NoParametersReturningJavaLangDoubleObject] test... ");
            try
            {
                object res;
                NktJavaObject javaObj;

                res = testObj.InvokeMethod("Test_NoParametersReturningJavaLangDoubleObject", null);
                javaObj = res as NktJavaObject;
                if (javaObj == null)
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                res = javaObj.get_Field("value"); //accessing private field
                if (res.GetType() != typeof(double))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((double)res != 10.0)
                {
                    Console.WriteLine("Error: Returned data mismatch");
                    return false;
                }
            }
            catch (System.Exception ex)
            {
                Console.WriteLine("failed.");
                Console.WriteLine(ex.ToString());
                return false;
            }
            Console.WriteLine("OK");

            Console.Write("  EventCallbackTests[WithParametersReturningInt] test... ");
            try
            {
                object res;

                res = testObj.InvokeMethod("Test_WithParametersReturningInt", null);
                if (res.GetType() != typeof(int))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((int)res != 124)
                {
                    Console.WriteLine("Error: Returned data mismatch");
                    return false;
                }
            }
            catch (System.Exception ex)
            {
                Console.WriteLine("failed.");
                Console.WriteLine(ex.ToString());
                return false;
            }
            Console.WriteLine("OK");

            Console.Write("  EventCallbackTests[WithParametersReturningJavaLangDoubleObject] test... ");
            try
            {

                object res;
                NktJavaObject javaObj;

                res = testObj.InvokeMethod("Test_WithParametersReturningJavaLangDoubleObject", null);
                javaObj = res as NktJavaObject;
                if (javaObj == null)
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                res = javaObj.get_Field("value"); //accessing private field
                if (res.GetType() != typeof(double))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((double)res != 124.0)
                {
                    Console.WriteLine("Error: Returned data mismatch");
                    return false;
                }
            }
            catch (System.Exception ex)
            {
                Console.WriteLine("failed.");
                Console.WriteLine(ex.ToString());
                return false;
            }
            Console.WriteLine("OK");
            return true;
        }
    }
}
