namespace SeminarioTPL
{
    public class SequentialMandelbrot : IMandelbrot
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

        private byte[] CalculateMandel(Parameters parameters, int line)
        {
            var m = new byte[parameters.Size];
            var i = line;

            var im = parameters.InitB + (parameters.Step * i);
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
                m[j] = (byte)(255 - ((k * 255 / parameters.Iterations)));
            }

            return m;

        }

    }
}