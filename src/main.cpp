#include <atomic>
#include <mutex>
#include <stdio.h>
#include <string>
#include <thread>
#include <future>
#include <functional>
#include <chrono>
#include <assert.h>
#include "libloaderapi.h"

#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define PCG_AS_DLL false 

#if (PCG_AS_DLL)
HMODULE module;
PCG_FUNC DefineHorizontal;
PCG_FUNC DefineBiomes;
PCG_FUNC DefineHillsHolesIslands;
PCG_FUNC DefineCabins;
PCG_FUNC DefineCastles;
PCG_FUNC DefineSurface;
PCG_FUNC GenerateHills;
PCG_FUNC GenerateLakes;
PCG_FUNC GenerateIslands;
#include "utils.h"

#else
#include "pcg.h"
#endif


using namespace std::chrono_literals;
/************************************************
* 
* DRAW LOGIC
*
*************************************************/

#define DIRT            (Color){151, 107, 75, 255}
#define C_SPACE           (Color){51, 102, 153, 255}
#define C_SURFACE         (Color){155, 209, 255, 255}
#define C_UNDERGROUND     (Color){151, 107, 75, 255}
#define C_CAVERN          (Color){128, 128, 128, 255}
#define C_HELL            (Color){0, 0, 0, 255}

void DrawHorizontal(Map& map)
{
    for (auto& ref: map.HorizontalAreas())
    {
        auto& area = ref.get();
        Rect rect = area.bbox();
        Color color;
        switch (area.GetType())
        {
            case HorizontalAreas::SPACE:
                color = C_SPACE;
                break;
            case HorizontalAreas::SURFACE:
                color = C_SURFACE;
                break;
            case HorizontalAreas::UNDERGROUND:
                color = C_CAVERN;
                break;
            case HorizontalAreas::CAVERN:
                color = C_CAVERN;
                break;
            case HorizontalAreas::HELL:
                color = C_HELL;
                break;
            default:
                break;
        }
        DrawRectangle(rect.x, rect.y, rect.w, rect.h, color);
    }
};

void DrawBiomes(Map& map)
{
    for (auto& biome: map.Biomes())
    {
        for (auto& p: *biome)
        {
            switch (biome->GetType())
            {
                case Biomes::TUNDRA:
                    DrawPixel(p.x, p.y, (Color){255, 255, 255, 32});
                    break;
                case Biomes::JUNGLE:
                    DrawPixel(p.x, p.y, (Color){92, 68, 73, 32});
                    break;
                case Biomes::FOREST:
                    DrawPixel(p.x, p.y, (Color){0, 255, 0, 32});
                    break;
                case Biomes::OCEAN:
                    DrawPixel(p.x, p.y, (Color){0, 0, 255, 32});
                    break;
                default:
                    break;
            };
        }
    }
};

void DrawMiniBiomes(Map& map)
{
    for (auto& biome: map.MiniBiomes())
    {
        for (auto& p: *biome)
        {
            switch (biome->GetType())
            {
                case MiniBiomes::HILL:
                    DrawPixel(p.x, p.y, C_UNDERGROUND);
                    break;
                case MiniBiomes::HOLE:
                    DrawPixel(p.x, p.y, C_UNDERGROUND);
                    break;
                case MiniBiomes::FLOATING_ISLAND:
                    DrawPixel(p.x, p.y, YELLOW);
                    break;
                case MiniBiomes::CABIN:
                    DrawPixel(p.x, p.y, ORANGE);
                    break;
                case MiniBiomes::CASTLE:
                    DrawPixel(p.x, p.y, RED);
                    break;
                default:
                    break;
            };
        }
    }
};

void DrawSurfaceMiniBiomes(Map& map)
{
    for (auto& biome: map.SurfaceMiniBiomes())
    {
        for (auto& p: *biome)
        {
            switch (biome->GetType())
            {
                case MiniBiomes::SURFACE_PART:
                    DrawPixel(p.x, p.y, C_UNDERGROUND);
                    break;
                default:
                    break;
            };
        }
    }
};


/**************************************************
*
* PROCEDURAL GENERATION LOGIC 
*
**************************************************/
typedef void (*PCG_FUNC)(Map&);

std::atomic_bool GenerateStage0 { false };
std::atomic_bool GenerateStage1 { false };
std::atomic_bool GenerateStage2 { false };
std::atomic_bool GenerateStage3 { false };
std::atomic_bool GenerateStage4 { false };
std::atomic_bool DrawStage0 { false };
std::atomic_bool DrawStage1 { false };
std::atomic_bool DrawStage2 { false };
std::atomic_bool DrawStage3 { false };
std::atomic_bool DrawStage4 { false };

std::atomic_bool GenerationScheduled { false };
std::atomic_bool ScheduleThreadRunning { false };

#if (PCG_AS_DLL)
void PCGFunction(PCG_FUNC func, Map& map)
{
    map.ThreadIncrement();
    func(std::ref(map));
    map.ThreadDecrement();
};

bool LoadPCG()
{
    module = LoadLibrary("pcg.dll");
    if (module)
    {
        DefineHorizontal = (PCG_FUNC) GetProcAddress(module, "DefineHorizontal");
        DefineBiomes = (PCG_FUNC) GetProcAddress(module, "DefineBiomes");
        DefineHillsHolesIslands = (PCG_FUNC) GetProcAddress(module, "DefineHillsHolesIslands");
        DefineCabins = (PCG_FUNC) GetProcAddress(module, "DefineCabins");
        DefineCastles = (PCG_FUNC) GetProcAddress(module, "DefineCastles");
        DefineSurface = (PCG_FUNC) GetProcAddress(module, "DefineSurface");
        GenerateHills = (PCG_FUNC) GetProcAddress(module, "GenerateHills");
        GenerateLakes = (PCG_FUNC) GetProcAddress(module, "GenerateLakes");
        GenerateIslands = (PCG_FUNC) GetProcAddress(module, "GenerateIslands");
        return true;
    }
    return false;
};

void FreePCG()
{
    if (module) FreeLibrary(module);
};

bool ReloadPCG()
{
    FreePCG();
    return LoadPCG();
};

bool PCGLoaded()
{
    return module != NULL;
};
#else


void PCGFunction(PCG_FUNC func, Map& map)
{
    map.ThreadIncrement();
    func(std::ref(map));
    map.ThreadDecrement();
};
bool LoadPCG() { return true; };
void FreePCG() {};
bool ReloadPCG() { return true; };
bool PCGLoaded() { return true; }

#endif

void _PCGGen(Map& map)
{
    LoadPCG();
    printf("Dll load\n");

    // STAGE 0
    if (!map.ShouldForceStop() && GenerateStage0)
    {
        map.ClearStage0();
     
        auto define_horizontal_future = std::async(std::launch::async, PCGFunction, DefineHorizontal, std::ref(map));

        map.SetGenerationMessage("DEFINITION OF HORIZONTAL AREAS...");
        if (define_horizontal_future.wait_for(50s) == std::future_status::timeout)
        {
            map.SetForceStop(true);
            map.Error("DEFINITION OF HORIZONTAL AREAS INFEASIBLE");
        }
    }
    DrawStage0 = true;

    // STAGE 1
    if (!map.ShouldForceStop() && GenerateStage1)
    {
        map.ClearStage1();

        auto define_biomes_future = std::async(std::launch::async, PCGFunction, DefineBiomes, std::ref(map));

        map.SetGenerationMessage("DEFINITION OF BIOMES...");
        if (define_biomes_future.wait_for(50s) == std::future_status::timeout)
        {
            map.SetForceStop(true);
            map.Error("DEFINITION OF BIOMES INFEASIBLE");
        }
    }
    DrawStage1 = true;

    // STAGE 2
    if (!map.ShouldForceStop() && GenerateStage2)
    {
        map.ClearStage2();

        auto define_minibiomes_future = std::async(std::launch::async, PCGFunction, DefineHillsHolesIslands, std::ref(map));
        auto define_cabins_future = std::async(std::launch::async, PCGFunction, DefineCabins, std::ref(map));
        auto define_castles_future = std::async(std::launch::async, PCGFunction, DefineCastles, std::ref(map));

        map.SetGenerationMessage("DEFINITION OF HILLS, HOLES, ISLANDS...");
        if (define_minibiomes_future.wait_for(5s) == std::future_status::timeout)
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
    //DrawStage2 = true;

    if (!map.ShouldForceStop() && GenerateStage2)
    {
        map.ClearStage3();

        auto define_surface_future= std::async(std::launch::async, PCGFunction, DefineSurface, std::ref(map));
        
        map.SetGenerationMessage("DEFINITION OF SURFACE");
        if (define_surface_future.wait_for(5s) == std::future_status::timeout)
        {
            map.SetForceStop(true);
            map.Error("DEFINITION OF SURFACE INFEASIBLE");
        }
    }
    DrawStage3 = true;

    if (!map.ShouldForceStop() && GenerateStage4)
    {
        auto generate_hills_future = std::async(std::launch::async, PCGFunction, GenerateHills, std::ref(map));
        
        map.SetGenerationMessage("GENERATION OF HILLS");
        if (generate_hills_future.wait_for(5s) == std::future_status::timeout)
        {
            map.SetForceStop(true);
            map.Error("GENERATION OF HILLS INFEASIBLE");
        }
    }
    DrawStage4 = true;

    if (!map.ShouldForceStop())
    {
        GenerateStage0 = false;
        GenerateStage1 = false;
        GenerateStage2 = false;
        GenerateStage3 = false;
        GenerateStage4 = false;
    }

    // FINALIZE
    while (map.ThreadCount() > 0) std::this_thread::sleep_for(0s);

    FreePCG();
    printf("Free DLL\n");

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
            while (GenerationScheduled || map.IsGenerating())
            {
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


void PCGRegenerateHills(Map& map)
{
    GenerateStage4 = true;
    ScheduleGeneration(map);
};

void PCGRegenerateSurface(Map& map)
{
    GenerateStage3 = true;
    PCGRegenerateHills(map);
};

void PCGRegenerateMinibiomes(Map& map)
{
    GenerateStage2 = true;
    PCGRegenerateSurface(map);
};

void PCGRegenerateBiomes(Map& map)
{
    GenerateStage1 = true;
    PCGRegenerateMinibiomes(map);
};

void PCGRegenerateHorizontalAreas(Map& map)
{
    GenerateStage0 = true;
    PCGRegenerateBiomes(map);
};

/**************************************************
*
* GUI RELATED 
*
**************************************************/

void ChangeSeed()
{

};

void Pass(){};

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
    float em = 8.0; // EXTERNAL MARGIN
    float im = 8.0; // INTERNAL MARGIN
    Vector2 settings_anchor = {em, em};
    Vector2 map_view_anchor = {settings_anchor.x + 300 + im + im + em, settings_anchor.y};
    float settings_width = map_view_anchor.x - settings_anchor.x;
    // float settings_height = height - em - settings_anchor.y;
    float map_view_width = width - settings_width - em - em;
    float map_view_height = height - map_view_anchor.y - em; 

    camera.zoom = map_view_width / map_width;
    Rectangle map_view {map_view_anchor.x, map_view_anchor.y, map_view_width, map_view_height};

    bool SeedTextBoxEditMode = false;
    char SeedTextBoxText[128] = "0";
    int TabActive = 0;

    Rectangle ScrollView = {0, 0, 0, 0};
    Vector2 ScrollOffset = {0, 0};

    // CORE LOGIC INIT
    Map map;
    map.Width(map_width);
    map.Height(map_height);

    GenerateStage0 = true;
    GenerateStage1 = true;
    GenerateStage2 = true;
    GenerateStage3 = true;
    GenerateStage4 = true;
    ScheduleGeneration(map);

    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        if (DrawStage0)
        {
            BeginTextureMode(canvas);
                DrawHorizontal(map);
            EndTextureMode();

            DrawStage0 = false;
        }
        
        if (DrawStage1)
        {
            BeginTextureMode(canvas);
                DrawBiomes(map);
            EndTextureMode();

            DrawStage1 = false;
        }
   
        if (DrawStage2)
        {
            BeginTextureMode(canvas);
                DrawMiniBiomes(map);
            EndTextureMode();

            DrawStage2 = false;
        }

        if (DrawStage3)
        {
            BeginTextureMode(canvas);
                DrawSurfaceMiniBiomes(map);
            EndTextureMode();

            DrawStage3 = false;
        }

        if (DrawStage4)
        {
            BeginTextureMode(canvas);
                DrawMiniBiomes(map);
            EndTextureMode();

            DrawStage4 = false;
        }

        // RELOAD DLL
        if (IsKeyDown(KEY_R) && !map.IsGenerating())
        {
            map.ClearAll();
            ScheduleGeneration(map);
        }

        // DRAW LOGIC
        BeginDrawing();
            ClearBackground((Color){60, 56, 54, 255});

            if (PCGLoaded()) // IF DLL LOADED SHOW SETTINGS WITH MAP
            {
                GuiGroupBox((Rectangle){em, em, 2 * em + 300, height - 2 * em}, "SETTINGS");

                if (GuiButton((Rectangle){2 * em + im + 92, 2 * em, 64, 24}, "Seed"))
                    ChangeSeed();

                if (GuiTextBox((Rectangle){ 2 * em, 2 * em, 92, 24}, SeedTextBoxText, 128, SeedTextBoxEditMode))
                    SeedTextBoxEditMode = !SeedTextBoxEditMode;

                TabActive = GuiToggleGroup((Rectangle){2 * em, 2 * em + 2 * 24, 92, 24}, "MATERIALS;ENTITIES;WORLD", TabActive);
                if (TabActive == 0)
                {
                    GuiLabel((Rectangle){2 * em, 2 * em + 24, width - 4 * 8, 24}, "Settings about materials");
                    
                    GuiLabel((Rectangle){2 * em + 92, 5 * em + 2 * 24, 92, 24}, "FREQUENCY");
                    GuiLabel((Rectangle){2 * em + 2 * 92 + im, 5 * em + 2 * 24, 92, 24}, "SIZE");

                    GuiLabel((Rectangle){2 * em, 2 * em + 4 * 24, 92, 24}, "COPPER ORE");
                    GuiLabel((Rectangle){2 * em, 2 * em + im + 5 * 24, 92, 24}, "IRON ORE");
                    GuiLabel((Rectangle){2 * em, 2 * em + 2 * im + 6 * 24, 92, 24}, "SILVER ORE");
                    GuiLabel((Rectangle){2 * em, 2 * em + 3 * im + 7 * 24, 92, 24}, "GOLD ORE");

                    if (map.CopperFrequency(GuiSliderBar((Rectangle){2 * em + 92, 2 * em + 4 * 24, 92, 24}, "", "", map.CopperFrequency(), 0.0, 1.0)))
                        Pass();
                    if (map.IronFrequency(GuiSliderBar((Rectangle){2 * em + 92, 2 * em + im + 5 * 24, 92, 24}, "", "", map.IronFrequency(), 0.0, 1.0)))
                        Pass();
                    if (map.SilverFrequency(GuiSliderBar((Rectangle){2 * em + 92, 2 * em + 2 * im + 6 * 24, 92, 24}, "", "", map.SilverFrequency(), 0.0, 1.0)))
                        Pass();
                    if (map.GoldFrequency(GuiSliderBar((Rectangle){2 * em + 92, 2 * em + 3 * im + 7 * 24, 92, 24}, "", "", map.GoldFrequency(), 0.0, 1.0)))
                        Pass();

                    if (map.CopperSize(GuiSliderBar((Rectangle){2 * em + im + 2 * 92, 2 * em + 4 * 24, 92, 24}, "", "", map.CopperSize(), 0.0, 1.0)))
                        Pass();
                    if (map.IronSize(GuiSliderBar((Rectangle){2 * em + im + 2 * 92, 2 * em + im + 5 * 24, 92, 24}, "", "", map.IronSize(), 0.0, 1.0)))
                        Pass();
                    if (map.SilverSize(GuiSliderBar((Rectangle){2 * em + im + 2 * 92, 2 * em + 2 * im + 6 * 24, 92, 24}, "", "", map.SilverSize(), 0.0, 1.0)))
                        Pass();
                    if (map.GoldSize(GuiSliderBar((Rectangle){2 * em + im + 2 * 92, 2 * em + 3 * im + 7 * 24, 92, 24}, "", "", map.GoldSize(), 0.0, 1.0)))
                        Pass();
                }
                else if (TabActive == 1)
                {
                    GuiLabel((Rectangle){2 * 8, 2 * 8 + 24, width - 4 * 8, 24}, "Settings about entities in map");

                    GuiLabel((Rectangle){2 * em + 92, 5 * em + 2 * 24, 92, 24}, "FREQUENCY");

                    GuiLabel((Rectangle){2 * em, 2 * em + 4 * 24, 92, 24}, "HILLS");
                    GuiLabel((Rectangle){2 * em, 2 * em + im + 5 * 24, 92, 24}, "HOLES");
                    GuiLabel((Rectangle){2 * em, 2 * em + 2 * im + 6 * 24, 92, 24}, "CABINS");
                    GuiLabel((Rectangle){2 * em, 2 * em + 3 * im + 7 * 24, 92, 24}, "ISLANDS");

                    if (map.HillsFrequency(GuiSliderBar((Rectangle){2 * em + 92, 2 * em + 4 * 24, 92, 24}, "", "", map.HillsFrequency(), 0.0, 1.0)))
                        PCGRegenerateMinibiomes(map);
                    if (map.HolesFrequency(GuiSliderBar((Rectangle){2 * em + 92, 2 * em + im + 5 * 24, 92, 24}, "", "", map.HolesFrequency(), 0.0, 1.0)))
                        PCGRegenerateMinibiomes(map);
                    if (map.CabinsFrequency(GuiSliderBar((Rectangle){2 * em + 92, 2 * em + 2 * im + 6 * 24, 92, 24}, "", "", map.CabinsFrequency(), 0.0, 1.0)))
                        PCGRegenerateMinibiomes(map);
                    if (map.IslandsFrequency(GuiSliderBar((Rectangle){2 * em + 92, 2 * em + 3 * im + 7 * 24, 92, 24}, "", "", map.IslandsFrequency(), 0.0, 1.0)))
                        PCGRegenerateMinibiomes(map);

                }
                else
                {
                    GuiLabel((Rectangle){2 * 8, 2 * 8 + 24, width - 4 * 8, 24}, "Lorem ipsum");
                }

                ScrollView = GuiScrollPanel(map_view, (Rectangle){0, 0, map_width * camera.zoom, map_height * camera.zoom}, &ScrollOffset);
                DrawRectangleRec(map_view, {40, 40, 40, 255});

                BeginScissorMode(map_view.x, map_view.y, map_view.width, map_view.height);
                    BeginMode2D(camera);
                        DrawTextureRec(canvas.texture, (Rectangle) { 0, 0, map_width, -map_height}, {map_view_anchor.x * 1 / camera.zoom, map_view_anchor.y * 1 / camera.zoom}, WHITE);        
                    EndMode2D();
                EndScissorMode();

                auto mx = GetMouseX();
                auto my = GetMouseY();

                if ((mx > map_view.x) && (mx < map_view.x + map_view.width) && (my > map_view.y) && (my < map_view.y + map_view.height))
                {
                    auto x = (mx - map_view.x) * 1 / camera.zoom;
                    auto y = (my - map_view.y) * 1 / camera.zoom;
                    std::string t = "[";
                    t += std::to_string((int)x);
                    t += ":";
                    t += std::to_string((int)y);
                    t += "]";
                    DrawText(t.c_str(), mx, my - 16, 16, WHITE);
                }

                if (map.IsGenerating())
                { 
                    DrawText(map.GetGenerationMessage().c_str(), em, height, 8, WHITE);
                }
                else if (map.HasError()) 
                { 
                    auto error = map.Error();
                    auto error_width = (float) (error.size() * 6 + 2 * em);
                    if (error_width < 100) error_width = 100;

                    if (GuiMessageBox((Rectangle){width / 2 - error_width / 2, height / 2 - 50, error_width, 100}, "ERROR", error.c_str(), "OK") != -1)
                    {
                        map.PopError();    
                    }
                };
            } 
            else // IF NOT, SHOW WARNING
            {
                if (GuiMessageBox((Rectangle){(width / 2) - 100, height / 2 - 100, 200, 100}, "ERROR", "PCG.DLL NOT FOUND", "OK") != -1)
                {
                    CloseWindow();
                    return 0;
                }
            }
        EndDrawing();
    }

    CloseWindow();        // Close window and OpenGL context
    return 0;
}
