#include "eq_platform.h"
//#include <cstdio>


GAME_UPDATE_AND_RENDER(game_GameUpdateAndRender)
{
    printf("FROM NEWWWWW WIN ENGINE!!");
}


/*

struct playing_sound
{
    v2 CurrentVolume;
    v2 dCurrentVolume;
    v2 TargetVolume;

    real32 dSample;

    sound_id ID;
    real32 SamplesPlayed;
    playing_sound *Next;
};

struct audio
{
    Memory_arena *audio;
    Memory_arena *perm_arena;
    playing_sound *firstPlayingSound;
    playing_sound *firstFreePlayingSound;

    v2 masterVolume;
};

struct AudioState
{
    MemoryArena *permArena;
    PlayingSound *firstPlayingSound;
    PlayingSound *firstFreePlayingSound;

    v2 masterVolume;
};


struct coll_audio_state
{
    memory_arena *permArena;
    playing_sound *firstPlayingSound;
    playing_sound *firstFreePlayingSound;

    v2 masterVolume;
};



internal P2D_RendererTexture resources_GetBitmap(GameAssets *assets, BitmapId id); assets->permArena
internal p2d_renderer_texture resources_GetBitmap(game_assets *assets, bitmap_id id); assets->perm_arena



internal P2D_HHABitmap *resources_get_bitmap_info(game_assets *Assets, bitmap_id ID);
internal s16 *resources_GetSoundSamples(game_assets *Assets, sound_id ID);
internal P2D_HHAAsset *resources_GetSoundInfo(game_assets *Assets, sound_id ID);*/