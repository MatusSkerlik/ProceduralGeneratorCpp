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
