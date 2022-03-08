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
        Y = {(double)s.y, (double)rect.y, (double) rect.y + rand() % (int)(rect.w * 0.3), (double)e.y};
    }
    else // LEFT
    {
        X = {(double)e.x, (double)rect.x + rect.w * 0.25, (double)rect.x, (double)e.x};
        Y = {(double)e.y, (double)rect.y, (double) rect.y + rand() % (int)(rect.w * 0.3), (double)s.y};
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
    std::vector<double> X {(double)rect.x, (double)rect.x + (double)rect.w / 4 + (rand() % (int)(rect.w / 2)), (double)rect.x + rect.w};
    std::vector<double> Y;

    double h = abs(rect.y - p.y);
    if (rect.x == p.x) // LEFT
        Y = {(double)p.y, (double)rect.y + (h / 4) + (rand() % (int)(h / 2)), (double)rect.y}; 
    else
        Y = {(double)rect.y, (double)rect.y + (h / 4) + (rand() % (int)(h / 2)), (double)p.y}; 
    
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
    auto quater_y = rect.y + rect.h / 3;
    auto full_y = rect.y + rect.h + 5;

    X = {(double)half_x, (double)rect.x + rect.w, (double)half_x + ((rand() % 11) - 5), (double)rect.x, (double)half_x};
    Y = {(double)full_y, (double)quater_y + ((rand() % 5) - 3), (double) quater_y + ((rand() % 5) - 3), (double)quater_y + ((rand() % 5) - 3), (double)full_y};

    std::vector<double> T { 0.0 };
    for(auto i=1; i < (int)X.size(); i++)
        T.push_back(T[i-1] + sqrt( (X[i]-X[i-1])*(X[i]-X[i-1]) + (Y[i]-Y[i-1])*(Y[i]-Y[i-1]) ));
    tk::spline sx(T,X), sy(T,Y);

    auto size = (int)T.back();
    Point polygon[size]; 
    for (auto t = 0; t < size; ++t) polygon[t] = {(int)sx(t),(int)sy(t)};
    
    for (auto x = rect.x; x <= rect.x + rect.w; ++x)
        for (auto y = rect.y; y <= rect.y + rect.h; ++y)
            if (CNPnPoly({x, y}, polygon, size)) arr.add({x, y});
};

/*
 * Namespace for Chasm algorithm related stuff
 */
namespace Chasm
{
    enum Direction {
        TOP, 
        LEFT, 
        BOTTOM, 
        RIGHT
    };

    typedef struct Point {

        enum Type {
            VOID, 
            WALL, 
            ACID, 
            NONE
        };

        Type type;
        int d; // durability
        std::unordered_set<Pixel, PixelHash, PixelEqual> visited_points;
    } Point;


    /**
     * Create pixel point mappint, where surface_structure is defined, there is wall, else void 
     */
    inline auto MappingFromRect(const Rect& rect, Map& map, int wall_durability)
    {
        std::unordered_map<Pixel, Chasm::Point, PixelHash, PixelEqual> mapping;
        for (auto x = rect.x; x < rect.x + rect.w; ++x)
        {
            for (auto y = rect.y; y < rect.y + rect.h; ++y)
            {
                auto p = (Pixel){x, y};
                auto meta = map.GetMetadata({x, y});
                if (meta.surface_structure != nullptr)
                    mapping[p] = {Chasm::Point::WALL, wall_durability};
                else
                    mapping[p] = {Chasm::Point::VOID, 0};
            }
        }
        return mapping;
    };

    inline auto Top(const Rect& rect, 
            int x, int y, 
            std::unordered_map<Pixel, Chasm::Point, PixelHash, PixelEqual>& mapping 
    ){
        Pixel k {x, y - 1};
        if (y > rect.y)
            return mapping[k];
        return (Chasm::Point){Chasm::Point::NONE, 0, {}};
    };

    inline auto Left(const Rect& rect, 
            int x, int y, 
            std::unordered_map<Pixel, Chasm::Point, PixelHash, PixelEqual>& mapping 
    ){
        Pixel k {x - 1, y};
        if (x > rect.x)
            return mapping[k];
        return (Chasm::Point){Chasm::Point::NONE, 0, {}};
    };

    inline auto Bottom(const Rect& rect, 
            int x, int y, 
            std::unordered_map<Pixel, Chasm::Point, PixelHash, PixelEqual>& mapping 
    ){
        Pixel k {x, y + 1};
        if (y < rect.y + rect.h - 1)
            return mapping[k];
        return (Chasm::Point){Chasm::Point::NONE, 0, {}};
    };

    inline auto Right(const Rect& rect, 
            int x, int y, 
            std::unordered_map<Pixel, Chasm::Point, PixelHash, PixelEqual>& mapping 
    ){
        Pixel k {x + 1, y};
        if (x < rect.x + rect.w - 1)
            return mapping[k];
        return (Chasm::Point){Chasm::Point::NONE, 0, {}};
    };

    inline auto TopLeft(const Rect& rect, 
            int x, int y, 
            std::unordered_map<Pixel, Chasm::Point, PixelHash, PixelEqual>& mapping 
    ){
        Pixel k {x - 1, y - 1};
        if (x > rect.x && y > rect.y)
            return mapping[k];
        return (Chasm::Point){Chasm::Point::NONE, 0, {}};
    };

    inline auto TopRight(const Rect& rect, 
            int x, int y, 
            std::unordered_map<Pixel, Chasm::Point, PixelHash, PixelEqual>& mapping 
    ){
        Pixel k {x + 1, y - 1};
        if (x < (rect.x + rect.w - 1) && y > rect.y)
            return mapping[k];
        return (Chasm::Point){Chasm::Point::NONE, 0, {}};
    };

    inline auto BottomLeft(const Rect& rect, 
            int x, int y, 
            std::unordered_map<Pixel, Chasm::Point, PixelHash, PixelEqual>& mapping 
    ){
        Pixel k {x - 1, y + 1};
        if (x > rect.x && y < (rect.y + rect.h - 1))
            return mapping[k];
        return (Chasm::Point){Chasm::Point::NONE, 0, {}};
    };

    inline auto BottomRight(const Rect& rect, 
            int x, int y, 
            std::unordered_map<Pixel, Chasm::Point, PixelHash, PixelEqual>& mapping 
    ){
        Pixel k {x + 1, y + 1};
        if (x < (rect.x + rect.w - 1) && y < (rect.y + rect.h - 1))
            return mapping[k];
        return (Chasm::Point){Chasm::Point::NONE, 0, {}};
    };

    inline auto Step(
            const Rect& rect, 
            std::unordered_map<Pixel, Chasm::Point, PixelHash, PixelEqual> mapping, 
            std::unordered_set<Pixel, PixelHash, PixelEqual> points
    ){
        for (auto x = rect.x; x < rect.x + rect.w; ++x)
        {
            for (auto y = rect.y; y < rect.y + rect.h; ++y)
            {
                auto p = (Pixel){x, y};
                auto cell = mapping[p];

                if (cell.type == Chasm::Point::ACID)
                {
                    auto p_v = (Pixel){0, 0};
                    auto min_d = rect.w * rect.h;
                    for (auto p: points)
                    {
                        if (cell.visited_points.count(p) == 0)
                        {
                            if ((p.x == x) && (p.y == y))
                            {
                                cell.visited_points.insert(p);
                            }
                            else
                            {
                                auto tmp_v = Pixel(x - p.x, y - p.y);
                                auto d = sqrt((tmp_v.x * tmp_v.x) + (tmp_v.y * tmp_v.y));
                                if (d < 3)
                                {
                                    cell.visited_points.insert(p);
                                }
                                else if (d < min_d)
                                {
                                    min_d = d;
                                    p_v = tmp_v;
                                }
                            }
                        }
                    }

                    if ((cell.visited_points.size() == points.size()) || cell.d <= 0)
                    {
                        cell.type = Chasm::Point::VOID;
                        cell.d = 0;
                        cell.visited_points = {};
                        mapping[p] = cell;
                    }
                    else
                    {
                        std::vector<Chasm::Direction> dirs;
                        if (p_v.x > 0) for (auto i = 0; i < p_v.x; ++i) dirs.push_back(Chasm::Direction::LEFT);
                        if (p_v.x < 0) for (auto i = 0; i < -p_v.x; ++i) dirs.push_back(Chasm::Direction::RIGHT);
                        if (p_v.y > 0) for (auto i = 0; i < p_v.y; ++i) dirs.push_back(Chasm::Direction::TOP);
                        if (p_v.y < 0) for (auto i = 0; i < -p_v.y; ++i) dirs.push_back(Chasm::Direction::BOTTOM);
                        auto direction = dirs[rand() % (dirs.size() - 1)];

                        Chasm::Point nb;
                        switch (direction)
                        {
                            case Chasm::Direction::TOP:
                                nb = Chasm::Top(rect, x, y, mapping);
                                break;
                            case Chasm::Direction::LEFT:
                                nb = Chasm::Left(rect, x, y, mapping);
                                break;
                            case Chasm::Direction::BOTTOM:
                                nb = Chasm::Bottom(rect, x, y, mapping);
                                break;
                            case Chasm::Direction::RIGHT:
                                nb = Chasm::Right(rect, x, y, mapping);
                                break;
                        }

                        if (nb.type != Chasm::Point::NONE && nb.type != Chasm::Point::ACID)
                        {
                            if (nb.type == Chasm::Point::WALL)
                            {
                                if (nb.d > 1)
                                {
                                    nb.d -= 1;
                                }
                                else
                                {
                                    if (cell.d > 1)
                                    {
                                        nb.type = Chasm::Point::ACID;
                                        nb.d = cell.d - 1;
                                        nb.visited_points = cell.visited_points;
                                    }
                                    else
                                    {
                                        nb.type = Chasm::Point::VOID;
                                        nb.d = 0;
                                        nb.visited_points = {};
                                    }
                                    cell.type = Chasm::Point::VOID;
                                    cell.d = 0;
                                    cell.visited_points = {};
                                }
                            }
                            else
                            {
                                nb.type = Chasm::Point::ACID; 
                                nb.d = cell.d;
                                nb.visited_points = cell.visited_points;
                                cell.type = Chasm::Point::VOID;
                                cell.d = 0;
                                cell.visited_points = {};
                            }

                            auto p0 = (Pixel){x, y - 1};
                            auto p1 = (Pixel){x - 1, y};
                            auto p2 = (Pixel){x, y + 1};
                            auto p3 = (Pixel){x + 1, y};
                            switch (direction)
                            {
                                case Chasm::Direction::TOP:
                                    mapping[p0] = nb;
                                    break;
                                case Chasm::Direction::LEFT:
                                    mapping[p1] = nb;
                                    break;
                                case Chasm::Direction::BOTTOM:
                                    mapping[p2] = nb;
                                    break;
                                case Chasm::Direction::RIGHT:
                                    mapping[p3] = nb;
                                    break;
                            }
                            mapping[p] = cell;
                        }
                    }
                }
            }
        }
        return mapping;
    };

    inline auto SmoothStep(
            const Rect& rect, 
            std::unordered_map<Pixel, Chasm::Point, PixelHash, PixelEqual> mapping 
    ){
        for (auto x = rect.x + 1; x < rect.x + rect.w - 1; ++x)
        {
            for (auto y = rect.y + 1; y < rect.y + rect.h - 1; ++y)
            {
                Pixel p {x, y};
                auto alive = 0;
                if (Chasm::Top(rect, x, y, mapping).type == Chasm::Point::WALL) ++alive;
                if (Chasm::TopLeft(rect, x, y, mapping).type == Chasm::Point::WALL) ++alive;
                if (Chasm::TopRight(rect, x, y, mapping).type == Chasm::Point::WALL) ++alive;
                if (Chasm::Left(rect, x, y, mapping).type == Chasm::Point::WALL) ++alive;
                if (Chasm::Right(rect, x, y, mapping).type == Chasm::Point::WALL) ++alive;
                if (Chasm::Bottom(rect, x, y, mapping).type == Chasm::Point::WALL) ++alive;
                if (Chasm::BottomLeft(rect, x, y, mapping).type == Chasm::Point::WALL) ++alive;
                if (Chasm::BottomRight(rect, x, y, mapping).type == Chasm::Point::WALL) ++alive;
                
                if (alive < 4) mapping[p] = {Chasm::Point::VOID, 0, {}}; 
            }
        }

        return mapping;
    };
};

inline void CreateChasm(const Rect& rect, PixelArray& arr, Map& map)
{
    auto steps = 80;
    auto smooth_steps = 5;
    auto spawn_every_x_steps = 8;
    auto rect_mapping = Chasm::MappingFromRect(rect, map, 2);

    auto points_count = 5 + rand() % 4;
    std::unordered_set<Pixel, PixelHash, PixelEqual> points;

    for (auto _ = 0; _ < points_count; ++_)
        points.insert({
            rect.x + (rand() % (rect.w - 1)), 
            rect.y + rect.h / 2 + (rand() % (int)((rect.h / 2) - 1))
        });

    // SPAWN ACID FROM TOP
    auto spawn_acid = [&](){
        for (auto x = rect.x; x < rect.x + rect.w; ++x)
        {
            auto p = (Pixel){x, rect.y};
            rect_mapping[p] = {Chasm::Point::ACID, 20, {}};
        }
    };

    // SIMULATE
    for (auto i = 0; i < steps; ++i)
    {
        if ((i % spawn_every_x_steps) == 0)
            spawn_acid();

        rect_mapping = Chasm::Step(rect, rect_mapping, points);
    }

    // SMOOTH
    for (auto i = 0; i < smooth_steps; ++i)
        rect_mapping = Chasm::SmoothStep(rect, rect_mapping);

    // PUSH RESULTS
    for (auto x = rect.x; x < rect.x + rect.w; ++x)
    {
        for (auto y = rect.y; y < rect.y + rect.h; ++y)
        {
            auto p = (Pixel){x, y};
            auto point = rect_mapping[p];
            if (point.type == Chasm::Point::WALL) arr.add(p);
        }
    }
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

    auto& ocean_left = map.Biome(Biomes::OCEAN_LEFT);
    PixelsOfRect(0, Surface.y, ocean_width, Surface.h, ocean_left); 

    auto& ocean_right = map.Biome(Biomes::OCEAN_RIGHT);
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
    int surface_parts = 4 + (int)(30 * map.SurfacePartsCount());
    int octaves = 1 + (int)(5 * map.SurfacePartsOctaves());
    int fq = 60 + (int)(200 * map.SurfacePartsFrequency());

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
        auto h = (int) surface_height / 4 + (rand() % (int)(surface_height / 4));

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

        auto& s_hill = map.SurfaceStructure(Structures::HILL);
        CreateHill(hill_rect, s_hill, sy, ey);
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

        for (auto p: hole)
        {
            auto meta = map.GetMetadata(p);
            meta.surface_structure = nullptr;
            map.SetMetadata(p, meta);
        }

        auto& s_hole = map.SurfaceStructure(Structures::HOLE);
        CreateHole(hole_rect, s_hole, sy, ey);
    }
};

EXPORT inline void GenerateIslands(Map& map)
{
    printf("GenerateIslands\n");

    auto islands = map.GetStructures(Structures::FLOATING_ISLAND);
    for (auto* island: islands)
    {
        auto island_rect = island->bbox();
        auto& s_island = map.SurfaceStructure(Structures::FLOATING_ISLAND);
        CreateIsland(island_rect, s_island, rand() % 2);
    }
};

EXPORT inline void GenerateCliffsTransitions(Map& map)
{
    printf("GenerateCliffsTransitions\n");

    auto surface_rect = map.Surface().bbox();
    auto* s_one = map.GetSurfaceBegin();
    auto* s_two = s_one->Next(); 

    do {
        Pixel p0 = {s_one->EndX(), s_one->EndY()};
        Pixel p1 = {s_two->StartX(), s_two->StartY()};
        auto meta0 = map.GetMetadata(p0);
        auto meta1 = map.GetMetadata(p1);
        
        printf("p0 = [%d,%d], p1 = [%d,%d]\n", p0.x, p0.y, p1.x, p1.y);

        if ((meta0.structure == nullptr) && (meta1.structure == nullptr))
        {
            auto sign = 1 ? p0.y < p1.y : -1;

            auto y_diff = abs(p0.y - p1.y);
            if (y_diff >= 25 && y_diff <= 35) // CLIFF
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

                auto& cliff = map.SurfaceStructure(Structures::CLIFF);
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
                    while (rect.w > 0) {
                        auto m = map.GetMetadata({rect.x + rect.w, rect.y});
                        if (m.structure != nullptr)
                            rect.w -= 1;
                        else
                            break;
                    }
                    auto* part = map.GetSurfacePart(rect.x + rect.w);
                    while (part == nullptr) { rect.w -= 1; part = map.GetSurfacePart(rect.x + rect.w); }
                    p = {rect.x + rect.w, part->GetY(rect.x + rect.w)};
                }
                else // LEFT
                {
                    rect.x = p1.x - rect.w;
                    rect.y = p1.y; 
                    while (rect.w > 0) {
                        auto m = map.GetMetadata({rect.x, rect.y});
                        if (m.structure != nullptr)
                        {
                            rect.w -= 1;
                            rect.x += 1;
                        }
                        else
                            break;
                    }
                    auto* part = map.GetSurfacePart(rect.x);
                    p = {rect.x, part->GetY(rect.x)};
                }

                if (abs(rect.y - p.y) > 2)
                {
                    auto& transition = map.SurfaceStructure(Structures::TRANSITION);
                    CreateTransition(rect, transition, p); 
                    //FillWithRect(rect, transition);
                }
            }
        }

        s_one = s_one->Next();
        s_two = s_two->Next();
    } while(s_one != nullptr && s_two != nullptr);
};

EXPORT inline void GenerateChasms(Map& map)
{
    printf("GenerateChasms");

    auto Surface = map.Surface();
    auto& ocean_left = map.Biome(Biomes::OCEAN_LEFT);
    auto ocean_left_rect = ocean_left.bbox();
    auto& ocean_right = map.Biome(Biomes::OCEAN_LEFT);
    auto ocean_right_rect = ocean_right.bbox();
    auto width = map.Width();
    auto surface_rect = Surface.bbox();
    auto chasms_count = 2 + (int)(map.ChasmFrequency() * 10);
    auto chasm_width = 50;

    for (auto c = 0; c < chasms_count + 1; ++c)
    {
        auto w = chasm_width + (rand() % 41) - 20;
        auto a_w = width - ocean_left_rect.w - ocean_right_rect.w - w; 
        auto x = ocean_left_rect.w + (rand() % a_w);

        Rect chasm_rect = {x - w / 2, surface_rect.y + surface_rect.h / 3, w, surface_rect.h / 2}; 

        auto& chasm = map.Structure(Structures::CHASM);
        FillWithRect(chasm_rect, chasm);

        auto& s_chasm = map.SurfaceStructure(Structures::CHASM);
        CreateChasm(chasm_rect, s_chasm, map);

        for (auto p: chasm)
        {
            if (!s_chasm.contains(p))
            {
                auto meta = map.GetMetadata(p);
                meta.surface_structure = nullptr;
                map.SetMetadata(p, meta);
            }
        }
    }
};

EXPORT inline void GenerateGrass(Map& map)
{
    printf("GenerateGrass\n");

    auto& Surface = map.Surface();
    auto surface_rect = Surface.bbox();
    auto& grass = map.SurfaceStructure(Structures::GRASS);
    auto* surface_part = map.GetSurfaceBegin();

    auto A_STRUCT = Structures::SURFACE_PART | Structures::HILL | Structures::HOLE | 
                Structures::CLIFF | Structures::TRANSITION | Structures::SURFACE_TUNNEL |
                Structures::CHASM;
    auto A_BIOMES = Biomes::FOREST | Biomes::JUNGLE;
    std::unordered_set<Pixel, PixelHash, PixelEqual> visited;
    std::vector<Pixel> queue;
    auto traverse = [&](Pixel start)
    {
       queue.push_back(start); 

       while (queue.size() > 0)
       {
            auto p = queue.back();
            queue.pop_back();
            if (visited.count(p) == 1)
                continue;

            visited.insert(p);

            auto info_this = map.GetMetadata(p);
            auto info_up = map.GetMetadata({p.x, p.y - 1}); 
            //auto info_down = map.GetMetadata({p.x, p.y + 1});
            auto info_left = map.GetMetadata({p.x - 1, p.y});
            auto info_right = map.GetMetadata({p.x + 1, p.y});

            if ((info_this.biome != nullptr) && (info_this.biome->GetType() & A_BIOMES) &&
                (info_this.surface_structure != nullptr) && (info_this.surface_structure->GetType() & A_STRUCT) &&
                ((info_up.surface_structure == nullptr) || (info_left.surface_structure == nullptr) || (info_right.surface_structure == nullptr)))
            {
                grass.add(p);

                queue.push_back({p.x - 1, p.y});
                queue.push_back({p.x + 1, p.y});
                queue.push_back({p.x, p.y + 1});
                queue.push_back({p.x, p.y - 1});
                queue.push_back({p.x - 1, p.y + 1});
                queue.push_back({p.x - 1, p.y - 1});
                queue.push_back({p.x + 1, p.y + 1});
                queue.push_back({p.x + 1, p.y - 1});
            }
       }
    };

    do 
    {
        auto sp = (Pixel){surface_part->StartX(), surface_rect.y};
        while (sp.y < (surface_rect.y + surface_rect.h - 1)) 
        {
            auto meta = map.GetMetadata(sp);
            if (meta.surface_structure == nullptr)
                sp.y += 1;
            else
                break;
        }
        auto ep = (Pixel){surface_part->EndX(), surface_rect.y};
        while (ep.y < (surface_rect.y + surface_rect.h - 1)) 
        {
            auto meta = map.GetMetadata(ep);
            if (meta.surface_structure == nullptr)
                ep.y += 1;
            else
                break;
        }
 
        traverse(sp);
        traverse(ep);
        surface_part = surface_part->Next();
    } 
    while (surface_part != nullptr);
};

EXPORT inline void GenerateTrees(Map& map)
{
    printf("Generate Trees\n");
    auto count = 150;

    auto* grass = map.GetSurfaceStructures(Structures::GRASS)[0];
    auto size = grass->size(); 

    auto check_placement = [&](Pixel start, int height) -> bool
    {
        int h = 1; 
        while (h < height + 1)
        {
            Pixel p = {start.x, start.y - h};
            auto info_this = map.GetMetadata(p); 
            auto info_left = map.GetMetadata({p.x - 1, p.y});
            auto info_right = map.GetMetadata({p.x + 1, p.y});
            
            if (info_this.surface_structure != nullptr || info_left.surface_structure != nullptr || info_right.surface_structure != nullptr)
                return false;
            ++h;
        };

        return true;
    };

    while (!map.ShouldForceStop() && count > 0)
    {
        auto h = 10 + rand() % 10;
        auto p = *std::next(grass->begin(), rand() % size);

        if (check_placement(p, h))
        {
            auto& tree = map.SurfaceStructure(Structures::TREE);
            auto _h = 0;
            while (_h < h + 1)
            {
                tree.add({p.x, p.y - _h});
                if ( _h > 5)
                {
                    if ((rand() % 6) == 5)
                    {
                        if ((rand() % 2) == 1)
                            tree.add({p.x - 1, p.y - _h});
                        else
                            tree.add({p.x + 1, p.y - _h});
                    }
                }
                ++_h;
            }
            --count;
        }
    };
};


};
