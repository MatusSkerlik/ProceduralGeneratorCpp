#include <stdio.h>
#include <utility>
#include <tuple>
#include "utils.h"

#define EXPORT __declspec(dllexport)

extern "C"
{

EXPORT void define_horizontal(DATA& data)
{
    printf("define horizontal\n");

    int width = data.WIDTH;
    int height = data.HEIGHT;

    Rect Space = Rect(0, 0, width, (int) 2 * height / 20);
    Rect Surface = Rect(0, Space.y + Space.h, width, (int) 4 * height / 20);
    Rect Underground = Rect(0, Surface.y + Surface.h, width, (int) 4 * height / 20);
    Rect Cavern = Rect(0, Underground.y + Underground.h, width, (int) 7 * height / 20);
    Rect Hell = Rect(0, Cavern.y + Cavern.h, width, (int) 3 * height / 20);

    data.HORIZONTAL_AREAS.push_back(std::make_pair(Space, C_SPACE));
    data.HORIZONTAL_AREAS.push_back(std::make_pair(Surface, C_SURFACE));
    data.HORIZONTAL_AREAS.push_back(std::make_pair(Underground, C_UNDERGROUND));
    data.HORIZONTAL_AREAS.push_back(std::make_pair(Cavern, C_CAVERN));
    data.HORIZONTAL_AREAS.push_back(std::make_pair(Hell, C_HELL));
}

EXPORT void define_biomes(DATA& data)
{
    printf("define biomes\n");

    int width = data.WIDTH;
    Rect Surface = data.HORIZONTAL_AREAS[1].first;

    PixelArray v1;
    v1.add(0, Surface.y);
    v1.add(250, Surface.y);
    v1.add(250, Surface.y + Surface.h);
    v1.add(0, Surface.y + Surface.h);
    Polygon ocean_left = Polygon(v1);

    PixelArray v2;
    v2.add(width - 250, Surface.y);
    v2.add(width, Surface.y);
    v2.add(width, Surface.y + Surface.h);
    v2.add(width - 250, Surface.y + Surface.h);
    Polygon ocean_right = Polygon(v2);

    PixelArray v0;
    v0.add(100, 100);
    v0.add(800, 200);
    v0.add(750, 500);
    v0.add(50, 150);
    Polygon ice_biome = Polygon(v0);

    data.BIOMES.push_back(ocean_left);
    data.BIOMES.push_back(ocean_right);
    data.BIOMES.push_back(ice_biome);
}

}
