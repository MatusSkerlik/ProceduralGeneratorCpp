#include <stdio.h>
#include <thread>
#include <functional>
#include "libloaderapi.h"
#include "raylib.h"
#include "utils.h"


typedef void (*PCG_FUNC)(Map&);

HMODULE module;
PCG_FUNC define_horizontal;
PCG_FUNC define_biomes;
PCG_FUNC define_minibiomes;

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

void LoadPCG()
{
    module = LoadLibrary("pcg.dll");
    define_horizontal = (PCG_FUNC) GetProcAddress(module, "define_horizontal");
    define_biomes = (PCG_FUNC) GetProcAddress(module, "define_biomes");
    define_minibiomes = (PCG_FUNC) GetProcAddress(module, "define_minibiomes");
};

void FreePCG()
{
    if (module) FreeLibrary(module);
}

void ReloadPCG()
{
    FreePCG();
    LoadPCG();
};

void PCGGen(Map* map)
{
    map->clear();
    define_horizontal(*map);
    define_biomes(*map);
    define_minibiomes(*map);
};

void PCGGenThread(Map* map)
{
    auto worker = std::thread(std::bind(PCGGen, map));
    worker.join();
};

int main(void)
{
    int width = 2 * 820;
    int height = 2 * 240;
    int map_width = 4200;
    int map_height = 1200;

    int mouse_x, mouse_y;
    Camera2D camera;
    camera.offset.x = 0;
    camera.offset.y = 0;
    camera.target.x = 0;
    camera.target.y = 0;
    camera.rotation = 0;
    camera.zoom = (float) width / map_width;

    Map map;
    map.width = map_width;
    map.height = map_height;

    InitWindow(width, height, "Procedural Terrain Generator");
    SetTargetFPS(60);  // Set our game to run at 60 frames-per-second

    RenderTexture canvas = LoadRenderTexture(map_width, map_height);

    LoadPCG();
    PCGGen(&map);
    FreePCG();

    BeginTextureMode(canvas);
        DrawHorizontal(map);
        DrawBiomes(map);
        DrawMiniBiomes(map);
    EndTextureMode();

    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
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

        if (IsMouseButtonPressed(0))
        {
            mouse_x = GetMouseX();
            mouse_y = GetMouseY();
        }

        if (IsMouseButtonDown(0))
        {
            camera.offset.x -= mouse_x - GetMouseX();
            camera.offset.y -= mouse_y - GetMouseY();
            mouse_x = GetMouseX();
            mouse_y = GetMouseY();
        }

        if (IsKeyDown(KEY_SPACE))
        {
            camera.offset.x = 0;
            camera.offset.y = 0;
            camera.zoom = (float) width / map_width;
        }

        if (IsKeyDown(KEY_R))
        {
            ReloadPCG();
            PCGGenThread(&map);
            FreePCG();
            BeginTextureMode(canvas);
                DrawHorizontal(map);
                DrawBiomes(map);
                DrawMiniBiomes(map);
            EndTextureMode();
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);
            BeginMode2D(camera);
                DrawTextureRec(canvas.texture, (Rectangle) { 0, 0, (float)canvas.texture.width, (float)-canvas.texture.height }, (Vector2) { 0, 0 }, WHITE);        
            EndMode2D();
            DrawFPS(0, 0);
        EndDrawing();
    }

    CloseWindow();        // Close window and OpenGL context
    return 0;
}
