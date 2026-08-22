#include "MenuState.h"
#include "../backend/Controls.h"
#include "../mobile/ScreenUtil.h"
#include <cmath>

void MenuState::Enter() {
    options = { "Play", "Options", "Credits", "Editor", "Quit" };
    selectedIndex = 0;
    inputCooldown = 0.0f;
    pulseTimer = 0.0f;
    activePointerId = -1;
    pressedIndex = -1;
    layoutWidth = 0;
    layoutHeight = 0;
}

void MenuState::Exit() {
    options.clear();
    buttons.clear();
    onSelect = nullptr;
}

void MenuState::SetOnSelect(std::function<void(int)> callback) {
    onSelect = std::move(callback);
}

void MenuState::SetEditorMode(bool enabled) {
    editorMode = enabled;
}

bool MenuState::IsEditorMode() const {
    return editorMode;
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

    int boxWidth = static_cast<int>((ScreenUtil::IsMobile() ? 320 : 240) * scale);
    int boxHeight = static_cast<int>((ScreenUtil::IsMobile() ? 76 : 44) * scale);
    int spacing = static_cast<int>((ScreenUtil::IsMobile() ? 96 : 60) * scale);

    int usableTop = safeArea.top;
    int usableBottom = viewportHeight - safeArea.bottom;
    int usableHeight = usableBottom - usableTop;

    int itemCount = static_cast<int>(options.size());
    int totalHeight = itemCount * spacing;
    int startY = usableTop + (usableHeight - totalHeight) / 2;

    buttons.clear();
    buttons.reserve(options.size());

    for (int i = 0; i < itemCount; i++) {
        MenuButton button;
        button.label = options[i];
        button.rect.x = (viewportWidth - boxWidth) / 2;
        button.rect.y = startY + i * spacing;
        button.rect.w = boxWidth;
        button.rect.h = boxHeight;
        buttons.push_back(button);
    }
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

int MenuState::HitTest(int x, int y) const {
    for (size_t i = 0; i < buttons.size(); i++) {
        const SDL_Rect& rect = buttons[i].rect;
        if (x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void MenuState::BeginPress(int x, int y) {
    pressedIndex = HitTest(x, y);
    if (pressedIndex >= 0) {
        selectedIndex = pressedIndex;
    }
}

void MenuState::EndPress(int x, int y) {
    int releasedIndex = HitTest(x, y);

    if (releasedIndex >= 0 && releasedIndex == pressedIndex) {
        selectedIndex = releasedIndex;
        ConfirmSelection();
    }

    pressedIndex = -1;
    activePointerId = -1;
}

void MenuState::CancelPress() {
    pressedIndex = -1;
    activePointerId = -1;
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
        if (activePointerId == static_cast<int>(event.tfinger.fingerId)) {
            int x = static_cast<int>(event.tfinger.x * layoutWidth);
            int y = static_cast<int>(event.tfinger.y * layoutHeight);
            if (HitTest(x, y) != pressedIndex) {
                CancelPress();
            }
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

        if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_F1) {
            editorMode = !editorMode;
        }
    }
}

void MenuState::Update(float deltaTime) {
    if (layoutWidth != ScreenUtil::GetWidth() || layoutHeight != ScreenUtil::GetHeight()) {
        RebuildLayout();
    }

    pulseTimer += deltaTime;

    if (inputCooldown > 0.0f) {
        inputCooldown -= deltaTime;
    }

    if (!ScreenUtil::IsMobile()) {
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
}

void MenuState::Render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 18, 18, 26, 255);
    SDL_RenderClear(renderer);

    if (buttons.empty()) {
        RebuildLayout();
    }

    for (size_t i = 0; i < buttons.size(); i++) {
        const SDL_Rect& rect = buttons[i].rect;
        bool isSelected = static_cast<int>(i) == selectedIndex;
        bool isPressed = static_cast<int>(i) == pressedIndex;

        if (isSelected) {
            float pulse = (std::sin(pulseTimer * 6.0f) + 1.0f) / 2.0f;
            Uint8 alpha = static_cast<Uint8>((isPressed ? 200 : 120) + pulse * 80);

            SDL_SetRenderDrawColor(renderer, 255, 80, 140, alpha);
            SDL_RenderFillRect(renderer, &rect);

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &rect);
        } else {
            SDL_SetRenderDrawColor(renderer, 40, 40, 55, 255);
            SDL_RenderFillRect(renderer, &rect);

            SDL_SetRenderDrawColor(renderer, 90, 90, 110, 255);
            SDL_RenderDrawRect(renderer, &rect);
        }
    }

    if (editorMode) {
        SDL_SetRenderDrawColor(renderer, 255, 210, 0, 255);
        SDL_Rect badge = { 12, 12, 110, 28 };
        SDL_RenderFillRect(renderer, &badge);
    }
}
