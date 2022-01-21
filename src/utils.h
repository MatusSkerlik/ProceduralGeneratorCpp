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
    Pixel(int _x, int _y): x{_x}, y{_y} {};
    const bool operator==(const Pixel& p) const { return x == p.x && y == p.y; }
} Pixel;

struct PixelHash
{
    size_t operator()(const Pixel& pixel) const { return pixel.x * 1000 + pixel.y; };    
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

class PixelArray
{
    protected:
        std::unordered_set<Pixel, PixelHash> _set_pixels;
        std::vector<Pixel> _vector_pixels;
    public:
        PixelArray()
        {
            printf("PixelArray();\n");
        };
        
        PixelArray(const PixelArray& arr)
        {
            printf("PixelArray&();\n");
            _set_pixels = arr._set_pixels;
            _vector_pixels = arr._vector_pixels;
        }

        PixelArray(PixelArray&& arr)
        {
            printf("PixelArray&&();\n");
            _set_pixels = std::move(arr._set_pixels);
            _vector_pixels = std::move(arr._vector_pixels);
        }

        auto operator[](int index) const { return _vector_pixels[index]; };
        auto begin() const { return this->_vector_pixels.begin(); };
        auto end() const { return this->_vector_pixels.end(); };
        auto size() const { return this->_vector_pixels.size(); }
        auto contains(Pixel pixel) const { return this->_set_pixels.find(pixel) != this->_set_pixels.end(); };

        void add(Pixel pixel) {

            auto result = _set_pixels.insert(pixel);
            if (result.second)
                _vector_pixels.push_back(pixel);
        };
        void add(int x, int y) { add((Pixel){x, y}); };

        Rect bbox() const
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

PixelArray FloodFill(const Rect& rect, int x, int y, const PixelArray& illegal)
{
    PixelArray result;
    std::vector<Pixel> queue {Pixel(x, y)};
    
    auto x2 = rect.x + rect.w;
    auto y2 = rect.y + rect.h;
    while (queue.size() > 0)
    {
        Pixel p = queue[queue.size() - 1];
        queue.pop_back();
        if (!illegal.contains(p))
        {
            result.add(p);
            
            Pixel p0 = {p.x - 1, p.y};
            Pixel p1 = {p.x + 1, p.y};
            Pixel p2 = {p.x, p.y - 1};
            Pixel p3 = {p.x, p.y + 1};

            if (p.x > rect.x && !result.contains(p0))
                queue.push_back(p0);
            if (p.x < x2 && !result.contains(p1))
                queue.push_back(p1);
            if (p.y > rect.y && ! result.contains(p2))
                queue.push_back(p2);
            if (p.y < y2 && !result.contains(p3))
                queue.push_back(p3);
        }

    }

    return result;
}

namespace HorizontalAreas
{
    enum Type: unsigned short { SPACE, SURFACE, UNDERGROUND, CAVERN, HELL };

    class Biome: public PixelArray
    {
        public:
            Type type;
            Biome(): PixelArray() {};
            Biome(Type _t): PixelArray(), type{_t} {};

            Biome(const Biome& biome): PixelArray(biome)
            {
                printf("HorizontalAreas::Biome&();\n");
                type = biome.type;
            }

            Biome(Biome&& biome): PixelArray(std::move(biome))
            {
                printf("HorizontalAreas::Biome&&();\n");
                type = biome.type;
            }

            Biome& operator=(Biome&& arr)
            {
                printf("HorizontalAreas::operator=&&\n");
                _set_pixels = std::move(arr._set_pixels);
                _vector_pixels = std::move(arr._vector_pixels);
                type = arr.type;
                return *this;
            }
    };

    Biome FromRect(const Rect& rect, Type type)
    {
        Biome biome {type};
        for (int x = rect.x; x < rect.x + rect.w; ++x)
            for (int y = rect.y; y < rect.y + rect.h; ++y)
                biome.add(x, y);
        return biome;
    }
}

namespace Biomes {

    enum Type: unsigned short { JUNGLE, TUNDRA, FOREST, OCEAN };

    class Biome: public PixelArray 
    {
        public:
            Type type;
            Biome(): PixelArray() {};
            Biome(Type _t): PixelArray(), type{_t} {};
    
            Biome(const Biome& biome): PixelArray(biome)
            {
                printf("Biomes::Biome&();\n");
                type = biome.type;
            }

            Biome(Biome&& biome): PixelArray(std::move(biome))
            {
                printf("Biomes::Biome&&();\n");
                type = biome.type;
            }

            Biome(PixelArray&& arr, Type _type): PixelArray(std::move(arr))
            {
                type = _type;
            }
    };


};

namespace MiniBiomes {
    
    enum Type: unsigned short { HILL, HOLE, CABIN, FLOATING_ISLAND, SURFACE_TUNNEL, UNDERGROUND_TUNNEL, CASTLE }; 
    
    class Biome: public PixelArray 
    {
        public:
            Type type;
            Biome(): PixelArray() {};
            Biome(Type _t): PixelArray(), type{_t} {};

            Biome(const Biome& biome): PixelArray(biome)
            {
                printf("MiniBiomes::Biome&();\n");
                type = biome.type;
            }

            Biome(Biome&& biome): PixelArray(std::move(biome))
            {
                printf("MiniBiomes::Biome&&();\n");
                type = biome.type;
            }

    };
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

class Map {
    public:
        int width;
        int height;
        
        HorizontalAreas::Biome Space;
        HorizontalAreas::Biome Surface;
        HorizontalAreas::Biome Underground;
        HorizontalAreas::Biome Cavern;
        HorizontalAreas::Biome Hell;

        std::vector<Biomes::Biome> _biomes;
        std::vector<MiniBiomes::Biome> _mini_biomes;

        Map(){
            printf("Map();\n");
        };

        std::vector<HorizontalAreas::Biome*> HorizontalAreas()
        {
            return {&Space, &Surface, &Underground, &Cavern, &Hell};
        }

        std::vector<Biomes::Biome>& Biomes()
        {
            return _biomes;
        }

        void Biomes(Biomes::Biome&& biome)
        {
            _biomes.push_back(std::move(biome));
        }

        std::vector<MiniBiomes::Biome>& MiniBiomes()
        {
            return _mini_biomes;
        }

        void MiniBiomes(MiniBiomes::Biome&& minibiome)
        {
            _mini_biomes.push_back(std::move(minibiome));
        }

        void clear()
        {
            _biomes.clear();
            _mini_biomes.clear();
        };
};
