#define _USE_MATH_DEFINES
#include <SDL2/SDL.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include <iostream>
using namespace std;

void DrawString(SDL_Surface* screen, int x, int y, const char* text, SDL_Surface* charset)
{
    int      px, py, c;
    SDL_Rect s, d;
    s.w = 8;
    s.h = 8;
    d.w = 8;
    d.h = 8;
    while (*text) 
	{
        c   = *text & 255;
        px  = (c % 16) * 8;
        py  = (c / 16) * 8;
        s.x = px;
        s.y = py;
        d.x = x;
        d.y = y;
        SDL_BlitSurface(charset, &s, screen, &d);
        x += 8;
        text++;
    };
};

void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y)
{
    SDL_Rect dest;
    dest.x = x - sprite->w / 2;
    dest.y = y - sprite->h / 2;
    dest.w = sprite->w;
    dest.h = sprite->h;
    SDL_BlitSurface(sprite, NULL, screen, &dest);
};

void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color)
{
    if (x < 0 || x >= surface->w || y < 0 || y >= surface->h)
        return;
    
    int    bpp  = surface->format->BytesPerPixel;
    Uint8* p    = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
    *(Uint32*)p = color;
};

void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color)
{
    for (int i = 0; i < l; i++) 
	{
        DrawPixel(screen, x, y, color);
        x += dx;
        y += dy;
    };
};

void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor)
{
    int i;
    DrawLine(screen, x, y, k, 0, 1, outlineColor);
    DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
    DrawLine(screen, x, y, l, 1, 0, outlineColor);
    DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
    for (i = y + 1; i < y + k - 1; i++)
        DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};

#define SCREEN_WIDTH 				 640
#define SCREEN_HEIGHT 				 480
#define SCALE 						 32
#define UNICORN_WIDTH_STANDARDIZED 	 2
#define UNICORN_HEIGHT_STANDARDIZED  1
#define PLATFORM_HEIGHT_STANDARDIZED 1
#define UNICORN_WIDTH_PX 			 64
#define UNICORN_HEIGHT_PX            32
#define GRAVITY                      15
#define DASH_SPEED_MULT 			 2

class Unicorn 
{
public:
    double x, y, dx, dy;
	double dash_time;
    Unicorn(double xx, double yy) 
	{
        x  = xx;
		y  = yy;
		dx = 0;
		dy = 0;
		dash_time = 0;
	}
};

class Platform 
{
public:
    double x, y, w, h;
    Platform(double xx, double yy, double ww, double hh)
	{
		x = xx;
		y = yy;
		w = ww;
		h = hh;
	}
};

class Level 
{
public:
    int       		 platform_count;
    vector<Platform> platforms;

    Level(unsigned platform_count_, vector<Platform> platforms_) 
	{
		platform_count = platform_count_;
		platforms 	   = platforms_;
	}   
};

enum Movement 
{
    AUTO,
    ARROWS
};

class GameState 
{
public:
	Unicorn* unicorn;
	Level*   level;
    double   time;
    bool     endGame;
	Movement movement;	 

    GameState(Level* lvl) 
	{
		unicorn = new Unicorn(0,3);
		time = 0;
		endGame = false;
		level = lvl;
		movement = AUTO;
	}

    void update(double timestep) 
	{

        // Update position
        unicorn->x += timestep * unicorn->dx;
		unicorn->y += timestep * unicorn->dy;

        // Update gravity (unless unicorn in dash)
		unicorn->dy -= GRAVITY * timestep * (unicorn->dash_time == 0);

		// Auto increment x velocity (if AUTO mode selected)
		if (movement == AUTO)
			unicorn->dx = 2 + time;

        // Multiply x velocity if unicorm is dashing
        if (unicorn->dash_time > 0) 
		{
            unicorn->dx *= DASH_SPEED_MULT;
			unicorn->dash_time -= 0.02;
        }
        if (unicorn->dash_time < 0)
			unicorn->dash_time = 0;
		
        // Collisions with platforms
        for (int i = 0; i < level->platform_count; i++) 
		{
			Platform p = level->platforms[i];

            if (p.x > unicorn->x + UNICORN_WIDTH_STANDARDIZED
			 || p.y < unicorn->y - UNICORN_HEIGHT_STANDARDIZED
             || p.x + p.w < unicorn->x
             || p.y - p.h > unicorn->y) 
			{
                continue; // No collision between unicorn and the platform
            }

            if (unicorn->y + unicorn->x - (p.y + p.x) >= 1 - UNICORN_WIDTH_STANDARDIZED) 
			{
                // Unicorn landed on platform from above
                unicorn->y = p.y + UNICORN_HEIGHT_STANDARDIZED;
                unicorn->dy = 0;	
            } 
            else 
			{
                // otherwise: unicorn is dead
                endGame = true;
                SDL_Delay(1000);
            }
        }
    }
	void restart() 
	{
		time = 0;
		unicorn->x = 0;
		unicorn->y = 3;
		unicorn->dy = 0;
		unicorn->dx = 0;
	}
};

int main(int argc, char** argv) 
{
    int           t1, t2, exit, frames, rc, n;
    double        delta, fpsTimer, fps, distance, etiSpeed;
	double 		  px, py, pw, ph;
    SDL_Event     event;
    SDL_Surface*  screen;
	SDL_Surface*  charset;
    SDL_Texture*  scrtex;
    SDL_Window*   window;
    SDL_Renderer* renderer;

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) 
	{
        printf("SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    rc = SDL_CreateWindowAndRenderer(
        SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);
    if (rc != 0) 
	{
        SDL_Quit();
        printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
        return 1;
    };

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_SetWindowTitle(window, "Unicorn Attack");

    screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

	char text[128];
    charset = SDL_LoadBMP("./assets/cs8x8.bmp");
    SDL_SetColorKey(charset, true, 0xFF);

    SDL_ShowCursor(SDL_DISABLE);
    t1       = SDL_GetTicks();
    frames   = 0;
    fpsTimer = 0;
    fps      = 0;
    exit     = 0;

	int RED   = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
    int PINK  = SDL_MapRGB(screen->format, 253,185,200);
    int BLACK = SDL_MapRGB(screen->format, 0,0,0);
	int WHITE = SDL_MapRGB(screen->format, 255,255,255);

    // Transfer data from file to standard stream
	freopen("lvl1.txt", "r", stdin);
	
    // Read platform position and dimensions
    vector<Platform> platforms;
	while (cin >> px >> py >> pw >> ph)
		platforms.push_back(Platform(px, py, pw, ph));

    GameState gs(new Level(platforms.size(), platforms));

    while (!exit && !gs.endGame) 
	{

        // Update time and timestep
        t2    = SDL_GetTicks();
        delta = (t2 - t1) * 0.001;
        t1    = t2;
        gs.time += delta;

        // Update fps
        fpsTimer += delta;
        if (fpsTimer > 0.5) 
		{
            fps    = frames * 2;
            frames = 0;
            fpsTimer -= 0.5;
        };

        // Update state of game variables
        gs.update(delta);

        // Clear display
        SDL_FillRect(screen, NULL, WHITE);

        // Draw Unicorn
        DrawRectangle(screen, SCALE, (SCREEN_HEIGHT-UNICORN_HEIGHT_PX)/2, UNICORN_WIDTH_PX, UNICORN_HEIGHT_PX, RED, RED);

        // Draw Platforms
        for (int i = 0; i < gs.level->platform_count; i++) 
		{
			Platform p = gs.level->platforms[i];
            // Transform platform position from standardized to pixel format,
            // and make it relative to the camera
            px = p.x - gs.unicorn->x + 1;
            py = (gs.unicorn->y + (screen->h / (double)SCALE - (double)UNICORN_HEIGHT_STANDARDIZED)/2 - p.y);

            DrawRectangle(screen, px*SCALE, py*SCALE, p.w*SCALE, p.h*SCALE, BLACK, PINK);
		}

		// Draw information in top box
		sprintf(text, "position(%.2f, %.2f) speed(%.2f, %.2f) dash: %.2f fps: %f", gs.unicorn->x, gs.unicorn->y, gs.unicorn->dx, gs.unicorn->dy, gs.unicorn->dash_time, fps);
        DrawString(screen, 4, 4, text, charset);

        SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
        SDL_RenderCopy(renderer, scrtex, NULL, NULL);
        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&event)) 
		{
            switch (event.type) 
			{
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    exit = 1;
                else if (event.key.keysym.sym == SDLK_UP)
                    gs.unicorn->dy = 10;
                else if (event.key.keysym.sym == SDLK_n)
                    gs.restart();
				else if (event.key.keysym.sym == SDLK_d) 
				{
					if (gs.movement == AUTO) 
					{
						gs.movement = ARROWS;
					}
					else
						gs.movement = AUTO;
                    gs.unicorn->dx = 0;
				}
                else if (event.key.keysym.sym == SDLK_RIGHT && gs.movement == ARROWS)
                    gs.unicorn->dx += 2;
				else if (event.key.keysym.sym == SDLK_LEFT && gs.movement == ARROWS)
                    gs.unicorn->dx -= 2;
				else if (event.key.keysym.sym == SDLK_x)
                    gs.unicorn->dash_time = 1;

                break;
            case SDL_QUIT:
                exit = 1;
                break;
            };
        };
        frames++;
    };

    SDL_FreeSurface(screen);
	SDL_FreeSurface(charset);
    SDL_DestroyTexture(scrtex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
};
