#include "MenuState.h"
#include "../backend/Controls.h"
#include <cmath>

void MenuState::Enter() {
    options = { "Play", "Options", "Credits", "Quit" };
    selectedIndex = 0;
    inputCooldown = 0.0f;
    pulseTimer = 0.0f;
}

void MenuState::Exit() {
    options.clear();
    onSelect = nullptr;
}

void MenuState::SetOnSelect(std::function<void(int)> callback) {
    onSelect = std::move(callback);
}

void MenuState::MoveSelection(int direction) {
    int count = static_cast<int>(options.size());
    selectedIndex = (selectedIndex + direction + count) % count;
    inputCooldown = inputDelay;
}

void MenuState::ConfirmSelection() {
    if (onSelect) {
        onSelect(selectedIndex);
    }
}

void MenuState::Update(float deltaTime) {
    pulseTimer += deltaTime;

    if (inputCooldown > 0.0f) {
        inputCooldown -= deltaTime;
    }

    if (inputCooldown <= 0.0f) {
        if (Controls::IsDown(SDL_SCANCODE_DOWN) || Controls::IsDown(SDL_SCANCODE_S)) {
            MoveSelection(1);
        } else if (Controls::IsDown(SDL_SCANCODE_UP) || Controls::IsDown(SDL_SCANCODE_W)) {
            MoveSelection(-1);
        }
    }

    if (Controls::JustPressed(SDL_SCANCODE_RETURN) || Controls::JustPressed(SDL_SCANCODE_SPACE)) {
        ConfirmSelection();
    }
}

void MenuState::Render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 18, 18, 26, 255);
    SDL_RenderClear(renderer);

    int viewportWidth = 0;
    int viewportHeight = 0;
    SDL_GetRendererOutputSize(renderer, &viewportWidth, &viewportHeight);

    int itemCount = static_cast<int>(options.size());
    int spacing = 60;
    int totalHeight = itemCount * spacing;
    int startY = (viewportHeight - totalHeight) / 2;

    for (int i = 0; i < itemCount; i++) {
        int boxWidth = 240;
        int boxHeight = 44;
        int x = (viewportWidth - boxWidth) / 2;
        int y = startY + i * spacing;

        SDL_Rect box = { x, y, boxWidth, boxHeight };

        if (i == selectedIndex) {
            float pulse = (std::sin(pulseTimer * 6.0f) + 1.0f) / 2.0f;
            Uint8 alpha = static_cast<Uint8>(120 + pulse * 100);

            SDL_SetRenderDrawColor(renderer, 255, 80, 140, alpha);
            SDL_RenderFillRect(renderer, &box);

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &box);
        } else {
            SDL_SetRenderDrawColor(renderer, 40, 40, 55, 255);
            SDL_RenderFillRect(renderer, &box);

            SDL_SetRenderDrawColor(renderer, 90, 90, 110, 255);
            SDL_RenderDrawRect(renderer, &box);
        }
    }
}
