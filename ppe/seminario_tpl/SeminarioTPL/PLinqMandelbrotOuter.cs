using System;
using System.Collections.Concurrent;
using System.Linq;
using System.Threading.Tasks;

namespace SeminarioTPL
{
    public class PLinqMandelbrotOuter : IMandelbrot
    {
        public byte[][] Calculate(Parameters parameters)
        {
            return CalculateMandel(parameters);
        }

        private byte[][] CalculateMandel(Parameters parameters)
        {
            return Enumerable
                .Range(0, parameters.Size)
                .AsParallel()
                .WithDegreeOfParallelism(parameters.ParallelismLevel)
                .Select(i =>
                {
                    var im = parameters.InitB + (parameters.Step * i);
                    byte[] m = new byte[parameters.Size];

                    for (int j = 0; j < parameters.Size; j++)
                    {
                        double cr;
                        var a = cr = parameters.InitA + parameters.Step * j;
                        var b = im;
                        var k = 0;
                        for (; k < parameters.Iterations; k++)
                        {
                            var a2 = a * a;
                            var b2 = b * b;
                            if ((a2 + b2) > 4.0) break;
                            b = 2 * a * b + im;
                            a = a2 - b2 + cr;
                        }

                        m[j] = (byte) (255 - ((k * 255 / parameters.Iterations)));
                    }

                    return new { Data = m, Line = i };
                })
                .OrderBy(x => x.Line)
                .Select(x => x.Data)
                .ToArray();
        }
    }
}