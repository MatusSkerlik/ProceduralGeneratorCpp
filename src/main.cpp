#include <stdio.h>
#include <thread>
#include <functional>
#include "libloaderapi.h"
#include "raylib.h"
#include "utils.h"


typedef void (*PCG_FUNC)(DATA&);

HMODULE module;
PCG_FUNC define_horizontal;
PCG_FUNC define_biomes;

void DrawHorizontal(DATA& data)
{
    for (auto area: data.HORIZONTAL_AREAS)
    {
        auto rect = std::get<0>(area);
        auto color = std::get<1>(area);
        DrawRectangle(rect.x, rect.y, rect.w, rect.h, (Color) {
            color.r, color.g, color.b, 255
        });
    }
};

void DrawBiomes(DATA& data)
{
    for (auto polygon: data.BIOMES)
    {
        for (auto& pixel: polygon.pixels)
        {
            DrawPixel(pixel.x, pixel.y, (Color) {
                0, 255, 0, 255
            });
        }
    }
}

void LoadPCG()
{
    module = LoadLibrary("pcg.dll");
    define_horizontal = (PCG_FUNC) GetProcAddress(module, "define_horizontal");
    define_biomes = (PCG_FUNC) GetProcAddress(module, "define_biomes");
}

void ReloadPCG()
{
    if (module) FreeLibrary(module);
    system("make pcg");
    LoadPCG();
}

void PCGGen(DATA* data)
{
    data->clear();
    define_horizontal(*data);
    define_biomes(*data);
}

void PCGGenThread(DATA* data)
{
    auto worker = std::thread(std::bind(PCGGen, data));
    worker.join();
}

int main(void)
{
    int width = 2*820;
    int height = 2*240;
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

    DATA data;
    data.WIDTH = map_width;
    data.HEIGHT = map_height;

    InitWindow(width, height, "Procedural Terrain Generator");
    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second


    LoadPCG();
    PCGGen(&data);

    while (!WindowShouldClose())    // Detect window close button or ESC key
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
            PCGGenThread(&data);
        }

        BeginDrawing();
        BeginMode2D(camera);
        ClearBackground(RAYWHITE);
        DrawHorizontal(data);
        DrawBiomes(data);
        EndMode2D();
        EndDrawing();
    }

    CloseWindow();        // Close window and OpenGL context
    return 0;
}
