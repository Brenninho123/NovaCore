#pragma once

#include "State.h"
#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include <functional>

enum class EditorAction {
    NewProject,
    OpenProject,
    Settings,
    Exit
};

class MenuState : public State {
public:
    void Enter() override;
    void Exit() override;
    void HandleEvent(const SDL_Event& event) override;
    void Update(float deltaTime) override;
    void Render(SDL_Renderer* renderer) override;

    void SetOnAction(std::function<void(EditorAction)> callback);
    void SetOnOpenRecentProject(std::function<void(int)> callback);
    void SetRecentProjects(const std::vector<std::string>& projects);

private:
    struct EditorButton {
        SDL_Rect rect;
        std::string label;
        EditorAction action;
    };

    struct RecentEntry {
        SDL_Rect rect;
        std::string label;
    };

    struct LogEntry {
        std::string message;
        SDL_LogPriority priority;
    };

    static void SDLCALL LogCapture(void* userdata, int category, SDL_LogPriority priority, const char* message);

    void RebuildLayout();
    int HitTestToolbar(int x, int y) const;
    int HitTestRecent(int x, int y) const;

    void BeginPress(int x, int y);
    void EndPress(int x, int y);
    void CancelPress();

    void TriggerAction(EditorAction action);
    void ScrollRecentList(float delta);

    std::vector<EditorButton> toolbarButtons;
    std::vector<RecentEntry> recentButtons;
    std::vector<std::string> recentProjects;

    SDL_Rect consolePanelRect{};
    SDL_Rect recentPanelRect{};
    SDL_Rect viewportPanelRect{};
    SDL_Rect statusBarRect{};

    float recentScrollOffset = 0.0f;
    float recentScrollTarget = 0.0f;
    float recentListContentHeight = 0.0f;

    int pressedToolbarIndex = -1;
    int pressedRecentIndex = -1;
    int activePointerId = -1;

    bool draggingRecentList = false;
    int dragStartY = 0;
    float dragStartScroll = 0.0f;

    float actionCooldown = 0.0f;
    const float actionCooldownDuration = 0.25f;

    float fpsAccumulator = 0.0f;
    int fpsFrameCount = 0;
    float currentFps = 0.0f;

    int layoutWidth = 0;
    int layoutHeight = 0;

    static std::vector<LogEntry> logBuffer;
    static const size_t maxLogEntries = 40;
    static SDL_LogOutputFunction previousLogFunction;
    static void* previousLogUserdata;
    static bool logCaptureInstalled;

    std::function<void(EditorAction)> onAction;
    std::function<void(int)> onOpenRecentProject;
};
