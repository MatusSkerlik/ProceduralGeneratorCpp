#include <functional>
#include <stdio.h>
#include <mutex>
#include <memory>
#include <cmath>
#include <unordered_set>
#include <vector>
#include <tuple>
#include <limits>
#include <atomic>

typedef struct Rect {
    Rect() = default;
    Rect(const Rect& r): x{r.x}, y{r.y}, w{r.w}, h{r.h} {};
    Rect(int _x, int _y, int _w, int _h): x{_x}, y{_y}, w{_w}, h{_h} {};
    int x;
    int y;
    int w;
    int h;
} Rect;

inline auto RectIntersection(const Rect& r0, const Rect& r1)
{
    auto x0 = std::max(r0.x, r1.x);
    auto y0 = std::max(r0.y, r0.y);
    auto x1 = std::min(r0.x + r0.w, r1.x + r1.w);
    auto y1 = std::min(r0.y + r0.h, r1.y + r1.h);

    return (Rect){x0, y0, x1 - x0, y1 - y0};
};

inline auto RectUnion(const Rect& r0, const Rect& r1)
{
    auto x0 = std::min(r0.x, r1.x);
    auto y0 = std::min(r0.y, r0.y);
    auto x1 = std::max(r0.x + r0.w, r1.x + r1.w);
    auto y1 = std::max(r0.y + r0.h, r1.y + r1.h);

    return (Rect){x0, y0, x1 - x0, y1 - y0};
};

typedef struct Pixel {
    int x;
    int y;
    Pixel(int _x, int _y): x{_x}, y{_y} {};
    const bool operator==(const Pixel& p) const { return x == p.x && y == p.y; }
} Pixel;

struct PixelHash
{
    size_t operator()(const Pixel& pixel) const { return pixel.x * 4200 + pixel.y; };    
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
        Rect _bounding_box {0, 0, 0, 0};

    public:
        PixelArray()
        {
            printf("PixelArray();\n");
        };
        
        PixelArray(const PixelArray& arr)
        {
            printf("PixelArray&();\n");
            _set_pixels = arr._set_pixels;
            _bounding_box = arr._bounding_box;
        }

        PixelArray(PixelArray&& arr)
        {
            printf("PixelArray&&();\n");
            _set_pixels = std::move(arr._set_pixels);
            _bounding_box = std::move(arr._bounding_box);
        }

        ~PixelArray(){ printf("~PixelArray();\n"); };

        auto begin() const { return this->_set_pixels.begin(); };
        auto end() const { return this->_set_pixels.end(); };
        auto size() const { return this->_set_pixels.size(); };
        auto contains(Pixel pixel) const { return this->_set_pixels.find(pixel) != this->_set_pixels.end(); };
        void add(Pixel pixel) { _set_pixels.insert(pixel); };
        void add(int x, int y) { add((Pixel){x, y}); };
        void remove(Pixel pixel) { _set_pixels.erase(pixel); };
        void remove(int x, int y) { remove((Pixel){x, y}); };
        void clear() { _set_pixels.clear(); };
        void bbox(Rect rect) { _bounding_box = rect; };

        Rect bbox()
        {
        /*    if (_set_pixels.size() == 0)
            {
                _bounding_box = {0, 0, 0, 0};
                return _bounding_box;
            }

            if ((_bounding_box.x >= 0) && (_bounding_box.y >= 0) && (_bounding_box.w > 0) && (_bounding_box.h > 0))
                return _bounding_box;
        */
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

            _bounding_box = {minx, miny, maxx - minx, maxy - miny};
            return _bounding_box;
        };
};

namespace HorizontalAreas
{
    enum Type: unsigned short { SPACE, SURFACE, UNDERGROUND, CAVERN, HELL };

    class Area: public PixelArray
    {
        protected:
            Type type;
        public:
            Area(): PixelArray() {};
            Area(Type _t): PixelArray(), type{_t} {};

            auto GetType(){ return type; }
    };

}

namespace Biomes {

    enum Type: unsigned short { JUNGLE, TUNDRA, FOREST, OCEAN };

    class Biome: public PixelArray 
    {
        protected:
            Type type;
        public:
            Biome(): PixelArray() {};
            Biome(Type _t): PixelArray(), type{_t} {};

            auto GetType(){ return type; }
   };


};

namespace MiniBiomes {
    
    enum Type: unsigned short { SURFACE_PART, HILL, HOLE, CABIN, FLOATING_ISLAND, SURFACE_TUNNEL, UNDERGROUND_TUNNEL, CASTLE }; 
    
    class Biome: public PixelArray 
    {
        protected:
            Type type;
        public:
            Biome(): PixelArray() {};
            Biome(Type _t): PixelArray(), type{_t} {};

            auto GetType(){ return type; }
   };
 
    class SurfacePart: public Biome 
    {
        protected:
            int sx {0}; // START X
            int ex {0}; // END X 
            int sy {0}; // START Y
            int ey {0}; // END Y
            int by {0}; // BASE Y

            SurfacePart* before {nullptr};
            SurfacePart* next {nullptr};
            std::vector<int> ypsilons;

        public:
            SurfacePart(int _sx, int _ex, int _sy, int _ey, int _by, SurfacePart* _before, SurfacePart* _next): Biome(SURFACE_PART),
            sx{_sx}, ex{_ex}, sy{_sy}, ey{_ey}, by{_by}, 
            before{_before}, next{_next} 
            {};

            auto StartX() { return sx; };
            auto EndX() { return ex; };
            auto StartY() { return sy; };
            auto EndY() { return ey; };
            auto BaseY() { return by; };

            auto Before() { return before; };
            void SetBefore(SurfacePart* _before) { before = _before; };
            auto Next() { return next; };
            void SetNext(SurfacePart* _next) { next = _next; };

            void AddY(int y) { ypsilons.push_back(y); };
            auto GetY(int x) { return ypsilons.at(x - sx); };
            auto GetYpsilons() { return ypsilons; };
   };

};

class Map {
    private:
        int _WIDTH;
        int _HEIGHT;

        float _COPPER_FREQUENCY = 0.5;
        float _COPPER_SIZE = 0.5;
        float _IRON_FREQUENCY = 0.5;
        float _IRON_SIZE = 0.5;
        float _SILVER_FREQUENCY = 0.5;
        float _SILVER_SIZE = 0.5;
        float _GOLD_FREQUENCY = 0.5;
        float _GOLD_SIZE = 0.5;

        float _HILLS_FREQUENCY = 0.5;
        float _HOLES_FREQUENCY = 0.5;
        float _CABINS_FREQUENCY = 0.5;
        float _ISLANDS_FREQUENCY = 0.5;

        std::atomic_bool _force_stop {false };
        std::atomic_bool _generating { false };
        std::atomic_int _thread_count { 0 };
        std::string _generation_message;
        std::mutex mutex;

        HorizontalAreas::Area _space {HorizontalAreas::SPACE};
        HorizontalAreas::Area _surface {HorizontalAreas::SURFACE};
        HorizontalAreas::Area _underground {HorizontalAreas::UNDERGROUND};
        HorizontalAreas::Area _cavern {HorizontalAreas::CAVERN};
        HorizontalAreas::Area _hell {HorizontalAreas::HELL};

        std::vector<std::unique_ptr<Biomes::Biome>> _biomes;
        std::vector<std::unique_ptr<MiniBiomes::Biome>> _mini_biomes;
        std::vector<std::unique_ptr<MiniBiomes::Biome>> _surface_mini_biomes;
        std::vector<std::string> _errors;

    public:
        Map(){ printf("Map();\n"); };

        auto Width()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _WIDTH;
        };
        
        auto Height()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _HEIGHT;
        };

        auto Width(int w)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (_WIDTH != w)
            {
                _WIDTH = w;
                return true;
            }
            return false;
        };

        auto Height(int h)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (_HEIGHT != h)
            {
                _HEIGHT = h;
                return true;
            }
            return false;
        };

        auto CopperFrequency()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _COPPER_FREQUENCY;
        };

        auto CopperFrequency(float fq)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (_COPPER_FREQUENCY != fq)
            {
                _COPPER_FREQUENCY = fq;
                return true;
            }
            return false;
        };

        auto CopperSize()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _COPPER_SIZE;
        };

        auto CopperSize(float s)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (_COPPER_SIZE != s)
            {
                _COPPER_SIZE = s;
                return true;
            }
            return false;
        };

        auto IronFrequency()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _IRON_FREQUENCY;
        };

        auto IronFrequency(float fq)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (_IRON_FREQUENCY != fq)
            {
                _IRON_FREQUENCY = fq;
                return true;
            }
            return false;
        };

        auto IronSize()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _IRON_SIZE;
        };

        auto IronSize(float s)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (_IRON_SIZE != s)
            {
                _IRON_SIZE = s;
                return true;
            }
            return false;
        };

        auto SilverFrequency()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _SILVER_FREQUENCY;
        };

        auto SilverFrequency(float fq)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (_SILVER_FREQUENCY != fq)
            {
                _SILVER_FREQUENCY = fq;
                return true;
            }
            return false;
        };

        auto SilverSize()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _SILVER_SIZE;
        };

        auto SilverSize(float s)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (_SILVER_SIZE != s)
            {
                _SILVER_SIZE = s;
                return true;
            }
            return false;
        };

        auto GoldFrequency()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _GOLD_FREQUENCY;
        };

        auto GoldFrequency(float fq)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (_GOLD_FREQUENCY != fq)
            {
                _GOLD_FREQUENCY = fq;
                return true;
            }
            return false;
        };

        auto GoldSize()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _GOLD_SIZE;
        };

        auto GoldSize(float s)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (_GOLD_SIZE != s)
            {
                _GOLD_SIZE = s;
                return true;
            }
            return false;
        };

        auto HillsFrequency()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _HILLS_FREQUENCY;
        };

        auto HillsFrequency(float fq)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (_HILLS_FREQUENCY != fq)
            {
                _HILLS_FREQUENCY = fq;
                return true;
            }
            return false;
        };

        auto HolesFrequency()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _HOLES_FREQUENCY;
        };

        auto HolesFrequency(float fq)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (_HOLES_FREQUENCY != fq)
            {
                _HOLES_FREQUENCY = fq;
                return true;
            }
            return false;
        };

        auto CabinsFrequency()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _CABINS_FREQUENCY;
        };

        auto CabinsFrequency(float fq)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (_CABINS_FREQUENCY != fq)
            {
                _CABINS_FREQUENCY = fq;
                return true;
            }
            return false;
        };

        auto IslandsFrequency()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _ISLANDS_FREQUENCY;
        };

        auto IslandsFrequency(float fq)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (_ISLANDS_FREQUENCY != fq)
            {
                _ISLANDS_FREQUENCY = fq;
                return true;
            }
            return false;
        };

        auto IsGenerating()
        {
            return _generating.load();
        };

        void SetGenerating(bool value)
        {
            _generating.store(value);
        };

        void ThreadIncrement()
        {
            _thread_count += 1;
        };

        void ThreadDecrement()
        {
            _thread_count -= 1;
        };

        auto ThreadCount()
        {
            return _thread_count.load();
        };
        
        void SetForceStop(bool value)
        {
            _force_stop.store(value);
        };

        auto ShouldForceStop()
        {
            return _force_stop.load();
        };

        void SetGenerationMessage(std::string msg)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            _generation_message = msg;
        };

        auto GetGenerationMessage()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _generation_message;
        };

        auto& Space()
        { 
            const std::lock_guard<std::mutex> lock(mutex);
            return _space; 
        };
        
        auto& Surface()
        { 
            const std::lock_guard<std::mutex> lock(mutex);
            return _surface; 
        };
        
        auto& Underground()
        { 
            const std::lock_guard<std::mutex> lock(mutex);
            return _underground; 
        };
        
        auto& Cavern()
        { 
            const std::lock_guard<std::mutex> lock(mutex);
            return _cavern; 
        };
        
        auto& Hell()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _hell; 
        };

        std::vector<std::reference_wrapper<HorizontalAreas::Area>> HorizontalAreas()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return {_space, _surface, _underground, _cavern, _hell};
        }

        Biomes::Biome& Biome(Biomes::Type type)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            _biomes.emplace_back(new Biomes::Biome(type));
            return *_biomes[_biomes.size() - 1];
        }

        Biomes::Biome* GetBiome(Biomes::Type type)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            for (auto& biome: _biomes)
                if (biome->GetType() == type)
                    return &*biome;
            return nullptr;
        }

        auto& Biomes()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _biomes;
        }

        auto& MiniBiomes()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _mini_biomes;
        }

        MiniBiomes::Biome& MiniBiome(MiniBiomes::Type type)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            _mini_biomes.emplace_back(new MiniBiomes::Biome(type));
            return *_mini_biomes[_mini_biomes.size() - 1];
        }

        auto& SurfaceMiniBiomes()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _surface_mini_biomes;
        }

        MiniBiomes::Biome& SurfaceMiniBiome(MiniBiomes::Type type)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            _surface_mini_biomes.emplace_back(new MiniBiomes::Biome(type));
            return *_surface_mini_biomes[_surface_mini_biomes.size() - 1];
        }
        
        MiniBiomes::SurfacePart& SurfacePart(int sx, int ex, int sy, int ey, int by)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            _surface_mini_biomes.emplace_back(new MiniBiomes::SurfacePart(sx, ex, sy, ey, by, nullptr, nullptr));
            return static_cast<MiniBiomes::SurfacePart&>(*_surface_mini_biomes[_surface_mini_biomes.size() - 1]);
        };

        void ClearStage0()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            _space.clear();
            _surface.clear();
            _underground.clear();
            _cavern.clear();
            _hell.clear();
        };

        void ClearStage1()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            _biomes.clear();
        };

        void ClearStage2()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            _mini_biomes.clear();
        };

        void ClearStage3()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            _surface_mini_biomes.clear();
        };

        void ClearAll()
        {
            ClearStage0();
            ClearStage1();
            ClearStage2();
        };

        void Error(std::string msg)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            _errors.push_back(msg);
        };

        auto Error()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _errors.back();
        };

        bool HasError()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _errors.size() > 0;
        };

        auto PopError()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            auto msg = _errors.back();
            _errors.pop_back();
            return msg;
        };
};

inline void FillWithRect(const Rect& rect, PixelArray& arr)
{
    for (int x = rect.x; x < rect.x + rect.w; ++x)
        for (int y = rect.y; y < rect.y + rect.h; ++y)
            arr.add(x, y);
    arr.bbox(rect);
}

inline void UnitedPixelArea(const PixelArray& pixels, int x, int y, PixelArray& fill)
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

inline void PixelsAroundCircle(int x, int y, float radius, PixelArray& array)
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

inline void PixelsAroundRect(int x, int y, int w, int h, PixelArray& array)
{
    int minx = x - w / 2;
    int maxx = x + w / 2;
    int miny = y - h / 2;
    int maxy = y + h / 2;

    for (int i = minx; i < maxx; i++)
        for (int j = miny; j < maxy; j++)
            array.add(i, j);
};

inline void PixelsOfRect(int x, int y, int w, int h, PixelArray& array)
{
   for (int i = x; i < x + w; i++)
        for (int j = y; j < y + h; j++)
            array.add(i, j);
};
