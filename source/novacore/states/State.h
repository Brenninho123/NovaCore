#pragma once

#include <SDL2/SDL.h>

class State {
public:
    virtual ~State() = default;

    virtual void Enter() {}
    virtual void Exit() {}

    virtual void Pause() {}
    virtual void Resume() {}

    virtual void HandleEvent(const SDL_Event& event) { (void)event; }
    virtual void Update(float deltaTime) { (void)deltaTime; }
    virtual void Render(SDL_Renderer* renderer) { (void)renderer; }

    bool IsTransparent() const { return transparent; }
    bool BlocksUpdate() const { return blocksUpdate; }

protected:
    bool transparent = false;
    bool blocksUpdate = false;
};
