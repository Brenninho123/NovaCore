#include "Controls.h"

std::unordered_map<SDL_Scancode, bool> Controls::currentState;
std::unordered_map<SDL_Scancode, bool> Controls::previousState;

void Controls::Init() {
    currentState.clear();
    previousState.clear();
}

void Controls::Update() {
    previousState = currentState;
}

void Controls::HandleEvent(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        currentState[event.key.keysym.scancode] = true;
    } else if (event.type == SDL_KEYUP) {
        currentState[event.key.keysym.scancode] = false;
    }
}

bool Controls::IsDown(SDL_Scancode key) {
    auto it = currentState.find(key);
    return it != currentState.end() && it->second;
}

bool Controls::JustPressed(SDL_Scancode key) {
    bool current = IsDown(key);
    auto it = previousState.find(key);
    bool previous = it != previousState.end() && it->second;
    return current && !previous;
}

bool Controls::JustReleased(SDL_Scancode key) {
    bool current = IsDown(key);
    auto it = previousState.find(key);
    bool previous = it != previousState.end() && it->second;
    return !current && previous;
}
