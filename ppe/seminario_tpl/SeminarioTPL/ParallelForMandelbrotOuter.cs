using System;
using System.Collections.Concurrent;
using System.Linq;
using System.Threading.Tasks;

namespace SeminarioTPL
{
    public class ParallelForMandelbrotOuter : IMandelbrot
    {
        public byte[][] Calculate(Parameters parameters)
        {
            ParallelOptions options = new ParallelOptions
            {
                MaxDegreeOfParallelism = parameters.ParallelismLevel == 0? -1 : parameters.ParallelismLevel
            };
            var results = CalculateMandel(parameters, options);
            return results;
        }

        private byte[][] CalculateMandel(Parameters parameters,
            ParallelOptions options)
        {
            var m = new byte[parameters.Size][];

            Parallel.For(0, parameters.Size, options, i =>
            {
                var im = parameters.InitB + (parameters.Step * i);
                m[i] = new byte[parameters.Size];
                for (var j = 0; j < parameters.Size; j++)
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

                    m[i][j] = (byte) (255 - ((k * 255 / parameters.Iterations)));
                }
            });

            return m;
        }
    }
}