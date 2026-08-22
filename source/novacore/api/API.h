#pragma once

#include <SDL2/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

void NovaAPI_Init(SDL_Renderer* renderer);
void NovaAPI_Shutdown();

int NovaAPI_LoadTexture(const char* path);
void NovaAPI_UnloadTexture(const char* path);

int NovaAPI_LoadSound(const char* path);
int NovaAPI_LoadMusic(const char* path);

int NovaAPI_KeyDown(int scancode);
int NovaAPI_KeyJustPressed(int scancode);
int NovaAPI_KeyJustReleased(int scancode);

int NovaAPI_GetControllerCount();
int NovaAPI_ControllerButtonDown(int controllerIndex, int button);
float NovaAPI_ControllerAxis(int controllerIndex, int axis);
void NovaAPI_ControllerRumble(int controllerIndex, float lowFreq, float highFreq, int durationMs);

int NovaAPI_LoadFont(const char* name, const char* path, int size);
void NovaAPI_DrawText(const char* fontName, const char* text, int x, int y, int r, int g, int b, int a, int align);
void NovaAPI_MeasureText(const char* fontName, const char* text, int* outWidth, int* outHeight);

int NovaAPI_LoadShader(const char* name, const char* vertexPath, const char* fragmentPath);
void NovaAPI_UseShader(const char* name);
void NovaAPI_UnuseShader();
void NovaAPI_SetShaderFloat(const char* shaderName, const char* uniform, float value);
void NovaAPI_SetShaderInt(const char* shaderName, const char* uniform, int value);

float NovaAPI_GetScreenWidth();
float NovaAPI_GetScreenHeight();
int NovaAPI_IsMobile();

void NovaAPI_LogInfo(const char* message);
void NovaAPI_LogWarn(const char* message);
void NovaAPI_LogError(const char* message);

#ifdef __cplusplus
}
#endif
