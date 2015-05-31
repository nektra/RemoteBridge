using System;
using System.Collections.Generic;
using Nektra.RemoteBridge;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace CSharpJavaTest
{
    class BooleanTests
    {
        public static bool RunTests(int pid, NktRemoteBridge remoteBridge)
        {
            //bool<->bool test
            Console.Write("  Boolean single value test... ");
            try
            {
                bool bBaseBool = true;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaBooleanTest", "Test", bBaseBool);
                if (res.GetType() != typeof(bool))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((bool)res != (!bBaseBool))
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

            //bool<->bool array test
            Console.Write("  Boolean array test... ");
            try
            {
                bool[, ,] arrayBool = new bool[4, 6, 5];
                object res;
                int i, j, k;

                for (i = 0; i < 4; i++)
                {
                    for (j = 0; j < 6; j++)
                    {
                        for (k = 0; k < 5; k++)
                        {
                            arrayBool[i, j, k] = (((i+j+k) & 1) != 0) ? true : false;
                        }
                    }
                }
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaBooleanTest", "Test3D", new object[] { arrayBool });
                if (res.GetType() != typeof(bool[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                bool[, ,] arrayBoolRes = (bool[, ,])res;
                if (arrayBoolRes.GetLength(0) != 4 || arrayBoolRes.GetLength(1) != 6 || arrayBoolRes.GetLength(2) != 5)
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
                            if (arrayBoolRes[i, j, k] != (!arrayBool[i, j, k]))
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
