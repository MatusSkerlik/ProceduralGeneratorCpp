#include <memory>
#include <stdio.h>
#include <functional>
#include <stdexcept>
#include <atomic>
#include <string>
#include <thread>
#include <future>
#include <chrono>
#include <mutex>
#include <random>

#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wenum-compare"
#include "raygui.h"
#pragma GCC diagnostic pop

//#define DEBUG 
#include "gui.h"
#include "pcg.h"
#include "scene.h"


using namespace std::chrono_literals;


/**************************************************
*
* PROCEDURAL GENERATION LOGIC 
*
**************************************************/
std::unique_ptr<Scene> scene {new DefaultScene()};
std::atomic_bool GenerationScheduled { false };
std::atomic_bool ScheduleThreadRunning { false };
std::atomic_bool SceneDrawReady{ false };

void _PCGGen(Map& map)
{
    if (scene != nullptr)
    {
        scene->Run(map);
        SceneDrawReady = true;
    }
    map.SetGenerationMessage("");
    map.SetGenerating(false);
};

void PCGGen(Map& map)
{
    map.SetGenerating(true); 
    map.SetForceStop(false);
    auto thread = std::thread(_PCGGen, std::ref(map));
    thread.detach();
};

void ScheduleGeneration(Map& map)
{
    GenerationScheduled = true;
    if (!ScheduleThreadRunning)
    {
        ScheduleThreadRunning = true;
        auto _ = std::thread([&](){
            while (GenerationScheduled || map.IsGenerating() || !map.IsInitialized())
            {
                if (!map.IsInitialized())
                    map.Init();
                if (map.IsGenerating())
                    map.SetForceStop(true);
                GenerationScheduled = false;
                std::this_thread::sleep_for(0.25s);
            }
            // HERE WE ARE SURE THAT LAST GENERATION EXITED
            PCGGen(map);
            ScheduleThreadRunning = false;
        });
        _.detach();
    }
};

/**************************************************
*
* GUI RELATED 
*
**************************************************/

std::string random_string()
{
     std::string str("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
     std::random_device rd;
     std::mt19937 generator(rd());
     std::shuffle(str.begin(), str.end(), generator);
     return str.substr(0, 32);    // assumes 32 < number of characters in str         
}

int main(void)
{
    // BASE CONSTANTS
    const float width = 1640;
    const float height = 600;
    const float map_width = 4200;
    const float map_height = 1200;

    // RAYLIB INIT
    InitWindow(width, height + 16, "Procedural Terrain Generator");
    SetTargetFPS(60); 
    GuiLoadStyle("style.rgs");

    RenderTexture canvas = LoadRenderTexture(map_width, map_height);
    Camera2D camera {{0, 0}, {0, 0}, 0, 1};
   
    // RAYGUI RELATED
    camera.zoom = width / map_width;

    Rectangle map_view {0, 0, width, height};
    Vector2 ScrollOffset = {0, 0};

    Map map;
    // GUI
    HeaderLayout StructuresControl(0, 0, 200, 400, "Structures Settings");
    StructuresControl.CreateLabel(92 + 8, 0, 92, 24, "FREQUENCY");
    StructuresControl.CreateLabel(0, 24 + 4, 92, 24, "HILLS");
    StructuresControl.CreateLabel(0, 48 + 8, 92, 24, "HOLES");
    StructuresControl.CreateLabel(0, 72 + 12, 92, 24, "ISLANDS");
    StructuresControl.CreateLabel(0, 96 + 16, 92, 24, "CHASMS");
    StructuresControl.CreateLabel(0, 120 + 20, 92, 24, "LAKES");
    StructuresControl.CreateLabel(0, 144 + 24, 92, 24, "TREES");
    StructuresControl.CreateSliderBar(92 + 8, 24 + 4, 92, 24, map.HillsFrequency(), 
    [&](float fq)
    { 
        map.HillsFrequency(fq);
        ScheduleGeneration(map);
    }); 
    StructuresControl.CreateSliderBar(92 + 8, 48 + 8, 92, 24, map.HolesFrequency(),
    [&](float fq)
    {
        map.HolesFrequency(fq);
        ScheduleGeneration(map);
    }); 
    StructuresControl.CreateSliderBar(92 + 8, 72 + 12, 92, 24, map.IslandsFrequency(), 
    [&](float fq)
    {
        map.IslandsFrequency(fq);
        ScheduleGeneration(map);
    }); 
    StructuresControl.CreateSliderBar(92 + 8, 96 + 16, 92, 24, map.ChasmFrequency(), 
    [&](float fq)
    {
        map.ChasmFrequency(fq);
        ScheduleGeneration(map);
    }); 
    StructuresControl.CreateSliderBar(92 + 8, 120 + 20, 92, 24, map.LakeFrequency(), 
    [&](float fq)
    {
        map.LakeFrequency(fq);
        ScheduleGeneration(map);
    }); 
    StructuresControl.CreateSliderBar(92 + 8, 144 + 24, 92, 24, map.TreeFrequency(), 
    [&](float fq)
    {
        map.TreeFrequency(fq);
        ScheduleGeneration(map);
    }); 
    StructuresControl.Hide();

    HeaderLayout SurfaceControl(200 + 8, 0, 200, 400, "Surface Settings");
    SurfaceControl.CreateLabel(0, 0, 92, 24, "PARTS");
    SurfaceControl.CreateLabel(0, 24 + 4, 92, 24, "FREQUENCY");
    SurfaceControl.CreateLabel(0, 48 + 8, 92, 24, "OCTAVES");
    SurfaceControl.CreateSliderBar(92 + 8, 0, 92, 24, map.SurfacePartsCount(),
    [&](float fq)
    {
        map.SurfacePartsCount(fq);
        ScheduleGeneration(map);
    }); 
    SurfaceControl.CreateSliderBar(92 + 8, 24 + 4, 92, 24, map.SurfacePartsFrequency(),
    [&](float fq)
    {
        map.SurfacePartsFrequency(fq);
        ScheduleGeneration(map);
    }); 
    SurfaceControl.CreateSliderBar(92 + 8, 48 + 8, 92, 24, map.SurfacePartsOctaves(),
    [&](float fq)
    {
        map.SurfacePartsOctaves(fq);
        ScheduleGeneration(map);
    }); 
    SurfaceControl.Hide();

    HeaderLayout SceneControl {400 + 16, 0, 200, 400, "Scenes"};
    auto& scene0 = SceneControl.CreateToogleButton(0, 0, 92, 24, "DEFAULT", [&](bool){}, true); 
    auto& scene1 = SceneControl.CreateToogleButton(0, 24, 92, 24, "SCENE1"); 
    auto& scene2 = SceneControl.CreateToogleButton(0, 48, 92, 24, "SCENE2"); 
    auto& scene3 = SceneControl.CreateToogleButton(0, 72, 92, 24, "SCENE3"); 

    scene0.SetOnClickListener([&](bool active){
        if (active)
        {
            scene1.SetOff();
            scene2.SetOff();
            scene3.SetOff();
            scene.reset(new DefaultScene());
            ScheduleGeneration(map);
        }
    });
    
    scene1.SetOnClickListener([&](bool active){
        if (active)
        {
            scene0.SetOff();
            scene2.SetOff();
            scene3.SetOff();
            scene.reset(new Scene0());
            ScheduleGeneration(map);
        }
    });

    scene2.SetOnClickListener([&](bool active){
        if (active)
        {
            scene0.SetOff();
            scene1.SetOff();
            scene3.SetOff();
        }
    });

    scene3.SetOnClickListener([&](bool active){
        if (active)
        {
            scene0.SetOff();
            scene1.SetOff();
            scene2.SetOff();
        }
    });
    SceneControl.Hide();

    // CORE LOGIC INIT
    ScheduleGeneration(map);

    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        if (SceneDrawReady)
        {
            BeginTextureMode(canvas);
                if (scene != nullptr) scene->Render(map);
            EndTextureMode();
            SceneDrawReady = false;
        }
        // RELOAD DLL
        if (IsKeyDown(KEY_R) && !map.IsGenerating())
        {
            map.ClearAll();
            ScheduleGeneration(map);
        }

        if (IsKeyPressed(KEY_I))
        {
            auto image = LoadImageFromTexture(canvas.texture);
            ImageFlipVertical(&image);
            auto filename = "img/" + random_string() + ".png";
            ExportImage(image, filename.c_str()); 
        }

        if (IsKeyDown(KEY_Q))
        {
            camera.zoom *= 0.95;
        }
        if (IsKeyDown(KEY_E))
        {
            camera.zoom *= 1.05;
        }

        if ((map_width * camera.zoom) < width || (map_height * camera.zoom) < height)
        {
            camera.offset = {0, 0};
            camera.target = {0, 0};
        }
        else
        {
            // TODO
            camera.offset = {0, 0}; 
            camera.target = {0 ,0};
        }

        if (IsKeyDown(KEY_A))
            ScrollOffset.x -= 20 * camera.zoom;
        if (IsKeyDown(KEY_D))
            ScrollOffset.x += 20 * camera.zoom;
        if (IsKeyDown(KEY_W))
            ScrollOffset.y -= 20 * camera.zoom;
        if (IsKeyDown(KEY_S))
            ScrollOffset.y += 20 * camera.zoom;

        if (ScrollOffset.x < 0)
            ScrollOffset.x = 0;

        if ((map_width * camera.zoom) > width)
        {
            if (ScrollOffset.x > ((map_width * camera.zoom) - width))
                ScrollOffset.x = ((map_width * camera.zoom) - width);
        }
        else
        {
            ScrollOffset.x = 0;
        }

        if (ScrollOffset.y < 0)
            ScrollOffset.y = 0;

        if ((map_height * camera.zoom) > height)
        {
            if (ScrollOffset.y > ((map_height * camera.zoom) - height))
                ScrollOffset.y = ((map_height * camera.zoom) - height);
        }
        else
        {
            ScrollOffset.y = 0;
        }

        // DRAW LOGIC
        BeginDrawing();
            ClearBackground((Color){60, 56, 54, 255});

            DrawRectangleRec(map_view, {40, 40, 40, 255});

            BeginScissorMode(map_view.x, map_view.y, map_view.width, map_view.height);
                BeginMode2D(camera);
                    DrawTextureRec(canvas.texture, (Rectangle) { 0, 0, map_width, -map_height}, {-ScrollOffset.x / camera.zoom, -ScrollOffset.y / camera.zoom}, WHITE);        
                EndMode2D();
            EndScissorMode();


            // DRAW STRUCTURE INFO CLOSE TO MOUSE CURSOR
            auto mx = GetMouseX();
            auto my = GetMouseY();
            if ((mx > map_view.x) && (mx < map_view.x + map_view.width) && (my > map_view.y) && (my < map_view.y + map_view.height))
            {
                int x = ScrollOffset.x / camera.zoom + (mx - map_view.x) * 1 / camera.zoom;
                int y = ScrollOffset.y / camera.zoom + (my - map_view.y) * 1 / camera.zoom;
                std::string t = "[";
                t += std::to_string((int)x);
                t += ":";
                t += std::to_string((int)y);
                t += "]";
                DrawText(t.c_str(), mx, my - 16, 16, WHITE);

                try {
                    auto info = map.GetMetadata({x, y});
                    if (info.biome != nullptr)
                    {
                        switch (info.biome->GetType())
                        {
                            case Biomes::FOREST:
                                DrawText("FOREST", mx, my - 32, 16, RED);
                                break;
                            case Biomes::JUNGLE:
                                DrawText("JUNGLE", mx, my - 32, 16, RED);
                                break;
                            case Biomes::TUNDRA:
                                DrawText("TUNDRA", mx, my - 32, 16, RED);
                                break;
                            case Biomes::OCEAN_LEFT:
                                DrawText("OCEAN", mx, my - 32, 16, RED);
                                break;
                            case Biomes::OCEAN_RIGHT:
                                DrawText("OCEAN", mx, my - 32, 16, RED);
                                break;
                            case Biomes::OCEAN_DESERT_LEFT:
                                DrawText("DESERT", mx, my - 32, 16, RED);
                                break;
                            case Biomes::OCEAN_DESERT_RIGHT:
                                DrawText("DESERT", mx, my - 32, 16, RED);
                                break;
                        }
                    }
                    if (info.surface_structure != nullptr)
                    {
                        switch (info.surface_structure->GetType())
                        {
                            case Structures::HILL:
                                DrawText("HILL", mx, my - 48, 16, BLUE);
                                break;
                            case Structures::HOLE:
                                DrawText("HOLE", mx, my - 48, 16, BLUE);
                                break;
                            case Structures::CABIN:
                                DrawText("CABIN", mx, my - 48, 16, BLUE);
                                break;
                            case Structures::CASTLE:
                                DrawText("CASTLE", mx, my - 48, 16, BLUE);
                                break;
                            case Structures::SURFACE_PART:
                                DrawText("SUTFACE_PART", mx, my - 48, 16, BLUE);
                                break;
                            case Structures::CHASM:
                                DrawText("CHASM", mx, my - 48, 16, BLUE);
                                break;
                            case Structures::CLIFF:
                                DrawText("CLIFF", mx, my - 48, 16, BLUE);
                                break;
                            case Structures::TRANSITION:
                                DrawText("TRANSITION", mx, my - 48, 16, BLUE);
                                break;
                            case Structures::FLOATING_ISLAND:
                                DrawText("ISLAND", mx, my - 48, 16, BLUE);
                                break;
                            default:
                                break;
                        }
                    }
                } catch (std::out_of_range& e){};
            }

            StructuresControl.Render();
            SurfaceControl.Render();
            SceneControl.Render();
            
            // DRAW STATUS BAR
            if (map.IsGenerating())
            { 
                DrawText(map.GetGenerationMessage().c_str(), 8, height + 4, 8, WHITE);
            }
            else
            {
                DrawText("DONE...", 8, height + 4, 8, WHITE);

                if (map.HasError()) 
                { 
                    auto error = map.Error();
                    auto error_width = (float) (error.size() * 6 + 2 * 8);
                    if (error_width < 100) error_width = 100;

                    if (GuiMessageBox((Rectangle){width / 2 - error_width / 2, height / 2 - 50, error_width, 100}, "ERROR", error.c_str(), "OK") != -1)
                    {
                        map.PopError();    
                    }
                }
            }
        EndDrawing();
    }

    CloseWindow();        // Close window and OpenGL context
    return 0;
}
