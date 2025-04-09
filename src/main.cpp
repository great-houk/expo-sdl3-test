#define WIDTH 272
#define HEIGHT 144
#define PIXEL_SIZE 5
#define MAX_ASTEROIDS 10

#include <cmath> // For fmod function
#include <stdio.h> // For printf function

// Example function to modify the pixel buffer
void draw_rect(char *pixel_buf, int x, int y, int w, int h, char r, char g, char b) {
    // Check if the rectangle is completely outside the screen
    if (x >= WIDTH || y >= HEIGHT || x + w <= 0 || y + h <= 0) {
        return; // Don't draw if completely outside
    }
    
    // Clamp coordinates to screen boundaries
    int start_x = (x < 0) ? 0 : x;
    int start_y = (y < 0) ? 0 : y;
    int end_x = (x + w > WIDTH) ? WIDTH : x + w;
    int end_y = (y + h > HEIGHT) ? HEIGHT : y + h;
    
    // Draw only the visible portion
    for (int i = start_x; i < end_x; i++) {
        for (int j = start_y; j < end_y; j++) {
            // IIRC the color order is GRB, but ideally 
            // make it so swapping colors isn't hard in case
            // I'm wrong. For now render RGB
            pixel_buf[j * WIDTH * 4 + i * 4 + 0] = r;
            pixel_buf[j * WIDTH * 4 + i * 4 + 1] = g;
            pixel_buf[j * WIDTH * 4 + i * 4 + 2] = b;
            // This fourth byte has to exist for alignment reasons, can hold whatever as it's never read
            pixel_buf[j * WIDTH * 4 + i * 4 + 3] = 0;
        }
    }
}

struct Asteroid {
    float x;
    float y;
    int width;
    int height;
    float speed_x;
    float speed_y;
};

// Array to store all asteroids
struct Asteroid asteroids[MAX_ASTEROIDS];
int asteroid_count = 0;

// Function to initialize an asteroid
void init_asteroid(char *pixel_buf, float x, float y, int width, int height, float speed_x, float speed_y) {
    // Check if we have room for another asteroid
    if (asteroid_count >= MAX_ASTEROIDS) {
        return; // No more room
    }
    
    // Initialize the asteroid
    asteroids[asteroid_count].x = x;
    asteroids[asteroid_count].y = y;
    asteroids[asteroid_count].width = width;
    asteroids[asteroid_count].height = height;
    asteroids[asteroid_count].speed_x = speed_x;
    asteroids[asteroid_count].speed_y = speed_y;
    
    // Convert float position to int for drawing
    int draw_x = (int)asteroids[asteroid_count].x;
    int draw_y = (int)asteroids[asteroid_count].y;
    
    // Draw the asteroid
    draw_rect(pixel_buf, draw_x, draw_y, width, height, 86, 107, 114);
    
    // Increment the asteroid count
    asteroid_count++;
}

void move_asteroid(char *pixel_buf, struct Asteroid *myAsteroid) {
    // Update the asteroid's position based on its speed
    myAsteroid->x += myAsteroid->speed_x;
    myAsteroid->y += myAsteroid->speed_y;
    
    // Handle wrapping around the screen horizontally
    if (myAsteroid->x > WIDTH) {
        // If the asteroid goes off the right edge, wrap to the left
        myAsteroid->x = 0 - myAsteroid->width;
    } else if (myAsteroid->x + myAsteroid->width < 0) {
        // If the asteroid goes off the left edge, wrap to the right
        myAsteroid->x = WIDTH + myAsteroid->width;
    }

    // Handle wrapping around the screen vertically
    // Check if the asteroid is completely below the screen
    if (myAsteroid->y > HEIGHT) {
        // Wrap to the top of the screen
        myAsteroid->y = 0 - myAsteroid->height;
    } 
    // Check if the asteroid is completely above the screen
    else if (myAsteroid->y + myAsteroid->height < 0) {
        // Wrap to the bottom of the screen
        myAsteroid->y = HEIGHT + myAsteroid->height;
    }
    
    // Debug output to check asteroid position
    printf("Asteroid position: x=%.2f, y=%.2f\n", myAsteroid->x, myAsteroid->y);
    
    // Convert float position to int for drawing
    int draw_x = (int)myAsteroid->x;
    int draw_y = (int)myAsteroid->y;
    
    // Redraw the asteroid at its new position
    draw_rect(pixel_buf, draw_x, draw_y, myAsteroid->width, myAsteroid->height, 86, 107, 114);
}

// Runs once at startup
void init(char *pixel_buf, int *ind) {
    // Set background to black
    draw_rect(pixel_buf, 0, 0, WIDTH, HEIGHT, 0, 0, 0);
    
    // Initialize the asteroid with a very slow speed (0.2 pixels per frame)
    init_asteroid(pixel_buf, 100, 0, 10, 10, 0.2f, 0.05f);
    
    // Init ind
    *ind = 0;
}

void update(char *pixel_buf, int *ind) {
    // Clear the screen (or just the area where the asteroid was)
    draw_rect(pixel_buf, 0, 0, WIDTH, HEIGHT, 0, 0, 0);
    
    // Move and redraw all asteroids
    for (int i = 0; i < asteroid_count; i++) {
        move_asteroid(pixel_buf, &asteroids[i]);
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
