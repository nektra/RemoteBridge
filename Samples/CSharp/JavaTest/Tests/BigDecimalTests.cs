using System;
using System.Collections.Generic;
using Nektra.RemoteBridge;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace CSharpJavaTest
{
    class BigDecimalTests
    {
        public static bool RunTests(int pid, NktRemoteBridge remoteBridge)
        {
            //decimal<->decimal test
            Console.Write("  BigDecimal single value test... ");
            try
            {
                Decimal nBaseDecimal = 10.0M;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaBigDecimalTest", "Test", nBaseDecimal);
                if (res.GetType() != typeof(Decimal))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((Decimal)res != 255.0M - nBaseDecimal)
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

            //decimal<->decimal array test
            Console.Write("  BigDecimal array test... ");
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
                            arrayDecimal[i, j, k] = new Decimal(i+j+k);
                        }
                    }
                }
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaBigDecimalTest", "Test3D", new object[] { arrayDecimal });
                if (res.GetType() != typeof(Decimal[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                Decimal[, ,] arrayDecimalRes = (Decimal[, ,])res;
                if (arrayDecimalRes.GetLength(0) != 4 || arrayDecimalRes.GetLength(1) != 6 || arrayDecimalRes.GetLength(2) != 5)
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
                            if (arrayDecimalRes[i, j, k] != 255.0M - arrayDecimal[i, j, k])
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

            //float<->decimal test
            Console.Write("  BigDecimal single value test using floats... ");
            try
            {
                float nBaseFloat = 10.0F;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaBigDecimalTest", "Test", nBaseFloat);
                if (res.GetType() != typeof(Decimal))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((Decimal)res != 255.0M - (Decimal)nBaseFloat)
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

            //float<->decimal array test
            Console.Write("  BigDecimal array test using floats... ");
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
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaBigDecimalTest", "Test3D", new object[] { arrayFloat });
                if (res.GetType() != typeof(Decimal[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                Decimal[, ,] arrayDecimalRes = (Decimal[, ,])res;
                if (arrayDecimalRes.GetLength(0) != 4 || arrayDecimalRes.GetLength(1) != 6 || arrayDecimalRes.GetLength(2) != 5)
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
                            if (arrayDecimalRes[i, j, k] != 255.0M - (Decimal)arrayFloat[i, j, k])
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

            //double<->decimal test
            Console.Write("  BigDecimal single value test using doubles... ");
            try
            {
                double nBaseDouble = 10.0;
                object res;

                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaBigDecimalTest", "Test", nBaseDouble);
                if (res.GetType() != typeof(Decimal))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                if ((Decimal)res != 255.0M - (Decimal)nBaseDouble)
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

            //double<->decimal array test
            Console.Write("  BigDecimal array test using doubles... ");
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
                res = remoteBridge.InvokeJavaStaticMethod(pid, "JavaBigDecimalTest", "Test3D", new object[] { arrayDouble });
                if (res.GetType() != typeof(Decimal[, ,]))
                {
                    Console.WriteLine("Error: Invalid returned type");
                    return false;
                }
                Decimal[, ,] arrayDecimalRes = (Decimal[, ,])res;
                if (arrayDecimalRes.GetLength(0) != 4 || arrayDecimalRes.GetLength(1) != 6 || arrayDecimalRes.GetLength(2) != 5)
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
                            if (arrayDecimalRes[i, j, k] != 255.0M - (Decimal)arrayDouble[i, j, k])
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
