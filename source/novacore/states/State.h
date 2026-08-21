#pragma once

#include <SDL2/SDL.h>

class State {
public:
    virtual ~State() = default;

    virtual void Enter() {}
    virtual void Exit() {}
    virtual void Update(float deltaTime) { (void)deltaTime; }
    virtual void Render(SDL_Renderer* renderer) { (void)renderer; }
};
