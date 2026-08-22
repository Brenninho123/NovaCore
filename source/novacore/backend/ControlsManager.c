#include "ControlsManager.h"
#include <string.h>

typedef struct {
    SDL_GameController* handle;
    SDL_JoystickID instanceId;
    Uint32 currentButtons;
    Uint32 previousButtons;
    float axisValues[SDL_CONTROLLER_AXIS_MAX];
    int connected;
} ControllerSlot;

static ControllerSlot slots[NOVACORE_MAX_CONTROLLERS];

static int FindSlotByInstanceId(SDL_JoystickID instanceId) {
    int i;
    for (i = 0; i < NOVACORE_MAX_CONTROLLERS; i++) {
        if (slots[i].connected && slots[i].instanceId == instanceId) {
            return i;
        }
    }
    return -1;
}

static int FindFreeSlot(void) {
    int i;
    for (i = 0; i < NOVACORE_MAX_CONTROLLERS; i++) {
        if (!slots[i].connected) {
            return i;
        }
    }
    return -1;
}

void ControlsManager_Init(void) {
    memset(slots, 0, sizeof(slots));

    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0) {
        SDL_Log("SDL_InitSubSystem(GAMECONTROLLER) failed: %s", SDL_GetError());
        return;
    }

    int joystickCount = SDL_NumJoysticks();
    int i;

    for (i = 0; i < joystickCount; i++) {
        if (SDL_IsGameController(i)) {
            int slot = FindFreeSlot();
            if (slot == -1) {
                break;
            }

            SDL_GameController* controller = SDL_GameControllerOpen(i);
            if (controller) {
                SDL_Joystick* joystick = SDL_GameControllerGetJoystick(controller);

                slots[slot].handle = controller;
                slots[slot].instanceId = SDL_JoystickInstanceID(joystick);
                slots[slot].connected = 1;
                slots[slot].currentButtons = 0;
                slots[slot].previousButtons = 0;
            }
        }
    }
}

void ControlsManager_Shutdown(void) {
    int i;
    for (i = 0; i < NOVACORE_MAX_CONTROLLERS; i++) {
        if (slots[i].connected && slots[i].handle) {
            SDL_GameControllerClose(slots[i].handle);
        }
    }

    memset(slots, 0, sizeof(slots));
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}

void ControlsManager_HandleEvent(const SDL_Event* event) {
    if (!event) {
        return;
    }

    if (event->type == SDL_CONTROLLERDEVICEADDED) {
        int slot = FindFreeSlot();
        if (slot != -1) {
            SDL_GameController* controller = SDL_GameControllerOpen(event->cdevice.which);
            if (controller) {
                SDL_Joystick* joystick = SDL_GameControllerGetJoystick(controller);

                slots[slot].handle = controller;
                slots[slot].instanceId = SDL_JoystickInstanceID(joystick);
                slots[slot].connected = 1;
                slots[slot].currentButtons = 0;
                slots[slot].previousButtons = 0;

                SDL_Log("Controller connected: %s", SDL_GameControllerName(controller));
            }
        }
        return;
    }

    if (event->type == SDL_CONTROLLERDEVICEREMOVED) {
        int slot = FindSlotByInstanceId(event->cdevice.which);
        if (slot != -1) {
            SDL_Log("Controller disconnected: %s", SDL_GameControllerName(slots[slot].handle));
            SDL_GameControllerClose(slots[slot].handle);
            memset(&slots[slot], 0, sizeof(ControllerSlot));
        }
        return;
    }

    if (event->type == SDL_CONTROLLERBUTTONDOWN || event->type == SDL_CONTROLLERBUTTONUP) {
        int slot = FindSlotByInstanceId(event->cbutton.which);
        if (slot != -1) {
            Uint32 mask = (Uint32)1 << event->cbutton.button;

            if (event->type == SDL_CONTROLLERBUTTONDOWN) {
                slots[slot].currentButtons |= mask;
            } else {
                slots[slot].currentButtons &= ~mask;
            }
        }
        return;
    }

    if (event->type == SDL_CONTROLLERAXISMOTION) {
        int slot = FindSlotByInstanceId(event->caxis.which);
        if (slot != -1 && event->caxis.axis < SDL_CONTROLLER_AXIS_MAX) {
            slots[slot].axisValues[event->caxis.axis] = event->caxis.value / 32767.0f;
        }
        return;
    }
}

void ControlsManager_Update(void) {
    int i;
    for (i = 0; i < NOVACORE_MAX_CONTROLLERS; i++) {
        slots[i].previousButtons = slots[i].currentButtons;
    }
}

int ControlsManager_GetControllerCount(void) {
    int count = 0;
    int i;
    for (i = 0; i < NOVACORE_MAX_CONTROLLERS; i++) {
        if (slots[i].connected) {
            count++;
        }
    }
    return count;
}

int ControlsManager_IsControllerConnected(int controllerIndex) {
    if (controllerIndex < 0 || controllerIndex >= NOVACORE_MAX_CONTROLLERS) {
        return 0;
    }
    return slots[controllerIndex].connected;
}

const char* ControlsManager_GetControllerName(int controllerIndex) {
    if (controllerIndex < 0 || controllerIndex >= NOVACORE_MAX_CONTROLLERS || !slots[controllerIndex].connected) {
        return "";
    }
    const char* name = SDL_GameControllerName(slots[controllerIndex].handle);
    return name ? name : "Unknown Controller";
}

int ControlsManager_IsButtonDown(int controllerIndex, SDL_GameControllerButton button) {
    if (controllerIndex < 0 || controllerIndex >= NOVACORE_MAX_CONTROLLERS || !slots[controllerIndex].connected) {
        return 0;
    }
    Uint32 mask = (Uint32)1 << button;
    return (slots[controllerIndex].currentButtons & mask) != 0;
}

int ControlsManager_IsButtonJustPressed(int controllerIndex, SDL_GameControllerButton button) {
    if (controllerIndex < 0 || controllerIndex >= NOVACORE_MAX_CONTROLLERS || !slots[controllerIndex].connected) {
        return 0;
    }
    Uint32 mask = (Uint32)1 << button;
    int current = (slots[controllerIndex].currentButtons & mask) != 0;
    int previous = (slots[controllerIndex].previousButtons & mask) != 0;
    return current && !previous;
}

int ControlsManager_IsButtonJustReleased(int controllerIndex, SDL_GameControllerButton button) {
    if (controllerIndex < 0 || controllerIndex >= NOVACORE_MAX_CONTROLLERS || !slots[controllerIndex].connected) {
        return 0;
    }
    Uint32 mask = (Uint32)1 << button;
    int current = (slots[controllerIndex].currentButtons & mask) != 0;
    int previous = (slots[controllerIndex].previousButtons & mask) != 0;
    return !current && previous;
}

float ControlsManager_GetAxis(int controllerIndex, SDL_GameControllerAxis axis) {
    if (controllerIndex < 0 || controllerIndex >= NOVACORE_MAX_CONTROLLERS || !slots[controllerIndex].connected) {
        return 0.0f;
    }
    if (axis < 0 || axis >= SDL_CONTROLLER_AXIS_MAX) {
        return 0.0f;
    }
    return slots[controllerIndex].axisValues[axis];
}

float ControlsManager_GetAxisDeadzoned(int controllerIndex, SDL_GameControllerAxis axis, float deadzone) {
    float value = ControlsManager_GetAxis(controllerIndex, axis);

    if (value > -deadzone && value < deadzone) {
        return 0.0f;
    }

    return value;
}

void ControlsManager_Rumble(int controllerIndex, float lowFrequency, float highFrequency, int durationMs) {
    if (controllerIndex < 0 || controllerIndex >= NOVACORE_MAX_CONTROLLERS || !slots[controllerIndex].connected) {
        return;
    }

    Uint16 low = (Uint16)(lowFrequency * 65535.0f);
    Uint16 high = (Uint16)(highFrequency * 65535.0f);

    SDL_GameControllerRumble(slots[controllerIndex].handle, low, high, (Uint32)durationMs);
}

void ControlsManager_StopRumble(int controllerIndex) {
    ControlsManager_Rumble(controllerIndex, 0.0f, 0.0f, 0);
}
