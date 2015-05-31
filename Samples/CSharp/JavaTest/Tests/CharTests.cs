using System;
using System.Collections.Generic;
using Nektra.RemoteBridge;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace CSharpJavaTest
{
    class CharTests
    {
        public static bool RunTests(int pid, NktRemoteBridge remoteBridge)
        {
            //short<->char test
            Console.Write("  Char single value test using shorts... ");
            try
            {
                char nBaseChar = (char)10;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaCharTest", "Test", nBaseChar);
                if (res.GetType() != typeof(short))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((short)res != 255 - (short)nBaseChar)
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

            //short<->char array test
            Console.Write("  Char array test using shorts... ");
            try
            {
                char[, ,] arrayChar = new char[4, 6, 5];
                object res;
                int i, j, k;

                for (i = 0; i < 4; i++)
                {
                    for (j = 0; j < 6; j++)
                    {
                        for (k = 0; k < 5; k++)
                        {
                            arrayChar[i, j, k] = (char)(i + j + k);
                        }
                    }
                }
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaCharTest", "Test3D", new object[] { arrayChar });
                if (res.GetType() != typeof(short[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                short[, ,] arrayCharRes = (short[, ,])res;
                if (arrayCharRes.GetLength(0) != 4 || arrayCharRes.GetLength(1) != 6 || arrayCharRes.GetLength(2) != 5)
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
                            if (arrayCharRes[i, j, k] != 255 - (short)arrayChar[i, j, k])
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
