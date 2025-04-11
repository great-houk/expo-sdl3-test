#define WIDTH 272
#define HEIGHT 144
#define PIXEL_SIZE 5
#define MAX_ASTEROIDS 4
#define PLAYER_SPEED 0.25f
#define ROTATION_SPEED 18.0f
#define THRUST_ACCELERATION 0.03f
#define FRICTION 0.995f
#define M_PI 3.14159265358979323846
#define MAX_BULLETS 1000

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_init.h>
#include <string_view>
#include <cmath> // For fmod function
#include <stdio.h> // For printf function

static int ind = 0;


struct AppContext {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    SDL_AppResult app_quit = SDL_APP_CONTINUE;
};

// Player state
struct Player {
    float x;
    float y;
    float velocity_x;
    float velocity_y;
    float rotation; // in degrees
    int lives;
    int score;
    bool invulnerable; // Flag to indicate invulnerability period
    int invulnerable_timer; // Timer for invulnerability period
};

struct Player player = {
    WIDTH / 2,  // x
    HEIGHT / 2, // y
    0,          // velocity_x
    0,          // velocity_y
    0,          // rotation
    5,          // lives
    0,          // score
    false,      // invulnerable
    0           // invulnerable_timer
};


struct EVENTS {

    bool left_flag;
    bool right_flag;

    bool shoot_flag;
    bool thrust_flag;

};



// Custom random number generator using Linear Congruential Generator (LCG)
// These constants are from Numerical Recipes
static unsigned long next = 1;
static const unsigned long a = 1664525;
static const unsigned long c = 1013904223;
static const unsigned long m = 4294967295; // 2^32 - 1

// Initialize the random number generator with a seed
void init_random(unsigned long seed) {
    next = seed;
}

// Generate a random number between 0 and m-1
unsigned long random() {
    next = (a * next + c) % m;
    return next;
}

// Generate a random number between min and max (inclusive)
int random_range(int min, int max) {
    return min + (random() % (max - min + 1));
}

// Generate a random float between 0.0 and 1.0
float random_float() {
    return (float)random() / m;
}

// Function to generate a random speed within a range
float generate_random_speed() {
    // Generate a random float between -0.03 and 0.03
    // This ensures asteroids can travel in different directions
    // with a minimum absolute speed of 0.01
    float speed = (random_float() * 0.06f) - 0.03f;
    
    // Ensure minimum speed in either direction
    if (speed > 0 && speed < 0.01f) {
        speed = 0.01f;
    } else if (speed < 0 && speed > -0.01f) {
        speed = -0.01f;
    }
    
    return speed;
}

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

// Bullet structure
struct Bullet {
    float x;
    float y;
    float velocity_x;
    float velocity_y;
    bool active;
    int lifetime; // Timer for how long the bullet exists
};

struct Bullet bullets[MAX_BULLETS];
int bullet_count = 0;

// Function to create a bullet
void create_bullet(float x, float y, float rotation) {
    // Check if we have room for another bullet
    if (bullet_count >= MAX_BULLETS) {
        return; // No more room
    }
    
    // Convert rotation to radians
    float rad = rotation * M_PI / 180.0f;
    
    // Calculate bullet velocity (faster than player)
    float bullet_speed = 0.3f;
    
    // Initialize the bullet
    bullets[bullet_count].x = x;
    bullets[bullet_count].y = y;
    bullets[bullet_count].velocity_x = sin(rad) * bullet_speed;
    bullets[bullet_count].velocity_y = -cos(rad) * bullet_speed;
    bullets[bullet_count].active = true;
    bullets[bullet_count].lifetime = 240; // Bullet will disappear after 240 frames (about 4 seconds at 60 FPS)
    
    // Increment the bullet count
    bullet_count++;
}

// Function to update and draw bullets
void update_bullets(char *pixel_buf) {
    for (int i = 0; i < bullet_count; i++) {
        if (!bullets[i].active) continue;
        
        // Update bullet position
        bullets[i].x += bullets[i].velocity_x;
        bullets[i].y += bullets[i].velocity_y;
        
        // Decrease lifetime
        bullets[i].lifetime--;
        
        // Check if bullet has expired
        if (bullets[i].lifetime <= 0) {
            bullets[i].active = false;
            continue;
        }
        
        // Handle wrapping around the screen
        if (bullets[i].x > WIDTH) {
            bullets[i].x = 0;
        } else if (bullets[i].x < 0) {
            bullets[i].x = WIDTH;
        }
        
        if (bullets[i].y > HEIGHT) {
            bullets[i].y = 0;
            bullets[i].lifetime = 0;
        } else if (bullets[i].y < 0) {
            bullets[i].y = HEIGHT;
            bullets[i].lifetime = 0;
        }
        
        // Draw the bullet (yellow)
        int draw_x = (int)bullets[i].x;
        int draw_y = (int)bullets[i].y;
        draw_rect(pixel_buf, draw_x, draw_y, 2, 2, 255, 255, 0);
    }
}

// Function to check for bullet-asteroid collisions
void check_bullet_collisions(char *pixel_buf) {
    for (int i = 0; i < bullet_count; i++) {
        if (!bullets[i].active) continue;
        
        for (int j = 0; j < asteroid_count; j++) {
            // Simple collision detection
            float dx = bullets[i].x - asteroids[j].x;
            float dy = bullets[i].y - asteroids[j].y;
            float distance = sqrt(dx * dx + dy * dy);
            
            if (distance < 10) { // Collision detected
                // Deactivate the bullet
                bullets[i].active = false;
                
                // Increment score
                player.score += 10;
                printf("Score: %d\n", player.score);
                
                // Respawn the asteroid in a new position far from the player
                float new_x, new_y;
                bool valid_position = false;
                
                // Try to find a position that's far from the player
                for (int attempts = 0; attempts < 10; attempts++) {
                    new_x = random_range(1, WIDTH - 10);
                    new_y = random_range(1, HEIGHT - 10);
                    
                    // Check distance from player
                    float player_dx = new_x - player.x;
                    float player_dy = new_y - player.y;
                    float player_distance = sqrt(player_dx * player_dx + player_dy * player_dy);
                    
                    // If the new position is far enough from the player, use it
                    if (player_distance > 50) {
                        valid_position = true;
                        break;
                    }
                }
                
                // If we couldn't find a valid position, just use a random one
                if (!valid_position) {
                    new_x = random_range(1, WIDTH - 10);
                    new_y = random_range(1, HEIGHT - 10);
                }
                
                // Update asteroid position and speed
                asteroids[j].x = new_x;
                asteroids[j].y = new_y;
                asteroids[j].speed_x = generate_random_speed();
                asteroids[j].speed_y = generate_random_speed();
                
                // Break out of the inner loop since this bullet is now inactive
                break;
            }
        }
    }
}

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
        myAsteroid->x = 0;
    } else if (myAsteroid->x + myAsteroid->width < 0) {
        // If the asteroid goes off the left edge, wrap to the right   
        myAsteroid->x = WIDTH;
    }

    // Handle wrapping around the screen vertically   
    // Check if the asteroid is completely below the screen
    if (myAsteroid->y + myAsteroid->height > HEIGHT) {
        // Wrap to the top of the screen
        myAsteroid->speed_y = -myAsteroid->speed_y;
    } 
    // Check if the asteroid is completely above the screen
    else if (myAsteroid->y < 0) {
        // Wrap to the bottom of the screen
        myAsteroid->speed_y = -myAsteroid->speed_y;
    }
    
    
    // Convert float position to int for drawing
    int draw_x = (int)myAsteroid->x;
    int draw_y = (int)myAsteroid->y;
    
    // Redraw the asteroid at its new position
    draw_rect(pixel_buf, draw_x, draw_y, myAsteroid->width, myAsteroid->height, 86, 107, 114);
}

// Function to draw a player (ship-shaped)
void draw_player(char *pixel_buf, float x, float y, float rotation) {
    // Convert float position to int for drawing
    int draw_x = (int)x;
    int draw_y = (int)y;
    
    // Convert rotation to radians
    float rad = rotation * M_PI / 180.0f;
    
    // Calculate rotated points for the ship triangle
    // Ship is a triangle pointing up
    int size = 8; // Size of the ship
    int points[3][2];
    
    // Calculate the three points of the triangle
    // Front point (nose)
    points[0][0] = draw_x + (int)(size * sin(rad));
    points[0][1] = draw_y - (int)(size * cos(rad));
    
    // Back left point
    points[1][0] = draw_x + (int)(size * sin(rad + 2.61799f)); // 150 degrees
    points[1][1] = draw_y - (int)(size * cos(rad + 2.61799f));
    
    // Back right point
    points[2][0] = draw_x + (int)(size * sin(rad - 2.61799f)); // -150 degrees
    points[2][1] = draw_y - (int)(size * cos(rad - 2.61799f));
    
    // Draw the ship triangle (white color)
    for (int i = 0; i < 3; i++) {
        int next = (i + 1) % 3;
        // Draw line between points[i] and points[next]
        int x0 = points[i][0];
        int y0 = points[i][1];
        int x1 = points[next][0];
        int y1 = points[next][1];
        
        // Simple line drawing algorithm
        int dx = abs(x1 - x0);
        int dy = abs(y1 - y0);
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;
        int err = dx - dy;
        
        while (true) {
            if (x0 >= 0 && x0 < WIDTH && y0 >= 0 && y0 < HEIGHT) {
                draw_rect(pixel_buf, x0, y0, 1, 1, 127, 127, 127);
            }
            
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 > -dy) { err -= dy; x0 += sx; }
            if (e2 < dx) { err += dx; y0 += sy; }
        }
    }
}

// Function to draw a simple character using rectangles
void draw_char(char *pixel_buf, int x, int y, char c, char r, char g, char b) {
    // Simple 5x7 pixel font for ASCII characters
    // This is a very basic implementation that only handles a few characters
    // For a full implementation, you would need a complete font bitmap
    
    switch (c) {
        case '0':
            draw_rect(pixel_buf, x, y, 5, 1, r, g, b);
            draw_rect(pixel_buf, x, y, 1, 7, r, g, b);
            draw_rect(pixel_buf, x+4, y, 1, 7, r, g, b);
            draw_rect(pixel_buf, x, y+6, 5, 1, r, g, b);
            break;
        case '1':
            draw_rect(pixel_buf, x+2, y, 1, 7, r, g, b);
            // draw_rect(pixel_buf, x+1, y+1, 3, 1, r, g, b);
            break;
        case '2':
            draw_rect(pixel_buf, x, y, 5, 1, r, g, b);      // Top horizontal
            draw_rect(pixel_buf, x+4, y+1, 1, 2, r, g, b);  // Right vertical at top
            draw_rect(pixel_buf, x, y+3, 5, 1, r, g, b);    // Middle horizontal
            draw_rect(pixel_buf, x, y+4, 1, 2, r, g, b);    // Left vertical at bottom
            draw_rect(pixel_buf, x, y+6, 5, 1, r, g, b);    // Bottom horizontal
            break;
        case '3':
            draw_rect(pixel_buf, x, y, 5, 1, r, g, b);
            draw_rect(pixel_buf, x+4, y+1, 1, 5, r, g, b);
            draw_rect(pixel_buf, x, y+6, 5, 1, r, g, b);
            draw_rect(pixel_buf, x, y+3, 5, 1, r, g, b);
            break;
        case '4':
            draw_rect(pixel_buf, x, y, 1, 4, r, g, b);      // Left vertical line (top portion)
            draw_rect(pixel_buf, x+4, y, 1, 7, r, g, b);    // Right vertical line (full height)
            draw_rect(pixel_buf, x, y+3, 5, 1, r, g, b);    // Middle horizontal line
            break;
        case '5':
            draw_rect(pixel_buf, x, y, 5, 1, r, g, b);
            draw_rect(pixel_buf, x, y+1, 1, 2, r, g, b);
            draw_rect(pixel_buf, x, y+3, 5, 1, r, g, b);
            draw_rect(pixel_buf, x+4, y+4, 1, 2, r, g, b);
            draw_rect(pixel_buf, x, y+6, 5, 1, r, g, b);
            break;
        case '6':
            draw_rect(pixel_buf, x, y, 5, 1, r, g, b);
            draw_rect(pixel_buf, x, y+1, 1, 5, r, g, b);
            draw_rect(pixel_buf, x, y+3, 5, 1, r, g, b);
            draw_rect(pixel_buf, x+4, y+4, 1, 2, r, g, b);
            draw_rect(pixel_buf, x, y+6, 5, 1, r, g, b);
            break;
        case '7':
            draw_rect(pixel_buf, x, y, 5, 1, r, g, b);
            draw_rect(pixel_buf, x+4, y+1, 1, 5, r, g, b);
            break;
        case '8':
            draw_rect(pixel_buf, x, y, 5, 1, r, g, b);
            draw_rect(pixel_buf, x, y+1, 1, 5, r, g, b);
            draw_rect(pixel_buf, x+4, y+1, 1, 5, r, g, b);
            draw_rect(pixel_buf, x, y+3, 5, 1, r, g, b);
            draw_rect(pixel_buf, x, y+6, 5, 1, r, g, b);
            break;
        case '9':
            draw_rect(pixel_buf, x, y, 5, 1, r, g, b);
            draw_rect(pixel_buf, x, y+1, 1, 2, r, g, b);
            draw_rect(pixel_buf, x+4, y+1, 1, 5, r, g, b);
            draw_rect(pixel_buf, x, y+3, 5, 1, r, g, b);
            draw_rect(pixel_buf, x, y+6, 5, 1, r, g, b);
            break;
    case 'S':
        draw_rect(pixel_buf, x, y, 5, 1, r, g, b);      // Top horizontal
        draw_rect(pixel_buf, x, y+1, 1, 2, r, g, b);    // Left vertical (top portion)
        draw_rect(pixel_buf, x, y+3, 5, 1, r, g, b);    // Middle horizontal
        draw_rect(pixel_buf, x+4, y+4, 1, 2, r, g, b);  // Right vertical (bottom portion)
        draw_rect(pixel_buf, x, y+6, 5, 1, r, g, b);    // Bottom horizontal
        break;

    case 'c':
        // Current implementation shows a full box, but lowercase 'c' should be open on the right top
        draw_rect(pixel_buf, x+1, y, 4, 1, r, g, b);    // Top horizontal (slightly indented)
        draw_rect(pixel_buf, x, y+1, 1, 5, r, g, b);    // Left vertical
        draw_rect(pixel_buf, x+1, y+6, 4, 1, r, g, b);  // Bottom horizontal
        draw_rect(pixel_buf, x+4, y+1, 1, 1, r, g, b);  // Small top-right mark
        draw_rect(pixel_buf, x+4, y+5, 1, 1, r, g, b);  // Small bottom-right mark
        break;

    case 'o':
        // 'o' should be a complete oval/rectangle without openings
        draw_rect(pixel_buf, x+1, y, 3, 1, r, g, b);    // Top horizontal
        draw_rect(pixel_buf, x, y+1, 1, 5, r, g, b);    // Left vertical
        draw_rect(pixel_buf, x+1, y+6, 3, 1, r, g, b);  // Bottom horizontal
        draw_rect(pixel_buf, x+4, y+1, 1, 5, r, g, b);  // Right vertical
        break;

    case 'r':
        // 'r' should have a stem and a hook at the top right
        draw_rect(pixel_buf, x, y, 1, 7, r, g, b);      // Left vertical (full height)
        draw_rect(pixel_buf, x+1, y, 3, 1, r, g, b);    // Top horizontal
        draw_rect(pixel_buf, x+4, y+1, 1, 2, r, g, b);  // Right vertical (small hook)
        break;

        case 'e':
        // 'e' has issues with vertical positions for right segments
        draw_rect(pixel_buf, x+1, y, 3, 1, r, g, b);    // Top horizontal
        draw_rect(pixel_buf, x, y+1, 1, 5, r, g, b);    // Left vertical
        draw_rect(pixel_buf, x+1, y+3, 3, 1, r, g, b);  // Middle horizontal
        draw_rect(pixel_buf, x+1, y+6, 3, 1, r, g, b);  // Bottom horizontal
        draw_rect(pixel_buf, x+4, y+1, 1, 2, r, g, b);  // Right vertical (top portion)
        draw_rect(pixel_buf, x+4, y+4, 1, 2, r, g, b);  // Right vertical (bottom portion)
        break;



        case ':':
            draw_rect(pixel_buf, x+2, y+2, 1, 1, r, g, b);
            draw_rect(pixel_buf, x+2, y+4, 1, 1, r, g, b);
            break;
        case ' ':
            // Space character - do nothing
            break;
    }
}

// Function to draw a string using the pixel buffer
void draw_text(char *pixel_buf, int x, int y, const char *text, char r, char g, char b) {
    int char_width = 6; // Width of each character including spacing
    int pos_x = x;
    
    for (int i = 0; text[i] != '\0'; i++) {
        draw_char(pixel_buf, pos_x, y, text[i], r, g, b);
        pos_x += char_width;
    }
}

// Runs once at startup
void init(char *pixel_buf, int *ind) {
    // Set a fixed seed for reproducibility
    init_random(54321);
    
    // Set background to black
    draw_rect(pixel_buf, 0, 0, WIDTH, HEIGHT, 0, 0, 0);
    
    // Initialize the asteroid with random speeds
    for (int i = 0; i < MAX_ASTEROIDS; i++) {
        float rand_x = random_range(1, WIDTH - 10);
        float rand_y = random_range(1, HEIGHT - 10);
        float rand_speed_x = generate_random_speed();
        float rand_speed_y = generate_random_speed();
        init_asteroid(pixel_buf, rand_x, rand_y, 10, 10, rand_speed_x, rand_speed_y);
    }
    
    // Draw the player at the center of the screen
    draw_player(pixel_buf, WIDTH / 2, HEIGHT / 2, 0);
    
    // Init ind
    *ind = 0;
}

EVENTS control_handler(EVENTS *key_events) {
    char buffer[CONTROLLER_MESSAGE_LENGTH + 1]; // +1 for null

    for (int i = 0; i < CONTROLLER_MESSAGE_LENGTH; ++i) {
        buffer[i] = (char)multicore_fifo_push_blocking();
    }

    int x = ((int)buffer[0])100 + ((int)buffer[1])10 + (int)buffer[2];
    int y = ((int)buffer[3])100 + ((int)buffer[4])*10 + (int)buffer[5];
    bool shoot_button = (buffer[6] == '1');
    bool thrust_button = (buffer[7] == '1');


   if(x > 20) {
        key_events->right_flag = true;
    } else if (x < -20) {
        key_events->left_flag = true;
    } else {
        key_events->right_flag = false;
        key_events->left_flag = false;
    }


    key_events->shoot_flag = shoot_button;
    key_events->thrust_flag = thrust_button;
}

void handle_events(EVENTS *key_events) {

    float rad = player.rotation * M_PI / 180.0f;
        
    if (key_events->left_flag) {
        player.rotation -= ROTATION_SPEED;
    }
    if (key_events->right_flag) {
        player.rotation += ROTATION_SPEED;
    }
    if (key_events->thrust_flag) {
        // Apply thrust in the direction the ship is facing
        player.velocity_x += sin(rad) * THRUST_ACCELERATION;
        player.velocity_y -= cos(rad) * THRUST_ACCELERATION;
    }
    if (key_events->shoot_flag) {
        // Calculate the position at the front of the ship
        float ship_size = 8.0f; // Same as in draw_player
        float bullet_x = player.x + sin(rad) * ship_size;
        float bullet_y = player.y - cos(rad) * ship_size;
        
        // Create a bullet at the front of the ship
        create_bullet(bullet_x, bullet_y, player.rotation);
    }
}

void update(char *pixel_buf, int *ind, struct AppContext* app) {
    // Clear the screen
    draw_rect(pixel_buf, 0, 0, WIDTH, HEIGHT, 0, 0, 0);

    struct EVENTS key_events;
    EVENTS events = control_handler(&key_events);
    handle_events(&key_events);
    
    // Update player position
    player.x += player.velocity_x;
    player.y += player.velocity_y;
    
    // Apply friction
    player.velocity_x *= FRICTION;
    player.velocity_y *= FRICTION;
    
    // Handle player wrapping around the screen horizontally
    if (player.x > WIDTH) {
        player.x = 0;
    } else if (player.x < 0) {
        player.x = WIDTH;
    }
    
    // Prevent player from going off the screen vertically
    // Add a small margin to account for the player's size
    const int player_margin = 10;
    if (player.y > HEIGHT - player_margin) {
        player.y = HEIGHT - player_margin;
        // Bounce off the bottom by reversing vertical velocity
        player.velocity_y = -player.velocity_y * 0.5f;
    } else if (player.y < player_margin) {
        player.y = player_margin;
        // Bounce off the top by reversing vertical velocity
        player.velocity_y = -player.velocity_y * 0.5f;
    }
    
    // Update invulnerability timer
    if (player.invulnerable) {
        player.invulnerable_timer--;
        if (player.invulnerable_timer <= 0) {
            player.invulnerable = false;
        }
    }
    
    // Move and redraw all asteroids
    for (int i = 0; i < asteroid_count; i++) {
        move_asteroid(pixel_buf, &asteroids[i]);
        
        // Check for collision with player
        if (!player.invulnerable) {
            float dx = player.x - asteroids[i].x;
            float dy = player.y - asteroids[i].y;
            float distance = sqrt(dx * dx + dy * dy);
            if (distance < 10) { // Simple collision detection
                player.lives--;
                printf("Collision! Lives remaining: %d\n", player.lives);
                
                // Set invulnerability period (1000 frames)
                player.invulnerable = true;
                player.invulnerable_timer = 1000;
                
                if (player.lives <= 0) {
                    player.lives = 5;
                    player.score = 0;
                    player.x = WIDTH / 2;
                    player.y = HEIGHT / 2;
                    player.velocity_x = 0;
                    player.velocity_y = 0;
                    player.rotation = 0;             
                }
                
                // Break out of the loop to prevent multiple collisions in the same frame
                break;
            }
        }
    }
    
    // Update and draw bullets
    update_bullets(pixel_buf);
    
    // Check for bullet-asteroid collisions
    check_bullet_collisions(pixel_buf);
    
    // Draw the player
    // Flash the player when invulnerable
    if (!player.invulnerable || (player.invulnerable_timer / 5) % 2 == 0) {
        draw_player(pixel_buf, player.x, player.y, player.rotation);
    }
    
    // Draw score
    char score_text[32];
    sprintf(score_text, "Score: %d", player.score);
    
    // Draw lives as rectangles in top-right corner
    const int life_rect_size = 8;
    const int life_rect_spacing = 2;
    const int life_rect_y = 5;
    
    for (int i = 0; i < player.lives; i++) {
        int life_rect_x = WIDTH - (i + 1) * (life_rect_size + life_rect_spacing);
        draw_rect(pixel_buf, life_rect_x, life_rect_y, life_rect_size, life_rect_size, 127, 127, 127);
    }
    
    // Draw score text in top-left corner using our custom text drawing function
    draw_text(pixel_buf, 10, 10, score_text, 127, 127, 127);
}


///////////////////
// Ignore below here, this is all example code 
// so you can see what your code is doing.
// Unless you want to add more game state
///////////////////

SDL_AppResult SDL_Fail(){
    SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
    return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    // init the library, here we make a window so we only need the Video capabilities.
    if (!SDL_Init(SDL_INIT_VIDEO)){
        return SDL_Fail();
    }
    
    // create a window
    SDL_Window* window = SDL_CreateWindow("SDL Minimal Sample", WIDTH * PIXEL_SIZE, HEIGHT * PIXEL_SIZE, 0);
    if (!window){
        return SDL_Fail();
    }
    
    // create a renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer){
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
    AppContext* context = new AppContext();
    context->window = window;
    context->renderer = renderer;
    context->texture = texture;
    *appstate = context;

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
    
    if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP) {
        float rad = player.rotation * M_PI / 180.0f;
        
        switch (event->key.scancode) {
            case SDL_SCANCODE_LEFT:
                if (event->type == SDL_EVENT_KEY_DOWN) {
                    player.rotation -= ROTATION_SPEED;
                }
                break;
            case SDL_SCANCODE_RIGHT:
                if (event->type == SDL_EVENT_KEY_DOWN) {
                    player.rotation += ROTATION_SPEED;
                }
                break;
            case SDL_SCANCODE_SPACE:
                if (event->type == SDL_EVENT_KEY_DOWN) {
                    // Apply thrust in the direction the ship is facing
                    player.velocity_x += sin(rad) * THRUST_ACCELERATION;
                    player.velocity_y -= cos(rad) * THRUST_ACCELERATION;
                }
                break;
            case SDL_SCANCODE_UP:
                if (event->type == SDL_EVENT_KEY_DOWN) {
                    // Calculate the position at the front of the ship
                    float ship_size = 8.0f; // Same as in draw_player
                    float bullet_x = player.x + sin(rad) * ship_size;
                    float bullet_y = player.y - cos(rad) * ship_size;
                    
                    // Create a bullet at the front of the ship
                    create_bullet(bullet_x, bullet_y, player.rotation);
                }
                break;
        }
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
    update((char *)pixel_buf, &ind, app);
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
