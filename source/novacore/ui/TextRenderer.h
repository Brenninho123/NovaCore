#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <unordered_map>

enum class TextAlign {
    Left,
    Center,
    Right
};

class TextRenderer {
public:
    static bool Init();
    static void Shutdown();

    static bool LoadFont(const std::string& name, const std::string& path, int size);
    static void UnloadFont(const std::string& name);

    static void DrawText(SDL_Renderer* renderer, const std::string& fontName, const std::string& text, int x, int y, SDL_Color color, TextAlign align = TextAlign::Left);
    static void DrawTextWrapped(SDL_Renderer* renderer, const std::string& fontName, const std::string& text, int x, int y, int maxWidth, SDL_Color color, TextAlign align = TextAlign::Left);

    static void MeasureText(const std::string& fontName, const std::string& text, int& outWidth, int& outHeight);

    static void ClearTextureCache();
    static void TrimTextureCache(size_t maxEntries);

    static bool HasFont(const std::string& name);

private:
    struct CachedTexture {
        SDL_Texture* texture = nullptr;
        int width = 0;
        int height = 0;
        Uint32 lastUsedTicks = 0;
    };

    static std::string BuildCacheKey(const std::string& fontName, const std::string& text, SDL_Color color, int wrapWidth);
    static CachedTexture* GetOrCreateTexture(SDL_Renderer* renderer, const std::string& fontName, const std::string& text, SDL_Color color, int wrapWidth);

    static std::unordered_map<std::string, TTF_Font*> fonts;
    static std::unordered_map<std::string, CachedTexture> textureCache;

    static bool initialized;
};
