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

int main(void)
{
    int width = 1640;
    int height = 800;
    int map_width = 4200;
    int map_height = 1200;

    int mouse_x, mouse_y;
    Camera2D camera {{0, 0}, {0, 0}, 0, (float) width / map_width};

    Map map;
    map.Width(map_width);
    map.Height(map_height);

    InitWindow(width, height, "Procedural Terrain Generator");
    SetTargetFPS(60); 
    RenderTexture canvas = LoadRenderTexture(map_width, map_height);
    
    if (LoadPCG())
        PCGGen(map);

    // load and generate map
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
        // zoom map logic 
        if (GetMouseWheelMove() != 0)
        {
            if (GetMouseWheelMove() > 0)
            {
                camera.zoom *= 1.1;
            }
            else
            {
                camera.zoom *= 0.9;
            }

            if (camera.zoom * width > map_width)
                camera.zoom = (float) map_width / width;
            if (camera.zoom < 0.25)
                camera.zoom = 0.25;
            if (camera.zoom > 1)
                camera.zoom = 1;
        }
            
        // move map init
        if (IsMouseButtonPressed(0))
        {
            mouse_x = GetMouseX();
            mouse_y = GetMouseY();
        }

        // move map logic 
        if (IsMouseButtonDown(0)) // left mouse button
        {
            camera.offset.x -= mouse_x - GetMouseX();
            camera.offset.y -= mouse_y - GetMouseY();
            mouse_x = GetMouseX();
            mouse_y = GetMouseY();
        }

        // reset camera view
        if (IsKeyDown(KEY_SPACE))
        {
            camera.offset.x = 0;
            camera.offset.y = 0;
            camera.zoom = (float) width / map_width;
        }

        // reload pgo.dll
        if (IsKeyDown(KEY_R) && !PCGGenerating)
            if (ReloadPCG())
                PCGGen(map);

        BeginDrawing();
            ClearBackground(RAYWHITE);
            if (PCGLoaded())
            {
                BeginMode2D(camera);
                    DrawTextureRec(canvas.texture, (Rectangle) { 0, 0, (float)canvas.texture.width, (float)-canvas.texture.height }, (Vector2) { 0, 0 }, WHITE);        
                EndMode2D();
            } 
            else 
            {
                if (GuiMessageBox((Rectangle){(float)(width / 2) - 100, (float)(height / 2) - 100, 200, 100}, "Error", "pcg.dll not found.", "ok") != -1)
                {
                    CloseWindow();
                    return 0;
                }
            }
            DrawFPS(0, 0);
        EndDrawing();
    }

    CloseWindow();        // Close window and OpenGL context
    return 0;
}
