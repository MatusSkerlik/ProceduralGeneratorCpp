#include <memory>
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
    public:
        PixelArray()
        {
            printf("PixelArray();\n");
        };
        
        PixelArray(const PixelArray& arr)
        {
            printf("PixelArray&();\n");
            _set_pixels = arr._set_pixels;
        }

        PixelArray(PixelArray&& arr)
        {
            printf("PixelArray&&();\n");
            _set_pixels = std::move(arr._set_pixels);
        }

        auto begin() const { return this->_set_pixels.begin(); };
        auto end() const { return this->_set_pixels.end(); };
        auto size() const { return this->_set_pixels.size(); }
        auto contains(Pixel pixel) const { return this->_set_pixels.find(pixel) != this->_set_pixels.end(); };

        void add(Pixel pixel) {
            _set_pixels.insert(pixel);
        };
        void add(int x, int y) { add((Pixel){x, y}); };

        void remove(Pixel pixel) { _set_pixels.erase(pixel); };

        void remove(int x, int y) { remove((Pixel){x, y}); };

        void clear() { _set_pixels.clear(); };

        Rect bbox() const
        {
            if (_set_pixels.size() == 0)
                return (Rect){0, 0, 0, 0};

            int minx = std::numeric_limits<int>::max();
            int miny = std::numeric_limits<int>::max();
            int maxx = std::numeric_limits<int>::min();
            int maxy = std::numeric_limits<int>::min();

            for (auto pixel: _set_pixels)
            {
                if (pixel.x < minx) minx = pixel.x;
                if (pixel.x > maxx) maxx = pixel.x;
                if (pixel.y < miny) miny = pixel.y;
                if (pixel.y > maxy) maxy = pixel.y;
            }

            return (Rect){minx, miny, maxx - minx, maxy - miny};
        };
};

void FillWithRect(const Rect& rect, PixelArray& arr)
{
    for (int x = rect.x; x < rect.x + rect.w; ++x)
        for (int y = rect.y; y < rect.y + rect.h; ++y)
            arr.add(x, y);
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
    };

}

namespace Biomes {

    enum Type: unsigned short { JUNGLE, TUNDRA, FOREST, OCEAN };

    class Biome: public PixelArray 
    {
        public:
            Type type;
            Biome(): PixelArray() {};
            Biome(Type _t): PixelArray(), type{_t} {};
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
   };
};

class Map {
    public:
        int width;
        int height;
        
        std::unique_ptr<HorizontalAreas::Biome> Space;
        std::unique_ptr<HorizontalAreas::Biome> Surface;
        std::unique_ptr<HorizontalAreas::Biome> Underground;
        std::unique_ptr<HorizontalAreas::Biome> Cavern;
        std::unique_ptr<HorizontalAreas::Biome> Hell;

        std::vector<std::unique_ptr<Biomes::Biome>> _biomes;
        std::vector<std::unique_ptr<MiniBiomes::Biome>> _mini_biomes;

        Map(){
            printf("Map();\n");

            Space.reset(new HorizontalAreas::Biome(HorizontalAreas::SPACE));
            Surface.reset(new HorizontalAreas::Biome(HorizontalAreas::SURFACE));
            Underground.reset(new HorizontalAreas::Biome(HorizontalAreas::UNDERGROUND));
            Cavern.reset(new HorizontalAreas::Biome(HorizontalAreas::CAVERN));
            Hell.reset(new HorizontalAreas::Biome(HorizontalAreas::HELL));
        };

        std::vector<HorizontalAreas::Biome*> HorizontalAreas()
        {
            return {Space.get(), Surface.get(), Underground.get(), Cavern.get(), Hell.get()};
        }

        auto Biome(Biomes::Type type)
        {
            _biomes.emplace_back(new Biomes::Biome(type));
            return _biomes[_biomes.size() - 1].get();
        }

        Biomes::Biome* GetBiome(Biomes::Type type)
        {
            for (auto& biome: _biomes)
            {
                if (biome->type == type)
                    return biome.get();
            }
            return nullptr;
        }

        auto& Biomes()
        {
            return _biomes;
        }

        auto& MiniBiomes()
        {
            return _mini_biomes;
        }

        auto MiniBiome(MiniBiomes::Type type)
        {
            _mini_biomes.emplace_back(new MiniBiomes::Biome(type));
            return _mini_biomes[_mini_biomes.size() - 1].get();
        }

        void clear()
        {
            for (auto& area: HorizontalAreas())
                area->clear();
            _biomes.clear();
            _mini_biomes.clear();

        };
};

void UnitedPixelArea(const PixelArray& pixels, int x, int y, PixelArray& fill)
{
    std::vector<Pixel> queue {Pixel(x, y)};
    
    while (queue.size() > 0)
    {
        auto p = queue[queue.size() - 1];
        queue.pop_back();
        if (pixels.contains(p))
        {
            fill.add(p);
            
            Pixel p0 = {p.x - 1, p.y};
            Pixel p1 = {p.x + 1, p.y};
            Pixel p2 = {p.x, p.y - 1};
            Pixel p3 = {p.x, p.y + 1};

            if (!fill.contains(p0))
                queue.push_back(p0);
            if (!fill.contains(p1))
                queue.push_back(p1);
            if (!fill.contains(p2))
                queue.push_back(p2);
            if (!fill.contains(p3))
                queue.push_back(p3);
        }
    }
}

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
