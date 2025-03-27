#define WIDTH 272
#define HEIGHT 144
#define PIXEL_SIZE 5

// Example function to modify the pixel buffer
void draw_rect(char *pixel_buf, int x, int y, int w, int h, char r, char g, char b) {
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            // IIRC the color order is GRB, but ideally 
            // make it so swapping colors isn't hard in case
            // I'm wrong. For now render RGB
            pixel_buf[(y + j) * WIDTH * 4 + (x + i) * 4 + 0] = r;
            pixel_buf[(y + j) * WIDTH * 4 + (x + i) * 4 + 1] = g;
            pixel_buf[(y + j) * WIDTH * 4 + (x + i) * 4 + 2] = b;
            // This fourth byte has to exist for alignment reasons, can hold whatever as it's never read
            pixel_buf[(y + j) * WIDTH * 4 + (x + i) * 4 + 3] = 0;
        }
    }
}


// Runs once at startup
void init(char *pixel_buf /* Put whatever else you want in here for game data for ex I decided to pass an int */, int *ind) {
    // Clear screen to a dim blue
    draw_rect(pixel_buf, 0, 0, WIDTH, HEIGHT, 50, 50, 100);
    // Init ind
    *ind = 0;
}
 
void update(char *pixel_buf, int *ind) {
    // Change screen gradually
    for (int i = 0; i < 40; i++) {
        // Update one pixel at a time
        pixel_buf[*ind] += 100;
        *ind += 4;
        // Wrap ind
        if (*ind >= WIDTH * HEIGHT * 4) {
            *ind = (*ind % 4) + 1;
        }
        if (*ind == 3) {
            *ind = 0;
        }
    }
}

///////////////////
// Ignore below here, this is all example code 
// so you can see what your code is doing.
// Unless you want to add more game state
///////////////////

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_init.h>
#include <cmath>
#include <string_view>

static int ind = 0;

struct AppContext {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    SDL_AppResult app_quit = SDL_APP_CONTINUE;
};

SDL_AppResult SDL_Fail(){
    SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
    return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    // init the library, here we make a window so we only need the Video capabilities.
    if (not SDL_Init(SDL_INIT_VIDEO)){
        return SDL_Fail();
    }
    
    // create a window
    SDL_Window* window = SDL_CreateWindow("SDL Minimal Sample", WIDTH * PIXEL_SIZE, HEIGHT * PIXEL_SIZE, 0);
    if (not window){
        return SDL_Fail();
    }
    
    // create a renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (not renderer){
        return SDL_Fail();
    }

    // Make the render texture
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XBGR8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    
    // print some information about the window
    SDL_ShowWindow(window);
    {
        int width, height, bbwidth, bbheight;
        SDL_GetWindowSize(window, &width, &height);
        SDL_GetWindowSizeInPixels(window, &bbwidth, &bbheight);
        SDL_Log("Window size: %ix%i", width, height);
        SDL_Log("Backbuffer size: %ix%i", bbwidth, bbheight);
        if (width != bbwidth){
            SDL_Log("This is a highdpi environment.");
        }
    }

    // set up the application data
    *appstate = new AppContext{
       .window = window,
       .renderer = renderer,
       .texture = texture,
    };

    // Call init
    char* pixel_buf;
    int temp;
    SDL_LockTexture(texture, NULL, (void**)&pixel_buf, &temp);
    init(pixel_buf, &ind);
    SDL_UnlockTexture(texture);
    
    SDL_SetRenderVSync(renderer, -1);   // enable vysnc
    
    SDL_Log("Application started successfully!");

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event* event) {
    auto* app = (AppContext*)appstate;
    
    if (event->type == SDL_EVENT_QUIT) {
        app->app_quit = SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    auto* app = (AppContext*)appstate;

    // draw a color
    auto time = SDL_GetTicks() / 1000.f;
    auto red = (std::sin(time) + 1) / 2.0 * 255;
    auto green = (std::sin(time / 2) + 1) / 2.0 * 255;
    auto blue = (std::sin(time) * 2 + 1) / 2.0 * 255;
    
    SDL_SetRenderDrawColor(app->renderer, red, green, blue, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(app->renderer);

    char* pixel_buf;
    int temp;
    SDL_LockTexture(app->texture, NULL, (void**)&pixel_buf, &temp);
    update((char *)pixel_buf, &ind);
    SDL_UnlockTexture(app->texture);

    // Renderer uses the painter's algorithm to make the text appear above the image, we must render the image first.
    SDL_RenderTexture(app->renderer, app->texture, NULL, NULL);

    SDL_RenderPresent(app->renderer);

    return app->app_quit;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    auto* app = (AppContext*)appstate;
    if (app) {
        SDL_DestroyRenderer(app->renderer);
        SDL_DestroyWindow(app->window);
        SDL_DestroyTexture(app->texture);

        delete app;
    }

    SDL_Log("Application quit successfully!");
    SDL_Quit();
}
