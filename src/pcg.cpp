#include <stdio.h>
#include <iterator>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <tuple>
#include <cstdlib>
#include <string>
#include <algorithm>
#include "utils.h"
#include "csp.h"
#include "spline.h"

#define EXPORT __declspec(dllexport)

bool StrContains(std::string str, std::string pattern)
{
    return str.find(pattern) != std::string::npos;
}

template <typename V, typename D>
class DistanceConstraint: public BinaryConstraint<V, D>
{
    public:
        float distance;
        DistanceConstraint(std::pair<V, V> _pair, D _distance)
        {
           this->pair = _pair; 
           this->variables = {_pair.first, _pair.second};
           this->distance = _distance;
        };
        
        DistanceConstraint(const DistanceConstraint& constraint)
        {
            this->variables = constraint.variables;
            this->distance = constraint.distance;
            this->pair = constraint.pair;
        };

        virtual bool satisfied(std::unordered_map<V, D>& assignment) const override
        {
            if (assignment.find(this->pair.first) == assignment.end())
                return true;
            if (assignment.find(this->pair.second) == assignment.end())
                return true;

            auto value0 = assignment.at(this->pair.first);
            auto value1 = assignment.at(this->pair.second);

            if (abs(value0 - value1) < distance)
                return false;
            else
                return true;
        };
};

void CreateHill(const Rect& rect, PixelArray& arr)
{
   double sx = rect.x;
   double cx = rect.x + (int)(rect.w / 4) + (rand() % (int)(rect.w / 4));
   double ex = rect.x + rect.w;
   double sy = rect.y + rect.h - (rand() % (int)(rect.h / 3));
   double ey = sy + (-8 + rand() % 17);
   double cy = std::min(sy, ey) - (20 + rand() % 40);

   std::vector<double> X = {sx, cx, ex};
   std::vector<double> Y = {sy, cy, ey};

   tk::spline s;
   s.set_boundary(tk::spline::first_deriv, -1, tk::spline::first_deriv, 1);
   s.set_boundary(tk::spline::second_deriv, 0, tk::spline::second_deriv, 0);
   s.set_points(X, Y, tk::spline::cspline);

   for (int x = rect.x; x < rect.x + rect.w; ++x)
   {
       int _y = (int) s(x);
       for (int y = _y; y < rect.y + rect.h; ++y)
       {
            arr.add(x, y);
       }
   }
};

void CreateHole(const Rect& rect, PixelArray& arr)
{
   double sx = rect.x;
   double cx = rect.x + (int)(rect.w / 4) + (rand() % (int)(rect.w / 4));
   double ex = rect.x + rect.w;
   double sy = rect.y + rect.h - (rand() % (int)(rect.h / 3));
   double ey = sy + (-8 + rand() % 17);
   double cy = std::min(sy, ey) + (8 + rand() % (int)(rect.y + rect.h - std::max(sy, ey) - 8));

   std::vector<double> X = {sx, cx, ex};
   std::vector<double> Y = {sy, cy, ey};

   tk::spline s;
   s.set_boundary(tk::spline::first_deriv, 0, tk::spline::first_deriv, 0);
   s.set_boundary(tk::spline::second_deriv, 0.1, tk::spline::second_deriv, 0.1);
   s.set_points(X, Y, tk::spline::cspline);

   for (int x = rect.x; x < rect.x + rect.w; ++x)
   {
       int _y = (int) s(x);
       for (int y = _y; y < rect.y + rect.h; ++y)
       {
            arr.add(x, y);
       }
   }
};


extern "C"
{

EXPORT void define_horizontal(Map& map)
{
    printf("define horizontal\n");

    int width = map.width;
    int height = map.height;

    Rect Space = Rect(0, 0, width, (int) 2 * height / 20);
    Rect Surface = Rect(0, Space.y + Space.h, width, (int) 4 * height / 20);
    Rect Underground = Rect(0, Surface.y + Surface.h, width, (int) 4 * height / 20);
    Rect Cavern = Rect(0, Underground.y + Underground.h, width, (int) 7 * height / 20);
    Rect Hell = Rect(0, Cavern.y + Cavern.h, width, (int) 3 * height / 20);
    
    map.Space = HorizontalAreas::FromRect(Space, HorizontalAreas::SPACE);
    map.Surface = HorizontalAreas::FromRect(Surface, HorizontalAreas::SURFACE);
    map.Underground = HorizontalAreas::FromRect(Underground, HorizontalAreas::UNDERGROUND);
    map.Cavern = HorizontalAreas::FromRect(Cavern, HorizontalAreas::CAVERN);
    map.Hell = HorizontalAreas::FromRect(Hell, HorizontalAreas::HELL);
}

EXPORT void define_biomes(Map& map)
{
    printf("define biomes\n");

    int width = map.width;
    int ocean_width = 250;
    int tundra_width = 600;
    int jungle_width = 600;
    Rect Surface = map.Surface.bbox();
    Rect Hell = map.Hell.bbox();

    Biomes::Biome ocean_left {Biomes::OCEAN};
    PixelsOfRect(0, Surface.y, ocean_width, Surface.h, ocean_left); 

    Biomes::Biome ocean_right {Biomes::OCEAN};
    PixelsOfRect(width - ocean_width, Surface.y, ocean_width, Surface.h, ocean_right);

    // USE CSP TO FIND LOCATIONS FOR Biomes
    std::unordered_set<std::string> variables {"jungle", "tundra"};
    std::unordered_set<int> domain;
    for (int i = ocean_width + 50; i < width - 2 * ocean_width - 50; i+=50) { domain.insert(i); }
    std::unordered_map<std::string, std::unordered_set<int>> domains {{"jungle", domain}, {"tundra", domain}};
    DistanceConstraint<std::string, int> c0 {std::make_pair("jungle", "tundra"), std::max(tundra_width, jungle_width)};
    CSPSolver<std::string, int> solver {variables, domains};
    solver.add_constraint(c0);
    auto result = solver.backtracking_search({}); 
    int jungle_x = result["jungle"];  
    int tundra_x = result["tundra"];

    Biomes::Biome jungle {Biomes::JUNGLE};
    for (int i = Surface.y; i < Hell.y - 100; i++)
    {
        PixelsAroundRect(jungle_x, i, jungle_width, 2, jungle);
    }

    Biomes::Biome tundra {Biomes::TUNDRA};
    for (int i = Surface.y; i < Hell.y - 200; i++)
    {
        PixelsAroundRect(tundra_x, i, tundra_width, 2, tundra);
    }

    PixelArray forest_mask;
    for (auto pixel: ocean_left) { forest_mask.add(pixel); }
    for (auto pixel: ocean_right) { forest_mask.add(pixel); }
    for (auto pixel: jungle) { forest_mask.add(pixel); }
    for (auto pixel: tundra) { forest_mask.add(pixel); }
    
    auto forest = Biomes::Biome(FloodFill(Rect(0, Surface.y, width, Hell.y - Surface.y), 0, Surface.y + Surface.h + 1, forest_mask), Biomes::FOREST);

    map.Biomes(std::move(forest));
    map.Biomes(std::move(ocean_left));
    map.Biomes(std::move(ocean_right));
    map.Biomes(std::move(jungle));
    map.Biomes(std::move(tundra));
} 

EXPORT void define_minibiomes(Map& map)
{
    int width = map.width;
    
    int hill_count = 8;
    int hole_count = 7;
    int floating_island_count = 4;

    int ocean_width = 250;
    int hill_width = 80;
    int hole_width = 60;
    int floating_island_width = 120;

    Rect Surface = map.Surface.bbox(); 

    // DEFINE SURFACE MINIBiomes
    std::unordered_set<std::string> variables;
    for (auto i = 0; i < hill_count; ++i) { variables.insert("hill" + std::to_string(i)); }
    for (auto i = 0; i < hole_count; ++i) { variables.insert("hole" + std::to_string(i)); }
    for (auto i = 0; i < floating_island_count; ++i) { variables.insert("island" + std::to_string(i)); }
    std::unordered_set<int> domain;
    for (auto i = ocean_width + 50; i < width - 2 * ocean_width - 50; i+=50) { domain.insert(i); }
    std::unordered_map<std::string, std::unordered_set<int>> domains;
    for (auto& var: variables) { domains[var] = domain; }
    std::vector<DistanceConstraint<std::string, int>> constraints;
    for (auto it0 = variables.begin(); it0 != variables.end(); it0++)
    {
        for (auto it1 = std::next(it0); it1 != variables.end(); it1++)
        {
            auto& var0 = *it0;
            auto& var1 = *it1;

            if ((StrContains(var0, "hole") || StrContains(var0, "hill")) && (StrContains(var1, "hole") || StrContains(var1, "hill")))
            {
                int min_d = 20 + rand() % 80;
                DistanceConstraint<std::string, int> constraint {make_pair(var0, var1), min_d + std::max(hill_width, hole_width)};
                constraints.push_back(std::move(constraint));
            } 
            else if ((StrContains(var0, "hill") && StrContains(var1, "island")) || (StrContains(var0, "island") && StrContains(var1, "hill")))
            {
                int min_d = 20 + rand() % 80;
                DistanceConstraint<std::string, int> constraint {make_pair(var0, var1), min_d + std::max(hill_width, floating_island_width)};
                constraints.push_back(std::move(constraint));
            }
            else if (StrContains(var0, "island") && StrContains(var1, "island"))
            {
                int min_d = 20 + rand() % 80;
                DistanceConstraint<std::string, int> constraint {make_pair(var0, var1), min_d + floating_island_width};
                constraints.push_back(std::move(constraint));
            }
        }
    }
    CSPSolver<std::string, int> solver {variables, domains};
    for (auto& constraint: constraints) { solver.add_constraint(constraint); }
    auto result = solver.backtracking_search({});
    for (auto& var: variables)
    {
        MiniBiomes::Biome arr;
        auto x = result[var];
        if (var.find("hole") != std::string::npos)
        {
            Rect hole ((int) x - hole_width / 2, Surface.y, hole_width, Surface.h);
            CreateHole(hole, arr);
            arr.type = MiniBiomes::HOLE;
        }
        else if (var.find("hill") != std::string::npos) 
        {
            Rect hill ((int) x - hill_width/ 2, Surface.y, hill_width, Surface.h);
            CreateHill(hill, arr);
            arr.type = MiniBiomes::HILL;
        }
        else
        {
            Rect island ((int) x - floating_island_width / 2, Surface.y, floating_island_width, 50);
            PixelsAroundRect(island.x, island.y, island.w, island.h, arr);
            arr.type = MiniBiomes::FLOATING_ISLAND;
        }
        map.MiniBiomes(std::move(arr));
    }


}

}
