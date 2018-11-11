namespace SeminarioTPL
{
    public class NoopDrawing : IImageDrawing
    {
        public void DrawLine(byte[] data, int line){}

        public void SaveTo(string path)
        {
        }
    }
}