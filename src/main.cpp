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
#include "utils.h"
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
    if (!map.ShouldForceStop())
        GenerationDone(map);
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
        GenerateSurface(map);
        ScheduleGeneration(map);
    }); 
    StructuresControl.CreateSliderBar(92 + 8, 48 + 8, 92, 24, map.HolesFrequency(),
    [&](float fq)
    {
        map.HolesFrequency(fq);
        GenerateSurface(map);
        ScheduleGeneration(map);
    }); 
    StructuresControl.CreateSliderBar(92 + 8, 72 + 12, 92, 24, map.IslandsFrequency(), 
    [&](float fq)
    {
        map.IslandsFrequency(fq);
        GenerateSurface(map);
        ScheduleGeneration(map);
    }); 
    StructuresControl.CreateSliderBar(92 + 8, 96 + 16, 92, 24, map.ChasmFrequency(), 
    [&](float fq)
    {
        map.ChasmFrequency(fq);
        GenerateSurface(map);
        ScheduleGeneration(map);
    }); 
    StructuresControl.CreateSliderBar(92 + 8, 120 + 20, 92, 24, map.LakeFrequency(), 
    [&](float fq)
    {
        map.LakeFrequency(fq);
        GenerateSurface(map);
        ScheduleGeneration(map);
    }); 
    StructuresControl.CreateSliderBar(92 + 8, 144 + 24, 92, 24, map.TreeFrequency(), 
    [&](float fq)
    {
        map.TreeFrequency(fq);
        GenerateSurface(map);
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
        GenerateSurface(map);
        ScheduleGeneration(map);
    }); 
    SurfaceControl.CreateSliderBar(92 + 8, 24 + 4, 92, 24, map.SurfacePartsFrequency(),
    [&](float fq)
    {
        map.SurfacePartsFrequency(fq);
        GenerateSurface(map);
        ScheduleGeneration(map);
    }); 
    SurfaceControl.CreateSliderBar(92 + 8, 48 + 8, 92, 24, map.SurfacePartsOctaves(),
    [&](float fq)
    {
        map.SurfacePartsOctaves(fq);
        GenerateSurface(map);
        ScheduleGeneration(map);
    }); 
    SurfaceControl.Hide();

    HeaderLayout CaveControl(400 + 16, 0, 200, 400, "Cave Settings");
    CaveControl.CreateLabel(0, 0, 92, 24, "FREQUENCY");
    CaveControl.CreateLabel(0, 24 + 4, 92, 24, "STROKE");
    CaveControl.CreateLabel(0, 48 + 8, 92, 24, "POINTS");
    CaveControl.CreateLabel(0, 72 + 12, 92, 24, "CURVNESS");
    CaveControl.CreateSliderBar(92 + 8, 0, 92, 24, map.CaveFrequency(),
    [&](float fq)
    {
        map.CaveFrequency(fq);
        GenerateUnderground(map);
        ScheduleGeneration(map);
    }); 
    CaveControl.CreateSliderBar(92 + 8, 24 + 4, 92, 24, map.CaveStrokeSize(),
    [&](float fq)
    {
        map.CaveStrokeSize(fq);
        GenerateUnderground(map);
        ScheduleGeneration(map);
    }); 
    CaveControl.CreateSliderBar(92 + 8, 48 + 8, 92, 24, map.CavePointsSize(),
    [&](float fq)
    {
        map.CavePointsSize(fq);
        GenerateUnderground(map);
        ScheduleGeneration(map);
    }); 
    CaveControl.CreateSliderBar(92 + 8, 72 + 12, 92, 24, map.CaveCurvness(),
    [&](float fq)
    {
        map.CaveCurvness(fq);
        GenerateUnderground(map);
        ScheduleGeneration(map);
    }); 
    CaveControl.Hide();

    HeaderLayout MaterialControl(600 + 24, 0, 350, 400, "Material Control");
    MaterialControl.CreateLabel(92 + 8, 0, 92, 24, "FREQUENCY");
    MaterialControl.CreateLabel(184 + 32, 0, 92, 24, "SIZE");
    MaterialControl.CreateLabel(0, 24 + 4, 92, 24, "COPPER");
    MaterialControl.CreateLabel(0, 48 + 8, 92, 24, "IRON");
    MaterialControl.CreateLabel(0, 72 + 16, 92, 24, "SILVER");
    MaterialControl.CreateLabel(0, 96 + 20, 92, 24, "GOLD");

    MaterialControl.CreateSliderBar(92 + 8, 24 + 4, 92, 24, map.CopperFrequency(),
    [&](float fq)
    {
        map.CopperFrequency(fq);
        GenerateSurface(map);
        GenerateUnderground(map);
        ScheduleGeneration(map);
    }); 

    MaterialControl.CreateSliderBar(92 + 8, 48 + 8, 92, 24, map.IronFrequency(),
    [&](float fq)
    {
        map.IronFrequency(fq);
        GenerateSurface(map);
        GenerateUnderground(map);
        ScheduleGeneration(map);
    }); 

    MaterialControl.CreateSliderBar(92 + 8, 72 + 12, 92, 24, map.SilverFrequency(),
    [&](float fq)
    {
        map.SilverFrequency(fq);
        GenerateUnderground(map);
        ScheduleGeneration(map);
    }); 

    MaterialControl.CreateSliderBar(92 + 8, 96 + 16, 92, 24, map.GoldFrequency(),
    [&](float fq)
    {
        map.GoldFrequency(fq);
        GenerateUnderground(map);
        ScheduleGeneration(map);
    }); 

    MaterialControl.CreateSliderBar(184 + 32, 24 + 4, 92, 24, map.CopperSize(),
    [&](float fq)
    {
        map.CopperSize(fq);
        GenerateSurface(map);
        GenerateUnderground(map);
        ScheduleGeneration(map);
    }); 

    MaterialControl.CreateSliderBar(184 + 32, 48 + 8, 92, 24, map.IronSize(),
    [&](float fq)
    {
        map.IronSize(fq);
        GenerateSurface(map);
        GenerateUnderground(map);
        ScheduleGeneration(map);
    }); 

    MaterialControl.CreateSliderBar(184 + 32, 72 + 12, 92, 24, map.SilverSize(),
    [&](float fq)
    {
        map.SilverSize(fq);
        GenerateUnderground(map);
        ScheduleGeneration(map);
    }); 

    MaterialControl.CreateSliderBar(184 + 32, 96 + 16, 92, 24, map.GoldSize(),
    [&](float fq)
    {
        map.GoldSize(fq);
        GenerateUnderground(map);
        ScheduleGeneration(map);
    }); 


    MaterialControl.Hide();

    HeaderLayout SceneControl {950 + 32, 0, 200, 400, "Scenes"};
    auto& scene0 = SceneControl.CreateToogleButton(0, 0, 92, 24, "DEFAULT", [&](bool){}, true); 
    auto& scene1 = SceneControl.CreateToogleButton(0, 24, 92, 24, "SCENE1"); 
    auto& scene2 = SceneControl.CreateToogleButton(0, 48, 92, 24, "SCENE2"); 
    auto& scene3 = SceneControl.CreateToogleButton(0, 72, 92, 24, "SCENE3"); 
    auto& scene4 = SceneControl.CreateToogleButton(0, 96, 92, 24, "SCENE4"); 
    auto& scene5 = SceneControl.CreateToogleButton(0, 120, 92, 24, "SCENE5"); 
    auto& scene6 = SceneControl.CreateToogleButton(0, 144, 92, 24, "SCENE6"); 
    auto& scene7 = SceneControl.CreateToogleButton(0, 168, 92, 24, "SCENE7"); 
    auto& scene8 = SceneControl.CreateToogleButton(0, 192, 92, 24, "SCENE8"); 
    auto& scene9 = SceneControl.CreateToogleButton(0, 216, 92, 24, "SCENE9"); 
    auto& scene10 = SceneControl.CreateToogleButton(0, 240, 92, 24, "SCENE10"); 
    auto& scene11 = SceneControl.CreateToogleButton(0, 264, 92, 24, "SCENE11"); 
    auto& scene12 = SceneControl.CreateToogleButton(0, 288, 92, 24, "SCENE12"); 

    auto scenes_off = [&](){
        scene0.SetOff();
        scene1.SetOff();
        scene2.SetOff();
        scene3.SetOff();
        scene4.SetOff();
        scene5.SetOff();
        scene6.SetOff();
        scene7.SetOff();
        scene8.SetOff();
        scene9.SetOff();
        scene10.SetOff();
        scene11.SetOff();
        scene12.SetOff();
    };

    scene0.SetOnClickListener([&](bool active){
        if (active)
        {
            scenes_off();
            scene0.SetOn();
            scene.reset(new DefaultScene());
            GenerateSurface(map);
            GenerateUnderground(map);
            ScheduleGeneration(map);
        }
    });
    
    scene1.SetOnClickListener([&](bool active){
        if (active)
        {
            scenes_off();
            scene1.SetOn();
            scene.reset(new Scene0());
            GenerateSurface(map);
            ScheduleGeneration(map);
        }
    });

    scene2.SetOnClickListener([&](bool active){
        if (active)
        {
            scenes_off();
            scene2.SetOn();
            scene.reset(new Scene1());
            GenerateSurface(map);
            ScheduleGeneration(map);
        }
    });

    scene3.SetOnClickListener([&](bool active){
        if (active)
        {
            scenes_off();
            scene3.SetOn();
            scene.reset(new Scene2());
            GenerateSurface(map);
            ScheduleGeneration(map);
        }
    });

    scene4.SetOnClickListener([&](bool active){
        if (active)
        {
            scenes_off();
            scene4.SetOn();
            scene.reset(new Scene3());
            GenerateSurface(map);
            ScheduleGeneration(map);
        }
    });

    scene5.SetOnClickListener([&](bool active){
        if (active)
        {
            scenes_off();
            scene5.SetOn();
            scene.reset(new Scene4());
            GenerateSurface(map);
            ScheduleGeneration(map);
        }
    });

    scene6.SetOnClickListener([&](bool active){
        if (active)
        {
            scenes_off();
            scene6.SetOn();
            scene.reset(new Scene5());
            GenerateSurface(map);
            ScheduleGeneration(map);
        }
    });
    
    scene7.SetOnClickListener([&](bool active){
        if (active)
        {
            scenes_off();
            scene7.SetOn();
            scene.reset(new Scene6());
            GenerateSurface(map);
            ScheduleGeneration(map);
        }
    });

    scene8.SetOnClickListener([&](bool active){
        if (active)
        {
            scenes_off();
            scene8.SetOn();
            scene.reset(new Scene7());
            GenerateSurface(map);
            ScheduleGeneration(map);
        }
    });

    scene9.SetOnClickListener([&](bool active){
        if (active)
        {
            scenes_off();
            scene9.SetOn();
            scene.reset(new Scene8());
            GenerateSurface(map);
            ScheduleGeneration(map);
        }
    });

    scene10.SetOnClickListener([&](bool active){
        if (active)
        {
            scenes_off();
            scene10.SetOn();
            scene.reset(new Scene9());
            GenerateUnderground(map);
            ScheduleGeneration(map);
        }
    });

    scene11.SetOnClickListener([&](bool active){
        if (active)
        {
            scenes_off();
            scene11.SetOn();
            scene.reset(new Scene10());
            GenerateUnderground(map);
            ScheduleGeneration(map);
        }
    });

    scene12.SetOnClickListener([&](bool active){
        if (active)
        {
            scenes_off();
            scene12.SetOn();
            scene.reset(new Scene11());
            GenerateUnderground(map);
            ScheduleGeneration(map);
        }
    });
    SceneControl.Hide();

    // CORE LOGIC INIT
    ScheduleGeneration(map);

    auto drag_x = 0;
    auto drag_y = 0;
    auto dragging = false;
    const auto min_zoom = (width / (1.5 * map_width));

    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        if (SceneDrawReady)
        {
            BeginTextureMode(canvas);
                if (scene != nullptr) scene->Render(map);
            EndTextureMode();
            SceneDrawReady = false;
        }

        if (IsKeyPressed(KEY_I))
        {
            auto image = LoadImageFromTexture(canvas.texture);
            ImageFlipVertical(&image);
            auto filename = "img/" + random_string() + ".png";
            ExportImage(image, filename.c_str()); 
        }

        auto mx = (float)GetMouseX();
        auto my = (float)GetMouseY();
        auto dx = mx - camera.offset.x;
        auto dy = my - camera.offset.y;
        auto x = camera.target.x + dx / camera.zoom; 
        auto y = camera.target.y + dy / camera.zoom; 

        camera.zoom += GetMouseWheelMove() * 0.075 * camera.zoom;

        if (camera.zoom < min_zoom)
            camera.zoom = min_zoom;

        if ((map_width * camera.zoom) < width && (map_height * camera.zoom) < height)
        {
            camera.offset = {0, 0};
            camera.target = {0, 0};
        }
        else
        {
            if (GetMouseWheelMove() != 0.0)
            {
                camera.offset = {mx, my}; 
                camera.target = {x, y};
            }
        }

        if (IsMouseButtonPressed(2))
        {
            dragging = true;
            camera.offset = {mx, my}; 
            camera.target = {x, y};
            drag_x = mx;
            drag_y = mx;
        }

        if (IsMouseButtonReleased(2))
        {
            dragging = false;
        }

        if (dragging)
        {
            camera.offset = {drag_x + mx - drag_x, drag_y + my - drag_y}; 
        }

        // DRAW LOGIC
        BeginDrawing();
            ClearBackground((Color){60, 56, 54, 255});

            DrawRectangleRec({0, 0, width, height}, {40, 40, 40, 255});

            BeginScissorMode(0, 0, width, height);
                BeginMode2D(camera);
                    DrawTextureRec(canvas.texture, (Rectangle) { 0, 0, map_width, -map_height}, {0, 0}, WHITE);        
                EndMode2D();
            EndScissorMode();


            // DRAW STRUCTURE INFO CLOSE TO MOUSE CURSOR
            if (map.IsInitialized() && x >= 0 && x < map_width && y >= 0 && y < map_height)
            {
                std::string t = "[";
                t += std::to_string((int)x);
                t += ":";
                t += std::to_string((int)y);
                t += "]";
                DrawText(t.c_str(), mx, my - 16, 16, WHITE);

                auto info = map.GetMetadata({(int)x, (int)y});
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
                if (info.generated_structure != nullptr)
                {
                    switch (info.generated_structure->GetType())
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
                        case Structures::CAVE:
                            DrawText("CAVE", mx, my - 48, 16, BLUE);
                            break;
                        case Structures::WATER:
                            DrawText("WATER", mx, my - 48, 16, BLUE);
                            break;
                        case Structures::LAVA:
                            DrawText("LAVA", mx, my - 48, 16, BLUE);
                            break;
                        case Structures::TREE:
                            DrawText("TREE", mx, my - 48, 16, BLUE);
                            break;
                        case Structures::GRASS:
                            DrawText("GRASS", mx, my - 48, 16, BLUE);
                            break;
                        case Structures::COPPER_ORE:
                            DrawText("COPPER_ORE", mx, my - 48, 16, BLUE);
                            break;
                        case Structures::IRON_ORE:
                            DrawText("IRON_ORE", mx, my - 48, 16, BLUE);
                            break;
                        case Structures::SILVER_ORE:
                            DrawText("SILVER_ORE", mx, my - 48, 16, BLUE);
                            break;
                        case Structures::GOLD_ORE:
                            DrawText("GOLD_ORE", mx, my - 48, 16, BLUE);
                            break;
                        default:
                            break;
                    }
                }
            }

            StructuresControl.Render();
            SurfaceControl.Render();
            CaveControl.Render();
            MaterialControl.Render();
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
