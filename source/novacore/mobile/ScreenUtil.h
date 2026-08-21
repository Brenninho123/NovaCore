#pragma once

#include <SDL2/SDL.h>

enum class Orientation {
    Landscape,
    Portrait
};

struct SafeArea {
    int top = 0;
    int bottom = 0;
    int left = 0;
    int right = 0;
};

class ScreenUtil {
public:
    static void Init(SDL_Window* window);
    static void Refresh();

    static int GetWidth();
    static int GetHeight();
    static float GetScale();
    static float GetDPI();

    static Orientation GetOrientation();
    static bool IsMobile();

    static SafeArea GetSafeArea();
    static void SetSafeArea(const SafeArea& area);

    static float ToDesignSpace(float pixelValue, float designReferenceHeight);
    static SDL_Rect FitToScreen(int contentWidth, int contentHeight);

private:
    static SDL_Window* window;
    static int width;
    static int height;
    static float dpi;
    static SafeArea safeArea;
};
