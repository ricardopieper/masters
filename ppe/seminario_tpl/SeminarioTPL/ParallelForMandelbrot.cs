using System;
using System.Collections.Concurrent;
using System.Linq;
using System.Threading.Tasks;

namespace SeminarioTPL
{
    public class ParallelForMandelbrot : IMandelbrot
    {
        public byte[][] Calculate(Parameters parameters)
        {
            ParallelOptions options = new ParallelOptions {MaxDegreeOfParallelism = parameters.ParallelismLevel};
            var partitioner = Partitioner.Create(0, parameters.Size, 1);
            byte[][] bytes = new byte[parameters.Size][];
            for (var i = 0; i < parameters.Size; i++)
            {
                bytes[i] = CalculateMandel(parameters,options, partitioner, i);
            }

            return bytes;
        }

        private byte[] CalculateMandel(Parameters parameters, 
            ParallelOptions options, 
            Partitioner<Tuple<int, int>> partitioner, 
            int line)
        {
            var m = new byte[parameters.Size];
            var i = line;

            var im = parameters.InitB + (parameters.Step * i);
            
            Parallel.ForEach(partitioner, options, range =>
            {
                for (int j = range.Item1; j < range.Item2; j++)
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
            });

            return m;
        }
    }
}