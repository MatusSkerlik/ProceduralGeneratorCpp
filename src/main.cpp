#include <stdio.h>
#include <thread>
#include <future>
#include <functional>
#include <assert.h>
#include "libloaderapi.h"
#include "utils.h"

#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"


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
    for (auto& area: map.HorizontalAreas())
    {
        Rect rect = area->bbox();
        Color color;
        switch (area->type)
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
            switch (biome->type)
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
            switch (biome->type)
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

HMODULE module;
PCG_FUNC define_horizontal;
PCG_FUNC define_biomes;
PCG_FUNC define_hills_holes_islands;
PCG_FUNC define_cabins;

std::atomic_bool PCGRegenerate { false };
std::atomic_bool DrawStage0 { false };
std::atomic_bool DrawStage1 { false };
std::atomic_bool DrawStage2 { false };


void PCGFunction(PCG_FUNC func, Map* map, std::promise<void>* barrier)
{
    func(*map);
    barrier->set_value();
};

bool LoadPCG()
{
    module = LoadLibrary("pcg.dll");
    if (module)
    {
        define_horizontal = (PCG_FUNC) GetProcAddress(module, "define_horizontal");
        define_biomes = (PCG_FUNC) GetProcAddress(module, "define_biomes");
        define_hills_holes_islands = (PCG_FUNC) GetProcAddress(module, "define_hills_holes_islands");
        define_cabins = (PCG_FUNC) GetProcAddress(module, "define_cabins");
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

void _PCGGen(Map& map)
{
    LoadPCG();
    if (!map.ForceStop())
    {
        // stage 0
        map.ThreadIncrement();
            auto _0 = std::thread(define_horizontal, std::ref(map));
            map.SetGenerationMessage("DEFINITION OF HORIZONTAL AREAS...");
            _0.join();
        map.ThreadDecrement();
        DrawStage0 = true;
    }

    if (!map.ForceStop())
    {
        // stage 1
        map.ThreadIncrement();
            auto _1 = std::thread(define_biomes, std::ref(map));
            map.SetGenerationMessage("DEFINITION OF BIOMES...");
            _1.join();
        map.ThreadDecrement();
        DrawStage1 = true;
    }

    if (!map.ForceStop())
    {
        //stage 2
        map.ThreadIncrement();
            auto _2 = std::thread(define_hills_holes_islands, std::ref(map));

            map.ThreadIncrement();
                auto _3 = std::thread(define_cabins, std::ref(map));
                map.SetGenerationMessage("DEFINITION OF HILLS, HOLES, ISLANDS...");
                _2.join();
            map.ThreadDecrement();
            
            map.SetGenerationMessage("DEFINITION OF UNDERGROUND CABINS...");
            _3.join();
        map.ThreadDecrement();
        DrawStage2 = true;
    }

    assert(map.ThreadCount() == 0);
    map.SetGenerationMessage("");
    map.ForceStop(false);
    map.SetGenerating(false);
    FreePCG();
};

void PCGGen(Map& map)
{
    PCGRegenerate = false;
    DrawStage0 = false;
    DrawStage1 = false;
    DrawStage2 = false;
    map.SetGenerationMessage("");
    map.ForceStop(false);
    map.SetGenerating(true); 
    auto thread = std::thread(_PCGGen, std::ref(map));
    thread.detach();
};

void PCGReGen(Map& map)
{
    map.ForceStop(true);
    PCGRegenerate = true;  
};

/**************************************************
*
* GUI RELATED 
*
**************************************************/

void ChangeSeed()
{

};


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

    PCGGen(map);

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

        // RELOAD DLL
        if (IsKeyDown(KEY_R) && !map.IsGenerating())
        {
            map.clear();
            PCGGen(map);
        }

        if (PCGRegenerate && !map.IsGenerating())
        {
            PCGRegenerate = false;
            map.clear();
            PCGGen(map);
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
                        PCGReGen(map);
                    if (map.IronFrequency(GuiSliderBar((Rectangle){2 * em + 92, 2 * em + im + 5 * 24, 92, 24}, "", "", map.IronFrequency(), 0.0, 1.0)))
                        PCGReGen(map);
                    if (map.SilverFrequency(GuiSliderBar((Rectangle){2 * em + 92, 2 * em + 2 * im + 6 * 24, 92, 24}, "", "", map.SilverFrequency(), 0.0, 1.0)))
                        PCGReGen(map);
                    if (map.GoldFrequency(GuiSliderBar((Rectangle){2 * em + 92, 2 * em + 3 * im + 7 * 24, 92, 24}, "", "", map.GoldFrequency(), 0.0, 1.0)))
                        PCGReGen(map);

                    if (map.CopperSize(GuiSliderBar((Rectangle){2 * em + im + 2 * 92, 2 * em + 4 * 24, 92, 24}, "", "", map.CopperSize(), 0.0, 1.0)))
                        PCGReGen(map);
                    if (map.IronSize(GuiSliderBar((Rectangle){2 * em + im + 2 * 92, 2 * em + im + 5 * 24, 92, 24}, "", "", map.IronSize(), 0.0, 1.0)))
                        PCGReGen(map);
                    if (map.SilverSize(GuiSliderBar((Rectangle){2 * em + im + 2 * 92, 2 * em + 2 * im + 6 * 24, 92, 24}, "", "", map.SilverSize(), 0.0, 1.0)))
                        PCGReGen(map);
                    if (map.GoldSize(GuiSliderBar((Rectangle){2 * em + im + 2 * 92, 2 * em + 3 * im + 7 * 24, 92, 24}, "", "", map.GoldSize(), 0.0, 1.0)))
                        PCGReGen(map);
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
                        PCGReGen(map);
                    if (map.HolesFrequency(GuiSliderBar((Rectangle){2 * em + 92, 2 * em + im + 5 * 24, 92, 24}, "", "", map.HolesFrequency(), 0.0, 1.0)))
                        PCGReGen(map);
                    if (map.CabinsFrequency(GuiSliderBar((Rectangle){2 * em + 92, 2 * em + 2 * im + 6 * 24, 92, 24}, "", "", map.CabinsFrequency(), 0.0, 1.0)))
                        PCGReGen(map);
                    if (map.IslandsFrequency(GuiSliderBar((Rectangle){2 * em + 92, 2 * em + 3 * im + 7 * 24, 92, 24}, "", "", map.IslandsFrequency(), 0.0, 1.0)))
                        PCGReGen(map);

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

                if (map.IsGenerating())
                { 
                    DrawText(map.GetGenerationMessage().c_str(), em, height, 8, WHITE);
                }
                else if (map.HasError()) 
                { 
                    auto error = map.Error();
                    auto error_width = (float) (error.size() * 4.8 + 2 * em);
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
