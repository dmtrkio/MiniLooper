#include <iostream>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "main_application.h"

auto clearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

SDL_Window *window;
SDL_Renderer *renderer;
bool gImguiInitialized = false;
float mainScale;

std::unique_ptr<MainApplication> app;

SDL_AppResult initializeSDL()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        std::cerr << "Error in SDL_Init: " << SDL_GetError() << std::endl;
        return SDL_APP_FAILURE;
    }

    mainScale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
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

void initializeImgui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    //ImGui::StyleColorsDark();
    ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(mainScale);
    style.FontScaleDpi = mainScale;

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    gImguiInitialized = true;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    std::cout << "SDL_AppInit" << std::endl;

    try {
        app = std::make_unique<MainApplication>(argc, argv);
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return SDL_APP_FAILURE;
    }

    if (const auto res = initializeSDL(); res != SDL_APP_CONTINUE) {
        return res;
    }

    initializeImgui();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
        SDL_Delay(10);
        return SDL_APP_CONTINUE;
    }

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    {
        if (!app) {
            return SDL_APP_FAILURE;
        }

        try {
            app->onFrame();
        } catch (std::exception &e) {
            std::cerr << e.what() << std::endl;
            return SDL_APP_FAILURE;
        }
    }

    ImGui::Render();
    const ImGuiIO& io = ImGui::GetIO();
    SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColorFloat(renderer, clearColor.x, clearColor.y, clearColor.z, clearColor.w);
    SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    ImGui_ImplSDL3_ProcessEvent(event);

    switch (event->type) {
        case SDL_EVENT_QUIT: {
            return SDL_APP_SUCCESS;
        }
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
            if (event->window.windowID == SDL_GetWindowID(window)) {
                return SDL_APP_SUCCESS;
            }
            break;
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
    if (gImguiInitialized) {
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    app.reset();

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