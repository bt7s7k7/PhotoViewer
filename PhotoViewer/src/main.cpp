#include "pch.h"
#include "lib/SDLHelper.h"
#undef main

int main(int argc, char ** argv) try {
	sdlhelp::handleSDLError(SDL_Init(SDL_INIT_VIDEO));
	
	throw std::runtime_error("test");
} catch (const std::exception & ex) {
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "PhotoViewer critical error", ex.what(), nullptr);
}