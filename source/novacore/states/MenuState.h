#pragma once

#include "State.h"
#include <vector>
#include <string>

class MenuState : public State {
public:
    void Enter() override;
    void Exit() override;
    void Update(float deltaTime) override;
    void Render(SDL_Renderer* renderer) override;

private:
    std::vector<std::string> options;
    int selectedIndex = 0;
};
