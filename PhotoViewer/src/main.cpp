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
	/* Resets the image offset and zoom to fit inside the window, called on image reload */
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
				// Zooming using the mousewheel
				if (event.type == SDL_MOUSEWHEEL) {
					auto scroll = (double)event.wheel.y;

					zoom += scroll * zoom * 0.1;
				}
				// Moving the image by dragging
				if (event.type == SDL_MOUSEMOTION) {
					// Test if the left mouse button is down
					if (SDL_GetMouseState(nullptr, nullptr) && SDL_BUTTON(1)) {
						offX += event.motion.xrel;
						offY += event.motion.yrel;
					}
				}

				if (event.type == SDL_KEYDOWN) {
					// Change to the previous image 
					if (event.key.keysym.sym == SDLK_LEFT) {
						/* Last valid image path */
						std::filesystem::directory_entry last;
						// We iterate thru the entire folder util we find the target picture, then we load the last one
						for (const auto& file : std::filesystem::directory_iterator(targetFile.getPath().parent_path())) {
							// Test if this is a valid file
							if (file.is_regular_file() && allowedExtensions.find(file.path().extension().string()) != allowedExtensions.end()) {
								// If file is our target file we load the previous one
								if (file.path() == targetFile.getPath()) {
									// If no previous image, give up
									if (last.path().empty()) {
										break;
									} else {
										// Loading the last image
										targetFile = last.path();
										resetImageTransform();
										break;
									}
								} else {
									// Save this file to be the previous file in the next iteration
									last = file;
								}
							}
						}
					// Changind to the next image
					} else if (event.key.keysym.sym == SDLK_RIGHT) {
						/* If the last image was our target image */
						bool was = false;
						// We iterate thru the target image directory until we find our target image, the we load the one after it
						for (const auto& file : std::filesystem::directory_iterator(targetFile.getPath().parent_path())) {
							// Test if the file is a valid image file
							if (file.is_regular_file() && allowedExtensions.find(file.path().extension().string()) != allowedExtensions.end()) {
								// If the file is our target file set to load the next one
								if (file.path() == targetFile.getPath()) {
									was = true;
								} else {
									if (was) {
										// Load this image because the previous one was the target image
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
		/* Window width */
		int width; 
		/* Window heigth */
		int height;
		/* Image width */
		int iWidth; 
		/* Image heigth */
		int iHeight;
		// Get window size
		sdlhelp::handleSDLError(SDL_GetRendererOutputSize(renderer.get(), &width, &height));
		// Set background color to black
		sdlhelp::handleSDLError(SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255));
		// Clear the window
		sdlhelp::handleSDLError(SDL_RenderClear(renderer.get()));
		/* Horizontal window center */
		int centerX = width / 2; 
		/* Vertival window center */
		int centerY = height / 2;

		{ /// Draw image --------------------------------------------------------------------------+
			/* Target image texture */
			auto texture = targetFile.getTexture(renderer.get());
			// Get the image size
			auto textureData = sdlhelp::queryTexture(texture);
			iWidth = textureData.w;
			iHeight = textureData.h;
			/* Minimal zoom limit, calculated to fit the image inside the window */
			double zoomLimit = std::min(std::min(
				1.0 / ((double)iWidth / width),
				1.0 / ((double)iHeight / height)
			), 1.0);
			// Enforce zoom limits
			if (zoom < zoomLimit) zoom = zoomLimit;
			if (zoom > 20) zoom = 20;

			double
				/* Width of the image after zooming */
				w = iWidth * zoom,
				/* Heigth of the image after zooming */
				h = iHeight * zoom;
			// Test if the image is bigger than the window horizontaly
			if (w <= width) {
				// If not reset the vertical offset
				offX = 0;
			} else {
				// Else limit the offset to prevent black bars
				offX = std::min(w / 2 - (double)width / 2, std::max(-w / 2 + (double)width / 2, offX));
			}

			// See previous
			if (h <= height) {
				offY = 0;
			} else {
				offY = std::min(h / 2 - (double)height / 2, std::max(-h / 2 + (double)height / 2, offY));
			}

			double
				/* Horizontal position of the image after offseting */
				x = centerX - w / 2 + offX,
				/* Vertical position of the image after offseting */
				y = centerY - h / 2 + offY;
			/* The destination rect to draw the image into */
			SDL_Rect drawRect{
				static_cast<int>(std::floor(x)),static_cast<int>(std::floor(y))
				,static_cast<int>(std::floor(w))
				,static_cast<int>(std::floor(h))
			};
			// Enable linear filtering if the image is smaller than its real size
			SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, zoom > 1 ? "2" : "0", SDL_HINT_OVERRIDE);
			// Render the image
			sdlhelp::handleSDLError(SDL_RenderCopy(renderer.get(), texture, nullptr, &drawRect));
		}

		// Rendering end --------------------------------------------------------------------------+
		SDL_RenderPresent(renderer.get());
		// Setting the title ----------------------------------------------------------------------+
		SDL_SetWindowTitle(window.get(), (
			std::string("PhotoViewer ") // Name of the program
			+ std::to_string(width) // Window dimesions
			+ "x"
			+ std::to_string(height)
			+ " "
			+ std::to_string(fpsMeter.fps()) // Display the fps
			+ "FPS :: "
			+ targetFile.getPath().filename().string().c_str() // Target file name
			+ " "
			+ std::to_string(iWidth) // Target file size
			+ " x "
			+ std::to_string(iHeight)
			).c_str());

		{ // FPS limiting -------------------------------------------------------------------------+
			fpsMeter.tick();
			// ms to wait to reach the target fps, this code is probably wrong because we get 100 fps instead of 60
			double wait = (0.017 - fpsMeter.delta()) * 1000;
			if (wait > 0)SDL_Delay(static_cast<unsigned int>(std::floor(wait)));
		}
	}

eventLoopExit: return 0;
} catch (const std::exception & ex) {
	// Display the error if any
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "PhotoViewer critical error", ex.what(), nullptr);

	return 1;
}