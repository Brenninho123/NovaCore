#include <SDL2/SDL.h>
#include <iostream>
#include <memory>

#include "novacore/Assets.h"
#include "novacore/backend/Controls.h"
#include "novacore/states/State.h"
#include "novacore/states/MenuState.h"

class Engine {
public:
    bool Init(const char* title, int width, int height) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
            std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
            return false;
        }

        window = SDL_CreateWindow(
            title,
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            width,
            height,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
        );

        if (!window) {
            std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
            return false;
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

        if (!renderer) {
            std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
            return false;
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        Assets::Init(renderer);
        Controls::Init();

        auto menu = std::make_unique<MenuState>();
        menu->SetOnSelect([this](int index) {
            OnMenuSelect(index);
        });

        currentState = std::move(menu);
        currentState->Enter();

        running = true;
        return true;
    }

    void Run() {
        Uint32 lastTime = SDL_GetTicks();

        while (running) {
            Uint32 currentTime = SDL_GetTicks();
            float deltaTime = (currentTime - lastTime) / 1000.0f;
            lastTime = currentTime;

            if (deltaTime > 0.1f) {
                deltaTime = 0.1f;
            }

            HandleEvents();
            Update(deltaTime);
            Render();
        }
    }

    void Shutdown() {
        if (currentState) {
            currentState->Exit();
            currentState.reset();
        }

        Assets::Shutdown();

        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
    }

private:
    void OnMenuSelect(int index) {
        switch (index) {
            case 0:
                std::cout << "Play selected" << std::endl;
                break;
            case 1:
                std::cout << "Options selected" << std::endl;
                break;
            case 2:
                std::cout << "Credits selected" << std::endl;
                break;
            case 3:
                running = false;
                break;
            default:
                break;
        }
    }

    void HandleEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            Controls::HandleEvent(event);
        }
    }

    void Update(float deltaTime) {
        if (currentState) {
            currentState->Update(deltaTime);
        }

        Controls::Update();
    }

    void Render() {
        if (currentState) {
            currentState->Render(renderer);
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool running = false;

    std::unique_ptr<State> currentState;
};

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    Engine engine;

    if (!engine.Init("NovaCore", 1280, 720)) {
        return 1;
    }

    engine.Run();
    engine.Shutdown();

    return 0;
}
