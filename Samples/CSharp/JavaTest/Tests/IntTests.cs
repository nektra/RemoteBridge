using System;
using System.Collections.Generic;
using Nektra.RemoteBridge;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace CSharpJavaTest
{
    class IntTests
    {
        public static bool RunTests(int pid, NktRemoteBridge remoteBridge)
        {
            //int<->int test
            Console.Write("  Integer single value test... ");
            try
            {
                int nBaseInt = 10;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaIntegerTest", "Test", nBaseInt);
                if (res.GetType() != typeof(int))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((int)res != 255 - nBaseInt)
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

            //int<->int array test
            Console.Write("  Integer array test... ");
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
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaIntegerTest", "Test3D", new object[] { arrayInt });
                if (res.GetType() != typeof(int[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                int[, ,] arrayIntRes = (int[, ,])res;
                if (arrayIntRes.GetLength(0) != 4 || arrayIntRes.GetLength(1) != 6 || arrayIntRes.GetLength(2) != 5)
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
                            if (arrayIntRes[i, j, k] != 255 - arrayInt[i, j, k])
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

            //byte<->int test
            Console.Write("  Integer single value test using bytes... ");
            try
            {
                byte nBaseByte = 10;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaIntegerTest", "Test", nBaseByte);
                if (res.GetType() != typeof(int))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((byte)(int)res != 255 - nBaseByte)
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

            //byte<->int array test
            Console.Write("  Integer array test using bytes... ");
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
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaIntegerTest", "Test3D", new object[] { arrayByte });
                if (res.GetType() != typeof(int[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                int[, ,] arrayIntRes = (int[, ,])res;
                if (arrayIntRes.GetLength(0) != 4 || arrayIntRes.GetLength(1) != 6 || arrayIntRes.GetLength(2) != 5)
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
                            if (arrayIntRes[i, j, k] != 255 - (int)arrayByte[i, j, k])
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

            //short<->int test
            Console.Write("  Integer single value test using shorts... ");
            try
            {
                short nBaseShort = 10;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaIntegerTest", "Test", nBaseShort);
                if (res.GetType() != typeof(int))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((short)(int)res != 255 - nBaseShort)
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

            //short<->int array test
            Console.Write("  Integer array test using shorts... ");
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
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaIntegerTest", "Test3D", new object[] { arrayShort });
                if (res.GetType() != typeof(int[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                int[, ,] arrayIntRes = (int[, ,])res;
                if (arrayIntRes.GetLength(0) != 4 || arrayIntRes.GetLength(1) != 6 || arrayIntRes.GetLength(2) != 5)
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
                            if (arrayIntRes[i, j, k] != 255 - (int)arrayShort[i, j, k])
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

            //long<->int test
            Console.Write("  Integer single value test using longs... ");
            try
            {
                long nBaseLong = 10;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaIntegerTest", "Test", nBaseLong);
                if (res.GetType() != typeof(int))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((long)(int)res != 255 - nBaseLong)
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

            //long<->int array test
            Console.Write("  Integer array test using longs... ");
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
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaIntegerTest", "Test3D", new object[] { arrayLong });
                if (res.GetType() != typeof(int[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                int[, ,]  arrayIntRes = (int[, ,])res;
                if (arrayIntRes.GetLength(0) != 4 || arrayIntRes.GetLength(1) != 6 || arrayIntRes.GetLength(2) != 5)
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
                            if ((long)arrayIntRes[i, j, k] != 255 - arrayLong[i, j, k])
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
