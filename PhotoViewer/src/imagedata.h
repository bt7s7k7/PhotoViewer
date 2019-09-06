#pragma once
#include "pch.h"

struct imagedata_t {
protected:
	std::filesystem::path path;
	sdlhelp::unique_surface_ptr surface;
	sdlhelp::unique_texture_ptr texture;
public:
	inline const std::filesystem::path& getPath() {
		if (path.empty()) throw std::runtime_error("Image is empty");
		return path;
	}

	inline bool empty() { return path.empty(); }
	inline operator bool() { return !empty(); }

	inline SDL_Surface* getSurface() {
		if (!surface) {
			surface.reset(sdlhelp::handleSDLError(IMG_Load(getPath().u8string().c_str())));
		}

		return surface.get();
	}

	inline SDL_Texture* getTexture(SDL_Renderer* renderer) {
		if (!texture) {
			texture.reset(sdlhelp::handleSDLError(SDL_CreateTextureFromSurface(renderer, getSurface())));
		}

		return texture.get();
	}


	inline imagedata_t(const std::filesystem::path& p) : path(p) {}
	inline imagedata_t() : path() {};
};
