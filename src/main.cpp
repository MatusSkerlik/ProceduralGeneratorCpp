#include <stdio.h>
#include <thread>
#include <future>
#include <functional>
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
                color = C_UNDERGROUND;
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
                    DrawPixel(p.x, p.y, (Color){255, 255, 255, 64});
                    break;
                case Biomes::JUNGLE:
                    DrawPixel(p.x, p.y, (Color){92, 68, 73, 64});
                    break;
                case Biomes::FOREST:
                    DrawPixel(p.x, p.y, (Color){0, 255, 0, 64});
                    break;
                case Biomes::OCEAN:
                    DrawPixel(p.x, p.y, (Color){0, 0, 255, 64});
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
                    DrawPixel(p.x, p.y, RED);
                    break;
                case MiniBiomes::HOLE:
                    DrawPixel(p.x, p.y, BLUE);
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

bool PCGDrawReady = false;
bool PCGGenerating = false;

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
    // stage 0
    printf("Stage0\n");
    auto _0 = std::thread(define_horizontal, std::ref(map));
    _0.join();

    // stage 1
    printf("Stage1\n");
    auto _1 = std::thread(define_biomes, std::ref(map));
    _1.join();

    //stage 2
    printf("Stage2\n");
    auto _2 = std::thread(define_hills_holes_islands, std::ref(map));
    auto _3 = std::thread(define_cabins, std::ref(map));
    _2.join();
    _3.join();

    PCGDrawReady = true;
    PCGGenerating = false;
};

void PCGGen(Map& map)
{
    PCGGenerating = true;
    auto thread = std::thread(_PCGGen, std::ref(map));
    thread.detach();
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
    const int width = 1640;
    const int height = 800;
    const int map_width = 4200;
    const int map_height = 1200;

    // RAYLIB INIT
    InitWindow(width, height, "Procedural Terrain Generator");
    SetTargetFPS(60); 
    GuiLoadStyle("style.rgs");

    RenderTexture canvas = LoadRenderTexture(map_width, map_height);
    Camera2D camera {{0, 0}, {0, 0}, 0, 1};
   
    // RAYLIB RELATED

    // RAYGUI RELATED
    float em = 8.0; // EXTERNAL MARGIN
    float im = 8.0; // INTERNAL MARGIN
    Vector2 settings_anchor = {em, em};
    Vector2 map_view_anchor = {settings_anchor.x + 300 + im + im + em, settings_anchor.y};
    float settings_width = map_view_anchor.x - settings_anchor.x;
    float settings_height = height - em - settings_anchor.y;
    float map_view_width = width - settings_width - em - em;
    float map_view_height = height - map_view_anchor.y - em; 

    Rectangle map_view {map_view_anchor.x, map_view_anchor.y, map_view_width, map_view_height};

    
    camera.target = map_view_anchor;
    camera.offset = map_view_anchor;

    bool SeedTextBoxEditMode = false;
    char SeedTextBoxText[128] = "0";
    int TabActive = 0;

    float SliderCopperFrequency = 0.5;
    float SliderIronFrequency = 0.5;
    float SliderGoldFrequency = 0.5;
    float SliderSilverFrequency = 0.5;
    float SliderCopperSize = 0.5;
    float SliderIronSize = 0.5;
    float SliderSilverSize = 0.5;
    float SliderGoldSize = 0.5;

    Rectangle ScrollView = {0, 0, 0, 0};
    Vector2 ScrollOffset = {0, 0};

    // CORE LOGIC INIT
    Map map;
    map.Width(map_width);
    map.Height(map_height);

    if (LoadPCG())
        PCGGen(map);

    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        if (PCGDrawReady)
        {
            FreePCG();
            BeginTextureMode(canvas);
                DrawHorizontal(map);
                //DrawBiomes(map);
                DrawMiniBiomes(map);
            EndTextureMode();

            PCGDrawReady = false;
        }

        // ZOOM LOGIC
        if (GetMouseWheelMove() != 0)
        {
            auto zoom_before = camera.zoom;
            if (GetMouseWheelMove() > 0)
            {
                camera.zoom *= 1.15;
            }
            else
            {
                camera.zoom *= .85;
            }

            if (camera.zoom * width > map_width)
                camera.zoom = (float) map_width / width;
            if (camera.zoom < 0.25)
                camera.zoom = 0.25;
            if (camera.zoom > 4)
                camera.zoom = 4;

            ScrollView.width = camera.zoom * map_width;
            ScrollView.height = camera.zoom * map_height;

            printf("ScrollOffset %f, %f\n", ScrollOffset.x, ScrollOffset.y);
            if (ScrollView.width < map_view_width - 14)
            {
                camera.target = map_view_anchor;
                camera.offset = map_view_anchor;
            }
            else
            {
                camera.target.x = -ScrollOffset.x + GetMouseX(); 
                camera.offset.x = -ScrollOffset.x + GetMouseX();
                camera.target.y = -ScrollOffset.y + GetMouseY();
                camera.offset.y = -ScrollOffset.y + GetMouseY();
            }
        }
            
        // RELOAD DLL
        if (IsKeyDown(KEY_R) && !PCGGenerating)
            if (ReloadPCG())
                PCGGen(map);

        // DRAW LOGIC
        BeginDrawing();
            ClearBackground((Color){89, 89, 89, 255});

            if (PCGLoaded()) // IF DLL LOADED SHOW SETTINGS WITH MAP
            {
                GuiGroupBox((Rectangle){8, 8, (float)2 * 8 + 300, height - 2 * 8}, "SETTINGS");

                if (GuiButton((Rectangle){(float) 2 * 8 + 4 + 92, 2 * 8, 64, 24}, "Seed"))
                    ChangeSeed();

                if (GuiTextBox((Rectangle){(float) 2 * 8, 2 * 8, 92, 24}, SeedTextBoxText, 128, SeedTextBoxEditMode))
                    SeedTextBoxEditMode = !SeedTextBoxEditMode;

                TabActive = GuiToggleGroup((Rectangle){2 * 8, 2 * 8 + 2 * 24, 92, 24}, "SOURCES;ENTITIES;WORLD", TabActive);
                if (TabActive == 0)
                {
                    GuiLabel((Rectangle){2 * 8, 2 * 8 + 24, (float) width - 4 * 8, 24}, "Sources info");
                    GuiLabel((Rectangle){2 * 8, 2 * 8 + 4 * 24, 92, 24}, "COPPER ORE");
                    GuiLabel((Rectangle){2 * 8, 2 * 8 + 4 + 5 * 24, 92, 24}, "IRON ORE");
                    GuiLabel((Rectangle){2 * 8, 2 * 8 + 2 * 4 + 6 * 24, 92, 24}, "SILVER ORE");
                    GuiLabel((Rectangle){2 * 8, 2 * 8 + 3 * 4 + 7 * 24, 92, 24}, "GOLD ORE");

                    SliderCopperFrequency = GuiSliderBar((Rectangle){2 * 8 + 92, 2 * 8 + 4 * 24, 92, 24}, "", "", SliderCopperFrequency, 0.0, 1.0);
                    SliderIronFrequency = GuiSliderBar((Rectangle){2 * 8 + 92, 2 * 8 + 4 + 5 * 24, 92, 24}, "", "", SliderIronFrequency, 0.0, 1.0);
                    SliderSilverFrequency = GuiSliderBar((Rectangle){2 * 8 + 92, 2 * 8 + 2 * 4 + 6 * 24, 92, 24}, "", "", SliderSilverFrequency, 0.0, 1.0);
                    SliderGoldFrequency = GuiSliderBar((Rectangle){2 * 8 + 92, 2 * 8 + 3 * 4 + 7 * 24, 92, 24}, "", "", SliderGoldFrequency, 0.0, 1.0);

                    SliderCopperSize = GuiSliderBar((Rectangle){2 * 8 + 4 + 2 * 92, 2 * 8 + 4 * 24, 92, 24}, "", "", SliderCopperSize, 0.0, 1.0);
                    SliderIronSize = GuiSliderBar((Rectangle){2 * 8 + 4 + 2 * 92, 2 * 8 + 4 + 5 * 24, 92, 24}, "", "", SliderIronSize, 0.0, 1.0);
                    SliderSilverSize = GuiSliderBar((Rectangle){2 * 8 + 4 + 2 * 92, 2 * 8 + 2 * 4 + 6 * 24, 92, 24}, "", "", SliderSilverSize, 0.0, 1.0);
                    SliderGoldSize = GuiSliderBar((Rectangle){2 * 8 + 4 + 2 * 92, 2 * 8 + 3 * 4 + 7 * 24, 92, 24}, "", "", SliderGoldSize, 0.0, 1.0);
                    

                }
                else if (TabActive == 1)
                {
                    GuiLabel((Rectangle){2 * 8, 2 * 8 + 24, (float) width - 4 * 8, 24}, "Entities info");
                }
                else
                {
                    GuiLabel((Rectangle){2 * 8, 2 * 8 + 24, (float) width - 4 * 8, 24}, "World info");
                }

                ScrollView = GuiScrollPanel(map_view, (Rectangle){0, 0, map_width * camera.zoom, map_height * camera.zoom}, &ScrollOffset);
                
                    printf("MapViewAnchor %f, %f, Camera.Target %f, %f\n", map_view_anchor.x, map_view_anchor.y, camera.target.x, camera.target.y);
                    BeginMode2D(camera);
                        DrawTextureRec(canvas.texture, (Rectangle) { 0, 0, map_width, -map_height}, {-(camera.target.x - map_view_anchor.x), -(camera.target.y - map_view_anchor.y)}, WHITE);        
                    EndMode2D();
                BeginScissorMode(map_view.x, map_view.y, map_view.width - 14, map_view.height - 14);
                EndScissorMode();
            } 
            else // IF NOT, SHOW WARNING
            {
                if (GuiMessageBox((Rectangle){(float)(width / 2) - 100, (float)(height / 2) - 100, 200, 100}, "Error", "pcg.dll not found.", "ok") != -1)
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
