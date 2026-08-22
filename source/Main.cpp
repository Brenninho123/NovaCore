#include <SDL2/SDL.h>

#if defined(NOVACORE_ANDROID)
#include <GLES2/gl2.h>
#else
#include <glad/glad.h>
#endif

#include <SDL2/SDL_image.h>
#include <memory>
#include <vector>
#include <string>

#include "novacore/Assets.h"
#include "novacore/backend/Controls.h"
#include "novacore/shaders/ShaderManager.h"
#include "novacore/mobile/ScreenUtil.h"
#include "novacore/states/State.h"
#include "novacore/states/MenuState.h"

#if defined(NOVACORE_DISCORD_ENABLED)
#include "novacore/api/discord/DiscordLogin.h"
#endif

struct EngineConfig {
    std::string title = "NovaCore";
    int width = 1280;
    int height = 720;
    float fixedTimestep = 1.0f / 60.0f;
    int maxUpdatesPerFrame = 5;
};

class StateManager {
public:
    void Push(std::unique_ptr<State> state) {
        if (!stack.empty()) {
            stack.back()->Pause();
        }

        state->Enter();
        stack.push_back(std::move(state));
    }

    void Pop() {
        if (stack.empty()) {
            return;
        }

        stack.back()->Exit();
        stack.pop_back();

        if (!stack.empty()) {
            stack.back()->Resume();
        }
    }

    void Replace(std::unique_ptr<State> state) {
        while (!stack.empty()) {
            stack.back()->Exit();
            stack.pop_back();
        }

        state->Enter();
        stack.push_back(std::move(state));
    }

    void Clear() {
        while (!stack.empty()) {
            stack.back()->Exit();
            stack.pop_back();
        }
    }

    void HandleEvent(const SDL_Event& event) {
        if (!stack.empty()) {
            stack.back()->HandleEvent(event);
        }
    }

    void Update(float deltaTime) {
        for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
            (*it)->Update(deltaTime);

            if ((*it)->BlocksUpdate()) {
                break;
            }
        }
    }

    void Render(SDL_Renderer* renderer) {
        size_t startIndex = stack.size();

        while (startIndex > 0) {
            startIndex--;
            if (!stack[startIndex]->IsTransparent()) {
                break;
            }
        }

        for (size_t i = startIndex; i < stack.size(); i++) {
            stack[i]->Render(renderer);
        }
    }

    State* Top() {
        return stack.empty() ? nullptr : stack.back().get();
    }

    bool IsEmpty() const {
        return stack.empty();
    }

private:
    std::vector<std::unique_ptr<State>> stack;
};

class Engine {
public:
    bool Init(const EngineConfig& cfg) {
        config = cfg;

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
            SDL_Log("SDL_Init failed: %s", SDL_GetError());
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
            config.title.c_str(),
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            config.width,
            config.height,
            windowFlags
        );

        if (!window) {
            SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
            return false;
        }

        glContext = SDL_GL_CreateContext(window);
        if (!glContext) {
            SDL_Log("SDL_GL_CreateContext failed: %s", SDL_GetError());
            return false;
        }

#if !defined(NOVACORE_ANDROID)
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
            SDL_Log("gladLoadGLLoader failed");
            return false;
        }
#endif

        SDL_GL_SetSwapInterval(1);

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

        if (!renderer) {
            SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
            return false;
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

#if !defined(NOVACORE_ANDROID)
        SDL_Surface* iconSurface = IMG_Load("arts/icon.png");
        if (iconSurface) {
            SDL_SetWindowIcon(window, iconSurface);
            SDL_FreeSurface(iconSurface);
        }
#endif

        ScreenUtil::Init(window);
        RefreshViewport();

        Assets::Init(renderer);
        Controls::Init();
        ShaderManager::Init();

#if defined(NOVACORE_DISCORD_ENABLED)
        DiscordLogin::Init("1540653184530251847");
#endif

        stateManager.Push(std::make_unique<MenuState>());

        auto* menu = dynamic_cast<MenuState*>(stateManager.Top());
        if (menu) {
            menu->SetOnSelect([this](int index) {
                OnMenuSelect(index);
            });
        }

        running = true;
        return true;
    }

    void Run() {
        Uint32 lastTime = SDL_GetTicks();
        float accumulator = 0.0f;

        while (running) {
            Uint32 currentTime = SDL_GetTicks();
            float frameTime = (currentTime - lastTime) / 1000.0f;
            lastTime = currentTime;

            if (frameTime > 0.25f) {
                frameTime = 0.25f;
            }

            HandleEvents();

            if (!paused) {
                accumulator += frameTime;

                int updates = 0;
                while (accumulator >= config.fixedTimestep && updates < config.maxUpdatesPerFrame) {
                    stateManager.Update(config.fixedTimestep);
                    Controls::Update();
                    accumulator -= config.fixedTimestep;
                    updates++;
                }
            }

            Render();

            if (stateManager.IsEmpty()) {
                running = false;
            }
        }
    }

    void Shutdown() {
        stateManager.Clear();

#if defined(NOVACORE_DISCORD_ENABLED)
        DiscordLogin::Shutdown();
#endif

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
                SDL_Log("Play selected");
                break;
            case 1:
#if defined(NOVACORE_DISCORD_ENABLED)
                DiscordLogin::StartLogin({ "identify" });
#endif
                SDL_Log("Options selected");
                break;
            case 2:
                SDL_Log("Credits selected");
                break;
            case 3:
                running = false;
                break;
            default:
                break;
        }
    }

    void RefreshViewport() {
        int drawableWidth = config.width;
        int drawableHeight = config.height;
        SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);
        glViewport(0, 0, drawableWidth, drawableHeight);
        ScreenUtil::Refresh();
    }

    void HandleEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;

                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        RefreshViewport();
                    }
                    break;

                case SDL_APP_WILLENTERBACKGROUND:
                    paused = true;
                    break;

                case SDL_APP_DIDENTERFOREGROUND:
                    paused = false;
                    break;

                default:
                    break;
            }

            stateManager.HandleEvent(event);
            Controls::HandleEvent(event);
        }
    }

    void Render() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        stateManager.Render(renderer);

        SDL_RenderPresent(renderer);
    }

    EngineConfig config;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_GLContext glContext = nullptr;

    bool running = false;
    bool paused = false;

    StateManager stateManager;
};

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    EngineConfig config;
    config.title = "NovaCore";
    config.width = 1280;
    config.height = 720;
    config.fixedTimestep = 1.0f / 60.0f;

    Engine engine;

    if (!engine.Init(config)) {
        return 1;
    }

    engine.Run();
    engine.Shutdown();

    return 0;
}
