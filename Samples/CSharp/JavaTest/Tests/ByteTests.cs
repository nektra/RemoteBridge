using System;
using System.Collections.Generic;
using Nektra.RemoteBridge;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace CSharpJavaTest
{
    class ByteTests
    {
        public static bool RunTests(int pid, NktRemoteBridge remoteBridge)
        {
            //byte<->byte test
            Console.Write("  Byte single value test... ");
            try
            {
                byte nBaseByte = 10;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaByteTest", "Test", nBaseByte);
                if (res.GetType() != typeof(byte))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((byte)res != 255 - nBaseByte)
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

            //byte<->byte array test
            Console.Write("  Byte array test... ");
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
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaByteTest", "Test3D", new object[] { arrayByte });
                if (res.GetType() != typeof(byte[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                byte[, ,] arrayByteRes = (byte[, ,])res;
                if (arrayByteRes.GetLength(0) != 4 || arrayByteRes.GetLength(1) != 6 || arrayByteRes.GetLength(2) != 5)
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
                            if (arrayByteRes[i, j, k] != 255 - arrayByte[i, j, k])
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

            //short<->byte test
            Console.Write("  Byte single value test using shorts... ");
            try
            {
                short nBaseShort = 10;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaByteTest", "Test", nBaseShort);
                if (res.GetType() != typeof(byte))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((short)(byte)res != 255 - nBaseShort)
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

            //short<->byte array test
            Console.Write("  Byte array test using shorts... ");
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
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaByteTest", "Test3D", new object[] { arrayShort });
                if (res.GetType() != typeof(byte[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                byte[, ,] arrayByteRes = (byte[, ,])res;
                if (arrayByteRes.GetLength(0) != 4 || arrayByteRes.GetLength(1) != 6 || arrayByteRes.GetLength(2) != 5)
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
                            if ((short)arrayByteRes[i, j, k] != 255 - arrayShort[i, j, k])
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

            //int<->byte test
            Console.Write("  Byte single value test using ints... ");
            try
            {
                int nBaseInt = 10;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaByteTest", "Test", nBaseInt);
                if (res.GetType() != typeof(byte))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((int)(byte)res != 255 - nBaseInt)
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

            //int<->byte array test
            Console.Write("  Byte array test using ints... ");
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
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaByteTest", "Test3D", new object[] { arrayInt });
                if (res.GetType() != typeof(byte[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                byte[, ,] arrayByteRes = (byte[, ,])res;
                if (arrayByteRes.GetLength(0) != 4 || arrayByteRes.GetLength(1) != 6 || arrayByteRes.GetLength(2) != 5)
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
                            if ((int)arrayByteRes[i, j, k] != 255 - arrayInt[i, j, k])
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

            //long<->byte test
            Console.Write("  Byte single value test using longs... ");
            try
            {
                long nBaseLong = 10;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaByteTest", "Test", nBaseLong);
                if (res.GetType() != typeof(byte))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((long)(byte)res != 255 - nBaseLong)
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

            //long<->byte array test
            Console.Write("  Byte array test using longs... ");
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
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaByteTest", "Test3D", new object[] { arrayLong });
                if (res.GetType() != typeof(byte[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                byte[, ,]  arrayByteRes = (byte[, ,])res;
                if (arrayByteRes.GetLength(0) != 4 || arrayByteRes.GetLength(1) != 6 || arrayByteRes.GetLength(2) != 5)
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
                            if ((long)arrayByteRes[i, j, k] != 255 - arrayLong[i, j, k])
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
