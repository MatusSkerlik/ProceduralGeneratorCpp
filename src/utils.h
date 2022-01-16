#include <stdio.h>
#include <vector>
#include <tuple>
#include <limits>

typedef struct Rect {
    Rect() = default;
    Rect(const Rect& r): x{r.x}, y{r.y}, w{r.w}, h{r.h} {};
    Rect(int _x, int _y, int _w, int _h): x{_x}, y{_y}, w{_w}, h{_h} {};
    int x;
    int y;
    int w;
    int h;
} Rect;

typedef struct Pixel {
    int x;
    int y;
} Pixel;

typedef struct RGB {
    RGB(unsigned char _r, unsigned char _g, unsigned char _b): r{_r}, g{_g}, b{_b} {};
    unsigned char r;
    unsigned char g;
    unsigned char b;
} RGB;

#define C_RED           RGB(255, 0, 0)
#define C_GREED         RGB(0, 255, 0)
#define C_BLUE          RGB(0, 0, 255)

#define C_SPACE         RGB(51, 102, 153)
#define C_SURFACE       RGB(155,209, 255)
#define C_UNDERGROUND   RGB(151, 107, 75)
#define C_CAVERN        RGB(128, 128, 128)
#define C_HELL          RGB(0, 0, 0)

class PixelArray
{
    private:
        std::vector<Pixel> pixels;

    public:
        PixelArray(){};

        auto operator[](int index){ return pixels[index]; };
        auto begin() { return this->pixels.begin(); };
        auto end() { return this->pixels.end(); };

        void add(Pixel pixel) { pixels.push_back(pixel); };
        void add(int x, int y) { pixels.push_back((Pixel){x, y}); };

        Rect bbox()
        {
            if (pixels.size() == 0)
                return (Rect){0, 0, 0, 0};

            int minx = std::numeric_limits<int>::max();
            int miny = std::numeric_limits<int>::max();
            int maxx = std::numeric_limits<int>::min();
            int maxy = std::numeric_limits<int>::min();

            for (auto pixel: pixels)
            {
                if (pixel.x < minx) minx = pixel.x;
                if (pixel.x > maxx) maxx = pixel.x;
                if (pixel.y < miny) miny = pixel.y;
                if (pixel.y > maxy) maxy = pixel.y;
            }

            return (Rect){minx, miny, maxx - minx, maxy - miny};
        };
};

class Polygon
{
    int pnpoly(int nvert, float *vertx, float *verty, float testx, float testy)
    {
      int i, j, c = 0;
      for (i = 0, j = nvert-1; i < nvert; j = i++) {
        if ( ((verty[i]>testy) != (verty[j]>testy)) &&
         (testx < (vertx[j]-vertx[i]) * (testy-verty[i]) / (verty[j]-verty[i]) + vertx[i]) )
           c = !c;
      }
      return c;
    }

    private:
        PixelArray _pixels(void)
        {
            PixelArray p;
            int size = vertices.size();
            float vertx[size];
            float verty[size];

            int i = 0;
            for (auto pixel: vertices)
            {
                vertx[i] = static_cast<float>(pixel.x);
                verty[i] = static_cast<float>(pixel.y);
                i++;
            }

            for (int x = bbox.x; x < bbox.x + bbox.w; x++)
            {
                for (int y = bbox.y; y < bbox.y + bbox.h; y++)
                {
                    if (pnpoly(size, vertx, verty, (float) x, (float) y)) p.add(x, y);
                }
            }
            return p;
        }

    public:
        std::vector<Pixel> vertices;
        PixelArray pixels;
        Rect bbox;
        Polygon(PixelArray v) { for (auto pixel: v) { vertices.push_back(pixel); }; bbox = v.bbox(); pixels = _pixels(); };
};

class DATA {
    public:
        int WIDTH;
        int HEIGHT;
        std::vector<std::pair<Rect, RGB>> HORIZONTAL_AREAS;
        std::vector<Polygon> BIOMES;

        void clear()
        {
            BIOMES.clear();
            HORIZONTAL_AREAS.clear();
        };
};
