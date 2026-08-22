#pragma once

#include "../../states/State.h"
#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include <functional>

enum class EditorPanel {
    Scene,
    Assets,
    Properties,
    Console
};

class EditorMenu : public State {
public:
    void Enter() override;
    void Exit() override;
    void HandleEvent(const SDL_Event& event) override;
    void Update(float deltaTime) override;
    void Render(SDL_Renderer* renderer) override;

    void SetOnExit(std::function<void()> callback);
    void SetOnSave(std::function<void()> callback);
    void SetProjectName(const std::string& name);

private:
    struct ToolbarButton {
        SDL_Rect rect;
        std::string label;
        std::function<void()> action;
    };

    struct TabButton {
        SDL_Rect rect;
        std::string label;
        EditorPanel panel;
    };

    void RebuildLayout();
    int HitTestToolbar(int x, int y) const;
    int HitTestTabs(int x, int y) const;

    void BeginPress(int x, int y);
    void EndPress(int x, int y);
    void CancelPress();

    void RenderScenePanel(SDL_Renderer* renderer, const SDL_Rect& rect);
    void RenderAssetsPanel(SDL_Renderer* renderer, const SDL_Rect& rect);
    void RenderPropertiesPanel(SDL_Renderer* renderer, const SDL_Rect& rect);
    void RenderConsolePanel(SDL_Renderer* renderer, const SDL_Rect& rect);
    void RenderPanelChrome(SDL_Renderer* renderer, const SDL_Rect& rect, Uint8 r, Uint8 g, Uint8 b);

    std::string projectName = "Untitled Project";

    std::vector<ToolbarButton> toolbarButtons;
    std::vector<TabButton> tabButtons;

    EditorPanel activeMobilePanel = EditorPanel::Scene;

    SDL_Rect toolbarRect{};
    SDL_Rect tabBarRect{};
    SDL_Rect scenePanelRect{};
    SDL_Rect assetsPanelRect{};
    SDL_Rect propertiesPanelRect{};
    SDL_Rect consolePanelRect{};

    int pressedToolbarIndex = -1;
    int pressedTabIndex = -1;
    int activePointerId = -1;

    float unsavedPulseTimer = 0.0f;
    bool hasUnsavedChanges = false;

    int layoutWidth = 0;
    int layoutHeight = 0;

    std::function<void()> onExit;
    std::function<void()> onSave;
};
