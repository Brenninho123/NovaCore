#include "ScreenUtil.h"
#include <algorithm>
#include <cstring>

SDL_Window* ScreenUtil::window = nullptr;
int ScreenUtil::width = 0;
int ScreenUtil::height = 0;
float ScreenUtil::dpi = 96.0f;
SafeArea ScreenUtil::safeArea;
Orientation ScreenUtil::lastOrientation = Orientation::Landscape;

int ScreenUtil::activeFingerCount = 0;
SDL_FingerID ScreenUtil::activeFingers[8] = {};

std::function<void(Orientation)> ScreenUtil::onOrientationChanged;
std::function<void(const SafeArea&)> ScreenUtil::onSafeAreaChanged;

void ScreenUtil::Init(SDL_Window* w) {
    window = w;

    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
    SDL_SetHint(SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH, "1");

    SDL_DisableScreenSaver();

    Refresh();
    lastOrientation = GetOrientation();
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

    NotifyOrientationIfChanged();
}

void ScreenUtil::NotifyOrientationIfChanged() {
    Orientation current = GetOrientation();

    if (current != lastOrientation) {
        lastOrientation = current;

        if (onOrientationChanged) {
            onOrientationChanged(current);
        }
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
    bool changed = area.top != safeArea.top ||
                   area.bottom != safeArea.bottom ||
                   area.left != safeArea.left ||
                   area.right != safeArea.right;

    safeArea = area;

    if (changed && onSafeAreaChanged) {
        onSafeAreaChanged(safeArea);
    }
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

int ScreenUtil::GetIntegerScale(int contentWidth, int contentHeight) {
    if (contentWidth <= 0 || contentHeight <= 0 || width <= 0 || height <= 0) {
        return 1;
    }

    int scaleX = width / contentWidth;
    int scaleY = height / contentHeight;

    int scale = std::min(scaleX, scaleY);
    return std::max(scale, 1);
}

SDL_Rect ScreenUtil::FitToScreenInteger(int contentWidth, int contentHeight) {
    if (contentWidth <= 0 || contentHeight <= 0 || width <= 0 || height <= 0) {
        return SDL_Rect{ 0, 0, width, height };
    }

    int scale = GetIntegerScale(contentWidth, contentHeight);

    int fitWidth = contentWidth * scale;
    int fitHeight = contentHeight * scale;

    int offsetX = (width - fitWidth) / 2;
    int offsetY = (height - fitHeight) / 2;

    return SDL_Rect{ offsetX, offsetY, fitWidth, fitHeight };
}

bool ScreenUtil::IsMultiTouchActive() {
    return activeFingerCount > 1;
}

int ScreenUtil::GetActiveTouchCount() {
    return activeFingerCount;
}

void ScreenUtil::SetOnOrientationChanged(std::function<void(Orientation)> callback) {
    onOrientationChanged = std::move(callback);
}

void ScreenUtil::SetOnSafeAreaChanged(std::function<void(const SafeArea&)> callback) {
    onSafeAreaChanged = std::move(callback);
}

void ScreenUtil::HandleEvent(const SDL_Event& event) {
    if (event.type == SDL_FINGERDOWN) {
        bool alreadyTracked = false;

        for (int i = 0; i < activeFingerCount; i++) {
            if (activeFingers[i] == event.tfinger.fingerId) {
                alreadyTracked = true;
                break;
            }
        }

        if (!alreadyTracked && activeFingerCount < 8) {
            activeFingers[activeFingerCount] = event.tfinger.fingerId;
            activeFingerCount++;
        }
    } else if (event.type == SDL_FINGERUP) {
        for (int i = 0; i < activeFingerCount; i++) {
            if (activeFingers[i] == event.tfinger.fingerId) {
                for (int j = i; j < activeFingerCount - 1; j++) {
                    activeFingers[j] = activeFingers[j + 1];
                }
                activeFingerCount--;
                break;
            }
        }
    } else if (event.type == SDL_WINDOWEVENT) {
        if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
            event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            Refresh();
        }
    }
}
