using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Reflection.Metadata;
using System.Threading.Tasks;

namespace SeminarioTPL
{
    public class Runner
    {
        private readonly IMandelbrot Algorithm;
        private readonly Parameters Parameters;
        private readonly Func<IImageDrawing> Drawer;

        public Runner(IMandelbrot algorithm, Func<IImageDrawing> factory, Parameters parameters)
        {
            this.Algorithm = algorithm;
            this.Parameters = parameters;
            this.Drawer = factory;
        }

        public IEnumerable<RunResult> Run(int retries, int maxThreads)
        {
            foreach (var threads in Enumerable.Range(0, maxThreads))
            {
                foreach (var retry in Enumerable.Range(1, retries))
                {
                    Parameters.ParallelismLevel = threads;
                    Stopwatch sw = Stopwatch.StartNew();
                    byte[][] data = Algorithm.Calculate(Parameters);
                    sw.Stop();


                    var d = Drawer();
                    foreach (var line in data.Select((x, i) => new {line = x, index = i}))
                    {
                        d.DrawLine(line.line, line.index);
                    }

                    d.SaveTo($"{Algorithm}-{threads}-{retry}.png");

                    yield return new RunResult(retry, threads, (int) sw.ElapsedMilliseconds);
                }
            }
        }
    }

    public class RunResult
    {
        public RunResult(int retry, int threads, int milliseconds)
        {
            this.Retry = retry;
            this.Threads = threads;
            this.Milliseconds = milliseconds;
        }

        public int Retry { get; }
        public int Threads { get; }
        public int Milliseconds { get; }
    }
}