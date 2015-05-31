using System;
using System.Collections.Generic;
using Nektra.RemoteBridge;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace CSharpJavaTest
{
    class ShortTests
    {
        public static bool RunTests(int pid, NktRemoteBridge remoteBridge)
        {
            //short<->short test
            Console.Write("  Short single value test... ");
            try
            {
                short nBaseShort = 10;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaShortTest", "Test", nBaseShort);
                if (res.GetType() != typeof(short))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((short)res != 255 - nBaseShort)
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

            //short<->short array test
            Console.Write("  Short array test... ");
            try
            {
                short[, ,] arrayShort = new short[4, 6, 5];
                object res;
                int i, j, k;

                for (i = 0; i < 4; i++)
                {
                    for (j = 0; j < 6; j++)
                    {
                        for (k = 0; k < 5; k++)
                        {
                            arrayShort[i, j, k] = (short)(i + j + k);
                        }
                    }
                }
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaShortTest", "Test3D", new object[] { arrayShort });
                if (res.GetType() != typeof(short[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                short[, ,] arrayShortRes = (short[, ,])res;
                if (arrayShortRes.GetLength(0) != 4 || arrayShortRes.GetLength(1) != 6 || arrayShortRes.GetLength(2) != 5)
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
                            if ((short)arrayShortRes[i, j, k] != 255 - arrayShort[i, j, k])
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

            //byte<->short test
            Console.Write("  Short single value test using bytes... ");
            try
            {
                byte nBaseByte = 10;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaShortTest", "Test", nBaseByte);
                if (res.GetType() != typeof(short))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((byte)(short)res != 255 - nBaseByte)
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

            //byte<->short array test
            Console.Write("  Short array test using bytes... ");
            try
            {
                byte[, ,] arrayByte = new byte[4, 6, 5];
                object res;
                int i, j, k;

                for (i = 0; i < 4; i++)
                {
                    for (j = 0; j < 6; j++)
                    {
                        for (k = 0; k < 5; k++)
                        {
                            arrayByte[i, j, k] = (byte)(i + j + k);
                        }
                    }
                }
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaShortTest", "Test3D", new object[] { arrayByte });
                if (res.GetType() != typeof(short[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                short[, ,] arrayShortRes = (short[, ,])res;
                if (arrayShortRes.GetLength(0) != 4 || arrayShortRes.GetLength(1) != 6 || arrayShortRes.GetLength(2) != 5)
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
                            if (arrayShortRes[i, j, k] != 255 - (short)arrayByte[i, j, k])
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

            //int<->short test
            Console.Write("  Short single value test using ints... ");
            try
            {
                int nBaseInt = 10;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaShortTest", "Test", nBaseInt);
                if (res.GetType() != typeof(short))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((int)(short)res != 255 - nBaseInt)
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

            //int<->short array test
            Console.Write("  Short array test using ints... ");
            try
            {
                int[, ,] arrayInt = new int[4, 6, 5];
                object res;
                int i, j, k;

                for (i = 0; i < 4; i++)
                {
                    for (j = 0; j < 6; j++)
                    {
                        for (k = 0; k < 5; k++)
                        {
                            arrayInt[i, j, k] = (int)(i + j + k);
                        }
                    }
                }
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaShortTest", "Test3D", new object[] { arrayInt });
                if (res.GetType() != typeof(short[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                short[, ,] arrayShortRes = (short[, ,])res;
                if (arrayShortRes.GetLength(0) != 4 || arrayShortRes.GetLength(1) != 6 || arrayShortRes.GetLength(2) != 5)
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
                            if ((int)arrayShortRes[i, j, k] != 255 - arrayInt[i, j, k])
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

            //long<->short test
            Console.Write("  Short single value test using longs... ");
            try
            {
                long nBaseLong = 10;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaShortTest", "Test", nBaseLong);
                if (res.GetType() != typeof(short))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((long)(short)res != 255 - nBaseLong)
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

            //long<->short array test
            Console.Write("  Short array test using longs... ");
            try
            {
                long[, ,] arrayLong = new long[4, 6, 5];
                object res;
                int i, j, k;

                for (i = 0; i < 4; i++)
                {
                    for (j = 0; j < 6; j++)
                    {
                        for (k = 0; k < 5; k++)
                        {
                            arrayLong[i, j, k] = (long)(i + j + k);
                        }
                    }
                }
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaShortTest", "Test3D", new object[] { arrayLong });
                if (res.GetType() != typeof(short[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                short[, ,]  arrayShortRes = (short[, ,])res;
                if (arrayShortRes.GetLength(0) != 4 || arrayShortRes.GetLength(1) != 6 || arrayShortRes.GetLength(2) != 5)
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
                            if ((long)arrayShortRes[i, j, k] != 255 - arrayLong[i, j, k])
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
