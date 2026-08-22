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
    void HandleEvent(const SDL_Event& event) override;
    void Update(float deltaTime) override;
    void Render(SDL_Renderer* renderer) override;

    void SetOnSelect(std::function<void(int)> callback);
    void SetEditorMode(bool enabled);
    bool IsEditorMode() const;

private:
    struct MenuButton {
        SDL_Rect rect;
        std::string label;
    };

    void MoveSelection(int direction);
    void ConfirmSelection();
    void RebuildLayout();

    int HitTest(int x, int y) const;
    void BeginPress(int x, int y);
    void EndPress(int x, int y);
    void CancelPress();

    std::vector<std::string> options;
    std::vector<MenuButton> buttons;
    int selectedIndex = 0;

    float inputCooldown = 0.0f;
    const float inputDelay = 0.15f;

    float pulseTimer = 0.0f;

    bool editorMode = false;

    int activePointerId = -1;
    int pressedIndex = -1;

    int layoutWidth = 0;
    int layoutHeight = 0;

    std::function<void(int)> onSelect;
};
