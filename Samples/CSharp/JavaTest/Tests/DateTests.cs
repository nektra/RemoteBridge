using System;
using System.Collections.Generic;
using Nektra.RemoteBridge;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace CSharpJavaTest
{
    class DateTests
    {
        public static bool RunTests(int pid, NktRemoteBridge remoteBridge)
        {
            //date<->date test
            Console.Write("  Date single value test... ");
            try
            {
                DateTime sBaseDate = new DateTime(2013, 10, 13, 16, 00, 00);
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaDateTest", "Test", sBaseDate);
                if (res.GetType() != typeof(DateTime))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((DateTime)res != sBaseDate.AddYears(1))
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

            //date<->date array test
            Console.Write("  Date array test... ");
            try
            {
                DateTime[, ,] arrayDate = new DateTime[4, 6, 5];
                object res;
                int i, j, k;

                for (i = 0; i < 4; i++)
                {
                    for (j = 0; j < 6; j++)
                    {
                        for (k = 0; k < 5; k++)
                        {
                            arrayDate[i, j, k] = new DateTime(2013, 10, 13, i, j, k);
                        }
                    }
                }
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaDateTest", "Test3D", new object[] { arrayDate });
                if (res.GetType() != typeof(DateTime[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                DateTime[, ,] arrayDateRes = (DateTime[, ,])res;
                if (arrayDateRes.GetLength(0) != 4 || arrayDateRes.GetLength(1) != 6 || arrayDateRes.GetLength(2) != 5)
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
                            if (arrayDateRes[i, j, k] != arrayDate[i, j, k].AddYears(1))
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
