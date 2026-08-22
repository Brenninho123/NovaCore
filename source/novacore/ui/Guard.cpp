#include "Guard.h"

bool Guard::blocked = false;
bool Guard::indefinite = false;
float Guard::remainingTime = 0.0f;

bool Guard::dimEnabled = true;
Uint8 Guard::dimR = 0;
Uint8 Guard::dimG = 0;
Uint8 Guard::dimB = 0;
Uint8 Guard::dimAlpha = 140;

void Guard::Init() {
    blocked = false;
    indefinite = false;
    remainingTime = 0.0f;
}

void Guard::Block(float duration) {
    blocked = true;
    indefinite = false;
    remainingTime = duration;
}

void Guard::BlockIndefinite() {
    blocked = true;
    indefinite = true;
    remainingTime = 0.0f;
}

void Guard::Unblock() {
    blocked = false;
    indefinite = false;
    remainingTime = 0.0f;
}

bool Guard::IsBlocked() {
    return blocked;
}

void Guard::Update(float deltaTime) {
    if (!blocked || indefinite) {
        return;
    }

    remainingTime -= deltaTime;

    if (remainingTime <= 0.0f) {
        Unblock();
    }
}

void Guard::Render(SDL_Renderer* renderer) {
    if (!blocked || !dimEnabled) {
        return;
    }

    int width = 0;
    int height = 0;
    SDL_GetRendererOutputSize(renderer, &width, &height);

    SDL_Rect fullscreen = { 0, 0, width, height };

    SDL_SetRenderDrawColor(renderer, dimR, dimG, dimB, dimAlpha);
    SDL_RenderFillRect(renderer, &fullscreen);
}

void Guard::SetDimColor(Uint8 r, Uint8 g, Uint8 b, Uint8 alpha) {
    dimR = r;
    dimG = g;
    dimB = b;
    dimAlpha = alpha;
}

void Guard::SetDimEnabled(bool enabled) {
    dimEnabled = enabled;
}

bool Guard::ConsumeClick() {
    if (blocked) {
        return true;
    }

    return false;
}
