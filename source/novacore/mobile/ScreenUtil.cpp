#include "ScreenUtil.h"
#include <algorithm>

SDL_Window* ScreenUtil::window = nullptr;
int ScreenUtil::width = 0;
int ScreenUtil::height = 0;
float ScreenUtil::dpi = 96.0f;
SafeArea ScreenUtil::safeArea;

void ScreenUtil::Init(SDL_Window* w) {
    window = w;
    Refresh();
}

void ScreenUtil::Refresh() {
    if (!window) {
        return;
    }

    SDL_GetWindowSize(window, &width, &height);

    float hdpi = 96.0f;
    float vdpi = 96.0f;
    float ddpi = 96.0f;

    int displayIndex = SDL_GetWindowDisplayIndex(window);
    if (SDL_GetDisplayDPI(displayIndex, &ddpi, &hdpi, &vdpi) == 0) {
        dpi = ddpi;
    }
}

int ScreenUtil::GetWidth() {
    return width;
}

int ScreenUtil::GetHeight() {
    return height;
}

float ScreenUtil::GetScale() {
    return dpi / 96.0f;
}

float ScreenUtil::GetDPI() {
    return dpi;
}

Orientation ScreenUtil::GetOrientation() {
    return width >= height ? Orientation::Landscape : Orientation::Portrait;
}

bool ScreenUtil::IsMobile() {
#if defined(__ANDROID__) || defined(NOVACORE_ANDROID) || defined(__IPHONEOS__)
    return true;
#else
    return false;
#endif
}

SafeArea ScreenUtil::GetSafeArea() {
    return safeArea;
}

void ScreenUtil::SetSafeArea(const SafeArea& area) {
    safeArea = area;
}

float ScreenUtil::ToDesignSpace(float pixelValue, float designReferenceHeight) {
    if (height <= 0) {
        return pixelValue;
    }

    float ratio = designReferenceHeight / static_cast<float>(height);
    return pixelValue * ratio;
}

SDL_Rect ScreenUtil::FitToScreen(int contentWidth, int contentHeight) {
    if (contentWidth <= 0 || contentHeight <= 0 || width <= 0 || height <= 0) {
        return SDL_Rect{ 0, 0, width, height };
    }

    float contentAspect = static_cast<float>(contentWidth) / static_cast<float>(contentHeight);
    float screenAspect = static_cast<float>(width) / static_cast<float>(height);

    int fitWidth = width;
    int fitHeight = height;

    if (screenAspect > contentAspect) {
        fitHeight = height;
        fitWidth = static_cast<int>(height * contentAspect);
    } else {
        fitWidth = width;
        fitHeight = static_cast<int>(width / contentAspect);
    }

    int offsetX = (width - fitWidth) / 2;
    int offsetY = (height - fitHeight) / 2;

    return SDL_Rect{ offsetX, offsetY, fitWidth, fitHeight };
}
