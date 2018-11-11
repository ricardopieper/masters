using System;
using System.Collections.Concurrent;
using System.Linq;
using System.Threading.Tasks;

namespace SeminarioTPL
{
    public class PLinqMandelbrot : IMandelbrot
    {
        public byte[][] Calculate(Parameters parameters)
        {
            byte[][] bytes = new byte[parameters.Size][];
            for (var i = 0; i < parameters.Size; i++)
            {
                bytes[i] = CalculateMandel(parameters, i);
            }

            return bytes;
        }

        private byte[] CalculateMandel(Parameters parameters,
            int line)
        {
            var i = line;

            var im = parameters.InitB + (parameters.Step * i);

            return Enumerable
                .Range(0, parameters.Size)
                .AsParallel()
                .WithDegreeOfParallelism(parameters.ParallelismLevel)
                .Select(j =>
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

                    return (byte) (255 - ((k * 255 / parameters.Iterations)));
                })
                .ToArray();
        }
    }
}