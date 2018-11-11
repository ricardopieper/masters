using System;
using System.Collections.Generic;
using System.Drawing;
using System.Globalization;
using System.Linq;

namespace SeminarioTPL
{
    class Program
    {
        static void Main(string[] args)
        {
            var dimensions = Convert.ToInt32(args[0]);
            var niter = Convert.ToInt32(args[1]);

            var parameters = new Parameters
            {
                InitA = -2.125,
                InitB = -1.5,
                Range = 3.0,
                Size = dimensions,
                Iterations = niter,
                ParallelismLevel = 8,
                Step = 3.0d / (double) dimensions,
            };

            IMandelbrot[] allAlgorithms =
            {
                //new SequentialMandelbrot(),
                new ParallelForMandelbrotOuter(),
                 //new PLinqMandelbrotOuter(), 
            };

            foreach (var mandelbrot in allAlgorithms)
            {
                Console.WriteLine("Parallel version: " + mandelbrot);

                mandelbrot.Calculate(parameters);
                var runner = new Runner(mandelbrot, () => new NoopDrawing(),  parameters);
                
                var results = runner.Run(5, 16);

                List<RunResult> printedResults = new List<RunResult>();

                foreach (var result in results)
                {
                    Console.WriteLine($"{result.Retry},{result.Threads},{result.Milliseconds}");
                    printedResults.Add(result);
                }

                Console.WriteLine("Average runtime by threads");

                IFormatProvider provider = CultureInfo.GetCultureInfo("PT-BR");
                
                printedResults.GroupBy(x => x.Threads)
                    .Select(x =>
                    {
                        var avg = x.Average(y => y.Milliseconds);
                        var stdev = Math.Sqrt(
                                x.Sum(y => ((double) y.Milliseconds - avg) * ((double) y.Milliseconds - avg)) /
                            x.Count());
                        return new
                        {
                            Threads = x.Key,
                            Average = avg,
                            Stdev = stdev
                        };
                    })
                    .ToList()
                    .ForEach(x => { Console.WriteLine($"{x.Threads}|{x.Average.ToString(provider)}|{x.Stdev.ToString(provider)}"); });
            }
        }
    }
}