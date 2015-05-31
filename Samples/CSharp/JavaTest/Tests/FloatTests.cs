using System;
using System.Collections.Generic;
using Nektra.RemoteBridge;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace CSharpJavaTest
{
    class FloatTests
    {
        public static bool RunTests(int pid, NktRemoteBridge remoteBridge)
        {
            //float<->float test
            Console.Write("  Float single value test... ");
            try
            {
                float nBaseFloat = 10.0F;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaFloatTest", "Test", nBaseFloat);
                if (res.GetType() != typeof(float))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((float)res != 255.0F - nBaseFloat)
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

            //float<->float array test
            Console.Write("  Float array test... ");
            try
            {
                float[, ,] arrayFloat = new float[4, 6, 5];
                object res;
                int i, j, k;

                for (i = 0; i < 4; i++)
                {
                    for (j = 0; j < 6; j++)
                    {
                        for (k = 0; k < 5; k++)
                        {
                            arrayFloat[i, j, k] = (float)(i + j + k);
                        }
                    }
                }
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaFloatTest", "Test3D", new object[] { arrayFloat });
                if (res.GetType() != typeof(float[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                float[, ,] arrayFloatRes = (float[, ,])res;
                if (arrayFloatRes.GetLength(0) != 4 || arrayFloatRes.GetLength(1) != 6 || arrayFloatRes.GetLength(2) != 5)
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
                            if (arrayFloatRes[i, j, k] != 255.0 - arrayFloat[i, j, k])
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

            //double<->float test
            Console.Write("  Float single value test using doubles... ");
            try
            {
                double nBaseDouble = 10.0;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaFloatTest", "Test", nBaseDouble);
                if (res.GetType() != typeof(float))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((double)(float)res != 255.0 - nBaseDouble)
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

            //double<->float array test
            Console.Write("  Float array test using doubles... ");
            try
            {
                double[, ,] arrayDouble = new double[4, 6, 5];
                object res;
                int i, j, k;

                for (i = 0; i < 4; i++)
                {
                    for (j = 0; j < 6; j++)
                    {
                        for (k = 0; k < 5; k++)
                        {
                            arrayDouble[i, j, k] = (double)(i + j + k);
                        }
                    }
                }
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaFloatTest", "Test3D", new object[] { arrayDouble });
                if (res.GetType() != typeof(float[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                float[, ,] arrayFloatRes = (float[, ,])res;
                if (arrayFloatRes.GetLength(0) != 4 || arrayFloatRes.GetLength(1) != 6 || arrayFloatRes.GetLength(2) != 5)
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
                            if ((double)arrayFloatRes[i, j, k] != 255.0 - arrayDouble[i, j, k])
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

            //decimal<->float test
            Console.Write("  Float single value test using BigDecimal... ");
            try
            {
                Decimal nBaseDecimal = 10.0M;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaFloatTest", "Test", nBaseDecimal);
                if (res.GetType() != typeof(float))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((Decimal)(float)(res) != 255.0M - nBaseDecimal)
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

            //decimal<->float array test
            Console.Write("  Float array test using BigDecimal... ");
            try
            {
                Decimal[, ,] arrayDecimal = new Decimal[4, 6, 5];
                object res;
                int i, j, k;

                for (i = 0; i < 4; i++)
                {
                    for (j = 0; j < 6; j++)
                    {
                        for (k = 0; k < 5; k++)
                        {
                            arrayDecimal[i, j, k] = new Decimal(i + j + k);
                        }
                    }
                }
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaFloatTest", "Test3D", new object[] { arrayDecimal });
                if (res.GetType() != typeof(float[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                float[, ,] arrayFloatRes = (float[, ,])res;
                if (arrayFloatRes.GetLength(0) != 4 || arrayFloatRes.GetLength(1) != 6 || arrayFloatRes.GetLength(2) != 5)
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
                            if ((Decimal)arrayFloatRes[i, j, k] != 255.0M - arrayDecimal[i, j, k])
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
