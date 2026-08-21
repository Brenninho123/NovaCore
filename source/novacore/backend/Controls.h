#pragma once

#include <SDL2/SDL.h>
#include <unordered_map>

class Controls {
public:
    static void Init();
    static void Update();
    static void HandleEvent(const SDL_Event& event);

    static bool IsDown(SDL_Scancode key);
    static bool JustPressed(SDL_Scancode key);
    static bool JustReleased(SDL_Scancode key);

private:
    static std::unordered_map<SDL_Scancode, bool> currentState;
    static std::unordered_map<SDL_Scancode, bool> previousState;
};
