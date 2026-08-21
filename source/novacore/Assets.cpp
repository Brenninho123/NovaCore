#include "Assets.h"
#include <SDL2/SDL_image.h>
#include <iostream>

SDL_Renderer* Assets::renderer = nullptr;
std::unordered_map<std::string, SDL_Texture*> Assets::textureCache;

void Assets::Init(SDL_Renderer* r) {
    renderer = r;

    int flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(flags) & flags) != flags) {
        std::cerr << "IMG_Init failed: " << IMG_GetError() << std::endl;
    }
}

void Assets::Shutdown() {
    ClearAll();
    IMG_Quit();
}

SDL_Texture* Assets::LoadTexture(const std::string& path) {
    auto it = textureCache.find(path);
    if (it != textureCache.end()) {
        return it->second;
    }

    SDL_Texture* texture = IMG_LoadTexture(renderer, path.c_str());
    if (!texture) {
        std::cerr << "Failed to load texture: " << path << " - " << IMG_GetError() << std::endl;
        return nullptr;
    }

    textureCache[path] = texture;
    return texture;
}

void Assets::UnloadTexture(const std::string& path) {
    auto it = textureCache.find(path);
    if (it != textureCache.end()) {
        SDL_DestroyTexture(it->second);
        textureCache.erase(it);
    }
}

void Assets::ClearAll() {
    for (auto& pair : textureCache) {
        SDL_DestroyTexture(pair.second);
    }
    textureCache.clear();
}
