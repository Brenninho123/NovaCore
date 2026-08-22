#ifndef NOVACORE_CONTROLS_MANAGER_H
#define NOVACORE_CONTROLS_MANAGER_H

#include <SDL2/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NOVACORE_MAX_CONTROLLERS 4

void ControlsManager_Init(void);
void ControlsManager_Shutdown(void);

void ControlsManager_HandleEvent(const SDL_Event* event);
void ControlsManager_Update(void);

int ControlsManager_GetControllerCount(void);
int ControlsManager_IsControllerConnected(int controllerIndex);
const char* ControlsManager_GetControllerName(int controllerIndex);

int ControlsManager_IsButtonDown(int controllerIndex, SDL_GameControllerButton button);
int ControlsManager_IsButtonJustPressed(int controllerIndex, SDL_GameControllerButton button);
int ControlsManager_IsButtonJustReleased(int controllerIndex, SDL_GameControllerButton button);

float ControlsManager_GetAxis(int controllerIndex, SDL_GameControllerAxis axis);
float ControlsManager_GetAxisDeadzoned(int controllerIndex, SDL_GameControllerAxis axis, float deadzone);

void ControlsManager_Rumble(int controllerIndex, float lowFrequency, float highFrequency, int durationMs);
void ControlsManager_StopRumble(int controllerIndex);

#ifdef __cplusplus
}
#endif

#endif
