#pragma once

#include <SDL2/SDL.h>
#include <functional>

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
    static SDL_Rect FitToScreenInteger(int contentWidth, int contentHeight);
    static int GetIntegerScale(int contentWidth, int contentHeight);

    static bool IsMultiTouchActive();
    static int GetActiveTouchCount();

    static void SetOnOrientationChanged(std::function<void(Orientation)> callback);
    static void SetOnSafeAreaChanged(std::function<void(const SafeArea&)> callback);

    static void HandleEvent(const SDL_Event& event);

private:
    static void NotifyOrientationIfChanged();

    static SDL_Window* window;
    static int width;
    static int height;
    static float dpi;
    static SafeArea safeArea;
    static Orientation lastOrientation;

    static int activeFingerCount;
    static SDL_FingerID activeFingers[8];

    static std::function<void(Orientation)> onOrientationChanged;
    static std::function<void(const SafeArea&)> onSafeAreaChanged;
};
