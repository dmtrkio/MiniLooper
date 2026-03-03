#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

SDL_Window *window;
SDL_Renderer *renderer;

SDL_AppResult initializeSDL()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        std::cerr << "Error in SDL_Init: " << SDL_GetError() << std::endl;
        return SDL_APP_FAILURE;
    }

    const float mainScale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    constexpr SDL_WindowFlags windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    window = SDL_CreateWindow("MiniLooper", static_cast<int>(1280 * mainScale), static_cast<int>(800 * mainScale), windowFlags);
    if (window == nullptr) {
        std::cerr << "Error in SDL_CreateWindow: " << SDL_GetError() << std::endl;
        return SDL_APP_FAILURE;
    }

    renderer = SDL_CreateRenderer(window, nullptr);
    SDL_SetRenderVSync(renderer, 1);
    if (renderer == nullptr) {
        //SDL_Log("Error: SDL_CreateRenderer: %s\n", SDL_GetError());
        std::cerr << "Error in SDL_CreateRenderer: " << SDL_GetError() << std::endl;
        return SDL_APP_FAILURE;
    }

    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    std::cout << "SDL_AppInit" << std::endl;

    return initializeSDL();
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    switch (event->type) {
        case SDL_EVENT_QUIT: {
            return SDL_APP_SUCCESS;
        }
        case SDL_EVENT_KEY_DOWN: {
            std::cout << "Key pressed: " << SDL_GetKeyName(event->key.key) << std::endl;
            break;
        }
        default: {
            break;
        }
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    constexpr auto resultToStr = [](const SDL_AppResult res) {
        switch (res) {
            case SDL_APP_CONTINUE: return("SDL_APP_CONTINUE");
            case SDL_APP_SUCCESS: return("SDL_APP_SUCCESS");
            case SDL_APP_FAILURE: return("SDL_APP_FAILURE");
            default: return("");
        }
    };

    std::cout << "SDL_AppQuit with result: " << resultToStr(result) << std::endl;
}