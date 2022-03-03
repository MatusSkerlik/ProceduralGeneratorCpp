#include <functional>
#include <stdio.h>
#include <mutex>
#include <memory>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <tuple>
#include <limits>
#include <atomic>

class Map;
namespace Structures { enum Type: unsigned short; class Structure; };
namespace Biomes { class Biome; };

typedef struct Rect 
{
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

typedef struct Pixel 
{
    int x;
    int y;
    Pixel(int _x, int _y): x{_x}, y{_y} {};
    const bool operator==(const Pixel& p) const { return x == p.x && y == p.y; }
} Pixel;

struct PixelHash
{
    inline size_t operator()(const Pixel& p) const 
    { 
        return p.y * 4200 + p.x;  
    };    
};

struct PixelEqual 
{
    inline bool operator()( const Pixel& left, const Pixel& right) const { return left.x == right.x && left.y == right.y; };    
};

typedef struct PixelMetadata
{
    Biomes::Biome* biome { nullptr };
    Structures::Structure* owner { nullptr };
} PixelMetadata;


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
        std::unordered_set<Pixel, PixelHash, PixelEqual> _set_pixels;
    public:
        PixelArray(){};
        
        PixelArray(const PixelArray& arr)
        {
            _set_pixels = arr._set_pixels;
        }

        PixelArray(PixelArray&& arr)
        {
            _set_pixels = std::move(arr._set_pixels);
        }

        ~PixelArray(){};

        auto begin() const { return this->_set_pixels.begin(); };
        auto end() const { return this->_set_pixels.end(); };
        auto size() const { return this->_set_pixels.size(); };
        auto contains(Pixel pixel) const { return (bool)_set_pixels.count(pixel); };
        virtual void add(Pixel pixel) { _set_pixels.insert(pixel); };
        virtual void add(int x, int y) { add((Pixel){x, y}); };
        virtual void remove(Pixel pixel) { _set_pixels.erase(pixel); };
        virtual void remove(int x, int y) { remove((Pixel){x, y}); };
        virtual void clear() { _set_pixels.clear(); };

        Rect bbox()
        {
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

            return {minx, miny, maxx - minx, maxy - miny};
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

    enum Type: unsigned short { 
        FOREST = 1, 
        JUNGLE = 2, 
        TUNDRA = 4, 
        OCEAN = 8 
    };

    class Biome: public PixelArray 
    {
        protected:
            Map& map;
            Type type;

        public:
            Biome(Map& _map): PixelArray(), map{_map} {};
            Biome(Map& _map, Type _t): PixelArray(), map{_map}, type{_t} {};
            ~Biome() { clear(); };

            auto GetType(){ return type; }
            void add(Pixel pixel) override;
            void remove(Pixel pixel) override;
            void clear() override;
   };
};

namespace Structures {
    
    enum Type: unsigned short { 
        SURFACE_PART = 1, 
        HILL = 2, 
        HOLE = 4, 
        CABIN = 8, 
        CLIFF = 16, 
        TRANSITION = 32, 
        SURFACE_TUNNEL = 64, 
        FLOATING_ISLAND = 128, 
        UNDERGROUND_TUNNEL = 256, 
        GRASS = 512, 
        CASTLE = 1024,
        TREE = 2048
    }; 
    
    class Structure: public PixelArray 
    {
        protected:
            Map& map;
            Type type;

        public:
            Structure(Map& _map): PixelArray(), map{_map} {};
            Structure(Map& _map, Type _t): PixelArray(), map{_map}, type{_t} {};
            ~Structure(){ clear(); };
            
            auto GetType(){ return type; }
            void add(Pixel pixel) override;
            void remove(Pixel pixel) override;
            void clear() override;
   };
 
    class SurfacePart: public Structure 
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
            SurfacePart(Map& _map, int _sx, int _ex, int _sy, int _ey, int _by, SurfacePart* _before, SurfacePart* _next): Structure(_map, SURFACE_PART),
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

namespace Special
{
    class Eraser: public PixelArray
    {
        private:
            Map& map;

        public:
            Eraser(Map& _map): PixelArray(), map{_map} {};
            void Erase(Pixel pixel);
    };
};

class Map {
    private:
        int _WIDTH {4200};
        int _HEIGHT {1200};

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

        std::atomic_bool _initialized { false };
        std::atomic_bool _force_stop {false };
        std::atomic_bool _generating { false };
        std::atomic_int _thread_count { 0 };
        std::string _generation_message;

        HorizontalAreas::Area _space {HorizontalAreas::SPACE};
        HorizontalAreas::Area _surface {HorizontalAreas::SURFACE};
        HorizontalAreas::Area _underground {HorizontalAreas::UNDERGROUND};
        HorizontalAreas::Area _cavern {HorizontalAreas::CAVERN};
        HorizontalAreas::Area _hell {HorizontalAreas::HELL};

        std::vector<std::unique_ptr<Biomes::Biome>> _biomes;
        std::vector<std::unique_ptr<Structures::Structure>> _structures;
        std::vector<std::unique_ptr<Structures::Structure>> _surface_structures;
        std::vector<std::unique_ptr<Special::Eraser>> _erasers;
        std::vector<std::string> _errors;

        std::unordered_map<Pixel, PixelMetadata, PixelHash, PixelEqual> _pixel_map;

    public:
        std::mutex mutex;

        Map(){};
        ~Map(){ ClearAll(); };

        void Init()
        {
            for (auto x = 0; x < this->Width(); ++x)
                for (auto y = 0; y < this->Height(); ++y) 
                    _pixel_map.emplace(std::make_pair((Pixel){x, y}, PixelMetadata()));
            _initialized = true;
        };

        bool IsInitialized()
        {
            return _initialized;
        }

        int Width()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _WIDTH;
        };
        
        int Height()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _HEIGHT;
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

        std::vector<HorizontalAreas::Area*> HorizontalAreas()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return {&_space, &_surface, &_underground, &_cavern, &_hell};
        }

        auto& Biome(Biomes::Type type)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            _biomes.emplace_back(new Biomes::Biome(*this, type));
            return *_biomes.back();
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

        auto& Structures()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _structures;
        }

        auto& Structure(Structures::Type type)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            _structures.emplace_back(new Structures::Structure(*this, type));
            return *_structures.back();
        }

        auto GetStructures(Structures::Type type)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            std::vector<Structures::Structure*> structures;
            for (auto& biome: _structures)
                if (biome->GetType() == type)
                    structures.push_back(biome.get());
            return structures;
        }

        auto& SurfaceStructures()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _surface_structures;
        }

        auto& SurfaceStructure(Structures::Type type)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            _surface_structures.emplace_back(new Structures::Structure(*this, type));
            return *_surface_structures.back();
        }
        
        auto& SurfacePart(int sx, int ex, int sy, int ey, int by)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            _surface_structures.emplace_back(new Structures::SurfacePart(*this, sx, ex, sy, ey, by, nullptr, nullptr));
            return static_cast<Structures::SurfacePart&>(*_surface_structures.back());
        };

        Structures::SurfacePart* GetRandomSurface()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            for (auto& biome: _surface_structures)
                if (biome->GetType() == Structures::SURFACE_PART)
                    return static_cast<Structures::SurfacePart*>(biome.get());
            return nullptr;
        };

        Structures::SurfacePart* GetSurfaceBegin()
        {
            Structures::SurfacePart* surface_part = GetRandomSurface();
            if (surface_part == nullptr) return nullptr;

            while(surface_part->Before() != nullptr)
                surface_part = surface_part->Before();
            return surface_part;
        };

        Structures::SurfacePart* GetSurfaceEnd()
        {
            Structures::SurfacePart* surface_part = GetRandomSurface();
            if (surface_part == nullptr) return nullptr;

            while (surface_part->Next() != nullptr)
                surface_part = surface_part->Next();
            return surface_part;
        };

        Structures::SurfacePart* GetSurfacePart(int sx)
        {
            Structures::SurfacePart* s_part = GetSurfaceBegin();
            if (s_part == nullptr) return nullptr;

            while (!(s_part->StartX() <= sx) || !(s_part->EndX() > sx)) { s_part = s_part->Next(); }
            return s_part;
        };

        auto& Eraser()
        {
            _erasers.emplace_back(new Special::Eraser(*this));
            return *_erasers.back();
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
            _structures.clear();
        };

        void ClearStage3()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            _surface_structures.clear();
        };

        void ClearAll()
        {
            ClearStage0();
            ClearStage1();
            ClearStage2();
            ClearStage3();

            _errors.clear();
            _erasers.clear();
            _pixel_map.clear();
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

        auto GetMetadata(Pixel pixel)
        {
            return _pixel_map.at(pixel);
        };

        void SetMetadata(Pixel p, PixelMetadata meta)
        {
            _pixel_map[p] = meta;
        };
};

inline void Biomes::Biome::add(Pixel pixel)
{
    auto meta = map.GetMetadata(pixel);
    meta.biome = this;
    map.SetMetadata(pixel, meta);
    PixelArray::add(pixel);
};

inline void Structures::Structure::add(Pixel pixel)
{
    auto meta = map.GetMetadata(pixel);
    meta.owner = this;
    map.SetMetadata(pixel, meta);
    PixelArray::add(pixel);
};

inline void Biomes::Biome::remove(Pixel pixel)
{
    auto meta = map.GetMetadata(pixel);
    meta.biome = nullptr;
    map.SetMetadata(pixel, meta);
    PixelArray::remove(pixel);
};

inline void Structures::Structure::remove(Pixel pixel)
{
    auto meta = map.GetMetadata(pixel);
    meta.owner = nullptr;
    map.SetMetadata(pixel, meta);
    PixelArray::remove(pixel);
};

inline void Biomes::Biome::clear()
{
    for (auto& p: _set_pixels)
    {
        auto meta = map.GetMetadata(p);
        meta.biome = nullptr;
        map.SetMetadata(p, meta);
    };

    PixelArray::clear();
};

inline void Structures::Structure::clear()
{
    for (auto& p: _set_pixels)
    {
        auto meta = map.GetMetadata(p);
        meta.owner = nullptr;
        map.SetMetadata(p, meta);
    };

    PixelArray::clear();
};

inline void Special::Eraser::Erase(Pixel pixel)
{
    auto meta = map.GetMetadata(pixel);
    if (meta.owner != nullptr)
    {
        meta.owner->remove(pixel);
    };
    PixelArray::add(pixel);
}

inline void FillWithRect(const Rect& rect, PixelArray& arr)
{
    for (int x = rect.x; x < rect.x + rect.w; ++x)
        for (int y = rect.y; y < rect.y + rect.h; ++y)
            arr.add(x, y);
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
