#include "pch.h"
#include "lib/SDLHelper.h"
#include "imagedata.h"
#include "lib/fpsMeter.h"
#undef main

int main(int argc, char** argv) try {
	sdlhelp::handleSDLError(SDL_Init(SDL_INIT_VIDEO));

	auto window = sdlhelp::unique_window_ptr(sdlhelp::handleSDLError(SDL_CreateWindow("PhotoViewer loading...", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 200, 200, SDL_WINDOW_RESIZABLE)));
	auto renderer = sdlhelp::unique_renderer_ptr(sdlhelp::handleSDLError(SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED)));

	imagedata_t targetFile;
	fpsMeter_t fpsMeter;

	if (argc >= 2) {
		targetFile = std::filesystem::path(argv[1]);
	} else {
		targetFile = std::filesystem::path(argv[0]).replace_filename("defaultImage.png");
	}

	while (true) {
		{ // SDL event polling --------------------------------------------------------------------+
			SDL_Event event;
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_WINDOWEVENT) {
					if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
						goto eventLoopExit;
					}
				}

			}
		}

		int width, height;
		sdlhelp::handleSDLError(SDL_GetRendererOutputSize(renderer.get(), &width, &height));

		sdlhelp::handleSDLError(SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255));
		sdlhelp::handleSDLError(SDL_RenderClear(renderer.get()));
		SDL_RenderPresent(renderer.get());

		SDL_SetWindowTitle(window.get(), (
			std::string("PhotoViewer ")
			+ std::to_string(width)
			+ "x"
			+ std::to_string(height)
			+ " "
			+ std::to_string(fpsMeter.fps())
			+ "FPS :: "
			+ targetFile.getPath().string().c_str()
			).c_str());

		{ // FPS limiting -------------------------------------------------------------------------+
			fpsMeter.tick();
			double wait = (0.017 - fpsMeter.delta()) * 1000;
			if (wait > 0)SDL_Delay(static_cast<unsigned int>(std::floor(wait)));
		}
	}

eventLoopExit: return 0;
} catch (const std::exception & ex) {
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "PhotoViewer critical error", ex.what(), nullptr);

	return 1;
}