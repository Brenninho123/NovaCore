#include "TextRenderer.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>

std::unordered_map<std::string, TTF_Font*> TextRenderer::fonts;
std::unordered_map<std::string, TextRenderer::CachedTexture> TextRenderer::textureCache;
bool TextRenderer::initialized = false;

bool TextRenderer::Init() {
    if (TTF_Init() != 0) {
        std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
        return false;
    }

    initialized = true;
    return true;
}

void TextRenderer::Shutdown() {
    ClearTextureCache();

    for (auto& pair : fonts) {
        TTF_CloseFont(pair.second);
    }
    fonts.clear();

    if (initialized) {
        TTF_Quit();
        initialized = false;
    }
}

bool TextRenderer::LoadFont(const std::string& name, const std::string& path, int size) {
    auto it = fonts.find(name);
    if (it != fonts.end()) {
        TTF_CloseFont(it->second);
        fonts.erase(it);
    }

    TTF_Font* font = TTF_OpenFont(path.c_str(), size);
    if (!font) {
        std::cerr << "Failed to load font: " << path << " - " << TTF_GetError() << std::endl;
        return false;
    }

    fonts[name] = font;
    return true;
}

void TextRenderer::UnloadFont(const std::string& name) {
    auto it = fonts.find(name);
    if (it != fonts.end()) {
        TTF_CloseFont(it->second);
        fonts.erase(it);
    }
}

bool TextRenderer::HasFont(const std::string& name) {
    return fonts.find(name) != fonts.end();
}

std::string TextRenderer::BuildCacheKey(const std::string& fontName, const std::string& text, SDL_Color color, int wrapWidth) {
    std::ostringstream key;
    key << fontName << "|" << text << "|"
        << static_cast<int>(color.r) << "," << static_cast<int>(color.g) << "," << static_cast<int>(color.b) << "," << static_cast<int>(color.a)
        << "|" << wrapWidth;
    return key.str();
}

TextRenderer::CachedTexture* TextRenderer::GetOrCreateTexture(SDL_Renderer* renderer, const std::string& fontName, const std::string& text, SDL_Color color, int wrapWidth) {
    if (text.empty()) {
        return nullptr;
    }

    auto fontIt = fonts.find(fontName);
    if (fontIt == fonts.end()) {
        std::cerr << "Font not loaded: " << fontName << std::endl;
        return nullptr;
    }

    std::string key = BuildCacheKey(fontName, text, color, wrapWidth);

    auto cacheIt = textureCache.find(key);
    if (cacheIt != textureCache.end()) {
        cacheIt->second.lastUsedTicks = SDL_GetTicks();
        return &cacheIt->second;
    }

    SDL_Surface* surface = nullptr;

    if (wrapWidth > 0) {
        surface = TTF_RenderUTF8_Blended_Wrapped(fontIt->second, text.c_str(), color, static_cast<Uint32>(wrapWidth));
    } else {
        surface = TTF_RenderUTF8_Blended(fontIt->second, text.c_str(), color);
    }

    if (!surface) {
        std::cerr << "TTF_RenderUTF8_Blended failed: " << TTF_GetError() << std::endl;
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

    CachedTexture entry;
    entry.texture = texture;
    entry.width = surface->w;
    entry.height = surface->h;
    entry.lastUsedTicks = SDL_GetTicks();

    SDL_FreeSurface(surface);

    if (!texture) {
        std::cerr << "SDL_CreateTextureFromSurface failed: " << SDL_GetError() << std::endl;
        return nullptr;
    }

    textureCache[key] = entry;
    return &textureCache[key];
}

void TextRenderer::DrawText(SDL_Renderer* renderer, const std::string& fontName, const std::string& text, int x, int y, SDL_Color color, TextAlign align) {
    CachedTexture* cached = GetOrCreateTexture(renderer, fontName, text, color, 0);
    if (!cached || !cached->texture) {
        return;
    }

    int drawX = x;

    if (align == TextAlign::Center) {
        drawX = x - cached->width / 2;
    } else if (align == TextAlign::Right) {
        drawX = x - cached->width;
    }

    SDL_Rect destRect = { drawX, y, cached->width, cached->height };
    SDL_RenderCopy(renderer, cached->texture, nullptr, &destRect);
}

void TextRenderer::DrawTextWrapped(SDL_Renderer* renderer, const std::string& fontName, const std::string& text, int x, int y, int maxWidth, SDL_Color color, TextAlign align) {
    CachedTexture* cached = GetOrCreateTexture(renderer, fontName, text, color, maxWidth);
    if (!cached || !cached->texture) {
        return;
    }

    int drawX = x;

    if (align == TextAlign::Center) {
        drawX = x - cached->width / 2;
    } else if (align == TextAlign::Right) {
        drawX = x - cached->width;
    }

    SDL_Rect destRect = { drawX, y, cached->width, cached->height };
    SDL_RenderCopy(renderer, cached->texture, nullptr, &destRect);
}

void TextRenderer::MeasureText(const std::string& fontName, const std::string& text, int& outWidth, int& outHeight) {
    outWidth = 0;
    outHeight = 0;

    auto fontIt = fonts.find(fontName);
    if (fontIt == fonts.end() || text.empty()) {
        return;
    }

    TTF_SizeUTF8(fontIt->second, text.c_str(), &outWidth, &outHeight);
}

void TextRenderer::ClearTextureCache() {
    for (auto& pair : textureCache) {
        if (pair.second.texture) {
            SDL_DestroyTexture(pair.second.texture);
        }
    }
    textureCache.clear();
}

void TextRenderer::TrimTextureCache(size_t maxEntries) {
    if (textureCache.size() <= maxEntries) {
        return;
    }

    std::vector<std::pair<std::string, Uint32>> entries;
    entries.reserve(textureCache.size());

    for (auto& pair : textureCache) {
        entries.emplace_back(pair.first, pair.second.lastUsedTicks);
    }

    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        return a.second < b.second;
    });

    size_t removeCount = textureCache.size() - maxEntries;

    for (size_t i = 0; i < removeCount && i < entries.size(); i++) {
        auto it = textureCache.find(entries[i].first);
        if (it != textureCache.end()) {
            if (it->second.texture) {
                SDL_DestroyTexture(it->second.texture);
            }
            textureCache.erase(it);
        }
    }
}
