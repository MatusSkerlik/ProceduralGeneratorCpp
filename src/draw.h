#ifndef DRAW
#define DRAW

#ifndef RAYLIB_H
#include "raylib.h"
#endif

#ifndef UTILS
#include "utils.h"
#endif

#ifndef PCG
#include "pcg.h"
#endif


/************************************************
* 
* DRAW LOGIC
*
*************************************************/
#define C_DIRT              (Color){151, 107, 75, 255}
#define C_MUD               (Color){92, 68, 73, 255}
#define C_SAND            (Color){255, 218, 56, 255}
#define C_ICE               (Color){255, 255, 255, 255}
#define C_GRASS           (Color){28, 216, 94, 255}
#define C_JGRASS          (Color){143, 215, 29, 255}
#define C_WATER           (Color){65, 88, 151, 255}

#define C_SPACE           (Color){51, 102, 153, 255}
#define C_SURFACE         (Color){155, 209, 255, 255}
#define C_UNDERGROUND     (Color){151, 107, 75, 255}
#define C_STONE          (Color){128, 128, 128, 255}

inline void DrawHorizontal(Map& map)
{
    for (auto* area: map.HorizontalAreas())
    {
        Rect rect = area->bbox();
        Color color;
        switch (area->GetType())
        {
            case HorizontalAreas::SPACE:
                color = C_SURFACE;
                break;
            case HorizontalAreas::SURFACE:
                color = C_SURFACE;
                break;
            case HorizontalAreas::UNDERGROUND:
                color = C_STONE;
                break;
            case HorizontalAreas::CAVERN:
                color = C_STONE;
                break;
            default:
                break;
        }
        DrawRectangle(rect.x, rect.y, rect.w, rect.h, color);
    }
};

inline void DrawBiomes(Map& map)
{
    for (auto& biome: map.Biomes())
    {
        for (auto& p: *biome)
        {
            switch (biome->GetType())
            {
                case Biomes::TUNDRA:
                    DrawPixel(p.x, p.y, (Color){255, 255, 255, 32});
                    break;
                case Biomes::JUNGLE:
                    DrawPixel(p.x, p.y, (Color){92, 68, 73, 32});
                    break;
                case Biomes::FOREST:
                    DrawPixel(p.x, p.y, (Color){0, 255, 0, 32});
                    break;
                case Biomes::OCEAN_LEFT:
                    DrawPixel(p.x, p.y, (Color){0, 0, 255, 32});
                    break;
                case Biomes::OCEAN_RIGHT:
                    DrawPixel(p.x, p.y, (Color){0, 0, 255, 32});
                    break;
                default:
                    break;
            };
        }
    }
};

inline void DrawStructures(Map& map)
{
    for (auto& biome: map.Structures())
    {
        for (auto& p: *biome)
        {
            switch (biome->GetType())
            {
                case Structures::HILL:
                    DrawPixel(p.x, p.y, C_UNDERGROUND);
                    break;
                case Structures::HOLE:
                    DrawPixel(p.x, p.y, C_UNDERGROUND);
                    break;
                case Structures::FLOATING_ISLAND:
                    DrawPixel(p.x, p.y, YELLOW);
                    break;
                case Structures::CABIN:
                    DrawPixel(p.x, p.y, ORANGE);
                    break;
                case Structures::CASTLE:
                    DrawPixel(p.x, p.y, RED);
                    break;
                default:
                    break;
            };
        }
    }
};

inline void DrawSurfaceStructures(Map& map)
{
    for (auto& biome: map.SurfaceStructures())
    {
        for (auto& p: *biome)
        {
            switch (biome->GetType())
            {
                case Structures::SURFACE_PART:
                    DrawPixel(p.x, p.y, C_UNDERGROUND);
                    break;
                default:
                    break;
            };
        }
    }
};

inline void DrawSurfaceBg(Map& map)
{
    auto Surface = map.Surface();
    auto surface_rect = Surface.bbox();
    auto* part = map.GetSurfaceBegin();

    while (part != nullptr)
    {
        for (auto x = part->StartX(); x < part->EndX() + 1; ++x)
        {
            auto y = part->GetY(x) + 1;

            for (auto _y = y; _y < surface_rect.y + surface_rect.h; ++_y)
            {
                DrawPixel(x, _y, (Color){84, 57, 42, 255});
            }

        }
        part = part->Next();
    }
};

inline void DrawSurfaceDebug(Map& map)
{
    auto surface_rect = map.Surface().bbox();
    for (auto x = surface_rect.x; x < surface_rect.x + surface_rect.w; ++x)
    {
        for (auto y = 0; y < surface_rect.y + surface_rect.h; ++y)
        {
            auto meta = map.GetMetadata({x, y});
            if (meta.structure != nullptr)
            {
                DrawPixel(x, y, (Color){255, 0, 0, 16});
            }
        }
    }
};

inline void DrawSurface(Map& map)
{
    auto surface_rect = map.Surface().bbox();
    for (auto x = surface_rect.x; x <= surface_rect.x + surface_rect.w; ++x)
    {
        for (auto y = 0; y <= surface_rect.y + surface_rect.h + 200; ++y)
        {
            auto meta = map.GetMetadata({x, y});
            if (meta.surface_structure != nullptr)
            {

                // BIOME RELATED
                if (meta.biome != nullptr)
                {
                    if (meta.biome->GetType() == Biomes::JUNGLE)
                        if (meta.surface_structure->GetType() == Structures::GRASS)
                            DrawPixel(x, y, C_JGRASS);
                        else
                            DrawPixel(x, y, C_MUD);
                    else if (meta.biome->GetType() == Biomes::TUNDRA)
                        DrawPixel(x, y, C_ICE);
                    else
                        if (meta.surface_structure->GetType() == Structures::GRASS)
                            DrawPixel(x, y, C_GRASS);
                        else
                            DrawPixel(x, y, C_DIRT);
                }
                else
                {
                    DrawPixel(x, y, C_DIRT);
                }
                
                // FOR EVERY SURFACE STRUCTURE
                if (meta.surface_structure->GetType() == Structures::TREE)
                    DrawPixel(x, y, (Color){191, 143, 111, 255});
                if (meta.surface_structure->GetType() == Structures::WATER)
                    DrawPixel(x, y, C_WATER);
                if (meta.surface_structure->GetType() == Structures::SAND)
                    DrawPixel(x, y, C_SAND);
                if (meta.surface_structure->GetType() == Structures::ORE)
                    DrawPixel(x, y, (Color){150, 67, 22, 255});
                // DEBUG
                /*
                if (meta.surface_structure->GetType() == Structures::TRANSITION)
                    DrawPixel(x, y, RED);
                if (meta.surface_structure->GetType() == Structures::HILL)
                    DrawPixel(x, y, BLUE);
                */
            }
        }
    }

    // DEBUG SURFACE Y
    /*
    for (auto x = surface_rect.x; x <= surface_rect.x + surface_rect.w; ++x)
    {
        auto* part = map.GetSurfacePart(x);
        if (part != nullptr) DrawPixel(x, part->GetY(x), ORANGE);
    }
    */
};


inline void DrawUnderground(Map& map)
{
    auto underground_rect = map.Underground().bbox();
    auto cavern_rect = map.Cavern().bbox();
    for (auto x = cavern_rect.x; x <= cavern_rect.x + cavern_rect.w; ++x)
    {
        for (auto y = underground_rect.y; y < underground_rect.y + + underground_rect.h + cavern_rect.h; ++y)
        {
            auto meta = map.GetMetadata({x, y});
                // BIOME RELATED
            if (meta.biome != nullptr)
            {
                if (meta.biome->GetType() == Biomes::JUNGLE)
                        DrawPixel(x, y, C_MUD);
                else if (meta.biome->GetType() == Biomes::TUNDRA)
                    DrawPixel(x, y, C_ICE);
                else
                    DrawPixel(x, y, C_STONE);

                if (meta.surface_structure != nullptr)
                {
                    // FOR EVERY UNDERGROUND STRUCTURE IN BIOME
                    if (meta.surface_structure->GetType() == Structures::CAVE)
                    {
                        if (y < cavern_rect.y)
                            DrawPixel(x, y, (Color){84, 57, 42, 255});
                        else 
                            DrawPixel(x, y, (Color){72, 64, 57, 255});
                    }
                }
            }
        }
    }
};


#endif // DRAW
