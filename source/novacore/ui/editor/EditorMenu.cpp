#include "EditorMenu.h"
#include "../../backend/Controls.h"
#include "../../mobile/ScreenUtil.h"
#include "../../ui/Guard.h"
#include <cmath>
#include <algorithm>

void EditorMenu::Enter() {
    toolbarButtons.clear();
    tabButtons.clear();

    activeMobilePanel = EditorPanel::Scene;

    pressedToolbarIndex = -1;
    pressedTabIndex = -1;
    activePointerId = -1;

    hasUnsavedChanges = false;
    unsavedPulseTimer = 0.0f;

    layoutWidth = 0;
    layoutHeight = 0;

    SDL_Log("Editor workspace opened: %s", projectName.c_str());
}

void EditorMenu::Exit() {
    toolbarButtons.clear();
    tabButtons.clear();

    onExit = nullptr;
    onSave = nullptr;

    SDL_Log("Editor workspace closed");
}

void EditorMenu::SetOnExit(std::function<void()> callback) {
    onExit = std::move(callback);
}

void EditorMenu::SetOnSave(std::function<void()> callback) {
    onSave = std::move(callback);
}

void EditorMenu::SetProjectName(const std::string& name) {
    projectName = name;
}

void EditorMenu::RebuildLayout() {
    int viewportWidth = ScreenUtil::GetWidth();
    int viewportHeight = ScreenUtil::GetHeight();

    if (viewportWidth <= 0 || viewportHeight <= 0) {
        return;
    }

    layoutWidth = viewportWidth;
    layoutHeight = viewportHeight;

    SafeArea safeArea = ScreenUtil::GetSafeArea();
    float scale = ScreenUtil::IsMobile() ? std::max(ScreenUtil::GetScale(), 1.0f) : 1.0f;

    int toolbarHeight = static_cast<int>((ScreenUtil::IsMobile() ? 88 : 48) * scale);
    int margin = static_cast<int>(10 * scale);

    toolbarRect = { safeArea.left, safeArea.top, viewportWidth - safeArea.left - safeArea.right, toolbarHeight };

    toolbarButtons.clear();

    int buttonWidth = static_cast<int>((ScreenUtil::IsMobile() ? 130 : 90) * scale);
    int buttonHeight = static_cast<int>((ScreenUtil::IsMobile() ? 64 : 32) * scale);
    int buttonY = toolbarRect.y + (toolbarHeight - buttonHeight) / 2;
    int buttonSpacing = static_cast<int>(8 * scale);

    ToolbarButton backButton;
    backButton.rect = { toolbarRect.x + margin, buttonY, buttonWidth, buttonHeight };
    backButton.label = "Back";
    backButton.action = [this]() {
        if (Guard::ConsumeClick()) return;
        Guard::Block(0.3f);
        if (onExit) onExit();
    };
    toolbarButtons.push_back(backButton);

    ToolbarButton saveButton;
    saveButton.rect = { backButton.rect.x + buttonWidth + buttonSpacing, buttonY, buttonWidth, buttonHeight };
    saveButton.label = "Save";
    saveButton.action = [this]() {
        if (Guard::ConsumeClick()) return;
        Guard::Block(0.3f);
        hasUnsavedChanges = false;
        if (onSave) onSave();
    };
    toolbarButtons.push_back(saveButton);

    int contentTop = toolbarRect.y + toolbarHeight + margin;
    int contentBottom = viewportHeight - safeArea.bottom;
    int contentLeft = safeArea.left + margin;
    int contentRight = viewportWidth - safeArea.right - margin;

    if (ScreenUtil::IsMobile()) {
        int tabBarHeight = static_cast<int>(72 * scale);
        contentBottom -= tabBarHeight;

        tabBarRect = { safeArea.left, viewportHeight - safeArea.bottom - tabBarHeight, viewportWidth - safeArea.left - safeArea.right, tabBarHeight };

        tabButtons.clear();

        struct TabDef { const char* label; EditorPanel panel; };
        TabDef tabs[] = {
            { "Scene", EditorPanel::Scene },
            { "Assets", EditorPanel::Assets },
            { "Props", EditorPanel::Properties },
            { "Console", EditorPanel::Console }
        };

        int tabWidth = tabBarRect.w / 4;

        for (int i = 0; i < 4; i++) {
            TabButton tab;
            tab.rect = { tabBarRect.x + i * tabWidth, tabBarRect.y, tabWidth, tabBarRect.h };
            tab.label = tabs[i].label;
            tab.panel = tabs[i].panel;
            tabButtons.push_back(tab);
        }

        SDL_Rect fullContent = { contentLeft, contentTop, contentRight - contentLeft, contentBottom - contentTop };
        scenePanelRect = fullContent;
        assetsPanelRect = fullContent;
        propertiesPanelRect = fullContent;
        consolePanelRect = fullContent;
    } else {
        int contentWidth = contentRight - contentLeft;
        int contentHeight = contentBottom - contentTop;

        int leftColumnWidth = static_cast<int>(contentWidth * 0.2f);
        int rightColumnWidth = static_cast<int>(contentWidth * 0.24f);
        int centerColumnWidth = contentWidth - leftColumnWidth - rightColumnWidth - margin * 2;

        int consoleHeight = static_cast<int>(contentHeight * 0.25f);
        int sceneHeight = contentHeight - consoleHeight - margin;

        assetsPanelRect = { contentLeft, contentTop, leftColumnWidth, contentHeight };
        scenePanelRect = { contentLeft + leftColumnWidth + margin, contentTop, centerColumnWidth, sceneHeight };
        consolePanelRect = { contentLeft + leftColumnWidth + margin, contentTop + sceneHeight + margin, centerColumnWidth, consoleHeight };
        propertiesPanelRect = { contentLeft + leftColumnWidth + centerColumnWidth + margin * 2, contentTop, rightColumnWidth, contentHeight };
    }
}

int EditorMenu::HitTestToolbar(int x, int y) const {
    for (size_t i = 0; i < toolbarButtons.size(); i++) {
        const SDL_Rect& rect = toolbarButtons[i].rect;
        if (x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int EditorMenu::HitTestTabs(int x, int y) const {
    for (size_t i = 0; i < tabButtons.size(); i++) {
        const SDL_Rect& rect = tabButtons[i].rect;
        if (x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void EditorMenu::BeginPress(int x, int y) {
    pressedToolbarIndex = HitTestToolbar(x, y);

    if (pressedToolbarIndex == -1 && ScreenUtil::IsMobile()) {
        pressedTabIndex = HitTestTabs(x, y);
    }
}

void EditorMenu::EndPress(int x, int y) {
    int releasedToolbar = HitTestToolbar(x, y);
    if (releasedToolbar >= 0 && releasedToolbar == pressedToolbarIndex) {
        toolbarButtons[releasedToolbar].action();
    }

    if (ScreenUtil::IsMobile()) {
        int releasedTab = HitTestTabs(x, y);
        if (releasedTab >= 0 && releasedTab == pressedTabIndex) {
            activeMobilePanel = tabButtons[releasedTab].panel;
        }
    }

    CancelPress();
}

void EditorMenu::CancelPress() {
    pressedToolbarIndex = -1;
    pressedTabIndex = -1;
    activePointerId = -1;
}

void EditorMenu::HandleEvent(const SDL_Event& event) {
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

    if (!ScreenUtil::IsMobile()) {
        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            BeginPress(event.button.x, event.button.y);
            return;
        }

        if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
            EndPress(event.button.x, event.button.y);
            return;
        }

        if (event.type == SDL_KEYDOWN) {
            bool ctrl = (event.key.keysym.mod & KMOD_CTRL) != 0;

            if (ctrl && event.key.keysym.scancode == SDL_SCANCODE_S) {
                if (!Guard::ConsumeClick()) {
                    Guard::Block(0.3f);
                    hasUnsavedChanges = false;
                    if (onSave) onSave();
                }
            } else if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                if (!Guard::ConsumeClick()) {
                    Guard::Block(0.3f);
                    if (onExit) onExit();
                }
            }
        }
    }
}

void EditorMenu::Update(float deltaTime) {
    if (layoutWidth != ScreenUtil::GetWidth() || layoutHeight != ScreenUtil::GetHeight()) {
        RebuildLayout();
    }

    unsavedPulseTimer += deltaTime;
    Guard::Update(deltaTime);
}

void EditorMenu::RenderPanelChrome(SDL_Renderer* renderer, const SDL_Rect& rect, Uint8 r, Uint8 g, Uint8 b) {
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_RenderFillRect(renderer, &rect);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 25);
    SDL_RenderDrawRect(renderer, &rect);
}

void EditorMenu::RenderScenePanel(SDL_Renderer* renderer, const SDL_Rect& rect) {
    RenderPanelChrome(renderer, rect, 6, 6, 10);

    SDL_Rect gizmo = { rect.x + rect.w / 2 - 4, rect.y + rect.h / 2 - 4, 8, 8 };
    SDL_SetRenderDrawColor(renderer, 255, 120, 60, 220);
    SDL_RenderFillRect(renderer, &gizmo);
}

void EditorMenu::RenderAssetsPanel(SDL_Renderer* renderer, const SDL_Rect& rect) {
    RenderPanelChrome(renderer, rect, 22, 22, 30);

    int rowHeight = 28;
    int rows = std::max(0, (rect.h - 12) / rowHeight);

    for (int i = 0; i < rows; i++) {
        SDL_Rect row = { rect.x + 6, rect.y + 6 + i * rowHeight, rect.w - 12, rowHeight - 4 };
        SDL_SetRenderDrawColor(renderer, 34, 34, 46, 255);
        SDL_RenderFillRect(renderer, &row);
    }
}

void EditorMenu::RenderPropertiesPanel(SDL_Renderer* renderer, const SDL_Rect& rect) {
    RenderPanelChrome(renderer, rect, 22, 22, 30);

    int fieldHeight = 24;
    int fieldSpacing = 10;
    int fieldCount = std::max(0, (rect.h - fieldSpacing) / (fieldHeight + fieldSpacing));

    for (int i = 0; i < fieldCount; i++) {
        SDL_Rect field = { rect.x + 8, rect.y + fieldSpacing + i * (fieldHeight + fieldSpacing), rect.w - 16, fieldHeight };
        SDL_SetRenderDrawColor(renderer, 40, 40, 54, 255);
        SDL_RenderFillRect(renderer, &field);
    }
}

void EditorMenu::RenderConsolePanel(SDL_Renderer* renderer, const SDL_Rect& rect) {
    RenderPanelChrome(renderer, rect, 10, 10, 14);
}

void EditorMenu::Render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 12, 12, 16, 255);
    SDL_RenderClear(renderer);

    if (toolbarButtons.empty()) {
        RebuildLayout();
    }

    SDL_SetRenderDrawColor(renderer, 24, 24, 32, 255);
    SDL_RenderFillRect(renderer, &toolbarRect);

    for (size_t i = 0; i < toolbarButtons.size(); i++) {
        const SDL_Rect& rect = toolbarButtons[i].rect;
        bool pressed = static_cast<int>(i) == pressedToolbarIndex;

        SDL_SetRenderDrawColor(renderer, pressed ? 255 : 46, pressed ? 130 : 46, pressed ? 70 : 60, 255);
        SDL_RenderFillRect(renderer, &rect);

        SDL_SetRenderDrawColor(renderer, 100, 100, 130, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }

    if (hasUnsavedChanges) {
        float pulse = (std::sin(unsavedPulseTimer * 5.0f) + 1.0f) / 2.0f;
        Uint8 alpha = static_cast<Uint8>(150 + pulse * 100);
        SDL_Rect dot = { toolbarRect.x + toolbarRect.w - 20, toolbarRect.y + toolbarRect.h / 2 - 4, 8, 8 };
        SDL_SetRenderDrawColor(renderer, 255, 200, 60, alpha);
        SDL_RenderFillRect(renderer, &dot);
    }

    if (ScreenUtil::IsMobile()) {
        switch (activeMobilePanel) {
            case EditorPanel::Scene: RenderScenePanel(renderer, scenePanelRect); break;
            case EditorPanel::Assets: RenderAssetsPanel(renderer, assetsPanelRect); break;
            case EditorPanel::Properties: RenderPropertiesPanel(renderer, propertiesPanelRect); break;
            case EditorPanel::Console: RenderConsolePanel(renderer, consolePanelRect); break;
        }

        SDL_SetRenderDrawColor(renderer, 20, 20, 28, 255);
        SDL_RenderFillRect(renderer, &tabBarRect);

        for (size_t i = 0; i < tabButtons.size(); i++) {
            const SDL_Rect& rect = tabButtons[i].rect;
            bool active = tabButtons[i].panel == activeMobilePanel;
            bool pressed = static_cast<int>(i) == pressedTabIndex;

            if (active || pressed) {
                SDL_SetRenderDrawColor(renderer, 255, 120, 60, pressed ? 255 : 180);
            } else {
                SDL_SetRenderDrawColor(renderer, 40, 40, 54, 255);
            }
            SDL_RenderFillRect(renderer, &rect);

            SDL_SetRenderDrawColor(renderer, 70, 70, 90, 255);
            SDL_RenderDrawRect(renderer, &rect);
        }
    } else {
        RenderAssetsPanel(renderer, assetsPanelRect);
        RenderScenePanel(renderer, scenePanelRect);
        RenderConsolePanel(renderer, consolePanelRect);
        RenderPropertiesPanel(renderer, propertiesPanelRect);
    }

    Guard::Render(renderer);
}
