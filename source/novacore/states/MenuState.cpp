#include "MenuState.h"
#include "../backend/Controls.h"
#include "../mobile/ScreenUtil.h"
#include "../ui/Guard.h"
#include "../ui/TextRenderer.h"
#include <algorithm>
#include <cmath>

std::vector<MenuState::LogEntry> MenuState::logBuffer;
SDL_LogOutputFunction MenuState::previousLogFunction = nullptr;
void* MenuState::previousLogUserdata = nullptr;
bool MenuState::logCaptureInstalled = false;

void SDLCALL MenuState::LogCapture(void* userdata, int category, SDL_LogPriority priority, const char* message) {
    (void)category;

    LogEntry entry;
    entry.message = message;
    entry.priority = priority;
    logBuffer.push_back(entry);

    if (logBuffer.size() > maxLogEntries) {
        logBuffer.erase(logBuffer.begin());
    }

    if (previousLogFunction) {
        previousLogFunction(userdata, category, priority, message);
    }
}

void MenuState::Enter() {
    if (!logCaptureInstalled) {
        SDL_LogGetOutputFunction(&previousLogFunction, &previousLogUserdata);
        SDL_LogSetOutputFunction(LogCapture, previousLogUserdata);
        logCaptureInstalled = true;
    }

    toolbarButtons.clear();
    recentButtons.clear();

    recentScrollOffset = 0.0f;
    recentScrollTarget = 0.0f;

    pressedToolbarIndex = -1;
    pressedRecentIndex = -1;
    activePointerId = -1;
    draggingRecentList = false;

    layoutWidth = 0;
    layoutHeight = 0;

    fpsAccumulator = 0.0f;
    fpsFrameCount = 0;
    currentFps = 0.0f;

    SDL_Log("NovaCore Editor started");
}

void MenuState::Exit() {
    toolbarButtons.clear();
    recentButtons.clear();
    recentProjects.clear();

    if (logCaptureInstalled) {
        SDL_LogSetOutputFunction(previousLogFunction, previousLogUserdata);
        logCaptureInstalled = false;
    }

    onAction = nullptr;
    onOpenRecentProject = nullptr;
}

void MenuState::SetOnAction(std::function<void(EditorAction)> callback) {
    onAction = std::move(callback);
}

void MenuState::SetOnOpenRecentProject(std::function<void(int)> callback) {
    onOpenRecentProject = std::move(callback);
}

void MenuState::SetRecentProjects(const std::vector<std::string>& projects) {
    recentProjects = projects;
    RebuildLayout();
}

void MenuState::RebuildLayout() {
    int viewportWidth = ScreenUtil::GetWidth();
    int viewportHeight = ScreenUtil::GetHeight();

    if (viewportWidth <= 0 || viewportHeight <= 0) {
        return;
    }

    layoutWidth = viewportWidth;
    layoutHeight = viewportHeight;

    SafeArea safeArea = ScreenUtil::GetSafeArea();
    float scale = ScreenUtil::IsMobile() ? std::max(ScreenUtil::GetScale(), 1.0f) : 1.0f;

    int toolbarHeight = static_cast<int>((ScreenUtil::IsMobile() ? 96 : 56) * scale);
    int statusBarHeight = static_cast<int>((ScreenUtil::IsMobile() ? 48 : 28) * scale);
    int margin = static_cast<int>(12 * scale);

    int contentTop = safeArea.top + toolbarHeight + margin;
    int contentBottom = viewportHeight - safeArea.bottom - statusBarHeight - margin;
    int contentLeft = safeArea.left + margin;
    int contentRight = viewportWidth - safeArea.right - margin;
    int contentHeight = contentBottom - contentTop;
    int contentWidth = contentRight - contentLeft;

    int recentPanelWidth = static_cast<int>(contentWidth * (ScreenUtil::IsMobile() ? 1.0f : 0.32f));

    if (ScreenUtil::IsMobile()) {
        recentPanelRect = { contentLeft, contentTop, contentWidth, contentHeight / 2 - margin / 2 };
        consolePanelRect = { contentLeft, contentTop + contentHeight / 2 + margin / 2, contentWidth, contentHeight / 2 - margin / 2 };
        viewportPanelRect = { 0, 0, 0, 0 };
    } else {
        recentPanelRect = { contentLeft, contentTop, recentPanelWidth, contentHeight };
        viewportPanelRect = { contentLeft + recentPanelWidth + margin, contentTop, contentWidth - recentPanelWidth - margin, static_cast<int>(contentHeight * 0.6f) };
        consolePanelRect = { contentLeft + recentPanelWidth + margin, contentTop + viewportPanelRect.h + margin, contentWidth - recentPanelWidth - margin, contentHeight - viewportPanelRect.h - margin };
    }

    toolbarButtons.clear();

    int buttonWidth = static_cast<int>((ScreenUtil::IsMobile() ? 140 : 110) * scale);
    int buttonHeight = static_cast<int>((ScreenUtil::IsMobile() ? 72 : 40) * scale);
    int buttonSpacing = static_cast<int>(10 * scale);
    int buttonY = safeArea.top + (toolbarHeight - buttonHeight) / 2;
    int buttonX = contentLeft;

    struct ActionDef { const char* label; EditorAction action; };
    ActionDef actions[] = {
        { "New Project", EditorAction::NewProject },
        { "Open Project", EditorAction::OpenProject },
        { "Settings", EditorAction::Settings },
        { "Exit", EditorAction::Exit }
    };

    for (const auto& def : actions) {
        EditorButton button;
        button.rect = { buttonX, buttonY, buttonWidth, buttonHeight };
        button.label = def.label;
        button.action = def.action;
        toolbarButtons.push_back(button);
        buttonX += buttonWidth + buttonSpacing;
    }

    recentButtons.clear();

    int entryHeight = static_cast<int>((ScreenUtil::IsMobile() ? 64 : 36) * scale);
    int entrySpacing = static_cast<int>(4 * scale);
    int entryY = recentPanelRect.y + margin - static_cast<int>(recentScrollOffset);

    for (const auto& project : recentProjects) {
        RecentEntry entry;
        entry.rect = { recentPanelRect.x + margin / 2, entryY, recentPanelRect.w - margin, entryHeight };
        entry.label = project;
        recentButtons.push_back(entry);
        entryY += entryHeight + entrySpacing;
    }

    recentListContentHeight = static_cast<float>(recentProjects.size()) * (entryHeight + entrySpacing);

    statusBarRect = { safeArea.left, viewportHeight - safeArea.bottom - statusBarHeight, viewportWidth - safeArea.left - safeArea.right, statusBarHeight };
}

int MenuState::HitTestToolbar(int x, int y) const {
    for (size_t i = 0; i < toolbarButtons.size(); i++) {
        const SDL_Rect& rect = toolbarButtons[i].rect;
        if (x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int MenuState::HitTestRecent(int x, int y) const {
    if (x < recentPanelRect.x || x > recentPanelRect.x + recentPanelRect.w) {
        return -1;
    }

    if (y < recentPanelRect.y || y > recentPanelRect.y + recentPanelRect.h) {
        return -1;
    }

    for (size_t i = 0; i < recentButtons.size(); i++) {
        const SDL_Rect& rect = recentButtons[i].rect;
        if (y >= rect.y && y <= rect.y + rect.h) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void MenuState::TriggerAction(EditorAction action) {
    if (Guard::ConsumeClick()) {
        return;
    }

    if (actionCooldown > 0.0f) {
        return;
    }

    actionCooldown = actionCooldownDuration;
    Guard::Block(actionCooldownDuration);

    if (onAction) {
        onAction(action);
    }
}

void MenuState::ScrollRecentList(float delta) {
    recentScrollTarget -= delta;

    float maxScroll = std::max(0.0f, recentListContentHeight - static_cast<float>(recentPanelRect.h));
    recentScrollTarget = std::clamp(recentScrollTarget, 0.0f, maxScroll);
}

void MenuState::BeginPress(int x, int y) {
    pressedToolbarIndex = HitTestToolbar(x, y);

    if (pressedToolbarIndex == -1) {
        pressedRecentIndex = HitTestRecent(x, y);

        if (x >= recentPanelRect.x && x <= recentPanelRect.x + recentPanelRect.w &&
            y >= recentPanelRect.y && y <= recentPanelRect.y + recentPanelRect.h) {
            draggingRecentList = true;
            dragStartY = y;
            dragStartScroll = recentScrollTarget;
        }
    }
}

void MenuState::EndPress(int x, int y) {
    if (draggingRecentList) {
        int dragDistance = std::abs(y - dragStartY);

        if (dragDistance < 6) {
            int releasedIndex = HitTestRecent(x, y);
            if (releasedIndex >= 0 && releasedIndex == pressedRecentIndex) {
                if (onOpenRecentProject) {
                    onOpenRecentProject(releasedIndex);
                }
            }
        }
    } else {
        int releasedToolbar = HitTestToolbar(x, y);
        if (releasedToolbar >= 0 && releasedToolbar == pressedToolbarIndex) {
            TriggerAction(toolbarButtons[releasedToolbar].action);
        }
    }

    CancelPress();
}

void MenuState::CancelPress() {
    pressedToolbarIndex = -1;
    pressedRecentIndex = -1;
    activePointerId = -1;
    draggingRecentList = false;
}

void MenuState::HandleEvent(const SDL_Event& event) {
    if (event.type == SDL_FINGERDOWN) {
        if (activePointerId == -1) {
            activePointerId = static_cast<int>(event.tfinger.fingerId);
            int x = static_cast<int>(event.tfinger.x * layoutWidth);
            int y = static_cast<int>(event.tfinger.y * layoutHeight);
            BeginPress(x, y);
        }
        return;
    }

    if (event.type == SDL_FINGERUP) {
        if (activePointerId == static_cast<int>(event.tfinger.fingerId)) {
            int x = static_cast<int>(event.tfinger.x * layoutWidth);
            int y = static_cast<int>(event.tfinger.y * layoutHeight);
            EndPress(x, y);
        }
        return;
    }

    if (event.type == SDL_FINGERMOTION) {
        if (activePointerId == static_cast<int>(event.tfinger.fingerId) && draggingRecentList) {
            int y = static_cast<int>(event.tfinger.y * layoutHeight);
            float delta = static_cast<float>(y - dragStartY);
            recentScrollTarget = dragStartScroll - delta;

            float maxScroll = std::max(0.0f, recentListContentHeight - static_cast<float>(recentPanelRect.h));
            recentScrollTarget = std::clamp(recentScrollTarget, 0.0f, maxScroll);
        }
        return;
    }

    if (!ScreenUtil::IsMobile()) {
        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            BeginPress(event.button.x, event.button.y);
            return;
        }

        if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
            EndPress(event.button.x, event.button.y);
            return;
        }

        if (event.type == SDL_MOUSEWHEEL) {
            ScrollRecentList(static_cast<float>(event.wheel.y) * 24.0f);
            return;
        }

        if (event.type == SDL_KEYDOWN) {
            bool ctrl = (event.key.keysym.mod & KMOD_CTRL) != 0;

            if (ctrl && event.key.keysym.scancode == SDL_SCANCODE_N) {
                TriggerAction(EditorAction::NewProject);
            } else if (ctrl && event.key.keysym.scancode == SDL_SCANCODE_O) {
                TriggerAction(EditorAction::OpenProject);
            } else if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                TriggerAction(EditorAction::Exit);
            }
        }
    }
}

void MenuState::Update(float deltaTime) {
    if (layoutWidth != ScreenUtil::GetWidth() || layoutHeight != ScreenUtil::GetHeight()) {
        RebuildLayout();
    }

    if (actionCooldown > 0.0f) {
        actionCooldown -= deltaTime;
    }

    recentScrollOffset += (recentScrollTarget - recentScrollOffset) * std::min(1.0f, deltaTime * 12.0f);

    if (std::abs(recentScrollOffset - recentScrollTarget) < 0.5f) {
        recentScrollOffset = recentScrollTarget;
    } else {
        RebuildLayout();
    }

    fpsAccumulator += deltaTime;
    fpsFrameCount++;

    if (fpsAccumulator >= 0.5f) {
        currentFps = static_cast<float>(fpsFrameCount) / fpsAccumulator;
        fpsAccumulator = 0.0f;
        fpsFrameCount = 0;
    }

    Guard::Update(deltaTime);
}

static void DrawPanelFrame(SDL_Renderer* renderer, const SDL_Rect& rect, Uint8 r, Uint8 g, Uint8 b) {
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_RenderFillRect(renderer, &rect);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 30);
    SDL_RenderDrawRect(renderer, &rect);
}

void MenuState::Render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 14, 14, 20, 255);
    SDL_RenderClear(renderer);

    if (toolbarButtons.empty()) {
        RebuildLayout();
    }

    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color dim = { 200, 200, 210, 255 };

    for (size_t i = 0; i < toolbarButtons.size(); i++) {
        const SDL_Rect& rect = toolbarButtons[i].rect;
        bool pressed = static_cast<int>(i) == pressedToolbarIndex;

        if (pressed) {
            SDL_SetRenderDrawColor(renderer, 255, 120, 60, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 50, 50, 68, 255);
        }
        SDL_RenderFillRect(renderer, &rect);

        SDL_SetRenderDrawColor(renderer, 100, 100, 130, 255);
        SDL_RenderDrawRect(renderer, &rect);

        int textWidth = 0, textHeight = 0;
        TextRenderer::MeasureText("default", toolbarButtons[i].label, textWidth, textHeight);
        TextRenderer::DrawText(renderer, "default", toolbarButtons[i].label,
            rect.x + rect.w / 2, rect.y + rect.h / 2 - textHeight / 2, white, TextAlign::Center);
    }

    DrawPanelFrame(renderer, recentPanelRect, 26, 26, 36);

    int headerHeight = 22;
    TextRenderer::DrawText(renderer, "small", "RECENT PROJECTS", recentPanelRect.x + 8, recentPanelRect.y + 4, dim, TextAlign::Left);

    SDL_Rect clipRect = recentPanelRect;
    clipRect.y += headerHeight;
    clipRect.h -= headerHeight;
    SDL_RenderSetClipRect(renderer, &clipRect);

    for (size_t i = 0; i < recentButtons.size(); i++) {
        const SDL_Rect& rect = recentButtons[i].rect;
        bool pressed = static_cast<int>(i) == pressedRecentIndex;

        SDL_SetRenderDrawColor(renderer, pressed ? 70 : 38, pressed ? 70 : 38, pressed ? 90 : 52, 255);
        SDL_RenderFillRect(renderer, &rect);

        int textHeight = 0, textWidth = 0;
        TextRenderer::MeasureText("small", recentButtons[i].label, textWidth, textHeight);
        TextRenderer::DrawText(renderer, "small", recentButtons[i].label,
            rect.x + 10, rect.y + rect.h / 2 - textHeight / 2, white, TextAlign::Left);
    }

    if (recentButtons.empty()) {
        TextRenderer::DrawText(renderer, "small", "No recent projects", recentPanelRect.x + 8, recentPanelRect.y + headerHeight + 8, dim, TextAlign::Left);
    }

    SDL_RenderSetClipRect(renderer, nullptr);

    if (viewportPanelRect.w > 0) {
        DrawPanelFrame(renderer, viewportPanelRect, 8, 8, 12);
        TextRenderer::DrawText(renderer, "small", "VIEWPORT", viewportPanelRect.x + 8, viewportPanelRect.y + 4, dim, TextAlign::Left);
    }

    DrawPanelFrame(renderer, consolePanelRect, 10, 10, 14);
    TextRenderer::DrawText(renderer, "small", "CONSOLE", consolePanelRect.x + 8, consolePanelRect.y + 4, dim, TextAlign::Left);

    SDL_Rect consoleClip = consolePanelRect;
    consoleClip.y += headerHeight;
    consoleClip.h -= headerHeight;
    SDL_RenderSetClipRect(renderer, &consoleClip);

    int lineHeight = 16;
    int visibleLines = consoleClip.h / lineHeight;
    int startIndex = std::max(0, static_cast<int>(logBuffer.size()) - visibleLines);

    for (size_t i = startIndex; i < logBuffer.size(); i++) {
        const LogEntry& entry = logBuffer[i];
        int lineY = consoleClip.y + static_cast<int>(i - startIndex) * lineHeight;

        SDL_Color color = { 150, 150, 150, 255 };
        if (entry.priority == SDL_LOG_PRIORITY_WARN) { color = { 220, 190, 60, 255 }; }
        else if (entry.priority >= SDL_LOG_PRIORITY_ERROR) { color = { 220, 70, 70, 255 }; }
        else if (entry.priority == SDL_LOG_PRIORITY_INFO) { color = { 90, 160, 220, 255 }; }

        TextRenderer::DrawText(renderer, "small", entry.message, consolePanelRect.x + 8, lineY, color, TextAlign::Left);
    }

    SDL_RenderSetClipRect(renderer, nullptr);

    SDL_SetRenderDrawColor(renderer, 8, 8, 12, 255);
    SDL_RenderFillRect(renderer, &statusBarRect);

    Uint8 fpsR = 60, fpsG = 200, fpsB = 90;
    if (currentFps < 45.0f) { fpsR = 220; fpsG = 190; fpsB = 60; }
    if (currentFps < 25.0f) { fpsR = 220; fpsG = 70; fpsB = 70; }

    char fpsText[32];
    SDL_snprintf(fpsText, sizeof(fpsText), "%.0f FPS", currentFps);

    SDL_Color fpsColor = { fpsR, fpsG, fpsB, 255 };
    TextRenderer::DrawText(renderer, "small", fpsText, statusBarRect.x + 8, statusBarRect.y + statusBarRect.h / 2 - 7, fpsColor, TextAlign::Left);

    SDL_Color platformColor = { static_cast<Uint8>(ScreenUtil::IsMobile() ? 90 : 160), static_cast<Uint8>(ScreenUtil::IsMobile() ? 200 : 160), 220, 255 };
    TextRenderer::DrawText(renderer, "small", ScreenUtil::IsMobile() ? "Mobile" : "Desktop", statusBarRect.x + 90, statusBarRect.y + statusBarRect.h / 2 - 7, platformColor, TextAlign::Left);

    Guard::Render(renderer);
}
