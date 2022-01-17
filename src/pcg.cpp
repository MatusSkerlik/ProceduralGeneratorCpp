#include <stdio.h>
#include <utility>
#include <tuple>
#include <cstdlib>
#include "utils.h"
#include "csp.h"

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
    Rect Cavern = data.HORIZONTAL_AREAS[3].first;

    PixelArray ocean_left;
    PixelsOfRect(0, Surface.y, 250, Surface.h, ocean_left); 

    PixelArray ocean_right;
    PixelsOfRect(width - 250, Surface.y, 250, Surface.h, ocean_right);

    PixelArray jungle;
    int jungle_width = 600;
    int jungle_x = (rand() % (int)(width - (2 * 250) - (jungle_width / 2))) + 250 + jungle_width / 2;
    for (int i = Surface.y; i < Cavern.y + Cavern.h; i++)
    {
        PixelsAroundRect(jungle_x, i, jungle_width, 2, jungle);
    }

    data.BIOMES.push_back(std::move(ocean_left));
    data.BIOMES.push_back(std::move(ocean_right));
    data.BIOMES.push_back(std::move(jungle));
}

}
