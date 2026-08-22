#include "API.h"

#include "../Assets.h"
#include "../backend/Controls.h"
#include "../backend/ControlsManager.h"
#include "../mobile/ScreenUtil.h"
#include "../shaders/ShaderManager.h"

#if defined(NOVACORE_TTF_ENABLED)
#include "../ui/TextRenderer.h"
#endif

static SDL_Renderer* g_renderer = nullptr;

void NovaAPI_Init(SDL_Renderer* renderer) {
    g_renderer = renderer;
}

void NovaAPI_Shutdown() {
    g_renderer = nullptr;
}

int NovaAPI_LoadTexture(const char* path) {
    if (!path) {
        return 0;
    }
    return Assets::LoadTexture(path) != nullptr ? 1 : 0;
}

void NovaAPI_UnloadTexture(const char* path) {
    if (!path) {
        return;
    }
    Assets::UnloadTexture(path);
}

int NovaAPI_LoadSound(const char* path) {
    if (!path) {
        return 0;
    }
    return Assets::LoadSound(path) != nullptr ? 1 : 0;
}

int NovaAPI_LoadMusic(const char* path) {
    if (!path) {
        return 0;
    }
    return Assets::LoadMusic(path) != nullptr ? 1 : 0;
}

int NovaAPI_KeyDown(int scancode) {
    return Controls::IsDown(static_cast<SDL_Scancode>(scancode)) ? 1 : 0;
}

int NovaAPI_KeyJustPressed(int scancode) {
    return Controls::JustPressed(static_cast<SDL_Scancode>(scancode)) ? 1 : 0;
}

int NovaAPI_KeyJustReleased(int scancode) {
    return Controls::JustReleased(static_cast<SDL_Scancode>(scancode)) ? 1 : 0;
}

int NovaAPI_GetControllerCount() {
    return ControlsManager_GetControllerCount();
}

int NovaAPI_ControllerButtonDown(int controllerIndex, int button) {
    return ControlsManager_IsButtonDown(controllerIndex, static_cast<SDL_GameControllerButton>(button));
}

float NovaAPI_ControllerAxis(int controllerIndex, int axis) {
    return ControlsManager_GetAxisDeadzoned(controllerIndex, static_cast<SDL_GameControllerAxis>(axis), 0.15f);
}

void NovaAPI_ControllerRumble(int controllerIndex, float lowFreq, float highFreq, int durationMs) {
    ControlsManager_Rumble(controllerIndex, lowFreq, highFreq, durationMs);
}

int NovaAPI_LoadFont(const char* name, const char* path, int size) {
#if defined(NOVACORE_TTF_ENABLED)
    if (!name || !path) {
        return 0;
    }
    return TextRenderer::LoadFont(name, path, size) ? 1 : 0;
#else
    (void)name;
    (void)path;
    (void)size;
    return 0;
#endif
}

void NovaAPI_DrawText(const char* fontName, const char* text, int x, int y, int r, int g, int b, int a, int align) {
#if defined(NOVACORE_TTF_ENABLED)
    if (!fontName || !text || !g_renderer) {
        return;
    }

    SDL_Color color = {
        static_cast<Uint8>(r),
        static_cast<Uint8>(g),
        static_cast<Uint8>(b),
        static_cast<Uint8>(a)
    };

    TextAlign alignment = TextAlign::Left;
    if (align == 1) alignment = TextAlign::Center;
    else if (align == 2) alignment = TextAlign::Right;

    TextRenderer::DrawText(g_renderer, fontName, text, x, y, color, alignment);
#else
    (void)fontName;
    (void)text;
    (void)x;
    (void)y;
    (void)r;
    (void)g;
    (void)b;
    (void)a;
    (void)align;
#endif
}

void NovaAPI_MeasureText(const char* fontName, const char* text, int* outWidth, int* outHeight) {
#if defined(NOVACORE_TTF_ENABLED)
    if (!fontName || !text || !outWidth || !outHeight) {
        return;
    }
    TextRenderer::MeasureText(fontName, text, *outWidth, *outHeight);
#else
    if (outWidth) *outWidth = 0;
    if (outHeight) *outHeight = 0;
#endif
}

int NovaAPI_LoadShader(const char* name, const char* vertexPath, const char* fragmentPath) {
    if (!name || !vertexPath || !fragmentPath) {
        return 0;
    }
    return ShaderManager::LoadShader(name, vertexPath, fragmentPath) ? 1 : 0;
}

void NovaAPI_UseShader(const char* name) {
    if (!name) {
        return;
    }
    ShaderManager::Use(name);
}

void NovaAPI_UnuseShader() {
    ShaderManager::Unuse();
}

void NovaAPI_SetShaderFloat(const char* shaderName, const char* uniform, float value) {
    if (!shaderName || !uniform) {
        return;
    }
    ShaderManager::SetUniformFloat(shaderName, uniform, value);
}

void NovaAPI_SetShaderInt(const char* shaderName, const char* uniform, int value) {
    if (!shaderName || !uniform) {
        return;
    }
    ShaderManager::SetUniformInt(shaderName, uniform, value);
}

float NovaAPI_GetScreenWidth() {
    return static_cast<float>(ScreenUtil::GetWidth());
}

float NovaAPI_GetScreenHeight() {
    return static_cast<float>(ScreenUtil::GetHeight());
}

int NovaAPI_IsMobile() {
    return ScreenUtil::IsMobile() ? 1 : 0;
}

void NovaAPI_LogInfo(const char* message) {
    if (message) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s", message);
    }
}

void NovaAPI_LogWarn(const char* message) {
    if (message) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "%s", message);
    }
}

void NovaAPI_LogError(const char* message) {
    if (message) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", message);
    }
}
