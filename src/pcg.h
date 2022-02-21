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
inline void CreateHill(const Rect& rect, PixelArray& arr)
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
inline void CreateHole(const Rect& rect, PixelArray& arr)
{
    double sx = rect.x;
    double cx = rect.x + (int)(rect.w / 4) + (rand() % (int)(rect.w / 4));
    double ex = rect.x + rect.w;
    double sy = rect.y + rect.h - (rand() % (int)(rect.h / 3)) - 32;
    double ey = sy + -8 + (rand() % 17);
    double cy = std::max(sy, ey) + (rand() % (int)(std::max(sy, ey) - rect.y - rect.h));

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

inline std::unordered_set<int> DomainInsidePixelArray(const Rect& rect, const PixelArray& arr, int step = 1)
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
EXPORT void define_horizontal(Map& map)
{
    printf("define horizontal\n");

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
EXPORT void define_biomes(Map& map)
{
    printf("define biomes\n");

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
            PixelsAroundRect(jungle_x + i, i, jungle_width, 2, jungle);
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
EXPORT void define_hills_holes_islands(Map& map)
{
    printf("define hills, holes, islands\n");

    int width = map.Width();
    
    int hill_count = map.HillsFrequency() * 12;
    int hole_count = map.HolesFrequency() * 10;
    int floating_island_count = map.IslandsFrequency() * 8;

    int ocean_width = 250;
    int hill_width = 80;
    int hole_width = 60;
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
            printf("Creating hole at x: %d\n", x);
            auto& hole = map.MiniBiome(MiniBiomes::HOLE);
            printf("After creating hole\n");
            Rect rect ((int) x - hole_width / 2, Surface.y, hole_width, Surface.h);
            CreateHole(rect, hole);
        }
        
        for (auto& var: hills)
        {
            printf("Creating hill\n");
            auto x = result[var];
            auto& hill = map.MiniBiome(MiniBiomes::HILL);
            Rect rect ((int) x - hill_width / 2, Surface.y, hill_width, Surface.h);
            CreateHill(rect, hill);
        }

        for (auto& var: islands)
        {
            printf("Creating island\n");
            auto x = result[var];
            auto& island = map.MiniBiome(MiniBiomes::FLOATING_ISLAND);
            Rect rect ((int) x - island_width / 2, Surface.y, island_width, 50);
            PixelsAroundRect(rect.x, rect.y, rect.w, rect.h, island);
        }
    }
};

/*
 * Define underground cabin minibiomes
 */ 
EXPORT void define_cabins(Map& map)
{
    printf("define cabins\n");

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
            printf("Creating cabin\n");
            auto v = result[var];
            auto x = (int) tundra_rect.x + + cabin_width / 2 + (v % tundra_rect.w); 
            auto y = (int) tundra_rect.y + cabin_height / 2 + (v / tundra_rect.w);

            auto& cabin = map.MiniBiome(MiniBiomes::CABIN); 
            PixelsAroundRect(x, y, cabin_width, cabin_height, cabin);
        }
    }
};

/**
 * Definition of castles for each biome
 */
EXPORT void define_castles(Map& map)
{
    printf("define castles\n");

    auto castle_width = 250;
    auto castle_height = 200;

    auto& Underground = map.Underground();
    auto& Cavern = map.Cavern(); 
    auto UCRect = RectUnion(Underground.bbox(), Cavern.bbox());

    auto& forest = *map.GetBiome(Biomes::FOREST);
    auto forest_rect = RectIntersection(UCRect, forest.bbox());

    auto& tundra = *map.GetBiome(Biomes::TUNDRA);
    auto tundra_rect = RectIntersection(UCRect, tundra.bbox());
    
    auto& jungle = *map.GetBiome(Biomes::JUNGLE);
    auto jungle_rect = RectIntersection(UCRect, jungle.bbox());

    printf("forest rect %d, %d, %d, %d\n", forest_rect.x, forest_rect.y, forest_rect.w, forest_rect.h);
    printf("jungle rect %d, %d, %d, %d\n", jungle_rect.x, jungle_rect.y, jungle_rect.w, jungle_rect.h);
    printf("tundra rect %d, %d, %d, %d\n", tundra_rect.x, tundra_rect.y, tundra_rect.w, tundra_rect.h);
    
    auto variables = CreateVariables({"forest_castle", "jungle_castle", "tundra_castle"});

    // DEFINITION OF UNION DOMAIN
    auto forest_domain = DomainInsidePixelArray(forest_rect, forest, 10);
    auto jungle_domain = DomainInsidePixelArray(jungle_rect, jungle, 10);
    auto tundra_domain = DomainInsidePixelArray(tundra_rect, tundra, 10);

    std::unordered_map<std::string, std::unordered_set<int>> domains;
    domains["forest_castle"] = forest_domain;
    domains["jungle_castle"] = jungle_domain;
    domains["tundra_castle"] = tundra_domain;

    // DEFINITION OF CONSTRAINTS
    InsidePixelArrayConstraint2D<std::string, int> c0 ("forest_castle", castle_width, castle_height, forest, forest_rect);
    InsidePixelArrayConstraint2D<std::string, int> c1 ("jungle_castle", castle_width, castle_height, jungle, jungle_rect);
    InsidePixelArrayConstraint2D<std::string, int> c2 ("tundra_castle", castle_width, castle_height, tundra, tundra_rect);

    // CREATION OF SOLVER
    CSPSolver<std::string, int> solver {variables, domains};

    // REGISTRATION OF CONSTRAINTS
    solver.add_constraint(c0);
    solver.add_constraint(c1);
    solver.add_constraint(c2);
        
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
        printf("forest_v: %d\n", forest_v);
        auto f_x = forest_rect.x + castle_width / 2 + (forest_v % forest_rect.w);
        auto f_y = forest_rect.y + castle_height / 2 + (forest_v / forest_rect.w);
        auto& forest_castle = map.MiniBiome(MiniBiomes::CASTLE);
        PixelsAroundRect(f_x, f_y, castle_width, castle_height, forest_castle);

        auto tundra_v = result["tundra_castle"];
        printf("tundra_v: %d\n", tundra_v);
        auto t_x = tundra_rect.x + castle_width / 2 + (tundra_v % tundra_rect.w);
        auto t_y = tundra_rect.y + castle_height / 2 + (tundra_v / tundra_rect.w);
        auto& tundra_castle = map.MiniBiome(MiniBiomes::CASTLE);
        PixelsAroundRect(t_x, t_y, castle_width, castle_height, tundra_castle);

        auto jungle_v = result["jungle_castle"];
        printf("jungle_v: %d\n", jungle_v);
        auto j_x = jungle_rect.x + castle_width / 2 + (jungle_v % jungle_rect.w);
        auto j_y = jungle_rect.y + castle_height / 2 + (jungle_v / jungle_rect.w);
        auto& jungle_castle = map.MiniBiome(MiniBiomes::CASTLE);
        PixelsAroundRect(j_x, j_y, castle_width, castle_height, jungle_castle);


        printf("forest castle %d:%d\n", f_x, f_y);
        printf("tundra castle %d:%d\n", t_x, t_y);
        printf("jungle castle %d:%d\n", j_x, j_y);
    }
};

};
