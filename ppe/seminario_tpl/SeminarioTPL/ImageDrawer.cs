using System;
using System.Drawing;

namespace SeminarioTPL
{
    public class ImageDrawer : IImageDrawing
    {
        private Image image;
        private Graphics graph;

        public ImageDrawer(int lines)
        {
            image = new Bitmap(lines, lines);
            graph = Graphics.FromImage(image);
            graph.Clear(Color.Black);
        }

        public void DrawLine(byte[] data, int line)
        {
            for (int i = 0; i < data.Length; i++)
            {
                byte d = data[i];
                Color c = Color.FromArgb(255, d, d, d);
                Rectangle r = new Rectangle(i, line, 1, 1);
                graph.FillRectangle(new SolidBrush(c), r);
            }
        }

        public void SaveTo(string path)
        {
            image.Save(path, System.Drawing.Imaging.ImageFormat.Png);
        }
    }

}