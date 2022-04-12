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
#define C_CLAY              (Color){146, 81, 68, 255}
#define C_SILT              (Color){106, 107, 118, 255}
#define C_SAND              (Color){255, 218, 56, 255}
#define C_SNOW              (Color){255, 255, 255, 255}
#define C_ICE               (Color){144, 195, 232, 255}
#define C_GRASS             (Color){28, 216, 94, 255}
#define C_JGRASS            (Color){143, 215, 29, 255}
#define C_WATER             (Color){65, 88, 151, 255}
#define C_LAVA              (Color){218, 30, 4, 255}
#define C_GOLD              (Color){185, 164, 23, 255}
#define C_SILVER            (Color){185, 194, 195, 255}
#define C_IRON              (Color){62, 82, 114, 255}
#define C_COPPER            (Color){150, 67, 22, 255} 

#define C_SPACE             (Color){51, 102, 153, 255}
#define C_SURFACE           (Color){155, 209, 255, 255}
#define C_UNDERGROUND       (Color){151, 107, 75, 255}
#define C_STONE             (Color){128, 128, 128, 255}

#define C_CAVE_BG_U         (Color){84, 57, 42, 255}
#define C_CAVE_BG_C         (Color){72, 64, 57, 255}

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
                color = C_DIRT;
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
            if (meta.defined_structure != nullptr)
            {
                DrawPixel(x, y, (Color){255, 0, 0, 16});
            }
        }
    }
};

inline void Draw(Map& map)
{
    auto width = map.Width();
    auto height = map.Height();
    //auto surface_rect = map.Surface().bbox();
    auto underground_rect = map.Underground().bbox();
    auto cavern_rect = map.Cavern().bbox();

    for (auto x = 0; x < width; ++x)
    {
        for (auto y = 0; y < height; ++y) 
        {
            auto meta = map.GetMetadata({x, y});
            if (meta.generated_structure != nullptr && meta.biome != nullptr)
            {
                if (meta.biome->GetType() == Biomes::JUNGLE)
                {
                    if (meta.generated_structure->GetType() == Structures::S_MATERIAL_BASE)
                        DrawPixel(x, y, C_STONE);
                    else if (meta.generated_structure->GetType() == Structures::S_MATERIAL_SEC)
                        DrawPixel(x, y, C_CLAY);
                    else if (meta.generated_structure->GetType() == Structures::U_MATERIAL_BASE)
                        DrawPixel(x, y, C_STONE);
                    else if (meta.generated_structure->GetType() == Structures::U_MATERIAL_SEC)
                        DrawPixel(x, y, C_CLAY);
                    else if (meta.generated_structure->GetType() == Structures::C_MATERIAL_BASE)
                        DrawPixel(x, y, C_STONE);
                    else if (meta.generated_structure->GetType() == Structures::C_MATERIAL_SEC)
                        DrawPixel(x, y, C_SILT);
                    else if (meta.generated_structure->GetType() == Structures::GRASS)
                        DrawPixel(x, y, C_JGRASS);
                    else
                        DrawPixel(x, y, C_MUD);
                }
                else if (meta.biome->GetType() == Biomes::TUNDRA)
                {
                    if (meta.generated_structure->GetType() == Structures::S_MATERIAL_BASE)
                        DrawPixel(x, y, C_ICE);
                    else if (meta.generated_structure->GetType() == Structures::S_MATERIAL_SEC)
                        DrawPixel(x, y, C_ICE);
                    else if (meta.generated_structure->GetType() == Structures::U_MATERIAL_BASE)
                        DrawPixel(x, y, C_ICE);
                    else if (meta.generated_structure->GetType() == Structures::U_MATERIAL_SEC)
                        DrawPixel(x, y, C_ICE);
                    else if (meta.generated_structure->GetType() == Structures::C_MATERIAL_BASE)
                        DrawPixel(x, y, C_ICE);
                    else if (meta.generated_structure->GetType() == Structures::C_MATERIAL_SEC)
                        DrawPixel(x, y, C_ICE);
                    else
                        DrawPixel(x, y, C_SNOW);
                }
                else
                {
                    if (meta.generated_structure->GetType() == Structures::S_MATERIAL_BASE)
                        DrawPixel(x, y, C_STONE);
                    else if (meta.generated_structure->GetType() == Structures::S_MATERIAL_SEC)
                        DrawPixel(x, y, C_CLAY);
                    else if (meta.generated_structure->GetType() == Structures::U_MATERIAL_BASE)
                        DrawPixel(x, y, C_STONE);
                    else if (meta.generated_structure->GetType() == Structures::U_MATERIAL_SEC)
                        DrawPixel(x, y, C_CLAY);
                    else if (meta.generated_structure->GetType() == Structures::C_MATERIAL_BASE)
                        DrawPixel(x, y, C_DIRT);
                    else if (meta.generated_structure->GetType() == Structures::C_MATERIAL_SEC)
                        DrawPixel(x, y, C_SILT);
                    else if (meta.generated_structure->GetType() == Structures::GRASS)
                        DrawPixel(x, y, C_GRASS);
                    else
                        DrawPixel(x, y, C_DIRT);
                }

                // FOR EVERY SURFACE STRUCTURE
                if (meta.generated_structure->GetType() == Structures::CAVE)
                {
                    if (y < cavern_rect.y)
                        DrawPixel(x, y, C_CAVE_BG_U);
                    else
                        DrawPixel(x, y, C_CAVE_BG_C);
                }
                if (meta.generated_structure->GetType() == Structures::FLOATING_ISLAND)
                    DrawPixel(x, y, C_DIRT);
                else if (meta.generated_structure->GetType() == Structures::TREE)
                    DrawPixel(x, y, (Color){191, 143, 111, 255});
                else if (meta.generated_structure->GetType() == Structures::WATER)
                    DrawPixel(x, y, C_WATER);
                else if (meta.generated_structure->GetType() == Structures::LAVA)
                    DrawPixel(x, y, C_LAVA);
                else if (meta.generated_structure->GetType() == Structures::SAND)
                    DrawPixel(x, y, C_SAND);
                else if (meta.generated_structure->GetType() == Structures::COPPER_ORE)
                    DrawPixel(x, y, C_COPPER); 
                else if (meta.generated_structure->GetType() == Structures::IRON_ORE)
                    DrawPixel(x, y, C_IRON); 
                else if (meta.generated_structure->GetType() == Structures::SILVER_ORE)
                    DrawPixel(x, y, C_SILVER); 
                else if (meta.generated_structure->GetType() == Structures::GOLD_ORE)
                    DrawPixel(x, y, C_GOLD); 
            }
            else if (meta.generated_structure == nullptr && meta.biome != nullptr)
            {
                if (y >= underground_rect.y)
                {
                    if (meta.biome->GetType() == Biomes::JUNGLE)
                        DrawPixel(x, y, C_MUD); 
                    else if (meta.biome->GetType() == Biomes::TUNDRA)
                        DrawPixel(x, y, C_SNOW); 
                }
            }
        }
    }
};

#endif // DRAW
