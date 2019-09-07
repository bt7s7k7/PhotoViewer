#include "pch.h"
#include "lib/SDLHelper.h"
#include "imagedata.h"
#include "lib/fpsMeter.h"
#undef main

int main(int argc, char** argv) try {
	sdlhelp::handleSDLError(SDL_Init(SDL_INIT_VIDEO));

	// Constants ----------------------------------------------------------------------------------+
	constexpr int MIN_WINDOW_SIZE = 200;
	static std::unordered_set<std::string> allowedExtensions = {
		".bmp",
		".cur",
		".gif",
		".ico",
		".jpg",
		".lbm",
		".pcx",
		".png",
		".pnm",
		".tga",
		".tif",
		".xcf",
		".xpm",
		".xv"
	};

	// Global values ------------------------------------------------------------------------------+
	sdlhelp::unique_window_ptr window;
	imagedata_t targetFile;
	fpsMeter_t fpsMeter;
	// Image selection ----------------------------------------------------------------------------+
	if (argc >= 2) {
		targetFile = std::filesystem::absolute(std::filesystem::path(argv[1]));
	} else {
		targetFile = std::filesystem::path(argv[0]).replace_filename("defaultImage.png");
	}

	{ /// Creating the window ---------------------------------------------------------------------+
		auto surface = targetFile.getSurface();
		window = sdlhelp::unique_window_ptr(sdlhelp::handleSDLError(SDL_CreateWindow(
			"PhotoViewer loading...", // Title will be changed automaticaly in update loop
			SDL_WINDOWPOS_CENTERED, // We want the window position to be centered
			SDL_WINDOWPOS_CENTERED,
			std::max(surface->w, MIN_WINDOW_SIZE), // Scale window to fit the image
			std::max(surface->h, MIN_WINDOW_SIZE),
			SDL_WINDOW_RESIZABLE // Window should be resizable
		)));
	}
	auto renderer = sdlhelp::unique_renderer_ptr(sdlhelp::handleSDLError(SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED)));


	/* Pixel size multiplyer */
	double zoom = 1;
	/* Horizontal image offset */
	double offX = 0; 
	/* Vertical image offset */
	double offY = 0;

	auto resetImageTransform = [&]() {
		auto surface = targetFile.getSurface();
		int w, h;
		SDL_GetRendererOutputSize(renderer.get(), &w, &h);

		zoom = 1;
		offX = 0, offY = 0;
		if ((surface->w * zoom) > w) {
			zoom = (double)w / (double)surface->w;
		}

		if ((surface->h * zoom) > h) {
			zoom = (double)h / (double)surface->h;
		}
	};

	resetImageTransform();

	while (true) {
		{ // SDL event polling --------------------------------------------------------------------+
			SDL_Event event;
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_WINDOWEVENT) {
					if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
						goto eventLoopExit;
					} else if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
						resetImageTransform();
					}
				}

				if (event.type == SDL_MOUSEWHEEL) {
					auto scroll = (double)event.wheel.y;

					zoom += scroll * zoom * 0.1;
				}

				if (event.type == SDL_MOUSEMOTION) {
					if (SDL_GetMouseState(nullptr, nullptr) && SDL_BUTTON(1)) {
						offX += event.motion.xrel;
						offY += event.motion.yrel;
					}
				}

				if (event.type == SDL_KEYDOWN) {
					if (event.key.keysym.sym == SDLK_LEFT) {
						std::filesystem::directory_entry last;
						for (const auto& file : std::filesystem::directory_iterator(targetFile.getPath().parent_path())) {
							if (file.is_regular_file() && allowedExtensions.find(file.path().extension().string()) != allowedExtensions.end()) {
								if (file.path() == targetFile.getPath()) {
									if (last.path().empty()) {
										break;
									} else {
										targetFile = last.path();
										resetImageTransform();
										break;
									}
								} else {
									last = file;
								}
							}
						}
					} else if (event.key.keysym.sym == SDLK_RIGHT) {
						bool was = false;
						for (const auto& file : std::filesystem::directory_iterator(targetFile.getPath().parent_path())) {
							if (file.is_regular_file() && allowedExtensions.find(file.path().extension().string()) != allowedExtensions.end()) {
								if (file.path() == targetFile.getPath()) {
									was = true;
								} else {
									if (was) {
										targetFile = file.path();
										resetImageTransform();
										break;
									}
								}
							}
						}
					}
				}
			}
		}

		/// Rendering start -----------------------------------------------------------------------+
		int width, height;
		int iWidth, iHeight;
		sdlhelp::handleSDLError(SDL_GetRendererOutputSize(renderer.get(), &width, &height));
		sdlhelp::handleSDLError(SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255));
		sdlhelp::handleSDLError(SDL_RenderClear(renderer.get()));

		int centerX = width / 2, centerY = height / 2;

		{ /// Draw image --------------------------------------------------------------------------+
			auto texture = targetFile.getTexture(renderer.get());
			auto textureData = sdlhelp::queryTexture(texture);
			iWidth = textureData.w;
			iHeight = textureData.h;

			double zoomLimit = std::min(std::min(
				1.0 / ((double)iWidth / width),
				1.0 / ((double)iHeight / height)
			), 1.0);

			if (zoom < zoomLimit) zoom = zoomLimit;
			if (zoom > 20) zoom = 20;

			double
				w = iWidth * zoom,
				h = iHeight * zoom;

			if (w <= width) {
				offX = 0;
			} else {
				offX = std::min(w / 2 - (double)width / 2, std::max(-w / 2 + (double)width / 2, offX));
			}

			if (h <= height) {
				offY = 0;
			} else {
				offY = std::min(h / 2 - (double)height / 2, std::max(-h / 2 + (double)height / 2, offY));
			}

			double
				x = centerX - w / 2 + offX,
				y = centerY - h / 2 + offY;

			SDL_Rect drawRect{
				static_cast<int>(std::floor(x)),static_cast<int>(std::floor(y))
				,static_cast<int>(std::floor(w))
				,static_cast<int>(std::floor(h))
			};
			SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, zoom > 1 ? "2" : "0", SDL_HINT_OVERRIDE);
			sdlhelp::handleSDLError(SDL_RenderCopy(renderer.get(), texture, nullptr, &drawRect));
		}

		// Rendering end --------------------------------------------------------------------------+
		SDL_RenderPresent(renderer.get());
		// Setting the title ----------------------------------------------------------------------+
		SDL_SetWindowTitle(window.get(), (
			std::string("PhotoViewer ")
			+ std::to_string(width)
			+ "x"
			+ std::to_string(height)
			+ " "
			+ std::to_string(fpsMeter.fps())
			+ "FPS :: "
			+ targetFile.getPath().filename().string().c_str()
			+ " "
			+ std::to_string(iWidth)
			+ " x "
			+ std::to_string(iHeight)
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