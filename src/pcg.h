#include <stdio.h>
#include <cassert>
#include <iterator>
#include <stdlib.h>
#include <tuple>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <vector>
#include "utils.h"
#include "csp.h"
#include "spline.h"
#include "polygon.h"

#define EXPORT __declspec(dllexport)

inline float cerp(float v0, float v1, float t)
{
    auto mu2 = (1 - cos(t * 3.14159)) / 2;
    return v0 * (1 - mu2) + v1 * mu2;
};

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

            return (abs(value0 - value1) >= distance);
        };
};

/**
 * Constraint that ensures that object will not intercept with each other
 */
template <typename V, typename D>
class NonIntersectionConstraint2D: public Constraint<V, D>
{
    protected:
        V v0, v1;
        D w0, h0, w1, h1;
        D width;
    public:
        NonIntersectionConstraint2D(V _v0, V _v1, D _w0, D _h0, D _w1, D _h1, D _width): 
            v0{_v0}, v1{_v1},
            w0{_w0}, h0{_h0}, w1{_w1}, h1{_h1},
            width{_width}
        {
           this->variables = {v0, v1};
        }

        virtual bool satisfied(const std::unordered_map<V, D>& assignment) const override
        {
            if (assignment.find(v0) == assignment.end())
                return true;
            if (assignment.find(v1) == assignment.end())
                return true;

            auto _v0 = assignment.at(v0);
            auto _v1 = assignment.at(v1);

            auto _x0 = (int) _v0 % width;
            auto _y0 = (int) _v0 / width;
            auto _x1 = (int) _v1 % width;
            auto _y1 = (int) _v1 / width;

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

/**
 * Constraint that ensures that object will be inside PixelArray 
 */
template <typename V, typename D>
class InsidePixelArrayConstraint2D: public Constraint<V, D>
{
    protected:
        V v0;
        D w, h;
        const PixelArray& arr;
        Rect rect;

    public:
        InsidePixelArrayConstraint2D(V _v0, D _w, D _h, PixelArray& _arr, Rect _rect): 
            v0{_v0},
            w{_w}, h{_h},
            arr{_arr},
            rect{_rect} 
        {
           this->variables = {v0, };
        };

        virtual bool satisfied(const std::unordered_map<V, D>& assignment) const override
        {
            if (assignment.find(v0) == assignment.end())
                return true;

            auto _v0 = assignment.at(v0);

            auto _x = (int) rect.x + _v0 % rect.w;
            auto _y = (int) rect.y + _v0 / rect.w;

            if (!arr.contains((Pixel){_x, _y}))
                return false;
            if (!arr.contains((Pixel){_x + w, _y}))
                return false;
            if (!arr.contains((Pixel){_x, _y + h}))
                return false;
            if (!arr.contains((Pixel){_x + w, _y + h}))
                return false;
           
            return true;
        };
};


/**
 * Create hill inside rect and fill array with pixels
 */
inline void CreateHill(const Rect& rect, PixelArray& arr, double sy, double ey)
{
    double sx = rect.x;
    double cx = rect.x + (int)(rect.w / 4) + (rand() % (int)(rect.w / 4));
    double ex = rect.x + rect.w;
    double cy = std::min(sy, ey) - 20 - (rand() % 30);

    std::vector<double> X = {sx, cx, ex};
    std::vector<double> Y = {sy, cy, ey};

    tk::spline s;
    s.set_boundary(tk::spline::first_deriv, -1, tk::spline::first_deriv, 1);
    s.set_boundary(tk::spline::second_deriv, 0, tk::spline::second_deriv, 0);
    s.set_points(X, Y, tk::spline::cspline);

    for (int x = rect.x; x <= rect.x + rect.w; ++x)
    {
       int _y = (int) s(x);
       for (int y = _y; y <= rect.y + rect.h; ++y)
       {
            arr.add({x, y});
       }
    }
};

/**
 * Create hole inside rect and fill array with pixels
 */
inline void CreateHole(const Rect& rect, PixelArray& arr, double sy, double ey)
{
    double sx = rect.x;
    double cx = rect.x + (int)(rect.w / 4) + (rand() % (int)(rect.w / 4));
    double ex = rect.x + rect.w;
    double cy = std::max(sy, ey) + 8 + (rand() % 30);

    std::vector<double> X = {sx, cx, ex};
    std::vector<double> Y = {sy, cy, ey};

    tk::spline s;
    s.set_boundary(tk::spline::first_deriv, 0, tk::spline::first_deriv, 0);
    s.set_boundary(tk::spline::second_deriv, 0.1, tk::spline::second_deriv, 0.1);
    s.set_points(X, Y, tk::spline::cspline);

    for (int x = rect.x; x <= rect.x + rect.w; ++x)
    {
       int _y = (int) s(x);
       for (int y = _y; y <= rect.y + rect.h; ++y)
       {
            arr.add({x, y});
       }
    }
};

/**
 * Create surface cliff on right or left side of surface part identified by pixels s and e
 */
inline void CreateCliff(const Rect& rect, PixelArray& arr, Pixel s, Pixel e)
{
    std::vector<double> X;
    std::vector<double> Y;
    if (s.y < e.y) // RIGHT
    {
        X = {(double)s.x, (double)rect.x + rect.w * 0.75, (double)rect.x + rect.w, (double)s.x};
        Y = {(double)s.y, (double)rect.y, (double) rect.y + rand() % (int)(rect.w * 0.5), (double)e.y};
    }
    else // LEFT
    {
        X = {(double)e.x, (double)rect.x + rect.w * 0.25, (double)rect.x, (double)e.x};
        Y = {(double)e.y, (double)rect.y, (double) rect.y + rand() % (int)(rect.w * 0.5), (double)s.y};
    }

    std::vector<double> T { 0.0 };
    for(auto i=1; i < (int)X.size(); i++)
        T.push_back(T[i-1] + sqrt( (X[i]-X[i-1])*(X[i]-X[i-1]) + (Y[i]-Y[i-1])*(Y[i]-Y[i-1]) ));
    tk::spline sx(T,X), sy(T,Y);

    auto size = (int)T.back();
    Point polygon[size + 2]; 
    if (s.y < e.y)
        polygon[0] = {e.x, e.y};
    else
        polygon[0] = {s.x, s.y};

    for (auto t = 1; t < size + 1; ++t) polygon[t] = {(int)sx(t - 1),(int)sy(t - 1)};

    if (s.y < e.y)
        polygon[size + 1] = {s.x, s.y};
    else
        polygon[size + 1] = {e.x, e.y};

    for (auto x = rect.x; x <= rect.x + rect.w; ++x)
        for (auto y = rect.y; y <= rect.y + rect.h; ++y)
            if (CNPnPoly({x, y}, polygon, size)) arr.add({x, y});
};

/*
 * Create surface transition from one surface part to another
 */
inline void CreateTransition(const Rect& rect, PixelArray& arr, Pixel p)
{
    std::vector<double> X {(double)rect.x, (double)rect.x + rect.w / 4 + rand() % (int)(rect.w / 2), (double)rect.x + rect.w};
    std::vector<double> Y;
    auto h = abs(rect.y - p.y);
    if (rect.x == p.x) // LEFT
    {
        Y = {(double)p.y, (double)rect.y + h / 4 + rand() % (int)h / 2, (double)rect.y }; 
    }
    else
    {
        Y = {(double)rect.y, (double)rect.y + h / 4 + rand() % (int)h / 2, (double)p.y }; 
    }
    
    tk::spline s(X, Y);
    for (int x = rect.x; x <= rect.x + rect.w; ++x)
    {
       int _y = (int) s(x);
       for (int y = _y; y <= rect.y + rect.h; ++y)
       {
            arr.add({x, y});
       }
    }

};

/*
 * Create island terrain
 */
inline void CreateIsland(const Rect& rect, PixelArray& arr, int type)
{
    std::vector<double> X; 
    std::vector<double> Y;

    auto half_x = rect.x + rect.w / 2;
    //auto half_y = rect.y + rect.h / 2;
    auto quater_y = rect.y + rect.h / 4;
    auto full_y = rect.y + rect.h;

    X = {(double)half_x, (double)rect.x + rect.w, (double)half_x, (double)rect.x, (double)half_x};
    Y = {(double)full_y, (double)quater_y, (double) quater_y, (double)quater_y, (double)full_y};

    std::vector<double> T { 0.0 };
    for(auto i=1; i < (int)X.size(); i++)
        T.push_back(T[i-1] + sqrt( (X[i]-X[i-1])*(X[i]-X[i-1]) + (Y[i]-Y[i-1])*(Y[i]-Y[i-1]) ));
    tk::spline sx(T,X), sy(T,Y);

    auto size = (int)T.back();
    Point polygon[size + 2]; 
    polygon[0] = {half_x, full_y};
    for (auto t = 1; t < size + 1; ++t) polygon[t] = {(int)sx(t - 1),(int)sy(t - 1)};
    polygon[size + 1] = {half_x, full_y};
    
    for (auto x = rect.x; x <= rect.x + rect.w; ++x)
        for (auto y = rect.y; y <= rect.y + rect.h; ++y)
            if (CNPnPoly({x, y}, polygon, size)) arr.add({x, y});
};

inline auto DomainInsidePixelArray(const Rect& rect, const PixelArray& arr, int step = 1)
{
    std::unordered_set<int> domain;
    for (auto v = 0; v < rect.w * rect.h; v += step)
    { 
        auto x = (int) rect.x + (v % rect.w);
        auto y = (int) rect.y + (v / rect.w);
        if (arr.contains({x, y})) domain.insert(v);
    } 
    return domain;
};


extern "C"
{

/**
 * Define horizonal areas
 */
EXPORT inline void DefineHorizontal(Map& map)
{
    printf("DefineHorizontal\n");

    int width = map.Width();
    int height = map.Height();

    Rect Space = Rect(0, 0, width, (int) 2 * height / 20);
    Rect Surface = Rect(0, Space.y + Space.h, width, (int) 4 * height / 20);
    Rect Underground = Rect(0, Surface.y + Surface.h, width, (int) 4 * height / 20 + 1);
    Rect Cavern = Rect(0, Underground.y + Underground.h, width, (int) 7 * height / 20);
    Rect Hell = Rect(0, Cavern.y + Cavern.h, width, (int) 3 * height / 20);
    
    FillWithRect(Space, map.Space());
    FillWithRect(Surface, map.Surface());
    FillWithRect(Underground, map.Underground());
    FillWithRect(Cavern, map.Cavern());
    FillWithRect(Hell, map.Hell());
}

/**
 * Define biome positions and shapes
 */
EXPORT inline void DefineBiomes(Map& map)
{
    printf("DefineBiomes\n");

    int width = map.Width();
    int ocean_width = 250;
    int tundra_width = 500;
    int jungle_width = 500;
    Rect Surface = map.Surface().bbox();
    Rect Hell = map.Hell().bbox();

    auto& ocean_left = map.Biome(Biomes::OCEAN);
    PixelsOfRect(0, Surface.y, ocean_width, Surface.h, ocean_left); 

    auto& ocean_right = map.Biome(Biomes::OCEAN);
    PixelsOfRect(width - ocean_width, Surface.y, ocean_width, Surface.h, ocean_right);

    // USE CSP TO FIND LOCATIONS FOR JUNGLE AND TUNDRA 
    // DEFINITION OF VARIABLES
    std::unordered_set<std::string> variables {"jungle", "tundra"};
    
    // DEFINITION OF DOMAIN
    auto domain = Domain(ocean_width + 50, width - 2 * ocean_width - 50, 50);
    
    // DEFINITION OF DOMAIN FOR EACH VARIABLE
    std::unordered_map<std::string, std::unordered_set<int>> domains;
    for (auto& var: variables) { domains[var] = domain; }
  
    // DEFINITION OF CONSTRAINTS
    std::vector<DistanceConstraint<std::string, int>> constraints;

    Between<std::string>(variables, [&](std::string v0, std::string v1){
        DistanceConstraint<std::string, int> c {v0, v1, std::max(jungle_width, tundra_width)};
        constraints.push_back(std::move(c));
    });
    
    // DEFINITION OF SOLVER
    CSPSolver<std::string, int> solver {variables, domains};
    for (auto& c: constraints) { solver.add_constraint(c); }

    // SEARCH FOR RESULT
    auto result = solver.backtracking_search({}, [&](){ return map.ShouldForceStop(); });
    if (map.ShouldForceStop())
        return;
    
    if (result.size() < variables.size())
    {
        map.Error("COULD NOT FIND SOLUTION FOR BIOME PLACEMENT");
        return;
    } 
    else 
    {
        // RESULT HANDLING
        // DEFINITION OF JUNGLE
        auto jungle_x = result["jungle"];  
        auto& jungle = map.Biome(Biomes::JUNGLE);
        for (int i = Surface.y; i < Hell.y; i++)
        {
            PixelsAroundRect(jungle_x - i, i, jungle_width, 2, jungle);
        }

        // DEFINITION OF TUNDRA
        auto tundra_x = result["tundra"];
        auto& tundra = map.Biome(Biomes::TUNDRA);
        for (int i = Surface.y; i < Hell.y; i++)
        {
            PixelsAroundRect(tundra_x - i, i, tundra_width, 2, tundra);
        }

        // DEFINITION OF FORESTS
        PixelArray forests;
        FillWithRect(Rect(0, Surface.y, width, Hell.y - Surface.y), forests);
        
        // REMOVE PIXELS WHICH DON'T BELONG TO ANOTHER BIOMES 
        for (auto pixel: ocean_left) { forests.remove(pixel); }
        for (auto pixel: ocean_right) { forests.remove(pixel); }
        for (auto pixel: jungle) { forests.remove(pixel); }
        for (auto pixel: tundra) { forests.remove(pixel); }

        while (forests.size() > 0)
        {
            auto& forest = map.Biome(Biomes::FOREST);
            auto p = *forests.begin(); 
            UnitedPixelArea(forests, p.x, p.y, forest);
            for (auto p: forest) { forests.remove(p); }
        }
    }
};

/*
 * Define minibiomes hill, hole, island
 */ 
EXPORT inline void DefineHillsHolesIslands(Map& map)
{
    printf("DefineHillsHolesIslands\n");

    int width = map.Width();
    
    int hill_count = map.HillsFrequency() * 12;
    int hole_count = map.HolesFrequency() * 10;
    int floating_island_count = map.IslandsFrequency() * 8;

    int ocean_width = 250;
    int hill_width = 80;
    int hole_width = 80;
    int island_width = 120;

    Rect Surface = map.Surface().bbox(); 

    // CSP RELATED
    // DEFINITION OF VARIABLES
    auto hills = CreateVariables("hill", 0, hill_count);
    auto holes = CreateVariables("hole", 0, hole_count);
    auto islands = CreateVariables("island", 0, floating_island_count);
    auto variables = JoinVariables(hills, JoinVariables(holes, islands));
    
    // DEFINITION OF DOMAIN
    auto domain = Domain(ocean_width + 50, width - 2 * ocean_width - 50, 50);
    
    // DEFINITION OF DOMAIN FOR EACH VARIABLE
    std::unordered_map<std::string, std::unordered_set<int>> domains;
    for (auto& var: variables) { domains[var] = domain; }

    // DEFINITION OF CONSTRAINTS
    std::vector<DistanceConstraint<std::string, int>> constraints;

    ForEach<std::string>(holes, hills, [&](std::string v0, std::string v1){
        int min_d = 20 + rand() % 80;
        DistanceConstraint<std::string, int> c{v0, v1, min_d + std::max(hill_width, hole_width)};
        constraints.push_back(std::move(c));
    });

    Between<std::string>(holes, [&](std::string v0, std::string v1){
        int min_d = 20 + rand() % 80;
        DistanceConstraint<std::string, int> c{v0, v1, min_d + hole_width};
        constraints.push_back(std::move(c));
    });

    Between<std::string>(hills, [&](std::string v0, std::string v1){
        int min_d = 20 + rand() % 80;
        DistanceConstraint<std::string, int> c{v0, v1, min_d + hill_width};
        constraints.push_back(std::move(c));
    });

    ForEach<std::string>(hills, islands, [&](std::string v0, std::string v1){
        int min_d = 20 + rand() % 80;
        DistanceConstraint<std::string, int> c{v0, v1, min_d + std::max(hill_width, island_width)};
        constraints.push_back(std::move(c));
    });

    Between<std::string>(islands, [&](std::string v0, std::string v1){
        int min_d = 20 + rand() % 80;
        DistanceConstraint<std::string, int> c{v0, v1, min_d + island_width};
        constraints.push_back(std::move(c));
    });
    
    // CREATION OF SOLVER
    CSPSolver<std::string, int> solver {variables, domains};

    // REGISTRATION OF CONSTRAINTS
    for (auto& constraint: constraints) { solver.add_constraint(constraint); }
    
    // SEARCH FOR RESULT
    auto result = solver.backtracking_search({}, [&](){ return map.ShouldForceStop(); });
    if (map.ShouldForceStop())
        return;

    if (result.size() < variables.size())
    {
        map.Error("COULD NOT FIND SOLUTION TO HILL, HOLE, ISLAND PLACEMENT");
        return;
    }
    else
    {
        // REUSLT HANDLING
        for (auto& var: holes)
        {
            auto x = result[var];
            auto& hole = map.Structure(Structures::HOLE);
            Rect rect ((int) x - hole_width / 2, Surface.y, hole_width, Surface.h);
            PixelsOfRect(rect.x, rect.y, rect.w, rect.h, hole);
        }
        
        for (auto& var: hills)
        {
            auto x = result[var];
            auto& hill = map.Structure(Structures::HILL);
            Rect rect ((int) x - hill_width / 2, Surface.y, hill_width, Surface.h);
            PixelsOfRect(rect.x, rect.y, rect.w, rect.h, hill);
        }

        for (auto& var: islands)
        {
            auto x = result[var];
            auto& island = map.Structure(Structures::FLOATING_ISLAND);
            Rect rect ((int) x - island_width / 2, Surface.y - rand() % 40, island_width, 50);
            PixelsOfRect(rect.x, rect.y, rect.w, rect.h, island);
        }
    }
};

/*
 * Define underground cabin minibiomes
 */ 
EXPORT inline void DefineCabins(Map& map)
{
    printf("DefineCabins\n");

    int cabin_count = map.CabinsFrequency() * 60;
    int cabin_width = 80;
    int cabin_height = 40;

    auto& Underground = map.Underground();
    auto& Cavern = map.Cavern(); 
    auto UCRect = RectUnion(Underground.bbox(), Cavern.bbox());

    auto& tundra = *map.GetBiome(Biomes::TUNDRA);
    // CABINS ALLOWED ONLY IN UNDERGROUND AND CAVERN
    auto tundra_rect = RectIntersection(UCRect, tundra.bbox());

    // CSP RELATED
    // DEFINITION OF VARIABLES
    auto variables = CreateVariables("cabin", 0, cabin_count); 

    // DEFINITION OF UNION DOMAIN
    auto domain = DomainInsidePixelArray(tundra_rect, tundra, 20);
    
    // DEFINITION OF DOMAIN FOR EACH VARIABLE
    std::unordered_map<std::string, std::unordered_set<int>> domains;
    for (auto var: variables) { domains[var] = domain; } 

    // DEFINITION OF CONSTRAINTS
    std::vector<NonIntersectionConstraint2D<std::string, int>> non_intersect_constraints;
    Between<std::string>(variables, [&](std::string v0, std::string v1){
        NonIntersectionConstraint2D<std::string, int> c (v0, v1, cabin_width, cabin_height, cabin_width, cabin_height, tundra_rect.w);
        non_intersect_constraints.push_back(std::move(c));
    });

    std::vector<InsidePixelArrayConstraint2D<std::string, int>> inside_pixelarray_constraints;
    for (auto& var: variables) 
    {
        InsidePixelArrayConstraint2D<std::string, int> c (var, cabin_width, cabin_height, tundra, tundra_rect);
        inside_pixelarray_constraints.push_back(std::move(c));
    }

    // CREATION OF SOLVER
    CSPSolver<std::string, int> solver {variables, domains};

    // REGISTRATION OF CONSTRAINTS
    for (auto& c: non_intersect_constraints) { solver.add_constraint(c); }
    for (auto& c: inside_pixelarray_constraints) { solver.add_constraint(c); }

    // SEARCH FOR RESULT
    auto result = solver.backtracking_search({}, [&](){ return map.ShouldForceStop(); });
    if (map.ShouldForceStop())
        return;

    if (result.size() < variables.size())
    {
        map.Error("COULD NOT FIND SOLUTION FOR CABIN PLACEMENT.");
        return;
    }
    else 
    {
        for (auto& var: variables) 
        {
            auto v = result[var];
            auto x = (int) tundra_rect.x + (v % tundra_rect.w); 
            auto y = (int) tundra_rect.y + (v / tundra_rect.w);

            auto& cabin = map.Structure(Structures::CABIN); 
            PixelsOfRect(x, y, cabin_width, cabin_height, cabin);
        }
    }
};

/**
 * Definition of castles for each biome
 */
EXPORT inline void DefineCastles(Map& map)
{
    printf("DefineCastles\n");

    auto castle_width = 250;
    auto castle_height = 150;

    auto& Underground = map.Underground();
    auto& Cavern = map.Cavern(); 
    auto UCRect = RectUnion(Underground.bbox(), Cavern.bbox());

    auto& forest = *map.GetBiome(Biomes::FOREST);
    auto forest_rect = RectIntersection(UCRect, forest.bbox());

    auto& tundra = *map.GetBiome(Biomes::TUNDRA);
    auto tundra_rect = RectIntersection(UCRect, tundra.bbox());
    
    auto& jungle = *map.GetBiome(Biomes::JUNGLE);
    auto jungle_rect = RectIntersection(UCRect, jungle.bbox());


    auto& hell = map.Hell();
    auto hell_rect = hell.bbox();

    auto variables = CreateVariables({"forest_castle", "jungle_castle", "tundra_castle", "hell_castle"});

    // DEFINITION OF UNION DOMAIN
    auto forest_domain = DomainInsidePixelArray(forest_rect, forest, 10);
    auto jungle_domain = DomainInsidePixelArray(jungle_rect, jungle, 10);
    auto tundra_domain = DomainInsidePixelArray(tundra_rect, tundra, 10);
    auto hell_domain =   DomainInsidePixelArray(hell_rect, hell, 10); 

    std::unordered_map<std::string, std::unordered_set<int>> domains;
    domains["forest_castle"] = forest_domain;
    domains["jungle_castle"] = jungle_domain;
    domains["tundra_castle"] = tundra_domain;
    domains["hell_castle"] = hell_domain;

    // DEFINITION OF CONSTRAINTS
    InsidePixelArrayConstraint2D<std::string, int> c0 ("forest_castle", castle_width, castle_height, forest, forest_rect);
    InsidePixelArrayConstraint2D<std::string, int> c1 ("jungle_castle", castle_width, castle_height, jungle, jungle_rect);
    InsidePixelArrayConstraint2D<std::string, int> c2 ("tundra_castle", castle_width, castle_height, tundra, tundra_rect);
    InsidePixelArrayConstraint2D<std::string, int> c3 ("hell_castle", castle_width, castle_height, hell, hell_rect);

    // CREATION OF SOLVER
    CSPSolver<std::string, int> solver {variables, domains};

    // REGISTRATION OF CONSTRAINTS
    solver.add_constraint(c0);
    solver.add_constraint(c1);
    solver.add_constraint(c2);
    solver.add_constraint(c3);
        
    // SEARCH FOR SOLUTION
    auto result = solver.backtracking_search({}, [&](){ return map.ShouldForceStop(); });
    if (map.ShouldForceStop())
        return;

    if (result.size() < variables.size())
    {
        map.Error("COULD NOT FIND SOLUTION FOR CASTLE PLACEMENT.");
        return;
    }
    else 
    {
        // RESULT HANDLING
        auto forest_v = result["forest_castle"];
        auto f_x = forest_rect.x + (forest_v % forest_rect.w);
        auto f_y = forest_rect.y + (forest_v / forest_rect.w);
        auto& forest_castle = map.Structure(Structures::CASTLE);
        PixelsOfRect(f_x, f_y, castle_width, castle_height, forest_castle);

        auto tundra_v = result["tundra_castle"];
        auto t_x = tundra_rect.x + (tundra_v % tundra_rect.w);
        auto t_y = tundra_rect.y + (tundra_v / tundra_rect.w);
        auto& tundra_castle = map.Structure(Structures::CASTLE);
        PixelsOfRect(t_x, t_y, castle_width, castle_height, tundra_castle);

        auto jungle_v = result["jungle_castle"];
        auto j_x = jungle_rect.x + (jungle_v % jungle_rect.w);
        auto j_y = jungle_rect.y + (jungle_v / jungle_rect.w);
        auto& jungle_castle = map.Structure(Structures::CASTLE);
        PixelsOfRect(j_x, j_y, castle_width, castle_height, jungle_castle);

        auto hell_v = result["hell_castle"];
        auto h_x = hell_rect.x + (hell_v % hell_rect.w);
        auto h_y = hell_rect.y + (hell_v / hell_rect.w);
        auto& hell_castle = map.Structure(Structures::CASTLE);
        PixelsOfRect(h_x, h_y, castle_width, castle_height, hell_castle);
    }
};

EXPORT inline void DefineSurface(Map& map)
{
    printf("DefineSurface\n");

    auto& Surface = map.Surface();
    auto surface_rect = Surface.bbox();
    auto width = map.Width();

    // TODO GUI CONTROLS
    auto left_ocean_width = 250;
    auto right_ocean_widht = 250;
    auto surface_parts = 10;
    auto octaves = 3;
    auto fq = 50;

    auto surface_x = left_ocean_width;
    //auto surface_y = surface_rect.y + surface_rect.h;
    auto surface_width = width - left_ocean_width - right_ocean_widht;
    auto surface_height = surface_rect.h;

    auto tmp_x = surface_x;
    auto tmp_avg_part_width = surface_width / surface_parts;
    auto tmp_parts = surface_parts;
    auto tmp_surface_width = surface_width;

    auto parts = std::vector<std::pair<int, int>>();

    for (auto i = 0; i < surface_parts; ++i)
    {
        tmp_parts = surface_parts - i;
        tmp_avg_part_width = tmp_surface_width / tmp_parts;

        auto w = (int) tmp_avg_part_width / 2 + (rand() % (int)(tmp_avg_part_width / 2));
        auto h = (int) surface_height / 4 + (rand() % (int)(surface_height / 3));

        parts.emplace_back(std::make_pair(w, h)); 

        tmp_surface_width -= w;
        tmp_x += w;
    }
    // ADD ONE LAST SURFACE PART IF NEEDED
    if ( tmp_surface_width > 0)
    {
        auto w = tmp_surface_width; 
        auto h = (int) surface_height / 4 + (rand() % (int)(surface_height / 3));
        parts.emplace_back(std::make_pair(w, h)); 
    }

    //CREATION OF FRACTAL NOISE
    tmp_x = surface_x;
    tmp_avg_part_width = surface_width / fq;
    tmp_parts = fq;
    tmp_surface_width = surface_width;
    auto ypsilons = std::vector<float>();

    auto nh = (float) rand() / RAND_MAX;
    for (auto i = 0; i < fq; ++i)
    {
        tmp_parts = fq - i;
        tmp_avg_part_width = tmp_surface_width / tmp_parts;

        auto w = (int) tmp_avg_part_width / 2 + (rand() % (int)(tmp_avg_part_width / 2));
        auto h = nh;
        nh = (float) rand() / RAND_MAX;

        for (auto x = 0; x < w; ++x) { ypsilons.emplace_back(cerp(h, nh, (float) x / w)); }

        tmp_surface_width -= w;
        tmp_x += w;
    }
    if ( tmp_surface_width > 0)
    {
        auto w = tmp_surface_width; 
        auto h = nh;
        nh = (float) rand() / RAND_MAX;
        for (auto x = 0; x < w; ++x) { ypsilons.emplace_back(cerp(h, nh, (float) x / w)); }
    }

    // CREATE SURFACE PARTS WITH FRACTAL NOISE
    Structures::SurfacePart* surface_part_before = nullptr;
    tmp_x = surface_x;
    for (auto i = 0; i < (int)parts.size(); ++i)
    {
        auto part = parts.at(i);
        auto w = std::get<0>(part);
        auto h = std::get<1>(part);

        float nsy = 0.0;
        auto fq = 2;
        auto lq = 0.5;
        for (auto o = 0; o < octaves; ++o) { nsy += ypsilons[(tmp_x * fq) % ypsilons.size()] * lq;    fq = pow(fq, o); lq = pow(lq, o); }
        float ney = 0.0;
        fq = 2;
        lq = 0.5;
        for (auto o = 0; o < octaves; ++o) { ney += ypsilons[((tmp_x + w) * fq) % ypsilons.size()] * lq;    fq = pow(fq, o); lq = pow(lq, o); }
        // CALLCULATE START AND END Y
        //                                           NOISE WIDTH   CENTER CORECTION 
        //                                                     N   C
        auto sy = surface_rect.y + surface_rect.h - h - (nsy * 8 - 4);
        auto ey = surface_rect.y + surface_rect.h - h - (ney * 8 - 4);

        auto& surface_part = map.SurfacePart(tmp_x, tmp_x + w - 1, sy, ey, surface_rect.y + surface_rect.h - h); 
        
        if (surface_part_before != nullptr) 
        { 
            surface_part_before->SetNext(&surface_part); 
            surface_part.SetBefore(surface_part_before);
        }

        // CALCULATE Y FOR SURFACE PART
        for (auto x = tmp_x; x < tmp_x + w; ++x)
        {
            float noise = 0.0;
            auto fq = 2;
            auto lq = 0.5;
            for (auto o = 0; o < octaves; ++o) 
            { 
                noise += ypsilons[(x * fq) % ypsilons.size()] * lq;    
                fq = pow(fq, o);
                lq = pow(lq, o);
            }

            //                                                                                        NOISE WIDTH   CENTER CORECTION 
            //                                                                                                  N   C
            for (auto _y = surface_rect.y + surface_rect.h; _y > surface_rect.y + surface_rect.h - h - (noise * 8 - 4); --_y) { surface_part.add((Pixel){x, _y}); }
            surface_part.AddY(surface_rect.y + surface_rect.h - h - (noise * 8 - 4));
        }
        tmp_x += w;
        
        // SET BEFORE SURFACE PART POINTER
        surface_part_before = &surface_part;
    }
};

EXPORT inline void GenerateHills(Map& map)
{
    printf("GenerateHills\n");

    auto hills = map.GetStructures(Structures::HILL);
    for (auto* ref: hills)
    {
        auto& hill = *ref; 
        auto hill_rect = hill.bbox(); 
        
        auto sx = hill_rect.x;
        auto ex = hill_rect.x + hill_rect.w;

        Structures::SurfacePart* s_part = map.GetSurfacePart(sx);
        Structures::SurfacePart* e_part = map.GetSurfacePart(ex);

        auto sy = s_part->GetY(sx);
        auto ey = e_part->GetY(ex);

        hill.clear();
        CreateHill(hill_rect, hill, sy, ey);
    }
};


EXPORT inline void GenerateHoles(Map& map)
{
    printf("GenerateHoles\n");

    auto holes = map.GetStructures(Structures::HOLE);
    for (auto* ref: holes)
    {
        auto& hole = *ref; 
        auto hole_rect = hole.bbox(); 
        
        auto sx = hole_rect.x;
        auto ex = hole_rect.x + hole_rect.w;

        Structures::SurfacePart* s_part = map.GetSurfacePart(sx);
        Structures::SurfacePart* e_part = map.GetSurfacePart(ex); 

        auto sy = s_part->GetY(sx);
        auto ey = e_part->GetY(ex);

        //auto& eraser = map.Eraser();
        //for (auto p: hole) { eraser.Erase(p); }

        hole.clear();
        CreateHole(hole_rect, hole, sy, ey);
    }
};

EXPORT inline void GenerateIslands(Map& map)
{
    printf("GenerateIslands\n");

    auto islands = map.GetStructures(Structures::FLOATING_ISLAND);
    for (auto* island: islands)
    {
        auto island_rect = island->bbox();
        island->clear();
        CreateIsland(island_rect, *island, rand() % 2);
    }
};

EXPORT inline void GenerateCliffsTransitions(Map& map)
{
    printf("GenerateCliffs\n");

    auto surface_rect = map.Surface().bbox();
    auto* s_one = map.GetSurfaceBegin();
    auto* s_two = s_one->Next(); 

    do {
        Pixel p0 = {s_one->EndX(), s_one->EndY()};
        Pixel p1 = {s_two->StartX(), s_two->StartY()};
        auto meta0 = map.GetMetadata(p0);
        auto meta1 = map.GetMetadata(p1);
        
        //printf("p0 = [%d,%d], p1 = [%d,%d]\n", p0.x, p0.y, p1.x, p1.y);

        if (((meta0.owner == nullptr) && (meta1.owner == nullptr)) || 
            ((meta0.owner != nullptr) && (meta0.owner->GetType() == Structures::SURFACE_PART) && 
             (meta1.owner != nullptr) && (meta1.owner->GetType() == Structures::SURFACE_PART)))
        {
            auto sign = 1 ? p0.y < p1.y : -1;

            auto y_diff = abs(p0.y - p1.y);
            if (y_diff >= 20 && y_diff <= 40) // CLIFF
            {
                Rect rect;
                rect.h = abs(p0.y - p1.y);
                rect.w = rect.h + rand() % 20;
                if (sign > 0) // RIGHT
                {
                    rect.x = p0.x;
                    rect.y = p0.y - rand() % 5; // POINTING UPWARDS
                }
                else // LEFT
                {
                    rect.x = p1.x - rect.w;
                    rect.y = p1.y - rand() % 5; // POINTING UPWARDS 
                }

                auto& cliff = map.Structure(Structures::CLIFF);
                CreateCliff(rect, cliff, p0, p1);
                //FillWithRect(rect, cliff);
            }
            else // TRANSITION
            { 
                Rect rect;
                rect.w = 40 + rand() % 30;
                rect.h = surface_rect.y + surface_rect.h - std::min(p0.y, p1.y); 

                Pixel p {0, 0};
                if (sign > 0) // RIGHT
                {
                    rect.x = p0.x;
                    rect.y = p0.y;
                    auto* part = map.GetSurfacePart(rect.x + rect.w);
                    p = {rect.x + rect.w, part->GetY(rect.x + rect.w)};
                }
                else // LEFT
                {
                    rect.x = p1.x - rect.w;
                    rect.y = p1.y; 
                    auto* part = map.GetSurfacePart(rect.x);
                    p = {rect.x, part->GetY(rect.x)};
                }

                
                auto& transition = map.Structure(Structures::TRANSITION);
                CreateTransition(rect, transition, p); 
                //FillWithRect(rect, transition);
            }
        }

        s_one = s_one->Next();
        s_two = s_two->Next();
    } while(s_two != nullptr && s_one->Next() != nullptr);
};


};
