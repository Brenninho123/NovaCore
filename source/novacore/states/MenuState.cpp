#include "MenuState.h"
#include "../backend/Controls.h"

void MenuState::Enter() {
    options = { "Play", "Options", "Credits", "Quit" };
    selectedIndex = 0;
}

void MenuState::Exit() {
    options.clear();
}

void MenuState::Update(float deltaTime) {
    (void)deltaTime;

    if (Controls::JustPressed(SDL_SCANCODE_DOWN)) {
        selectedIndex = (selectedIndex + 1) % static_cast<int>(options.size());
    }

    if (Controls::JustPressed(SDL_SCANCODE_UP)) {
        selectedIndex = (selectedIndex - 1 + static_cast<int>(options.size())) % static_cast<int>(options.size());
    }

    if (Controls::JustPressed(SDL_SCANCODE_RETURN)) {
        // selection confirmed, handled elsewhere once state manager exists
    }
}

void MenuState::Render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
    SDL_RenderClear(renderer);
}
