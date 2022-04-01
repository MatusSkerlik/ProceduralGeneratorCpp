#ifndef GUI
#define GUI

#include <functional>
#include <string>
#ifndef RAYLIB_H
#include "raylib.h"
#endif

#ifndef RAYGUI_H
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#endif

#ifndef UTILS
#include "utils.h"
#endif

class Renderable 
{
    public:
        virtual void Render(float ax, float ay) = 0;
};

class Button: public Renderable 
{
    private:
        float x, y;
        float w, h;
        std::string text;
        std::function<void ()> on_click;

    public:
        Button(float _x, float _y, float _w, float _h, 
                std::string _text, 
                std::function<void ()> _on_click = [](){}
        ): x{_x}, y{_y}, w{_w}, h{_h}, 
            text{_text}, 
            on_click{_on_click} 
        {};

        void SetOnClickListener(std::function<void ()> f) { on_click = f;} ;

        virtual void Render(float ax, float ay) override
        {
            if (GuiButton({ax + x, ay + y, w, h}, text.c_str())) on_click();
        };
};

class Label: public Renderable
{
    private:
        float x, y;
        float w, h;
        std::string text;
    public:
        Label(float _x, float _y, float _w, float _h, std::string _text): x{_x}, y{_y}, w{_w}, h{_h}, text{_text} {};

        virtual void Render(float ax, float ay) override
        {
            GuiLabel({ax + x, ay + y, w, h}, text.c_str());
        };
};

class ToogleButton: public Renderable
{
    private:
        float x, y;
        float w, h;
        bool active;
        std::string text;

        std::function<void (bool)> on_click;

    public:
        ToogleButton(float _x, float _y, float _w, float _h, 
                std::string _text, 
                std::function<void (bool)> _on_click = [](bool){}, 
                bool _active = false
        ):  x{_x}, y{_y}, w{_w}, h{_h}, 
            active{_active}, 
            text{_text}, 
            on_click{_on_click} 
        {};

        void SetOn() { active = true; };
        void SetOff() { active = false; };
        auto IsActive() { return active; };

        void SetOnClickListener(std::function<void (bool)> f) { on_click = f;} ;

        virtual void Render(float ax, float ay) override
        {
            auto a = GuiToggle({ax + x, ay + y, w, h}, text.c_str(), active);
            if (a != active)
            {
                active = a;
                on_click(active);
            }
        };
};
class SliderBar: public Renderable
{
    private:
        float x, y;
        float w, h;
        std::string text_left;
        std::string text_right;
        std::function<void (float)> on_change;

        // internal logic
        float value;

    public:
        SliderBar(float _x, float _y, float _w, float _h, 
                float _value,
                std::string _text_left = "0", 
                std::string _text_right = "1", 
                std::function<void (float)> _on_change = [](float){}
        ):  x{_x}, y{_y}, w{_w}, h{_h}, 
            text_left{_text_left}, 
            text_right{_text_right}, 
            on_change{_on_change}, 
            value{_value}
        {};

        auto GetValue() { return value; };
        void SetValue(float v) { value = v; } ;

        void SetOnChangeListener(std::function<void (float)> f) { on_change = f;} ;

        virtual void Render(float ax, float ay) override
        {
            auto v = GuiSliderBar({ax + x, ay + y, w, h}, text_left.c_str(), text_right.c_str(), value, 0, 1);  
            if (v != value)
            {
                value = v;
                on_change(value);
            }
        };
};

class Layout
{
    protected:
        float x, y;
        float w, h;
        Color bg {64, 64, 64, 192};
        std::vector<std::unique_ptr<Renderable>> renderables;

    public:
        Layout(float _x, float _y, float _w, float _h): x{_x}, y{_y}, w{_w}, h{_h} {};

        void SetX(float _x) { x = _x; };
        void SetY(float _y) { y = _y; };
        void SetWidth(float _w) { w = _w; };
        void SetHeight(float _h) { h = _h; };

        auto GetX() { return x; };
        auto GetY() { return y; };
        auto GetWidth() { return w; };
        auto GetHeight() { return h; };

        void SetBg(Color c){ bg = c; };

        virtual Label& CreateLabel(float lx, float ly, float w, float h, std::string text)
        {
            std::unique_ptr<Renderable> label {new Label(lx, ly, w, h, text)};
            renderables.push_back(std::move(label));
            return *dynamic_cast<Label*>(renderables.back().get());
        }; 

        virtual Button& CreateButton(float bx, float by, float w, float h, std::string text, 
                std::function<void ()> on_click = [](){}
        ){
            std::unique_ptr<Renderable> button {new Button(bx, by, w, h, text, on_click)};
            renderables.push_back(std::move(button));
            return *dynamic_cast<Button*>(renderables.back().get());
        };

        virtual ToogleButton& CreateToogleButton(float bx, float by, float w, float h, std::string text, 
                std::function<void (bool)> on_click = [](bool){}, 
                bool active = false
        ){
            std::unique_ptr<Renderable> button {new ToogleButton(bx, by, w, h, text, on_click, active)};
            renderables.push_back(std::move(button));
            return *dynamic_cast<ToogleButton*>(renderables.back().get());
        };

        virtual SliderBar& CreateSliderBar(float sx, float sy, float w, float h, float value, 
                std::function<void (float)> on_change = [](float){}
        ){
            std::unique_ptr<Renderable> slider_bar {new SliderBar(sx, sy, w, h, value, "0", "1", on_change)};
            renderables.push_back(std::move(slider_bar));
            return *dynamic_cast<SliderBar*>(renderables.back().get());
        };

        virtual void Render()
        {
            DrawRectangle(x, y, w, h, bg);
            for (auto& renderable: renderables) renderable->Render(x, y);
        };
};

class HeaderLayout: public Layout
{
    protected:
        Color hbg {32, 32, 32, 192};
        float hh = 16; // header height 
        std::string title;

        // inner logic
        bool drag = false; // drag event 
        bool hidden = false;
        float lmx = 0; // last mouse x (before drag)
        float lmy = 0; // last mouse y (before drag)

    public:
        HeaderLayout(float x, float y, float w, float h, std::string _title): Layout(x, y + 16, w, h), title{_title} {};

        void SetHeaderBg(Color c) { hbg = c; };
        void SetHeaderHeight(float h) { hh = h; };
        void Hide() { hidden = true; };
        void Show() { hidden = false; };

        virtual void Render() override
        {

            if (!drag)
            {
                auto mx = GetMouseX();
                auto my = GetMouseY();

                if (mx >= GetX() && mx <= GetX() + GetWidth() && my >= GetY() - hh && my <= GetY())
                {

                    if (mx >= GetX() + GetWidth() - 16 - 2 && mx <= GetX() + GetWidth() - 2)
                    {
                        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
                        if (IsMouseButtonDown(0)) hidden = !hidden;
                    }
                    else
                    {
                        SetMouseCursor(MOUSE_CURSOR_RESIZE_ALL);
                    }

                    if (IsMouseButtonDown(0))
                    {
                        drag = true;
                        lmx = mx;
                        lmy = my;
                    }
                }
                else 
                {
                    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
                }
            }
            else
            {
                if (IsMouseButtonUp(0))
                {
                    drag = false;
                }
                else
                {
                    auto nx = lmx - GetMouseX();
                    auto ny = lmy - GetMouseY();

                    SetX(GetX() - nx);
                    SetY(GetY() - ny);

                    lmx = GetMouseX();
                    lmy = GetMouseY();
                }
            }



            DrawRectangle(GetX(), GetY() - hh, GetWidth(), hh, hbg);
            DrawText(title.c_str(), GetX() + 2, GetY() - hh + 2, 12, WHITE);
            if (hidden)
            {
                GuiDrawIcon(RAYGUI_ICON_ARROW_DOWN_FILL, GetX() + GetWidth() - 16 - 2, GetY() - hh, 1, WHITE);
            }
            else
            {
                GuiDrawIcon(RAYGUI_ICON_ARROW_UP_FILL, GetX() + GetWidth() - 16 - 2, GetY() - hh, 1, WHITE);
                Layout::Render();
            }

        };
};



#endif // GUI
