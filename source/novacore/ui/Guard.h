#pragma once

#include <SDL2/SDL.h>

class Guard {
public:
    static void Init();

    static void Block(float duration);
    static void BlockIndefinite();
    static void Unblock();

    static bool IsBlocked();

    static void Update(float deltaTime);
    static void Render(SDL_Renderer* renderer);

    static void SetDimColor(Uint8 r, Uint8 g, Uint8 b, Uint8 alpha);
    static void SetDimEnabled(bool enabled);

    static bool ConsumeClick();

private:
    static bool blocked;
    static bool indefinite;
    static float remainingTime;

    static bool dimEnabled;
    static Uint8 dimR;
    static Uint8 dimG;
    static Uint8 dimB;
    static Uint8 dimAlpha;
};
