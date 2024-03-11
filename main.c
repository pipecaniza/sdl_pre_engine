#ifdef __MINGW32__
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "eq_platform.h"

//#define _LINUX_
#ifdef _LINUX_
	#include <sys/stat.h>
#endif

#define _WIN_
#ifdef _WIN_
	#include <windows.h>
	#include <stdio.h>
#endif

// Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

/*
	TODO(pipecaniza):
		* Test Draw a rectangle......Done
		* Test sin wave sound?
		* Load and unload lib........Done (Linux)
		* Define types...............Done
		* lock frames................Done

*/

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float r32;
typedef double r64;
typedef s8 b8;
typedef s32 b32;

#define false 0
#define true 1


#define function static

struct {
	void *objectHandler;
	GameUpdateAndRender *updateAndRender;
} typedef sdl_GameCode;

struct {
	char *path;
	s64 lastModificationTime;
} typedef sdl_FileInfo;


function b32
CheckIfFileChanged(sdl_FileInfo *fileInfo) 
{
	b32 result = false;

#ifdef _LINUX_
	struct stat fileStat;
    b32 statResult = stat(fileInfo->path, &fileStat);
    if (statResult == 0) {
		if (fileInfo->lastModificationTime != fileStat.st_mtim.tv_sec) {
			fileInfo->lastModificationTime = fileStat.st_mtim.tv_sec;
			result = true;
		}
    } else {
		printf("Unable to find the file: %s\n", fileInfo->path);
	}
#endif
#ifdef _WIN_
	WIN32_FILE_ATTRIBUTE_DATA fileAttr;
	b32 fileResult = GetFileAttributesExA(fileInfo->path, GetFileExInfoStandard, &fileAttr);
	if (fileResult) {
		FILETIME fileTime = fileAttr.ftLastWriteTime;
		ULARGE_INTEGER largeInt;
		largeInt.LowPart = fileTime.dwLowDateTime;
		largeInt.HighPart = fileTime.dwHighDateTime;
		if (fileInfo->lastModificationTime != largeInt.QuadPart) {
			fileInfo->lastModificationTime = largeInt.QuadPart;
			result = true;
		}
	}
#endif	
    return result;
}

function sdl_GameCode
LoadGameCode(char *path, char *functionName)
{
	sdl_GameCode result = {};
	result.objectHandler = SDL_LoadObject(path);
	if (!result.objectHandler) {
		printf("SDL could not locate the OS object: %s\n", SDL_GetError());
	} else {
		result.updateAndRender = (GameUpdateAndRender *)SDL_LoadFunction(result.objectHandler, functionName);
		if (!result.updateAndRender) {
			printf("SDL could not the function in the OS object: %s\n", SDL_GetError());
		}
	}
	return result;
}

function void
UnLoadGameCode(sdl_GameCode *gameCode)
{	
	if (gameCode && gameCode->objectHandler) {
		SDL_UnloadObject(gameCode->objectHandler);
	}
}



function void
DrawRect(void *pixels, s32 pitch, s32 x, s32 y, s32 h, s32 w, s32 ARGB)
{
	for (s32 currentX = 0; currentX < w; ++currentX) {
		for (s32 currentY = 0; currentY < h; ++currentY) {
			s32 *currentPixel = (s32*)((u8*)pixels + ((x + currentX) * sizeof(ARGB)) + ((y + currentY) * pitch));
			//s32 *CurrentPixel = (s32*)((s32*)Pixels + ((x + CurrentX)) + ((y + CurrentY) * Pitch / sizeof(s32)));
			*currentPixel = ARGB;
		}
	}
}

int main(int argc, char* args[])
{
	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO ) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
	}
	else
	{
		// Create window
		SDL_Window* window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (window == NULL)
		{
			printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		}
		else
		{
			// SDL_PIXELFORMAT_ARGB8888
			// SDL_PIXELTYPE_PACKED32
			// SDL_PACKEDORDER_ARGB
			// SDL_PACKEDLAYOUT_8888
			SDL_Renderer *renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED );
			SDL_RendererInfo info = {};
			int result = SDL_GetRendererInfo(renderer, &info);
			SDL_SetRenderDrawColor( renderer, 0xFF, 0xFF, 0xFF, 0xFF );
			// The surface contained by the window
			SDL_Surface* screenSurface = SDL_GetWindowSurface(window);
			SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 
								SCREEN_WIDTH, SCREEN_HEIGHT);
			SDL_Event windowEvent;			
			//SDL_Rect rect = {0, 0, 50, 50};

			//C:\\Users\\pipec\\OneDrive\\Documentos\\source\\sdl_engine_v1\\build\\win\\engine.dll
			//"/home/pipecaniza/Source/sdl_engine_v1/build/linux/engine.so"
			sdl_FileInfo gameCodeFileInfo = (sdl_FileInfo){"C:\\Users\\pipec\\OneDrive\\Documentos\\source\\sdl_engine_v1\\build\\win\\engine.dll", 0};
			sdl_GameCode gameCode = {};

			int color = 0;
			while (true) 
			{
				r64 lastTick = SDL_GetTicks64();

				if (CheckIfFileChanged(&gameCodeFileInfo))
				{
					UnLoadGameCode(&gameCode);
					gameCode = LoadGameCode(gameCodeFileInfo.path, "game_GameUpdateAndRender");
				}


				while (SDL_PollEvent(&windowEvent))
				{
					switch (windowEvent.type) {
						case SDL_QUIT:
							SDL_Quit();
							return 0;
					} 
				} 
				//++color;

				if(gameCode.updateAndRender)
				{
					gameCode.updateAndRender();
				}


				void *pixels;
				int pitch;
				SDL_LockTexture(texture, 0, &pixels, &pitch);
				//rect.x = (rect.x + 1) % SCREEN_WIDTH;
				//rect.y = (rect.y + 1) % SCREEN_HEIGHT;

				//SDL_DrawRect(screenSurface, NULL, SDL_MapRGB( screenSurface->format, 255, 255, 255));
				//SDL_DrawRect(screenSurface, &rect, SDL_MapRGB( screenSurface->format, 0, 0, 255));
				//SDL_UpdateWindowSurface(window);

				//SDL_memset(pixels, color, SCREEN_HEIGHT * pitch);
				DrawRect(pixels, pitch, 10, 20, 100, 100, 0xAA0000FF);
				DrawRect(pixels, pitch, 40, 40, 100, 100, 0x05FF00FF);
				SDL_UpdateTexture(texture, 0, pixels, pitch);

    			SDL_UnlockTexture(texture);
				

				SDL_RenderClear( renderer );

                //Render texture to screen
                SDL_RenderCopy( renderer, texture, NULL, NULL );

                //Update screen
                SDL_RenderPresent( renderer );

				{
					r32 workSecondsElapsed = (r32)(SDL_GetTicks64() - lastTick) / 1000.0f;
					r32 expectedFramesPerSeconds = 30.0f;
					r32 expectedSecondsPerFrame = 1.0f / expectedFramesPerSeconds;

					if (workSecondsElapsed < expectedSecondsPerFrame)
					{
						u32 millisecondsToWait = (expectedSecondsPerFrame - workSecondsElapsed) * 1000.0f;
						printf("To Wait:%d\n", millisecondsToWait);
						SDL_Delay((u32)millisecondsToWait);
					}


					r64 endTick = SDL_GetTicks64();
					printf("tick: %.5f\n", (endTick - lastTick) / 1000.0f);

					lastTick = endTick;
				}
			}	
		}
		// Destroy window
		SDL_DestroyWindow(window);
	}

	//Quit SDL subsystems
	SDL_Quit();

	return 0;
}
