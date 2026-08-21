#include "Assets.h"
#include <SDL2/SDL_image.h>
#include <iostream>
#include <vector>

SDL_Renderer* Assets::renderer = nullptr;
std::unordered_map<std::string, SDL_Texture*> Assets::textureCache;
std::unordered_map<std::string, Mix_Chunk*> Assets::soundCache;
std::unordered_map<std::string, Mix_Music*> Assets::musicCache;

void Assets::Init(SDL_Renderer* r) {
    renderer = r;

    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(imgFlags) & imgFlags) != imgFlags) {
        std::cerr << "IMG_Init failed: " << IMG_GetError() << std::endl;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "Mix_OpenAudio failed: " << Mix_GetError() << std::endl;
    }

    int mixFlags = MIX_INIT_OGG | MIX_INIT_MP3;
    if ((Mix_Init(mixFlags) & mixFlags) != mixFlags) {
        std::cerr << "Mix_Init failed: " << Mix_GetError() << std::endl;
    }
}

void Assets::Shutdown() {
    ClearAll();
    Mix_CloseAudio();
    Mix_Quit();
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

Mix_Chunk* Assets::LoadSound(const std::string& path) {
    auto it = soundCache.find(path);
    if (it != soundCache.end()) {
        return it->second;
    }

    Mix_Chunk* sound = Mix_LoadWAV(path.c_str());
    if (!sound) {
        std::cerr << "Failed to load sound: " << path << " - " << Mix_GetError() << std::endl;
        return nullptr;
    }

    soundCache[path] = sound;
    return sound;
}

void Assets::UnloadSound(const std::string& path) {
    auto it = soundCache.find(path);
    if (it != soundCache.end()) {
        Mix_FreeChunk(it->second);
        soundCache.erase(it);
    }
}

Mix_Music* Assets::LoadMusic(const std::string& path) {
    auto it = musicCache.find(path);
    if (it != musicCache.end()) {
        return it->second;
    }

    Mix_Music* music = Mix_LoadMUS(path.c_str());
    if (!music) {
        std::cerr << "Failed to load music: " << path << " - " << Mix_GetError() << std::endl;
        return nullptr;
    }

    musicCache[path] = music;
    return music;
}

void Assets::UnloadMusic(const std::string& path) {
    auto it = musicCache.find(path);
    if (it != musicCache.end()) {
        Mix_FreeMusic(it->second);
        musicCache.erase(it);
    }
}

void Assets::PreloadTextures(const std::vector<std::string>& paths) {
    for (const auto& path : paths) {
        LoadTexture(path);
    }
}

void Assets::ClearTextures() {
    for (auto& pair : textureCache) {
        SDL_DestroyTexture(pair.second);
    }
    textureCache.clear();
}

void Assets::ClearSounds() {
    for (auto& pair : soundCache) {
        Mix_FreeChunk(pair.second);
    }
    soundCache.clear();
}

void Assets::ClearMusic() {
    for (auto& pair : musicCache) {
        Mix_FreeMusic(pair.second);
    }
    musicCache.clear();
}

void Assets::ClearAll() {
    ClearTextures();
    ClearSounds();
    ClearMusic();
}

size_t Assets::TextureCount() {
    return textureCache.size();
}

size_t Assets::SoundCount() {
    return soundCache.size();
}

size_t Assets::MusicCount() {
    return musicCache.size();
}
