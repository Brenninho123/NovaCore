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
#include <cstring>

#include "novacore/Assets.h"
#include "novacore/backend/Controls.h"
#include "novacore/backend/ControlsManager.h"
#include "novacore/shaders/ShaderManager.h"
#include "novacore/mobile/ScreenUtil.h"
#include "novacore/ui/TextRenderer.h"
#include "novacore/api/API.h"
#include "novacore/states/State.h"
#include "novacore/states/MenuState.h"
#include "novacore/ui/editor/EditorMenu.h"

#if defined(NOVACORE_DISCORD_ENABLED)
#include "novacore/api/discord/DiscordLogin.h"
#endif

#include "../source/nc/NCScript.h"

struct EngineConfig {
    std::string title = "NovaCore";
    int width = 1280;
    int height = 720;
    float fixedTimestep = 1.0f / 60.0f;
    int maxUpdatesPerFrame = 5;
};

static NCValue NCNative_Log(NCValue* args, int argCount) {
    if (argCount > 0 && args[0].type == NC_VALUE_STRING) {
        NovaAPI_LogInfo(args[0].string);
    }
    NCValue result;
    result.type = NC_VALUE_NULL;
    result.number = 0;
    result.boolean = 0;
    result.string = nullptr;
    return result;
}

static NCValue NCNative_KeyDown(NCValue* args, int argCount) {
    NCValue result;
    result.type = NC_VALUE_BOOL;
    result.number = 0;
    result.string = nullptr;
    result.boolean = 0;

    if (argCount > 0 && args[0].type == NC_VALUE_NUMBER) {
        result.boolean = NovaAPI_KeyDown(static_cast<int>(args[0].number));
    }

    return result;
}

static NCValue NCNative_DrawText(NCValue* args, int argCount) {
    if (argCount >= 4 && args[0].type == NC_VALUE_STRING && args[1].type == NC_VALUE_STRING) {
        int x = args[2].type == NC_VALUE_NUMBER ? static_cast<int>(args[2].number) : 0;
        int y = args[3].type == NC_VALUE_NUMBER ? static_cast<int>(args[3].number) : 0;
        NovaAPI_DrawText(args[0].string, args[1].string, x, y, 255, 255, 255, 255, 0);
    }

    NCValue result;
    result.type = NC_VALUE_NULL;
    result.number = 0;
    result.boolean = 0;
    result.string = nullptr;
    return result;
}

static NCValue NCNative_ScreenWidth(NCValue* args, int argCount) {
    (void)args;
    (void)argCount;

    NCValue result;
    result.type = NC_VALUE_NUMBER;
    result.number = NovaAPI_GetScreenWidth();
    result.boolean = 0;
    result.string = nullptr;
    return result;
}

static NCValue NCNative_ScreenHeight(NCValue* args, int argCount) {
    (void)args;
    (void)argCount;

    NCValue result;
    result.type = NC_VALUE_NUMBER;
    result.number = NovaAPI_GetScreenHeight();
    result.boolean = 0;
    result.string = nullptr;
    return result;
}

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

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0) {
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
        ControlsManager_Init();
        ShaderManager::Init();

        if (!TextRenderer::Init()) {
            SDL_Log("TextRenderer::Init failed");
        }

        if (!TextRenderer::LoadFont("default", "assets/fonts/default.ttf", 18)) {
            SDL_Log("Failed to load default font, text will not render");
        }

        if (!TextRenderer::LoadFont("small", "assets/fonts/default.ttf", 13)) {
            SDL_Log("Failed to load small font, text will not render");
        }

        NovaAPI_Init(renderer);

        NCScript_Init();
        NCScript_RegisterNative("log", NCNative_Log);
        NCScript_RegisterNative("keyDown", NCNative_KeyDown);
        NCScript_RegisterNative("drawText", NCNative_DrawText);
        NCScript_RegisterNative("screenWidth", NCNative_ScreenWidth);
        NCScript_RegisterNative("screenHeight", NCNative_ScreenHeight);

#if defined(NOVACORE_DISCORD_ENABLED)
        DiscordLogin::Init("1540653184530251847");
#endif

        stateManager.Push(std::make_unique<MenuState>());

        auto* menu = dynamic_cast<MenuState*>(stateManager.Top());
        if (menu) {
            menu->SetOnAction([this](EditorAction action) {
                OnEditorAction(action);
            });

            menu->SetOnOpenRecentProject([this](int index) {
                OnOpenRecentProject(index);
            });

            menu->SetRecentProjects({});
        }

        running = true;
        return true;
    }

    void Run() {
        Uint32 lastTime = SDL_GetTicks();
        float accumulator = 0.0f;
        float cacheTrimTimer = 0.0f;

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
                    ControlsManager_Update();
                    accumulator -= config.fixedTimestep;
                    updates++;
                }
            }

            cacheTrimTimer += frameTime;
            if (cacheTrimTimer >= 2.0f) {
                TextRenderer::TrimTextureCache(256);
                cacheTrimTimer = 0.0f;
            }

            Render();

            if (stateManager.IsEmpty()) {
                running = false;
            }
        }
    }

    void Shutdown() {
        stateManager.Clear();

        NCScript_Shutdown();
        NovaAPI_Shutdown();

#if defined(NOVACORE_DISCORD_ENABLED)
        DiscordLogin::Shutdown();
#endif

        TextRenderer::Shutdown();
        ShaderManager::Shutdown();
        ControlsManager_Shutdown();
        Assets::Shutdown();

        if (glContext) SDL_GL_DeleteContext(glContext);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
    }

private:
    void OpenEditorWorkspace(const std::string& name) {
        auto editor = std::make_unique<EditorMenu>();
        editor->SetProjectName(name);

        editor->SetOnExit([this]() {
            stateManager.Pop();
        });

        editor->SetOnSave([]() {
            SDL_Log("Project saved");
        });

        stateManager.Push(std::move(editor));
    }

    void OnEditorAction(EditorAction action) {
        switch (action) {
            case EditorAction::NewProject:
                OpenEditorWorkspace("Untitled Project");
                break;
            case EditorAction::OpenProject:
#if defined(NOVACORE_DISCORD_ENABLED)
                DiscordLogin::StartLogin({ "identify" });
#endif
                SDL_Log("Open Project selected");
                break;
            case EditorAction::Settings:
                SDL_Log("Settings selected");
                break;
            case EditorAction::Exit:
                running = false;
                break;
        }
    }

    void OnOpenRecentProject(int index) {
        SDL_Log("Opening recent project index %d", index);
        OpenEditorWorkspace("Recent Project " + std::to_string(index));
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

            ScreenUtil::HandleEvent(event);
            ControlsManager_HandleEvent(&event);
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
