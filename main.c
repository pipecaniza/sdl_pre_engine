#ifdef _WIN64
 #include <SDL.h>
 #include <windows.h>
 #include <WinBase.h>
#else
 #include <SDL2/SDL.h>
 #include <sys/stat.h>
#endif

#include "eq_platform.h"

//todo(pipecaniza): remove this
#include <math.h>
#include <stdint.h>
#include <stdio.h>

// Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

/*
	TODO(pipecaniza):
		* Test Draw a rectangle......Done
		* Test sin wave sound?.......
		* Load and unload lib........Done (Linux and Windows, pending copy lib to avoid compilation errors)
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
#define Pi32 3.14159265359

#define local_persist static
#define global_persist static
#define function static

struct {
	void *objectHandler;
	GameUpdateAndRender *updateAndRender;
} typedef sdl_GameCode;

struct {
	char *path;
	s64 lastModificationTime;
} typedef sdl_FileInfo;



struct {
	u8 channels;
	u8 formatSizeInBytes;
	u16 samples;
	u16 frequency;
} typedef platform_AudioOutput;

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

s32 main(s32 argc, char* args[])
{
	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
	} else {
		// Create window
		SDL_Window* window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (window == NULL) {
			printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		} else {
			// SDL_PIXELFORMAT_ARGB8888
			// SDL_PIXELTYPE_PACKED32
			// SDL_PACKEDORDER_ARGB
			// SDL_PACKEDLAYOUT_8888
			SDL_Renderer *renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED );
			SDL_RendererInfo info = {};
			s32 result = SDL_GetRendererInfo(renderer, &info);
			SDL_SetRenderDrawColor( renderer, 0xFF, 0xFF, 0xFF, 0xFF );
			// The surface contained by the window
			SDL_Surface* screenSurface = SDL_GetWindowSurface(window);
			SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 
								SCREEN_WIDTH, SCREEN_HEIGHT);
			SDL_Event windowEvent;			
			//SDL_Rect rect = {0, 0, 50, 50};

			r32 expectedFramesPerSeconds = 30.0f;

			/* Audio test init */
			platform_AudioOutput audioOutput = {};
			audioOutput.frequency = 48000;
			audioOutput.channels = 1;
			audioOutput.formatSizeInBytes = sizeof(u16);
			audioOutput.samples = audioOutput.channels * audioOutput.formatSizeInBytes;//check this

			SDL_AudioSpec desiredAudioSpec = {};

			int r = SDL_GetAudioDeviceSpec(0, 0, &desiredAudioSpec);

			SDL_AudioSpec obtainedAudioSpec = {};
			SDL_AudioDeviceID audioDeviceId;
			{
				desiredAudioSpec.freq = audioOutput.frequency;
				desiredAudioSpec.format = AUDIO_S16SYS;
				desiredAudioSpec.channels = audioOutput.channels;
				desiredAudioSpec.samples = audioOutput.samples;
				audioDeviceId = SDL_OpenAudioDevice(NULL, 0, &desiredAudioSpec, &obtainedAudioSpec, 0);
				SDL_PauseAudioDevice(audioDeviceId, 0);
			}		

			//s32 audioFormatInBytes = SDL_AUDIO_BITSIZE(desiredAudioSpec.format) / 8;
			u32 audioBufferSizeInBytes = audioOutput.frequency / expectedFramesPerSeconds * audioOutput.channels * audioOutput.formatSizeInBytes*2;
			//todo(pipecaniza): change this to use an arena
			u16 *audioBuffer = (u16 *)malloc(audioBufferSizeInBytes);
			u32 audioBufferSize = audioBufferSizeInBytes / sizeof(u16);

			//C:\\Users\\pipec\\OneDrive\\Documentos\\source\\sdl_engine_v1\\build\\win\\engine.dll
			//"/home/pipecaniza/Source/sdl_engine_v1/build/linux/engine.so"
			sdl_FileInfo gameCodeFileInfo = (sdl_FileInfo){"C:\\Users\\pipec\\OneDrive\\Documentos\\source\\sdl_engine_v1\\build\\win\\engine.dll", 0};
			sdl_GameCode gameCode = {};

			s32 color = 0;
			while (true) {
				r64 lastTick = SDL_GetTicks64();
				printf("Early frame AudioSize: %i\n",  SDL_GetQueuedAudioSize(audioDeviceId));

				if (CheckIfFileChanged(&gameCodeFileInfo)) {
					UnLoadGameCode(&gameCode);
					gameCode = LoadGameCode(gameCodeFileInfo.path, "game_GameUpdateAndRender");
				}

				//printf("%s",SDL_GetError());
				while (SDL_PollEvent(&windowEvent)) {
					switch (windowEvent.type) {
						case SDL_QUIT:
							SDL_Quit();
							return 0;
					} 
				} 
				//++color;

				if(gameCode.updateAndRender) {
					gameCode.updateAndRender();
				}


				void *pixels;
				s32 pitch;
				SDL_LockTexture(texture, 0, &pixels, &pitch);
				//rect.x = (rect.x + 1) % SCREEN_WIDTH;
				//rect.y = (rect.y + 1) % SCREEN_HEIGHT;

				//SDL_DrawRect(screenSurface, NULL, SDL_MapRGB( screenSurface->format, 255, 255, 255));
				//SDL_DrawRect(screenSurface, &rect, SDL_MapRGB( screenSurface->format, 0, 0, 255));
				//SDL_UpdateWindowSurface(window);

				//SDL_memset(pixels, color, SCREEN_HEIGHT * pitch);
				DrawRect(pixels, pitch, 10, 20, 100, 100, 0xAA0000FF);
				DrawRect(pixels, pitch, 40, 40, 100, 100, 0x05FF00FF);
				//SDL_UpdateTexture(texture, 0, pixels, pitch);

    			SDL_UnlockTexture(texture);
				

				SDL_RenderClear( renderer );

                //Render texture to screen
                SDL_RenderCopy( renderer, texture, NULL, NULL );

                //Update screen
                SDL_RenderPresent( renderer );

				
				//Audio
				{
					s32 toneHz = 1000;					
					local_persist r32 tSine;
					s16 toneVolume = 3000;
					s32 wavePeriod = audioOutput.frequency / toneHz;

					u16 *sampleOut = audioBuffer;
					for(s32 sampleIndex = audioBufferSize / audioOutput.channels;
						sampleIndex > 0; // stereo
						--sampleIndex)
					{
						// TODO(casey): Draw this out for people
						r32 sineValue = sinf(tSine);
						s16 sampleValue = (s16)(sineValue * toneVolume);
						*sampleOut++ = sampleValue;
						//*sampleOut++ = sampleValue;

						tSine += 2.0f*Pi32*1.0f/(r32)wavePeriod;
					}

					//u32 queuedAudioSize =;
					printf("Pre queue AudioSize: %i\n",  SDL_GetQueuedAudioSize(audioDeviceId));
					s32 result = SDL_QueueAudio(audioDeviceId, audioBuffer, audioBufferSize);
					if (result < 0) {
						printf("Something went wrong in audio! SDL_Error: %s\n", SDL_GetError());
					}
					printf("After queue AudioSize: %i\n",  SDL_GetQueuedAudioSize(audioDeviceId));			
				}

				{
					r32 workSecondsElapsed = (r32)(SDL_GetTicks64() - lastTick) / 1000.0f;					
					r32 expectedSecondsPerFrame = 1.0f / expectedFramesPerSeconds;

					if (workSecondsElapsed < expectedSecondsPerFrame) {
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
