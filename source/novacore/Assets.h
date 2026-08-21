#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <string>
#include <unordered_map>

class Assets {
public:
    static void Init(SDL_Renderer* renderer);
    static void Shutdown();

    static SDL_Texture* LoadTexture(const std::string& path);
    static void UnloadTexture(const std::string& path);

    static Mix_Chunk* LoadSound(const std::string& path);
    static void UnloadSound(const std::string& path);

    static Mix_Music* LoadMusic(const std::string& path);
    static void UnloadMusic(const std::string& path);

    static void PreloadTextures(const std::vector<std::string>& paths);

    static void ClearTextures();
    static void ClearSounds();
    static void ClearMusic();
    static void ClearAll();

    static size_t TextureCount();
    static size_t SoundCount();
    static size_t MusicCount();

private:
    static SDL_Renderer* renderer;
    static std::unordered_map<std::string, SDL_Texture*> textureCache;
    static std::unordered_map<std::string, Mix_Chunk*> soundCache;
    static std::unordered_map<std::string, Mix_Music*> musicCache;
};
