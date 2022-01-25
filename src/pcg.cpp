#include <stdio.h>
#include <cassert>
#include <iterator>
#include <unordered_set>
#include <unordered_map>
#include <utility>
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
class DistanceConstraint: public Constraint<V, D>
{
    protected:
        V v0, v1;
        D distance;

    public:
        DistanceConstraint(V _v0, V _v1, D _distance): v0{_v0}, v1{_v1}, distance{_distance}
        {
           this->variables = {v0, v1};
        };
        
        DistanceConstraint(const DistanceConstraint& c): v0{c.v0}, v1{c.v1}, distance{c.distance}
        {
           this->variables = {v0, v1};
        };

        virtual bool satisfied(const std::unordered_map<V, D>& assignment) const override
        {
            if (assignment.find(v0) == assignment.end())
                return true;
            if (assignment.find(v1) == assignment.end())
                return true;

            auto value0 = assignment.at(v0);
            auto value1 = assignment.at(v1);

            if (abs(value0 - value1) < distance)
                return false;
            else
                return true;
        };
};

template <typename V, typename D>
class CoordsConstraint: public Constraint<V, D>
{
    protected:
        V x0, y0, x1, y1;
        D w0, h0, w1, h1;
    public:
        CoordsConstraint(V _x0, V _y0, V _x1, V _y1, D _w0, D _h0, D _w1, D _h1): 
            x0{_x0}, y0{_y0}, x1{_x1}, y1{_y1}, 
            w0{_w0}, h0{_h0}, w1{_w1}, h1{_h1} 
        {
           this->variables = {x0, y0, x1, y1};
        }

        virtual bool satisfied(const std::unordered_map<V, D>& assignment) const override
        {
            if (assignment.find(x0) == assignment.end())
                return true;
            if (assignment.find(y0) == assignment.end())
                return true;
            if (assignment.find(x1) == assignment.end())
                return true;
            if (assignment.find(y1) == assignment.end())
                return true;

            auto _x0 = assignment.at(x0);
            auto _x1 = assignment.at(x1);
            auto _y0 = assignment.at(y0);
            auto _y1 = assignment.at(y1);

            if ((_x0 <= _x1) && (_x0 + w0) >= (_x1 + w1) && (_y0 <= _y1) && (_y0 + h0) >= (_y1 + h1))
                return false;
            if (((_x0 <= _x1) && (_x1 <= _x0 + w0)) && ((_y0 <= _y1) && (_y1 <= _y0 + h0)))
                return false;
            if (((_x0 <= _x1 + w1) && (_x1 + w1 <= _x0 + w0)) && ((_y0 <= _y1) && (_y1 <= _y0 + h0)))
                return false;
            if (((_x0 <= _x1) && (_x1 <= _x0 + w0)) && ((_y0 <= _y1 + h1) && (_y1 + h1 <= _y0 + h0)))
                return false;
            if (((_x0 <= _x1 + w1) && (_x1 + w1 <= _x0 + w0)) && ((_y0 <= _y1 + h1) && (_y1 + h1 <= _y0 + h0)))
                return false;
            //TODO entitlement
            
            return true;
        };
};

template <typename V, typename D>
class InsidePixelArrayConstraint: public Constraint<V, D>
{
    protected:
        V x, y;
        D w, h;
        PixelArray& arr;

    public:
        InsidePixelArrayConstraint(V _x, V _y, D _w, D _h, PixelArray& _arr): 
            x{_x}, y{_y}, 
            w{_w}, h{_h}, 
            arr{_arr} 
        {
            this->variables = {x, y};
        };

        virtual bool satisfied(const std::unordered_map<V, D>& assignment) const override
        {
            if (assignment.find(x) == assignment.end())
                return true;
            if (assignment.find(y) == assignment.end())
                return true;
            
            auto _x = assignment.at(x);
            auto _y = assignment.at(y);

            Pixel p0 = {_x, _y};
            Pixel p1 = {_x + w, _y};
            Pixel p2 = {_x, _y + h};
            Pixel p3 = {_x + w, _y + h};
            
            if (arr.contains(p0) && arr.contains(p1) && arr.contains(p2) && arr.contains(p3))
                return true;
            return false;
        };
};

/**
 * Create hill inside rect and fill array with pixels
 */
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

/**
 * Create hole inside rect and fill array with pixels
 */
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

/**
 * Define horizonal areas
 */
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
    
    FillWithRect(Space, *map.Space);
    FillWithRect(Surface, *map.Surface);
    FillWithRect(Underground, *map.Underground);
    FillWithRect(Cavern, *map.Cavern);
    FillWithRect(Hell, *map.Hell);
}

/**
 * Define biomes positions and shapes
 */
EXPORT void define_biomes(Map& map)
{
    printf("define biomes\n");

    int width = map.width;
    int ocean_width = 250;
    int tundra_width = 600;
    int jungle_width = 600;
    Rect Surface = map.Surface->bbox();
    Rect Hell = map.Hell->bbox();

    auto ocean_left = map.Biome(Biomes::OCEAN);
    PixelsOfRect(0, Surface.y, ocean_width, Surface.h, *ocean_left); 

    auto ocean_right = map.Biome(Biomes::OCEAN);
    PixelsOfRect(width - ocean_width, Surface.y, ocean_width, Surface.h, *ocean_right);

    // USE CSP TO FIND LOCATIONS FOR Biomes
    std::unordered_set<std::string> variables {"jungle", "tundra"};
    std::unordered_set<int> domain;
    for (int i = ocean_width + 50; i < width - 2 * ocean_width - 50; i+=50) { domain.insert(i); }
    std::unordered_map<std::string, std::unordered_set<int>> domains {{"jungle", domain}, {"tundra", domain}};
    DistanceConstraint<std::string, int> c0 {"jungle", "tundra", std::max(tundra_width, jungle_width)};
    CSPSolver<std::string, int> solver {variables, domains};
    solver.add_constraint(c0);
    auto result = solver.backtracking_search({}); 
    int jungle_x = result["jungle"];  
    int tundra_x = result["tundra"];

    auto jungle = map.Biome(Biomes::JUNGLE);
    for (int i = Surface.y; i < Hell.y; i++)
    {
        PixelsAroundRect(jungle_x + i, i, jungle_width, 2, *jungle);
    }

    auto tundra = map.Biome(Biomes::TUNDRA);
    for (int i = Surface.y; i < Hell.y; i++)
    {
        PixelsAroundRect(tundra_x - i , i, tundra_width, 2, *tundra);
    }

    PixelArray all;
    FillWithRect(Rect(0, Surface.y, width, Hell.y - Surface.y), all);
    for (auto pixel: *ocean_left) { all.remove(pixel); }
    for (auto pixel: *ocean_right) { all.remove(pixel); }
    for (auto pixel: *jungle) { all.remove(pixel); }
    for (auto pixel: *tundra) { all.remove(pixel); }

    assert(all.size() > 0);

    while (all.size() > 0)
    {
        auto biome = map.Biome(Biomes::FOREST);
        auto& p = *all.begin(); 
        UnitedPixelArea(all, p.x, p.y, *biome);

        for (auto& p: *biome)
            all.remove(p);
    }
} 


/**
 * Create minibiomes inside layers and biomes (TODO)
 */ 
EXPORT void define_minibiomes(Map& map)
{
    int width = map.width;
    
    int hill_count = 8;
    int hole_count = 7;
    int floating_island_count = 4;

    int ocean_width = 250;
    int hill_width = 80;
    int hole_width = 60;
    int island_width = 120;

    Rect Surface = map.Surface->bbox(); 

    // DEFINE SURFACE MINIBiomes
    std::unordered_set<std::string> variables;
    std::unordered_map<std::string, std::unordered_set<int>> domains;
    std::vector<DistanceConstraint<std::string, int>> constraints;
    std::unordered_set<int> domain;
    for (auto i = 0; i < hill_count; ++i) { variables.insert("hill" + std::to_string(i)); }
    for (auto i = 0; i < hole_count; ++i) { variables.insert("hole" + std::to_string(i)); }
    for (auto i = 0; i < floating_island_count; ++i) { variables.insert("island" + std::to_string(i)); }
    for (auto i = ocean_width + 50; i < width - 2 * ocean_width - 50; i+=50) { domain.insert(i); }
    for (auto& var: variables) { domains[var] = domain; }
    for (auto it0 = variables.begin(); it0 != variables.end(); it0++)
    {
        for (auto it1 = std::next(it0); it1 != variables.end(); it1++)
        {
            auto& var0 = *it0;
            auto& var1 = *it1;

            if ((StrContains(var0, "hole") || StrContains(var0, "hill")) && (StrContains(var1, "hole") || StrContains(var1, "hill")))
            {
                int min_d = 20 + rand() % 80;
                DistanceConstraint<std::string, int> c{var0, var1, min_d + std::max(hill_width, hole_width)};
                constraints.push_back(std::move(c));
            } 
            else if ((StrContains(var0, "hill") && StrContains(var1, "island")) || (StrContains(var0, "island") && StrContains(var1, "hill")))
            {
                int min_d = 20 + rand() % 80;
                DistanceConstraint<std::string, int> c{var0, var1, min_d + std::max(hill_width, island_width)};
                constraints.push_back(std::move(c));
            }
            else if (StrContains(var0, "island") && StrContains(var1, "island"))
            {
                int min_d = 20 + rand() % 80;
                DistanceConstraint<std::string, int> c{var0, var1, min_d + island_width};
                constraints.push_back(std::move(c));
            }
        }
    }
    CSPSolver<std::string, int> solver {variables, domains};
    for (auto& constraint: constraints) { solver.add_constraint(constraint); }
    auto result = solver.backtracking_search({});

    // RESULT
    for (auto& var: variables)
    {
        auto x = result[var];

        if (var.find("hole") != std::string::npos)
        {
            auto arr = map.MiniBiome(MiniBiomes::HOLE);
            Rect hole ((int) x - hole_width / 2, Surface.y, hole_width, Surface.h);
            CreateHole(hole, *arr);
        }
        else if (var.find("hill") != std::string::npos) 
        {
            auto arr = map.MiniBiome(MiniBiomes::HILL);
            Rect hill ((int) x - hill_width / 2, Surface.y, hill_width, Surface.h);
            CreateHill(hill, *arr);
        }
        else
        {
            auto arr = map.MiniBiome(MiniBiomes::FLOATING_ISLAND);
            Rect island ((int) x - island_width / 2, Surface.y, island_width, 50);
            PixelsAroundRect(island.x, island.y, island.w, island.h, *arr);
        }
    }
    
    variables = {};
    domain = {};
    domains = {};
    constraints = {};
    std::vector<std::string> variables_x;
    std::vector<std::string> variables_y;
    std::unordered_set<int> domain_x;
    std::unordered_set<int> domain_y;
    std::vector<CoordsConstraint<std::string, int>> constraints_coords;
    std::vector<InsidePixelArrayConstraint<std::string, int>> constraints_index;
    //DEFINE UNDERGROUND MINIBIOMES
    int cabin_count = 10;
    int cabin_width = 60;
    int cabin_height = 30;

    Biomes::Biome* tundra = map.GetBiome(Biomes::TUNDRA);
    assert(tundra != nullptr);  

    Rect tundra_rect = tundra->bbox();
    for (auto i = 0; i < cabin_count; ++i) { variables_x.push_back("cabin" + std::to_string(i) + "_x"); }
    for (auto i = 0; i < cabin_count; ++i) { variables_y.push_back("cabin" + std::to_string(i) + "_y"); }
    for (auto i = 0; i < cabin_count; ++i) { variables.insert(variables_x[i]); variables.insert(variables_y[i]); }
    for (auto x = tundra_rect.x; x < tundra_rect.x + tundra_rect.w; x += 200) { domain_x.insert(x); } 
    for (auto y = tundra_rect.y; y < tundra_rect.y + tundra_rect.h; y += 200) { domain_y.insert(y); } 
    for (auto var: variables_x) { domains[var] = domain_x; } 
    for (auto var: variables_y) { domains[var] = domain_y; } 
    solver = {variables, domains}; 
    for (auto i = 0; i < cabin_count; ++i) 
    {
        auto var_x = variables_x[i];
        auto var_y = variables_y[i];

        InsidePixelArrayConstraint<std::string, int> c {var_x, var_y, cabin_width, cabin_height, *tundra};
        constraints_index.push_back(std::move(c));

        for (auto j = i + 1; j < cabin_count; ++j) 
        {
            auto var_x1 = variables_x[j];
            auto var_y1 = variables_y[j];
            CoordsConstraint<std::string, int> c0 {var_x, var_y, var_x1, var_y1, cabin_width, cabin_height, cabin_width, cabin_height};
            constraints_coords.push_back(std::move(c0));
        }
    }

    //for (auto& c: constraints_index) { solver.add_constraint(c); }
    for (auto& c: constraints_coords) { solver.add_constraint(c); }

    result = solver.backtracking_search({});
    for (auto i = 0; i < cabin_count; ++i) 
    {
        auto x = result[variables_x[i]];
        auto y = result[variables_y[i]];

        auto cabin = map.MiniBiome(MiniBiomes::CABIN); 
        PixelsAroundRect(x - (int)(cabin_width / 2), y - (int)(cabin_height / 2), cabin_width, cabin_height, *cabin);
    }
}

}
