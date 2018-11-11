namespace SeminarioTPL
{
    public interface IImageDrawing
    {
        void DrawLine(byte[] data, int line);

        void SaveTo(string path);
    }
}