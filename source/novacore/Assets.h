#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <unordered_map>

class Assets {
public:
    static void Init(SDL_Renderer* renderer);
    static void Shutdown();

    static SDL_Texture* LoadTexture(const std::string& path);
    static void UnloadTexture(const std::string& path);
    static void ClearAll();

private:
    static SDL_Renderer* renderer;
    static std::unordered_map<std::string, SDL_Texture*> textureCache;
};
