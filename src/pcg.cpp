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

#define EXPORT __declspec(dllexport)

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
    int ocean_width = 250;
    int ice_width = 600;
    int jungle_width = 600;
    Rect Surface = data.HORIZONTAL_AREAS[1].first;
    Rect Cavern = data.HORIZONTAL_AREAS[3].first;

    PixelArray ocean_left;
    PixelsOfRect(0, Surface.y, ocean_width, Surface.h, ocean_left); 

    PixelArray ocean_right;
    PixelsOfRect(width - ocean_width, Surface.y, ocean_width, Surface.h, ocean_right);

    // USE CSP TO FIND LOCATIONS FOR BIOMES
    std::unordered_set<std::string> variables {"jungle", "ice"};
    std::unordered_set<int> domain;
    for (int i = ocean_width + 50; i < width - 2 * ocean_width - 50; i+=50) { domain.insert(i); }
    std::unordered_map<std::string, std::unordered_set<int>> domains {{"jungle", domain}, {"ice", domain}};
    DistanceConstraint<std::string, int> c0 {std::make_pair("jungle", "ice"), std::max(ice_width, jungle_width)};
    CSPSolver<std::string, int> solver {variables, domains};
    solver.add_constraint(c0);
    auto result = solver.backtracking_search({}); 
    int jungle_x = result["jungle"];  
    int ice_x = result["ice"];

    PixelArray jungle;
    for (int i = Surface.y; i < Cavern.y + Cavern.h; i++)
    {
        PixelsAroundRect(jungle_x, i, jungle_width, 2, jungle);
    }

    PixelArray ice;
    for (int i = Surface.y; i < Cavern.y + Cavern.h; i++)
    {
        PixelsAroundRect(ice_x, i, ice_width, 2, ice);
    }


    data.BIOMES.push_back(std::move(ocean_left));
    data.BIOMES.push_back(std::move(ocean_right));
    data.BIOMES.push_back(std::move(jungle));
    data.BIOMES.push_back(std::move(ice));
} 

EXPORT void define_minibiomes(DATA& data)
{
    int width = data.WIDTH;
    int ocean_width = 250;
    int hills_count = 8;
    int hole_count = 8;
    int hills_width = 80;
    int hole_width = 60;
    Rect Surface = data.HORIZONTAL_AREAS[1].first;

    // DEFINE SURFACE MINIBIOMES
    std::unordered_set<std::string> variables;
    for (auto i = 0; i < hills_count; ++i) { variables.insert("hill" + std::to_string(i)); }
    for (auto i = 0; i < hole_count; ++i) { variables.insert("hole" + std::to_string(i)); }
    std::unordered_set<int> domain;
    for (auto i = ocean_width + 50; i < width - 2 * ocean_width - 50; i+=50) { domain.insert(i); }
    std::unordered_map<std::string, std::unordered_set<int>> domains;
    for (auto& var: variables) { domains[var] = domain; }
    std::vector<DistanceConstraint<std::string, int>> constraints;
    for (auto it0 = variables.begin(); it0 != variables.end(); it0++)
    {
        for (auto it1 = std::next(it0); it1 != variables.end(); it1++)
        {
            DistanceConstraint<std::string, int> constraint {make_pair(*it0, *it1), 100 + std::max(hills_width, hole_width)};
            constraints.push_back(std::move(constraint));
        }
    }
    CSPSolver<std::string, int> solver {variables, domains};
    for (auto& constraint: constraints) { solver.add_constraint(constraint); }
    auto result = solver.backtracking_search({});
    for (auto& var: variables)
    {
        PixelArray arr;
        auto x = result[var];
        for (int i = Surface.y; i < Surface.y + Surface.h; i++)
        {
            if (var.find("hole") != std::string::npos)
                PixelsAroundRect(x, i, hole_width, 2, arr);
            else
                PixelsAroundRect(x, i, hills_width, 2, arr);
        }
        data.MINI_BIOMES.push_back(std::move(arr));
    }
}

}
