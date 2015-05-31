using System;
using System.Collections.Generic;
using Nektra.RemoteBridge;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace CSharpJavaTest
{
    class LongTests
    {
        public static bool RunTests(int pid, NktRemoteBridge remoteBridge)
        {
            //long<->long test
            Console.Write("  Long single value test... ");
            try
            {
                long nBaseLong = 10;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaLongTest", "Test", nBaseLong);
                if (res.GetType() != typeof(long))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((long)res != 255 - nBaseLong)
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

            //long<->long array test
            Console.Write("  Long array test... ");
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
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaLongTest", "Test3D", new object[] { arrayLong });
                if (res.GetType() != typeof(long[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                long[, ,] arrayLongRes = (long[, ,])res;
                if (arrayLongRes.GetLength(0) != 4 || arrayLongRes.GetLength(1) != 6 || arrayLongRes.GetLength(2) != 5)
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
                            if (arrayLongRes[i, j, k] != 255 - arrayLong[i, j, k])
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

            //byte<->long test
            Console.Write("  Long single value test using bytes... ");
            try
            {
                byte nBaseByte = 10;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaLongTest", "Test", nBaseByte);
                if (res.GetType() != typeof(long))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((byte)(long)res != 255 - nBaseByte)
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

            //byte<->long array test
            Console.Write("  Long array test using bytes... ");
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
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaLongTest", "Test3D", new object[] { arrayByte });
                if (res.GetType() != typeof(long[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                long[, ,] arrayLongRes = (long[, ,])res;
                if (arrayLongRes.GetLength(0) != 4 || arrayLongRes.GetLength(1) != 6 || arrayLongRes.GetLength(2) != 5)
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
                            if (arrayLongRes[i, j, k] != 255 - (long)arrayByte[i, j, k])
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

            //short<->long test
            Console.Write("  Long single value test using shorts... ");
            try
            {
                short nBaseShort = 10;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaLongTest", "Test", nBaseShort);
                if (res.GetType() != typeof(long))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((short)(long)res != 255 - nBaseShort)
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

            //short<->long array test
            Console.Write("  Long array test using shorts... ");
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
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaLongTest", "Test3D", new object[] { arrayShort });
                if (res.GetType() != typeof(long[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                long[, ,] arrayLongRes = (long[, ,])res;
                if (arrayLongRes.GetLength(0) != 4 || arrayLongRes.GetLength(1) != 6 || arrayLongRes.GetLength(2) != 5)
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
                            if (arrayLongRes[i, j, k] != 255 - (long)arrayShort[i, j, k])
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

            //int<->long test
            Console.Write("  Long single value test using ints... ");
            try
            {
                int nBaseInt = 10;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaLongTest", "Test", nBaseInt);
                if (res.GetType() != typeof(long))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((int)(long)res != 255 - nBaseInt)
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

            //int<->long array test
            Console.Write("  Long array test using ints... ");
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
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaLongTest", "Test3D", new object[] { arrayInt });
                if (res.GetType() != typeof(long[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                long[, ,] arrayLongRes = (long[, ,])res;
                if (arrayLongRes.GetLength(0) != 4 || arrayLongRes.GetLength(1) != 6 || arrayLongRes.GetLength(2) != 5)
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
                            if (arrayLongRes[i, j, k] != 255 - (long)arrayInt[i, j, k])
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
