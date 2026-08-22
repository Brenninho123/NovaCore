#include <SDL2/SDL.h>

#if defined(NOVACORE_ANDROID)
#include <GLES2/gl2.h>
#else
#include <glad/glad.h>
#endif

#include <iostream>
#include <memory>

#include "novacore/Assets.h"
#include "novacore/backend/Controls.h"
#include "novacore/shaders/ShaderManager.h"
#include "novacore/states/State.h"
#include "novacore/states/MenuState.h"

class Engine {
public:
    bool Init(const char* title, int width, int height) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
            std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
            return false;
        }

#if defined(NOVACORE_ANDROID)
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

#if defined(NOVACORE_ANDROID)
        Uint32 windowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN;
#else
        Uint32 windowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;
#endif

        window = SDL_CreateWindow(
            title,
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            width,
            height,
            windowFlags
        );

        if (!window) {
            std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
            return false;
        }

        glContext = SDL_GL_CreateContext(window);
        if (!glContext) {
            std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << std::endl;
            return false;
        }

#if !defined(NOVACORE_ANDROID)
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
            std::cerr << "gladLoadGLLoader failed" << std::endl;
            return false;
        }
#endif

        SDL_GL_SetSwapInterval(1);

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

        if (!renderer) {
            std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
            return false;
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        int drawableWidth = width;
        int drawableHeight = height;
        SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);
        glViewport(0, 0, drawableWidth, drawableHeight);

        Assets::Init(renderer);
        Controls::Init();
        ShaderManager::Init();

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

        ShaderManager::Shutdown();
        Assets::Shutdown();

        if (glContext) SDL_GL_DeleteContext(glContext);
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

            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
                int drawableWidth = event.window.data1;
                int drawableHeight = event.window.data2;
                SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);
                glViewport(0, 0, drawableWidth, drawableHeight);
            }

            if (currentState) {
                currentState->HandleEvent(event);
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
    SDL_GLContext glContext = nullptr;
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
