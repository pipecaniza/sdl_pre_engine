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
#define EQ_INTERNAL //This enable hot reloading
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

// Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

/*
	TODO(pipecaniza):
		* Test Draw a rectangle......Done
		* Test sin wave sound?.......Done
		* Load and unload lib........Done
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
//TODO(pipecaniza): check max path
#define MAX_PATH_LENGTH 260

#define local_persist static
#define global_persist static
#define function static


#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

struct {
	void *objectHandler;
	GameUpdateAndRender *updateAndRender;
} typedef sdl_GameCode;

struct {
	char *path;
	s64 lastModificationTime;
} typedef sdl_FileInfo;


// do i need this?
struct {
	u8 channels;
	u8 formatSizeInBytes;
	u16 samples;
	u16 frequency;
} typedef sdl_AudioOutput;

function b32
sdl_CheckIfFileChanged(sdl_FileInfo *fileInfo) 
{
	b32 result = false;
#ifdef _WIN64
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
#else
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
    return result;
}

function sdl_GameCode
sdl_LoadGameCode(char *pathSourceLib, char *pathTempLib, char *functionName)
{
	#ifdef EQ_INTERNAL
	{
		SDL_RWops *sourceLib = SDL_RWFromFile(pathSourceLib, "rb");
		SDL_RWops *tempLib = SDL_RWFromFile(pathTempLib, "w+b");
		u64 sourceLibSize = SDL_RWsize(sourceLib);
		
		void *sourceLibContent = calloc(sourceLibSize, sizeof(u8));

		SDL_RWread(sourceLib, sourceLibContent, sizeof(u8), sourceLibSize);
		SDL_RWwrite(tempLib, sourceLibContent, sizeof(u8), sourceLibSize);

		SDL_RWclose(sourceLib);
		SDL_RWclose(tempLib);
		free(sourceLibContent);
	}
	#endif

	sdl_GameCode result = {0};
	#ifdef EQ_INTERNAL
	 result.objectHandler = SDL_LoadObject(pathTempLib);
	#else
	 result.objectHandler = SDL_LoadObject(pathSourceLib);
	#endif
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
sdl_UnLoadGameCode(sdl_GameCode *gameCode)
{
	if (gameCode && gameCode->objectHandler) {
		SDL_UnloadObject(gameCode->objectHandler);
	}
}



function void
sdl_DrawRect(void *pixels, s32 pitch, s32 x, s32 y, s32 h, s32 w, s32 ARGB)
{
	for (s32 currentX = 0; currentX < w; ++currentX) {
		for (s32 currentY = 0; currentY < h; ++currentY) {
			s32 *currentPixel = (s32*)((u8*)pixels + ((x + currentX) * sizeof(ARGB)) + ((y + currentY) * pitch));
			//s32 *CurrentPixel = (s32*)((s32*)Pixels + ((x + CurrentX)) + ((y + CurrentY) * Pitch / sizeof(s32)));
			*currentPixel = ARGB;
		}
	}
}

function void
string_Concat(char *bufferA, u64 sizeBufferA, char *bufferB, u64 sizeBufferB, char *bufferDestination, u64 sizeBufferDestination)
{
	assert(sizeBufferA + sizeBufferB <= sizeBufferDestination);
	for(u64 index = 0; index < sizeBufferA; ++index) {
		*bufferDestination++ = *bufferA++;
	}
	for(u64 index = 0; index < sizeBufferB; ++index) {
		*bufferDestination++ = *bufferB++;
	}
    *bufferDestination++ = 0;
}

function u64
string_Size(char *buffer)
{
	u64 bufferSize = 0;
	while (*buffer++) {
		++bufferSize;
	}
	return bufferSize;
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
			
			#ifdef _WIN64
			char *libFileName = "engine.dll";
			char *libTempFileName = "engine_temp.dll";
			#else
			char *libFileName = "engine.so";
			char *libTempFileName = "engine_temp.so";
			#endif
			char *executionPath = SDL_GetBasePath();
			char sourceGameCodeLibFullPath [MAX_PATH_LENGTH];
			string_Concat(executionPath, string_Size(executionPath), libFileName, string_Size(libFileName), sourceGameCodeLibFullPath, MAX_PATH_LENGTH);
			
			char tempGameCodeLibFullPath [MAX_PATH_LENGTH];
			string_Concat(executionPath, string_Size(executionPath), libTempFileName, string_Size(libTempFileName), tempGameCodeLibFullPath, MAX_PATH_LENGTH);

			sdl_FileInfo gameCodeFileInfo = (sdl_FileInfo){ sourceGameCodeLibFullPath, 0 };

			// SDL_PIXELFORMAT_ARGB8888
			// SDL_PIXELTYPE_PACKED32
			// SDL_PACKEDORDER_ARGB
			// SDL_PACKEDLAYOUT_8888
			SDL_Renderer *renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED );
			SDL_RendererInfo info = {0};
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
			sdl_AudioOutput audioOutput = {0};
			audioOutput.frequency = 48000;
			audioOutput.channels = 2;
			audioOutput.formatSizeInBytes = sizeof(s16);
			audioOutput.samples = audioOutput.frequency / expectedFramesPerSeconds * audioOutput.channels;//check this

			SDL_AudioSpec desiredAudioSpec = {0};

			int r = SDL_GetAudioDeviceSpec(0, 0, &desiredAudioSpec);

			SDL_AudioSpec obtainedAudioSpec = {0};
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
			u8 audioFramesLatency = 3;
			u32 audioBufferSizeInBytes = audioOutput.frequency / expectedFramesPerSeconds * audioOutput.channels * audioOutput.formatSizeInBytes*audioFramesLatency;
			//todo(pipecaniza): change this to use an arena
			s16 *audioBuffer = (s16 *)malloc(audioBufferSizeInBytes);
			u32 audioBufferSize = audioBufferSizeInBytes / audioOutput.formatSizeInBytes;

			// Remove this
			u32 wavSize;
			u8 *wavBuffer;
			{
				SDL_AudioSpec wavSpec;
				

				/* Load the WAV */
				SDL_AudioSpec *r = SDL_LoadWAV("test.wav", &wavSpec, &wavBuffer, &wavSize);
				if (!r) {
					printf("Error opening the audio file: %s\n", SDL_GetError());
				} else {
					/* Do stuff with the WAV data, and then... */
					//SDL_FreeWAV(wav_buffer);
				}
			}

			//C:\\Users\\pipec\\OneDrive\\Documentos\\source\\sdl_engine_v1\\build\\win\\engine.dll
			//"/home/pipecaniza/Source/sdl_engine_v1/build/linux/engine.so"
			
			sdl_GameCode gameCode = {0};

			s32 color = 0;
			while (true) {
				r64 lastTick = SDL_GetTicks64();
				printf("Early frame AudioSize: %i\n",  SDL_GetQueuedAudioSize(audioDeviceId));

				if (sdl_CheckIfFileChanged(&gameCodeFileInfo)) {
					sdl_UnLoadGameCode(&gameCode);
					gameCode = sdl_LoadGameCode(sourceGameCodeLibFullPath, tempGameCodeLibFullPath, "game_GameUpdateAndRender");
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
				sdl_DrawRect(pixels, pitch, 10, 20, 100, 100, 0xAA0000FF);
				sdl_DrawRect(pixels, pitch, 40, 40, 100, 100, 0x05FF00FF);
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
					local_persist b8 up = false;
					s16 toneVolume = 3000;
					s32 wavePeriod = audioOutput.frequency / toneHz;

					s32 currentAudioQueuedSizeInBytes = SDL_GetQueuedAudioSize(audioDeviceId);
					s32 pendingAudioToQueueInBytes = audioBufferSizeInBytes - currentAudioQueuedSizeInBytes;
					s32 audioSamplesToQueue = pendingAudioToQueueInBytes / audioOutput.formatSizeInBytes / audioOutput.channels;

					local_persist s16 *currentWavBufferSample = 0;
					if (!currentWavBufferSample)
						currentWavBufferSample = (s16 *)wavBuffer;

					s16 *sampleOut = audioBuffer;
					s8 repFormat = 0;
					s16 left, right;
					for(s32 sampleIndex = audioSamplesToQueue;
						sampleIndex > 0; // stereo
						--sampleIndex)
					{
						// TODO(casey): Draw this out for people
						r32 sineValue = sinf(tSine);
						s16 sampleValue = (s16)(sineValue * toneVolume);
						/*s16 sampleValue = 40 * toneVolume;
						up = (sampleIndex % 40) == 0 ? !up : up;
						if (up)
							*sampleOut++ = 50 * toneVolume;
						else
							*sampleOut++ = -50 * toneVolume;*/
						//*sampleOut++ = sampleValue;

						if (wavBuffer) {
							if (repFormat == 0) {
								left = *currentWavBufferSample++;
								right = *currentWavBufferSample++;
								repFormat = 6;
							}

							*sampleOut++ = left; //two channels (stereo)
							*sampleOut++ = right; //two channels (stereo)
							--repFormat;
							

							if (SDL_abs((s64)currentWavBufferSample - (s64)wavBuffer) >= wavSize) {
								currentWavBufferSample = (s16 *)wavBuffer;
							}
						}

						tSine += 2.0f*Pi32*1.0f/(r32)wavePeriod;
					}

					//u32 queuedAudioSize =;
					printf("Pre queue AudioSize: %i\n",  SDL_GetQueuedAudioSize(audioDeviceId));
					s32 result = SDL_QueueAudio(audioDeviceId, audioBuffer, pendingAudioToQueueInBytes);
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
