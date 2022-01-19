#include <stdio.h>
#include <cmath>
#include <unordered_set>
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
    const bool operator==(const Pixel& p) const { return x == p.x && y == p.y; }
} Pixel;

struct PixelHash
{
    size_t operator()(const Pixel& pixel) const { return pixel.x * 1000 + pixel.y; };    
};

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
        std::unordered_set<Pixel, PixelHash> _set_pixels;
        std::vector<Pixel> _vector_pixels;
    public:
        PixelArray(): _set_pixels{0} {
            printf("PixelArray();\n");
        };

        auto operator[](int index){ return _vector_pixels[index]; };
        auto begin() { return this->_vector_pixels.begin(); };
        auto end() { return this->_vector_pixels.end(); };
        auto size() { return this->_vector_pixels.size(); }

        void add(Pixel pixel) {

            auto result = _set_pixels.insert(pixel);
            if (result.second)
                _vector_pixels.push_back(pixel);
        };
        void add(int x, int y) { add((Pixel){x, y}); };

        Rect bbox()
        {
            if (_vector_pixels.size() == 0)
                return (Rect){0, 0, 0, 0};

            int minx = std::numeric_limits<int>::max();
            int miny = std::numeric_limits<int>::max();
            int maxx = std::numeric_limits<int>::min();
            int maxy = std::numeric_limits<int>::min();

            for (auto pixel: _vector_pixels)
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
        Polygon(PixelArray& v) { 
            printf("Polygon();\n");
            for (auto& pixel: v) { 
                vertices.push_back(pixel); 
            }; 
            bbox = v.bbox(); 
            pixels = std::move(_pixels()); 
        };
};

class Vector2D
{
    public:
        float x;
        float y;
        
        Vector2D(float _x, float _y): x{_x}, y{_y} {};
        Vector2D(int _x, int _y): x{(float)_x}, y{(float)_y} {};
        
        Vector2D add(const Vector2D& to)
        {
            return Vector2D(x + to.x, y + to.y);
        }

        Vector2D sub(const Vector2D& to)
        {
            return Vector2D(x - to.x, y - to.y);
        }

        float dist(const Vector2D& to)
        {
            return sqrt(pow(x - to.x, 2) + pow(y - to.y, 2));    
        }
    
};

void PixelsAroundCircle(int x, int y, float radius, PixelArray& array)
{
    auto center = Vector2D(x, y);
    auto from = Vector2D(0, 0);

    int minx = x - radius;
    int maxx = x + radius;
    int miny = y - radius;
    int maxy = y + radius;

    for (int i = minx; i < maxx; i++)
    {
        for (int j = miny; j < maxy; j++)
        {
            from.x = i;
            from.y = j;

            if (center.dist(from) < radius)
                array.add(i, j);
        }
    }
};

void PixelsAroundRect(int x, int y, int w, int h, PixelArray& array)
{
    int minx = x - w / 2;
    int maxx = x + w / 2;
    int miny = y - h / 2;
    int maxy = y + h / 2;

    for (int i = minx; i < maxx; i++)
    {
        for (int j = miny; j < maxy; j++)
        {
            array.add(i, j);
        }
    }
};

void PixelsOfRect(int x, int y, int w, int h, PixelArray& array)
{
   for (int i = x; i < x + w; i++)
   {
        for (int j = y; j < y + h; j++)
        {
            array.add(i, j);
        }
   }
}

class DATA {
    public:
        int WIDTH;
        int HEIGHT;

        std::vector<std::pair<Rect, RGB>> HORIZONTAL_AREAS;
        std::vector<PixelArray> BIOMES;
        std::vector<PixelArray> MINI_BIOMES;

        DATA(){
            printf("DATA();\n");
        };

        void clear()
        {
            HORIZONTAL_AREAS.clear();
            BIOMES.clear();
            MINI_BIOMES.clear();
        };
};
