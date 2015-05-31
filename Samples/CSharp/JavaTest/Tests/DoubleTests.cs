using System;
using System.Collections.Generic;
using Nektra.RemoteBridge;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace CSharpJavaTest
{
    class DoubleTests
    {
        public static bool RunTests(int pid, NktRemoteBridge remoteBridge)
        {
            //double<->double test
            Console.Write("  Double single value test... ");
            try
            {
                double nBaseDouble = 10.0;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaDoubleTest", "Test", nBaseDouble);
                if (res.GetType() != typeof(double))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((double)res != 255.0 - nBaseDouble)
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

            //double<->double array test
            Console.Write("  Double array test... ");
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
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaDoubleTest", "Test3D", new object[] { arrayDouble });
                if (res.GetType() != typeof(double[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                double[, ,] arrayDoubleRes = (double[, ,])res;
                if (arrayDoubleRes.GetLength(0) != 4 || arrayDoubleRes.GetLength(1) != 6 || arrayDoubleRes.GetLength(2) != 5)
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
                            if (arrayDoubleRes[i, j, k] != 255.0 - arrayDouble[i, j, k])
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

            //float<->double test
            Console.Write("  Double single value test using floats... ");
            try
            {
                float nBaseFloat = 10.0F;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaDoubleTest", "Test", nBaseFloat);
                if (res.GetType() != typeof(double))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((double)res != 255.0F - (double)nBaseFloat)
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

            //float<->double array test
            Console.Write("  Double array test using floats... ");
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
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaDoubleTest", "Test3D", new object[] { arrayFloat });
                if (res.GetType() != typeof(double[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                double[, ,] arrayDoubleRes = (double[, ,])res;
                if (arrayDoubleRes.GetLength(0) != 4 || arrayDoubleRes.GetLength(1) != 6 || arrayDoubleRes.GetLength(2) != 5)
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
                            if (arrayDoubleRes[i, j, k] != 255.0 - (double)arrayFloat[i, j, k])
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

            //decimal<->double test
            Console.Write("  Double single value test using BigDecimal... ");
            try
            {
                Decimal nBaseDecimal = 10.0M;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaDoubleTest", "Test", nBaseDecimal);
                if (res.GetType() != typeof(double))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((Decimal)(double)(res) != 255.0M - nBaseDecimal)
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

            //decimal<->double array test
            Console.Write("  Double array test using BigDecimal... ");
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
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaDoubleTest", "Test3D", new object[] { arrayDecimal });
                if (res.GetType() != typeof(double[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                double[, ,] arrayDoubleRes = (double[, ,])res;
                if (arrayDoubleRes.GetLength(0) != 4 || arrayDoubleRes.GetLength(1) != 6 || arrayDoubleRes.GetLength(2) != 5)
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
                            if ((Decimal)arrayDoubleRes[i, j, k] != 255.0M - arrayDecimal[i, j, k])
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
