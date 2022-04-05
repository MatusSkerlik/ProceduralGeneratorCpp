#ifndef SCENE
#define SCENE

#include <future>
#include <chrono>
#include <utility>

#ifndef RAYLIB_H
#include "raylib.h"
#endif

#ifndef UTILS
#include "utils.h"
#endif

#ifndef PCG
#include "pcg.h"
#endif

#ifndef DRAW 
#include "draw.h"
#endif


using namespace std::chrono_literals;


class Scene
{
    public:
        virtual void Run(Map& map) = 0;
        virtual void Render(Map& map) = 0;
};


class DefaultScene: public Scene
{
    private:
        bool GenerateStage0 = true;
        bool GenerateStage1 = true;
        bool GenerateStage2 = true;
        bool GenerateStage3 = true;
        bool GenerateStage4 = true;
        bool GenerateStage5 = true;

    public:
        virtual void Run(Map& map) override
        {
            std::vector<std::tuple<std::string, std::string, std::future<void>>> futures_to_wait;

            if (!map.ShouldForceStop() && GenerateStage0)
            {
                map.ClearStage0();
                map.SetGenerationMessage("DEFINITION OF HORIZONTAL AREAS...");
                DefineHorizontal(map);
            }

            if (!map.ShouldForceStop() && GenerateStage5)
            {
                map.ClearStage4();
                map.SetGenerationMessage("GENERATION OF CAVES...");
                auto generate_caves_future = std::async(std::launch::async, GenerateCaves, std::ref(map));
                futures_to_wait.emplace_back(
                        "GENERATION OF CAVES...", 
                        "GENERATION OF CAVES INFEASIBLE...", 
                        std::move(generate_caves_future)
                );
            }

            if (!map.ShouldForceStop() && GenerateStage1)
            {
                map.ClearStage1();
                map.SetGenerationMessage("DEFINITION OF BIOMES...");
                DefineBiomes(map);
            }

            if (!map.ShouldForceStop() && GenerateStage2)
            {
                map.ClearStage2();

                auto define_structures_future = std::async(std::launch::async, DefineHillsHolesIslands, std::ref(map));
                auto define_cabins_future = std::async(std::launch::async, DefineCabins, std::ref(map));
                auto define_castles_future = std::async(std::launch::async, DefineCastles, std::ref(map));

                map.SetGenerationMessage("DEFINITION OF HILLS, HOLES, ISLANDS...");
                if (define_structures_future.wait_for(5s) == std::future_status::timeout)
                {
                    map.SetForceStop(true);
                    map.Error("DEFINITION OF HILLS, HOLES, ISLANDS INFEASIBLE");
                }
                
                map.SetGenerationMessage("DEFINITION OF UNDERGROUND CABINS...");
                if (define_cabins_future.wait_for(5s) == std::future_status::timeout)
                {
                    map.SetForceStop(true);
                    map.Error("DEFINITION OF UNDERGROUND CABINS INFEASIBLE");
                }

                map.SetGenerationMessage("DEFINITION OF UNDERGROUND CASTLES...");
                if (define_cabins_future.wait_for(5s) == std::future_status::timeout)
                {
                    map.SetForceStop(true);
                    map.Error("DEFINITION OF UNDERGROUND CASTLES INFEASIBLE");
                }
            }

            if (!map.ShouldForceStop() && GenerateStage3)
            {
                map.ClearStage3();
                map.SetGenerationMessage("DEFINITION OF SURFACE...");
                DefineSurface(map);
            }

            if (!map.ShouldForceStop() && GenerateStage4)
            {
                map.SetGenerationMessage("GENERATION OF HILLS...");
                GenerateHills(map);
                map.SetGenerationMessage("GENERATION OF HOLES...");
                GenerateHoles(map);
                map.SetGenerationMessage("GENERATION OF CLIFFS AND TRANSITIONS...");
                GenerateCliffsTransitions(map);
                map.SetGenerationMessage("GENERATION OF LEFT OCEAN...");
                GenerateOceanLeft(map);
                map.SetGenerationMessage("GENERATION OF RIGHT OCEAN...");
                GenerateOceanRight(map);
                map.SetGenerationMessage("GENERATION OF CHASMS...");
                GenerateChasms(map);
                map.SetGenerationMessage("GENERATION OF LAKES...");
                GenerateLakes(map);
                map.SetGenerationMessage("GENERATION OF JUNGLE SWAMP...");
                GenerateJungleSwamp(map);
                map.SetGenerationMessage("GENERATION OF ISLANDS...");
                GenerateIslands(map);
                map.SetGenerationMessage("GENERATION OF GRASS...");
                GenerateGrass(map);

                auto generate_trees_future = std::async(std::launch::async, GenerateTrees, std::ref(map));
                map.SetGenerationMessage("GENERATION OF TREES...");
                if (generate_trees_future.wait_for(2s) == std::future_status::timeout)
                {
                    map.SetForceStop(true);
                    map.Error("DEFINITION OF TREES INFEASIBLE"); 
                }

                map.SetGenerationMessage("GENERATION OF ORES...");
                GenerateSurfaceOres(map);
                map.SetGenerationMessage("GENERATION OF UNDERGROUND MATERIALS...");
                GenerateMaterialUnderground(map);
            }
            
            for (auto& pair: futures_to_wait)
            {
                auto message = std::get<0>(pair);
                auto error_message = std::get<1>(pair);
                auto& future = std::get<2>(pair);
                map.SetGenerationMessage(message);
                if (future.wait_for(5s) == std::future_status::timeout)
                {
                    map.SetForceStop(true);
                    map.Error(error_message);
                }
            }

            if (!map.ShouldForceStop())
            {
                GenerateStage0 = false;
                GenerateStage1 = false;
                GenerateStage2 = true;
                GenerateStage3 = true;
                GenerateStage4 = true;
                GenerateStage5 = true;
            }
        };

        virtual void Render(Map& map) override
        {
            DrawHorizontal(map);
            DrawSurfaceBg(map);
            DrawSurface(map);
#ifdef DEBUG
            DrawSurfaceDebug(map);
#endif
            DrawUnderground(map);
        };

};


class Scene0: public Scene
{
    private:
        bool GenerateStage0 = true;
        bool GenerateStage1 = true;
        bool GenerateStage2 = true;

    public:
        virtual void Run(Map& map) override
        {
            if (!map.ShouldForceStop() && GenerateStage0)
            {
                map.ClearStage0();
                map.SetGenerationMessage("DEFINITION OF HORIZONTAL AREAS...");
                DefineHorizontal(map);
            }

            if (!map.ShouldForceStop() && GenerateStage1)
            {
                map.ClearStage2();

                auto define_structures_future = std::async(std::launch::async, DefineHillsHolesIslands, std::ref(map));
                auto define_cabins_future = std::async(std::launch::async, DefineCabins, std::ref(map));
                auto define_castles_future = std::async(std::launch::async, DefineCastles, std::ref(map));

                map.SetGenerationMessage("DEFINITION OF HILLS, HOLES, ISLANDS...");
                if (define_structures_future.wait_for(5s) == std::future_status::timeout)
                {
                    map.SetForceStop(true);
                    map.Error("DEFINITION OF HILLS, HOLES, ISLANDS INFEASIBLE");
                }
            }

            if (!map.ShouldForceStop() && GenerateStage2)
            {
                map.ClearStage3();
                map.SetGenerationMessage("DEFINITION OF SURFACE...");
                DefineSurface(map);
            }


            if (!map.ShouldForceStop())
            {
                GenerateStage0 = false;
                GenerateStage1 = true;
                GenerateStage2 = true;
            }
        };

        virtual void Render(Map& map) override
        {
            DrawHorizontal(map);
            DrawSurfaceBg(map);
            DrawSurface(map);
#ifdef DEBUG
            DrawSurfaceDebug(map);
#endif
        };
};


class Scene1: public Scene
{
    private:
        bool GenerateStage0 = true;
        bool GenerateStage1 = true;
        bool GenerateStage2 = true;
        bool GenerateStage3 = true;

    public:
        virtual void Run(Map& map) override
        {
            if (!map.ShouldForceStop() && GenerateStage0)
            {
                map.ClearStage0();
                map.SetGenerationMessage("DEFINITION OF HORIZONTAL AREAS...");
                DefineHorizontal(map);
            }

            if (!map.ShouldForceStop() && GenerateStage1)
            {
                map.ClearStage2();

                auto define_structures_future = std::async(std::launch::async, DefineHillsHolesIslands, std::ref(map));
                auto define_cabins_future = std::async(std::launch::async, DefineCabins, std::ref(map));
                auto define_castles_future = std::async(std::launch::async, DefineCastles, std::ref(map));

                map.SetGenerationMessage("DEFINITION OF HILLS, HOLES, ISLANDS...");
                if (define_structures_future.wait_for(5s) == std::future_status::timeout)
                {
                    map.SetForceStop(true);
                    map.Error("DEFINITION OF HILLS, HOLES, ISLANDS INFEASIBLE");
                }
            }

            if (!map.ShouldForceStop() && GenerateStage2)
            {
                map.ClearStage3();
                map.SetGenerationMessage("DEFINITION OF SURFACE...");
                DefineSurface(map);
            }

            if (!map.ShouldForceStop() && GenerateStage3)
            {
                map.SetGenerationMessage("GENERATION OF HILLS...");
                GenerateHills(map);
                map.SetGenerationMessage("GENERATION OF HOLES...");
                GenerateHoles(map);
            }

            if (!map.ShouldForceStop())
            {
                GenerateStage0 = false;
                GenerateStage1 = true;
                GenerateStage2 = true;
                GenerateStage3 = true;
            }
        };

        virtual void Render(Map& map) override
        {
            DrawHorizontal(map);
            DrawSurfaceBg(map);
            DrawSurface(map);
#ifdef DEBUG
            DrawSurfaceDebug(map);
#endif
        };

};


class Scene2: public Scene
{
    private:
        bool GenerateStage0 = true;
        bool GenerateStage1 = true;
        bool GenerateStage2 = true;
        bool GenerateStage3 = true;

    public:
        virtual void Run(Map& map) override
        {
            if (!map.ShouldForceStop() && GenerateStage0)
            {
                map.ClearStage0();
                map.SetGenerationMessage("DEFINITION OF HORIZONTAL AREAS...");
                DefineHorizontal(map);
            }

            if (!map.ShouldForceStop() && GenerateStage1)
            {
                map.ClearStage2();

                auto define_structures_future = std::async(std::launch::async, DefineHillsHolesIslands, std::ref(map));
                auto define_cabins_future = std::async(std::launch::async, DefineCabins, std::ref(map));
                auto define_castles_future = std::async(std::launch::async, DefineCastles, std::ref(map));

                map.SetGenerationMessage("DEFINITION OF HILLS, HOLES, ISLANDS...");
                if (define_structures_future.wait_for(5s) == std::future_status::timeout)
                {
                    map.SetForceStop(true);
                    map.Error("DEFINITION OF HILLS, HOLES, ISLANDS INFEASIBLE");
                }
            }

            if (!map.ShouldForceStop() && GenerateStage2)
            {
                map.ClearStage3();
                map.SetGenerationMessage("DEFINITION OF SURFACE...");
                DefineSurface(map);
            }

            if (!map.ShouldForceStop() && GenerateStage3)
            {
                map.SetGenerationMessage("GENERATION OF HILLS...");
                GenerateHills(map);
                map.SetGenerationMessage("GENERATION OF HOLES...");
                GenerateHoles(map);
                map.SetGenerationMessage("GENERATION OF CLIFFS AND TRANSITIONS...");
                GenerateCliffsTransitions(map);
            }

            if (!map.ShouldForceStop())
            {
                GenerateStage0 = false;
                GenerateStage1 = true;
                GenerateStage2 = true;
                GenerateStage3 = true;
            }
        };

        virtual void Render(Map& map) override
        {
            DrawHorizontal(map);
            DrawSurfaceBg(map);
            DrawSurface(map);
#ifdef DEBUG
            DrawSurfaceDebug(map);
#endif
        };

};



class Scene3: public Scene
{
    private:
        bool GenerateStage0 = true;
        bool GenerateStage1 = true;
        bool GenerateStage2 = true;
        bool GenerateStage3 = true;

    public:
        virtual void Run(Map& map) override
        {
            if (!map.ShouldForceStop() && GenerateStage0)
            {
                map.ClearStage0();
                map.SetGenerationMessage("DEFINITION OF HORIZONTAL AREAS...");
                DefineHorizontal(map);
            }

            if (!map.ShouldForceStop() && GenerateStage1)
            {
                map.ClearStage2();

                auto define_structures_future = std::async(std::launch::async, DefineHillsHolesIslands, std::ref(map));
                auto define_cabins_future = std::async(std::launch::async, DefineCabins, std::ref(map));
                auto define_castles_future = std::async(std::launch::async, DefineCastles, std::ref(map));

                map.SetGenerationMessage("DEFINITION OF HILLS, HOLES, ISLANDS...");
                if (define_structures_future.wait_for(5s) == std::future_status::timeout)
                {
                    map.SetForceStop(true);
                    map.Error("DEFINITION OF HILLS, HOLES, ISLANDS INFEASIBLE");
                }
            }

            if (!map.ShouldForceStop() && GenerateStage2)
            {
                map.ClearStage3();
                map.SetGenerationMessage("DEFINITION OF SURFACE...");
                DefineSurface(map);
            }

            if (!map.ShouldForceStop() && GenerateStage3)
            {
                map.SetGenerationMessage("GENERATION OF HILLS...");
                GenerateHills(map);
                map.SetGenerationMessage("GENERATION OF HOLES...");
                GenerateHoles(map);
                map.SetGenerationMessage("GENERATION OF CLIFFS AND TRANSITIONS...");
                GenerateCliffsTransitions(map);
                map.SetGenerationMessage("GENERATION OF LEFT OCEAN...");
                GenerateOceanLeft(map);
                map.SetGenerationMessage("GENERATION OF RIGHT OCEAN...");
                GenerateOceanRight(map);
            }

            if (!map.ShouldForceStop())
            {
                GenerateStage0 = false;
                GenerateStage1 = true;
                GenerateStage2 = true;
                GenerateStage3 = true;
            }
        };

        virtual void Render(Map& map) override
        {
            DrawHorizontal(map);
            DrawSurfaceBg(map);
            DrawSurface(map);
#ifdef DEBUG
            DrawSurfaceDebug(map);
#endif
        };

};


class Scene4: public Scene
{
    private:
        bool GenerateStage0 = true;
        bool GenerateStage1 = true;
        bool GenerateStage2 = true;
        bool GenerateStage3 = true;

    public:
        virtual void Run(Map& map) override
        {
            if (!map.ShouldForceStop() && GenerateStage0)
            {
                map.ClearStage0();
                map.SetGenerationMessage("DEFINITION OF HORIZONTAL AREAS...");
                DefineHorizontal(map);
            }

            if (!map.ShouldForceStop() && GenerateStage1)
            {
                map.ClearStage2();

                auto define_structures_future = std::async(std::launch::async, DefineHillsHolesIslands, std::ref(map));
                auto define_cabins_future = std::async(std::launch::async, DefineCabins, std::ref(map));
                auto define_castles_future = std::async(std::launch::async, DefineCastles, std::ref(map));

                map.SetGenerationMessage("DEFINITION OF HILLS, HOLES, ISLANDS...");
                if (define_structures_future.wait_for(5s) == std::future_status::timeout)
                {
                    map.SetForceStop(true);
                    map.Error("DEFINITION OF HILLS, HOLES, ISLANDS INFEASIBLE");
                }
            }

            if (!map.ShouldForceStop() && GenerateStage2)
            {
                map.ClearStage3();
                map.SetGenerationMessage("DEFINITION OF SURFACE...");
                DefineSurface(map);
            }

            if (!map.ShouldForceStop() && GenerateStage3)
            {
                map.SetGenerationMessage("GENERATION OF HILLS...");
                GenerateHills(map);
                map.SetGenerationMessage("GENERATION OF HOLES...");
                GenerateHoles(map);
                map.SetGenerationMessage("GENERATION OF CLIFFS AND TRANSITIONS...");
                GenerateCliffsTransitions(map);
                map.SetGenerationMessage("GENERATION OF LEFT OCEAN...");
                GenerateOceanLeft(map);
                map.SetGenerationMessage("GENERATION OF RIGHT OCEAN...");
                GenerateOceanRight(map);
                map.SetGenerationMessage("GENERATION OF CHASMS...");
                GenerateChasms(map);
            }

            if (!map.ShouldForceStop())
            {
                GenerateStage0 = false;
                GenerateStage1 = true;
                GenerateStage2 = true;
                GenerateStage3 = true;
            }
        };

        virtual void Render(Map& map) override
        {
            DrawHorizontal(map);
            DrawSurfaceBg(map);
            DrawSurface(map);
#ifdef DEBUG
            DrawSurfaceDebug(map);
#endif
        };

};


class Scene5: public Scene
{
    private:
        bool GenerateStage0 = true;
        bool GenerateStage1 = true;
        bool GenerateStage2 = true;
        bool GenerateStage3 = true;

    public:
        virtual void Run(Map& map) override
        {
            if (!map.ShouldForceStop() && GenerateStage0)
            {
                map.ClearStage0();
                map.SetGenerationMessage("DEFINITION OF HORIZONTAL AREAS...");
                DefineHorizontal(map);
            }

            if (!map.ShouldForceStop() && GenerateStage1)
            {
                map.ClearStage2();

                auto define_structures_future = std::async(std::launch::async, DefineHillsHolesIslands, std::ref(map));
                auto define_cabins_future = std::async(std::launch::async, DefineCabins, std::ref(map));
                auto define_castles_future = std::async(std::launch::async, DefineCastles, std::ref(map));

                map.SetGenerationMessage("DEFINITION OF HILLS, HOLES, ISLANDS...");
                if (define_structures_future.wait_for(5s) == std::future_status::timeout)
                {
                    map.SetForceStop(true);
                    map.Error("DEFINITION OF HILLS, HOLES, ISLANDS INFEASIBLE");
                }
            }

            if (!map.ShouldForceStop() && GenerateStage2)
            {
                map.ClearStage3();
                map.SetGenerationMessage("DEFINITION OF SURFACE...");
                DefineSurface(map);
            }

            if (!map.ShouldForceStop() && GenerateStage3)
            {
                map.SetGenerationMessage("GENERATION OF HILLS...");
                GenerateHills(map);
                map.SetGenerationMessage("GENERATION OF HOLES...");
                GenerateHoles(map);
                map.SetGenerationMessage("GENERATION OF CLIFFS AND TRANSITIONS...");
                GenerateCliffsTransitions(map);
                map.SetGenerationMessage("GENERATION OF LEFT OCEAN...");
                GenerateOceanLeft(map);
                map.SetGenerationMessage("GENERATION OF RIGHT OCEAN...");
                GenerateOceanRight(map);
                map.SetGenerationMessage("GENERATION OF CHASMS...");
                GenerateChasms(map);
                map.SetGenerationMessage("GENERATION OF LAKES...");
                GenerateLakes(map);
                map.SetGenerationMessage("GENERATION OF JUNGLE SWAMP...");
                GenerateJungleSwamp(map);
            }

            if (!map.ShouldForceStop())
            {
                GenerateStage0 = false;
                GenerateStage1 = true;
                GenerateStage2 = true;
                GenerateStage3 = true;
            }
        };

        virtual void Render(Map& map) override
        {
            DrawHorizontal(map);
            DrawSurfaceBg(map);
            DrawSurface(map);
#ifdef DEBUG
            DrawSurfaceDebug(map);
#endif
        };

};


class Scene6: public Scene
{
    private:
        bool GenerateStage0 = true;
        bool GenerateStage1 = true;
        bool GenerateStage2 = true;
        bool GenerateStage3 = true;

    public:
        virtual void Run(Map& map) override
        {
            if (!map.ShouldForceStop() && GenerateStage0)
            {
                map.ClearStage0();
                map.SetGenerationMessage("DEFINITION OF HORIZONTAL AREAS...");
                DefineHorizontal(map);
            }

            if (!map.ShouldForceStop() && GenerateStage1)
            {
                map.ClearStage2();

                auto define_structures_future = std::async(std::launch::async, DefineHillsHolesIslands, std::ref(map));
                auto define_cabins_future = std::async(std::launch::async, DefineCabins, std::ref(map));
                auto define_castles_future = std::async(std::launch::async, DefineCastles, std::ref(map));

                map.SetGenerationMessage("DEFINITION OF HILLS, HOLES, ISLANDS...");
                if (define_structures_future.wait_for(5s) == std::future_status::timeout)
                {
                    map.SetForceStop(true);
                    map.Error("DEFINITION OF HILLS, HOLES, ISLANDS INFEASIBLE");
                }
            }

            if (!map.ShouldForceStop() && GenerateStage2)
            {
                map.ClearStage3();
                map.SetGenerationMessage("DEFINITION OF SURFACE...");
                DefineSurface(map);
            }

            if (!map.ShouldForceStop() && GenerateStage3)
            {
                map.SetGenerationMessage("GENERATION OF HILLS...");
                GenerateHills(map);
                map.SetGenerationMessage("GENERATION OF HOLES...");
                GenerateHoles(map);
                map.SetGenerationMessage("GENERATION OF CLIFFS AND TRANSITIONS...");
                GenerateCliffsTransitions(map);
                map.SetGenerationMessage("GENERATION OF LEFT OCEAN...");
                GenerateOceanLeft(map);
                map.SetGenerationMessage("GENERATION OF RIGHT OCEAN...");
                GenerateOceanRight(map);
                map.SetGenerationMessage("GENERATION OF CHASMS...");
                GenerateChasms(map);
                map.SetGenerationMessage("GENERATION OF LAKES...");
                GenerateLakes(map);
                map.SetGenerationMessage("GENERATION OF JUNGLE SWAMP...");
                GenerateJungleSwamp(map);
                map.SetGenerationMessage("GENERATION OF ISLANDS...");
                GenerateIslands(map);
            }

            if (!map.ShouldForceStop())
            {
                GenerateStage0 = false;
                GenerateStage1 = true;
                GenerateStage2 = true;
                GenerateStage3 = true;
            }
        };

        virtual void Render(Map& map) override
        {
            DrawHorizontal(map);
            DrawSurfaceBg(map);
            DrawSurface(map);
#ifdef DEBUG
            DrawSurfaceDebug(map);
#endif
        };

};

#endif // SCENE
