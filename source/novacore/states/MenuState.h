#pragma once

#include "State.h"
#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include <functional>

class MenuState : public State {
public:
    void Enter() override;
    void Exit() override;
    void Update(float deltaTime) override;
    void Render(SDL_Renderer* renderer) override;

    void SetOnSelect(std::function<void(int)> callback);

private:
    void MoveSelection(int direction);
    void ConfirmSelection();

    std::vector<std::string> options;
    int selectedIndex = 0;

    float inputCooldown = 0.0f;
    const float inputDelay = 0.15f;

    float pulseTimer = 0.0f;

    std::function<void(int)> onSelect;
};
