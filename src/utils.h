#ifndef UTILS
#define UTILS

#include <stdio.h>
#include <functional>
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
#include <bitset>

class Map;
namespace Structures { 
    class DefinedStructure; 
    class GeneratedStructure;
};
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
    Structures::DefinedStructure* defined_structure { nullptr };
    Structures::GeneratedStructure* generated_structure { nullptr };
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

        Rect bbox() const 
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
    enum Type: unsigned short { SPACE, SURFACE, UNDERGROUND, CAVERN };

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

    const unsigned long FOREST              = 1 << 0; 
    const unsigned long TUNDRA              = 1 << 1; 
    const unsigned long JUNGLE              = 1 << 2; 
    const unsigned long OCEAN_LEFT          = 1 << 3; 
    const unsigned long OCEAN_RIGHT         = 1 << 4; 
    const unsigned long OCEAN_DESERT_LEFT   = 1 << 5; 
    const unsigned long OCEAN_DESERT_RIGHT  = 1 << 6; 

    class Biome: public PixelArray 
    {
        protected:
            Map& map;
            std::bitset<32> type;

        public:
            Biome(Map& _map): PixelArray(), map{_map} {};
            Biome(Map& _map, unsigned long _t): PixelArray(), map{_map}, type{_t} {};
            ~Biome() { clear(); };

            auto GetType() const { return type.to_ulong(); }
            void add(Pixel pixel) override;
            void remove(Pixel pixel) override;
            void clear() override;
   };
};

namespace Structures {
    
    const unsigned long SURFACE_PART         = 1 << 0; 
    const unsigned long HILL                 = 1 << 1;   
    const unsigned long HOLE                 = 1 << 2; 
    const unsigned long CABIN                = 1 << 3;
    const unsigned long CLIFF                = 1 << 4;
    const unsigned long TRANSITION           = 1 << 5;
    const unsigned long SURFACE_TUNNEL       = 1 << 6;
    const unsigned long FLOATING_ISLAND      = 1 << 7;
    const unsigned long UNDERGROUND_TUNNEL   = 1 << 8;
    const unsigned long GRASS                = 1 << 9;
    const unsigned long CASTLE               = 1 << 10;
    const unsigned long TREE                 = 1 << 11;
    const unsigned long CHASM                = 1 << 12;
    const unsigned long SAND                 = 1 << 13;
    const unsigned long WATER                = 1 << 14;
    const unsigned long CAVE                 = 1 << 15;
    const unsigned long COPPER_ORE           = 1 << 16;
    const unsigned long IRON_ORE             = 1 << 17;
    const unsigned long SILVER_ORE           = 1 << 18;
    const unsigned long GOLD_ORE             = 1 << 19;
    const unsigned long STONE                = 1 << 20;
    const unsigned long DIRT                 = 1 << 21;
    
    class DefinedStructure: public PixelArray 
    {
        protected:
            Map& map;
            std::bitset<32> type;

        public:
            DefinedStructure(Map& _map): PixelArray(), map{_map} {};
            DefinedStructure(Map& _map, unsigned long _t): PixelArray(), map{_map}, type{_t} {};
            ~DefinedStructure(){ clear(); };
            
            auto GetType() const { return type.to_ulong(); }
            void add(Pixel pixel) override;
            void remove(Pixel pixel) override;
            void clear() override;
    };

    class GeneratedStructure: public PixelArray 
    {
        protected:
            Map& map;
            std::bitset<32> type;

        public:
            GeneratedStructure(Map& _map): PixelArray(), map{_map} {};
            GeneratedStructure(Map& _map, unsigned long _t): PixelArray(), map{_map}, type{_t} {};
            ~GeneratedStructure(){ clear(); };
            
            auto GetType() const { return type.to_ulong(); }
            void add(Pixel pixel) override;
            void remove(Pixel pixel) override;
            void clear() override;
    };

 
    class SurfacePart: public GeneratedStructure 
    {
        protected:
            int sx {0}; // START X
            int ex {0}; // END X 

            SurfacePart* before {nullptr};
            SurfacePart* next {nullptr};
            std::vector<int> ypsilons;

        public:
            SurfacePart(Map& _map, int _sx, int _ex, SurfacePart* _before, SurfacePart* _next): GeneratedStructure(_map, SURFACE_PART),
            sx{_sx}, ex{_ex}, 
            before{_before}, next{_next} 
            {};

            auto StartX() const { return sx; };
            auto EndX() const { return ex; };
            auto StartY() const { return GetY(sx); };
            auto EndY() const { return GetY(ex); };

            auto Before() const { return before; };
            void SetBefore(SurfacePart* _before) { before = _before; };
            auto Next() const { return next; };
            void SetNext(SurfacePart* _next) { next = _next; };

            void AddY(int y) { ypsilons.push_back(y); };
            void SetY(int x, int y) { ypsilons[x - sx] = y; };
            int GetY(int x) const { return ypsilons.at(x - sx); };
            auto GetYpsilons() const { return ypsilons; };
   };
};

class Map {
    private:
        int _WIDTH {4200};
        int _HEIGHT {1200};

        float _COPPER_FREQUENCY = 0.0;
        float _COPPER_SIZE = 0.0;
        float _IRON_FREQUENCY = 0.0;
        float _IRON_SIZE = 0.0;
        float _SILVER_FREQUENCY = 0.0;
        float _SILVER_SIZE = 0.0;
        float _GOLD_FREQUENCY = 0.0;
        float _GOLD_SIZE = 0.0;

        float _HILLS_FREQUENCY = 0.5;
        float _HOLES_FREQUENCY = 0.2;
        float _CABINS_FREQUENCY = 0;
        float _ISLANDS_FREQUENCY = 0.5;
        float _CHASM_FREQUENCY = 0.0;
        float _TREE_FREQUENCY = 0.4;
        float _LAKE_FREQUENCY = 0.5;

        float _CAVE_FREQUENCY = 0.5;
        float _CAVE_STROKE_SIZE = 0.5;
        float _CAVE_POINTS_SIZE = 0.5;
        float _CAVE_CURVNESS = 0.5;
        
        float _SURFACE_PARTS_COUNT = 0.5;
        float _SURFACE_PARTS_FREQUENCY = 0.5;
        float _SURFACE_PARTS_OCTAVES = 0.25;

        std::atomic_bool _initialized { false };
        std::atomic_bool _force_stop {false };
        std::atomic_bool _generating { false };
        std::atomic_int _thread_count { 0 };
        std::string _generation_message;

        HorizontalAreas::Area _space {HorizontalAreas::SPACE};
        HorizontalAreas::Area _surface {HorizontalAreas::SURFACE};
        HorizontalAreas::Area _underground {HorizontalAreas::UNDERGROUND};
        HorizontalAreas::Area _cavern {HorizontalAreas::CAVERN};

        std::vector<std::unique_ptr<Biomes::Biome>> _biomes;
        std::vector<std::unique_ptr<Structures::DefinedStructure>> _structures;
        std::vector<std::unique_ptr<Structures::GeneratedStructure>> _generated_structures;
        std::vector<std::unique_ptr<Structures::GeneratedStructure>> _underground_structures;
        std::vector<std::string> _errors;

        std::unordered_map<Pixel, PixelMetadata, PixelHash, PixelEqual> _pixel_map;

    public:
        std::mutex mutex;

        Map(){};
        ~Map(){ ClearAll(); };

        void Init()
        {
            for (auto x = 0; x <= this->Width(); ++x)
                for (auto y = 0; y <= this->Height(); ++y) 
                    _pixel_map.insert({{x, y}, PixelMetadata()});
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

        auto ChasmFrequency()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _CHASM_FREQUENCY;
        };

        auto ChasmFrequency(float fq)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (_CHASM_FREQUENCY!= fq)
            {
                _CHASM_FREQUENCY = fq;
                return true;
            }
            return false;
        };

        auto TreeFrequency()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _TREE_FREQUENCY;
        };

        auto TreeFrequency(float fq)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (_TREE_FREQUENCY != fq)
            {
                _TREE_FREQUENCY = fq;
                return true;
            }
            return false;
        };

        auto LakeFrequency()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _LAKE_FREQUENCY;
        };

        auto LakeFrequency(float fq)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (_LAKE_FREQUENCY != fq)
            {
                _LAKE_FREQUENCY = fq;
                return true;
            }
            return false;
        };

        auto SurfacePartsCount()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _SURFACE_PARTS_COUNT;
        };

        auto SurfacePartsCount(float c)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (_SURFACE_PARTS_COUNT != c)
            {
                _SURFACE_PARTS_COUNT = c;
                return true;
            }
            return false;
        };

        auto SurfacePartsFrequency()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _SURFACE_PARTS_FREQUENCY;
        };

        auto SurfacePartsFrequency(float fq)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (_SURFACE_PARTS_FREQUENCY != fq)
            {
                _SURFACE_PARTS_FREQUENCY = fq;
                return true;
            }
            return false;
        };

        auto SurfacePartsOctaves()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _SURFACE_PARTS_OCTAVES;
        };

        auto SurfacePartsOctaves(float o)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (_SURFACE_PARTS_OCTAVES != o)
            {
                _SURFACE_PARTS_OCTAVES = o;
                return true;
            }
            return false;
        };

        auto CaveFrequency()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _CAVE_FREQUENCY;
        };

        auto CaveFrequency(float o)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (_CAVE_FREQUENCY != o)
            {
                _CAVE_FREQUENCY = o;
                return true;
            }
            return false;
        };
        
        auto CavePointsSize()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _CAVE_POINTS_SIZE;
        };

        auto CavePointsSize(float o)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (_CAVE_POINTS_SIZE != o)
            {
                _CAVE_POINTS_SIZE = o;
                return true;
            }
            return false;
        };

        auto CaveStrokeSize()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _CAVE_STROKE_SIZE;
        };

        auto CaveStrokeSize(float o)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (_CAVE_STROKE_SIZE != o)
            {
                _CAVE_STROKE_SIZE = o;
                return true;
            }
            return false;
        };

        auto CaveCurvness()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _CAVE_CURVNESS;
        };

        auto CaveCurvness(float o)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (_CAVE_CURVNESS != o)
            {
                _CAVE_CURVNESS = o;
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
        
        std::vector<HorizontalAreas::Area*> HorizontalAreas()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return {&_space, &_surface, &_underground, &_cavern};
        }

        auto& Biome(unsigned long type)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            _biomes.emplace_back(new Biomes::Biome(*this, type));
            return *_biomes.back();
        }

        Biomes::Biome* GetBiome(unsigned long type)
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

        auto& DefinedStructures()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _structures;
        }

        auto& DefinedStructure(unsigned long type)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            _structures.emplace_back(new Structures::DefinedStructure(*this, type));
            return *_structures.back();
        }

        auto GetDefinedStructures(unsigned long type)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            std::vector<Structures::DefinedStructure*> structures;
            for (auto& structure: _structures)
                if (structure->GetType() == type)
                    structures.push_back(structure.get());
            return structures;
        }

        auto& GeneratedStructures()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _generated_structures;
        }

        auto& GeneratedStructure(unsigned long type)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            _generated_structures.emplace_back(new Structures::GeneratedStructure(*this, type));
            return *_generated_structures.back();
        }

        auto GetGeneratedStructures(unsigned long type)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            std::vector<Structures::GeneratedStructure*> structures;
            for (auto& structure: _generated_structures)
                if (structure->GetType() == type)
                    structures.push_back(structure.get());
            return structures;
        }

        auto& UndergroundStructures()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            return _generated_structures;
        }

        auto& UndergroundStructure(unsigned long type)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            _underground_structures.emplace_back(new Structures::GeneratedStructure(*this, type));
            return *_underground_structures.back();
        }

        auto GetUndergroundStructures(unsigned long type)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            std::vector<Structures::GeneratedStructure*> structures;
            for (auto& structure: _underground_structures)
                if (structure->GetType() == type)
                    structures.push_back(structure.get());
            return structures;
        }

        auto& SurfacePart(int sx, int ex)
        {
            const std::lock_guard<std::mutex> lock(mutex);
            _generated_structures.emplace_back(new Structures::SurfacePart(*this, sx, ex, nullptr, nullptr));
            return static_cast<Structures::SurfacePart&>(*_generated_structures.back());
        };

        Structures::SurfacePart* GetRandomSurface()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            for (auto& biome: _generated_structures)
                if (biome->GetType() == Structures::SURFACE_PART)
                    return static_cast<Structures::SurfacePart*>(biome.get());
            return nullptr;
        };

        Structures::SurfacePart* GetSurfaceBegin()
        {
            Structures::SurfacePart* surface_part = GetRandomSurface();
            if (surface_part == nullptr) return nullptr;
            while(surface_part->Before() != nullptr) surface_part = surface_part->Before();
            return surface_part;
        };

        Structures::SurfacePart* GetSurfaceEnd()
        {
            Structures::SurfacePart* surface_part = GetRandomSurface();
            if (surface_part == nullptr) return nullptr;
            while (surface_part->Next() != nullptr) surface_part = surface_part->Next();
            return surface_part;
        };

        Structures::SurfacePart* GetSurfacePart(int x)
        {
            Structures::SurfacePart* s_part = GetSurfaceBegin();
            while ((s_part != nullptr) && !((s_part->StartX() <= x) && (s_part->EndX() >= x))) { s_part = s_part->Next(); }
            return s_part;
        };

        auto GetSurfaceY(int x)
        {
            return GetSurfacePart(x)->GetY(x);
        };

        void ClearStage0()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            _space.clear();
            _surface.clear();
            _underground.clear();
            _cavern.clear();
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
            _generated_structures.clear();
        };

        void ClearStage4()
        {
            const std::lock_guard<std::mutex> lock(mutex);
            _underground_structures.clear();
        };

        void ClearAll()
        {
            ClearStage0();
            ClearStage1();
            ClearStage2();
            ClearStage3();

            _errors.clear();
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

inline void Biomes::Biome::remove(Pixel pixel)
{
    auto meta = map.GetMetadata(pixel);
    meta.biome = nullptr;
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

inline void Structures::DefinedStructure::add(Pixel pixel)
{
    auto meta = map.GetMetadata(pixel);
    meta.defined_structure = this;
    map.SetMetadata(pixel, meta);
    PixelArray::add(pixel);
};

inline void Structures::DefinedStructure::remove(Pixel pixel)
{
    auto meta = map.GetMetadata(pixel);
    meta.defined_structure = nullptr;
    map.SetMetadata(pixel, meta);
    PixelArray::remove(pixel);
};


inline void Structures::DefinedStructure::clear()
{
    for (auto& p: _set_pixels)
    {
        auto meta = map.GetMetadata(p);
        meta.defined_structure = nullptr;
        map.SetMetadata(p, meta);
    };

    PixelArray::clear();
};

inline void Structures::GeneratedStructure::add(Pixel pixel)
{
    auto meta = map.GetMetadata(pixel);
    meta.generated_structure = this;
    map.SetMetadata(pixel, meta);
    PixelArray::add(pixel);
};

inline void Structures::GeneratedStructure::remove(Pixel pixel)
{
    auto meta = map.GetMetadata(pixel);
    meta.generated_structure = nullptr;
    map.SetMetadata(pixel, meta);
    PixelArray::remove(pixel);
};


inline void Structures::GeneratedStructure::clear()
{
    for (auto& p: _set_pixels)
    {
        auto meta = map.GetMetadata(p);
        meta.generated_structure = nullptr;
        map.SetMetadata(p, meta);
    };

    PixelArray::clear();
};

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

    for (int i = minx; i <= maxx; ++i)
    {
        for (int j = miny; j <= maxy; ++j)
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

    for (int i = minx; i <= maxx; ++i)
        for (int j = miny; j <= maxy; ++j)
            array.add(i, j);
};

inline void PixelsOfRect(int x, int y, int w, int h, PixelArray& array)
{
   for (int i = x; i <= x + w; ++i)
        for (int j = y; j <= y + h; ++j)
            array.add(i, j);
};

inline void PixelsOfRect(const Rect& rect, PixelArray& array)
{
    PixelsOfRect(rect.x, rect.y, rect.w, rect.h, array);
};

#endif
