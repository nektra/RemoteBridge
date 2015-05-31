using System;
using System.Collections.Generic;
using Nektra.RemoteBridge;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace CSharpJavaTest
{
    class StringTests
    {
        public static bool RunTests(int pid, NktRemoteBridge remoteBridge)
        {
            //bool<->bool test
            Console.Write("  String single value test... ");
            try
            {
                string szBaseString = "this is a test string";
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaStringTest", "Test", szBaseString);
                if (res.GetType() != typeof(string))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((string)res != "[" + szBaseString + "]")
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
            Console.WriteLine("  OK");

            //string<->string array test
            Console.Write("  String array test... ");
            try
            {
                string szBaseString = "this is a test string";
                string[, ,] arrayStr = new string[4, 6, 5];
                object res;
                int i, j, k;

                for (i = 0; i < 4; i++)
                {
                    for (j = 0; j < 6; j++)
                    {
                        for (k = 0; k < 5; k++)
                        {
                            arrayStr[i, j, k] = szBaseString + " " + i.ToString() + "/" + j.ToString() + "/" + k.ToString();
                        }
                    }
                }
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaStringTest", "Test3D", new object[] { arrayStr });
                if (res.GetType() != typeof(string[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                string[, ,] arrayStrRes = (string[, ,])res;
                if (arrayStrRes.GetLength(0) != 4 || arrayStrRes.GetLength(1) != 6 || arrayStrRes.GetLength(2) != 5)
                {
                    Console.WriteLine("Error: Returned array size mismatch");
                    return false;
                }
                for (i = 0; i < 4; i++)
                {
                    for (j = 0; j < 6; j++)
                    {
                        for (k = 0; k < 5; k++)
                        {
                            if (arrayStrRes[i, j, k] != "[" + arrayStr[i, j, k] + "]")
                            {
                                Console.WriteLine("Error: Returned data mismatch (" + i.ToString() + "," + j.ToString() + "," + k.ToString() + ")");
                                return false;
                            }
                        }
                    }
                }
            }
            catch (System.Exception ex)
            {
                Console.WriteLine("failed.");
                Console.WriteLine(ex.ToString());
                return false;
            }
            Console.WriteLine("  OK");
            return true;
        }
    }
}
