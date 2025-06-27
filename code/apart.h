#if !defined (APART_H)

#include <stdint.h>

#if !defined (COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif

#if !defined(COMPILER_LLVM)
#define COMPILER_LLVM 0
#endif

#if !COMPILER_MSVC && !COMPILER_LLVM
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#endif
#else
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif

#define local_persist static
#define global_variable static
#define internal static

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
#define pi32 3.14159265359f

#if APART_SLOW
#define Assert(expression) if (!(expression)) *(int*)0 = 0;
#else
#define Assert(expression)
#endif

#define Kilobytes(value) ((value) * 1024LL)
#define Megabytes(value) (Kilobytes(value) * 1024LL)
#define Gigabytes(value) (Megabytes(value) * 1024LL)
#define Terabytes(value) (Gigabytes(value) * 1024LL)

#define Minimum(a, b) ((a < b) ? (a) : (b))
#define Maximum(a, b) ((a > b) ? (a) : (b))

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef i32 bool32;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float r32;
typedef double r64;

typedef size_t memory_index;

struct thread_context
{
    int placeHolder;
};

#if APART_INTERNAL
struct debug_read_file_result
{
    u32 contentsSize;
    void* contents;
};

struct debug_open_file_result
{
    bool32 fileOpened;
    void* handle;
};

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context* thread, char* filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context* thread, void* memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(thread_context* thread, char* filename, u32 memorySize, void* memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#define DEBUG_PLATFORM_OPEN_FILE(name) debug_open_file_result name(char* filename)
typedef DEBUG_PLATFORM_OPEN_FILE(debug_platform_open_file);

#define DEBUG_PLATFORM_CLOSE_FILE(name) bool32 name(debug_open_file_result* openedFile)
typedef DEBUG_PLATFORM_CLOSE_FILE(debug_platform_close_file);

#define DEBUG_PLATFORM_WRITE_TO_FILE(name) bool32 name(debug_open_file_result* openedFile, void* memory, u32 memorySize)
typedef DEBUG_PLATFORM_WRITE_TO_FILE(debug_platform_write_to_file);

#endif

#define BITS_PER_SAMPLE 16
#define SAMPLES_PER_SEC 44100
#define AUDIO_BUFFER_SIZE_CYCLES 10
#define CYCLES_PER_SEC 220.0f


#define SAMPLES_PER_CYCLE (u32)(SAMPLES_PER_SEC / CYCLES_PER_SEC)
#define AUDIO_BUFFER_SIZE_SAMPLES  SAMPLES_PER_CYCLE * AUDIO_BUFFER_SIZE_CYCLES
#define AUDIO_BUFFER_SIZE_BYTES AUDIO_BUFFER_SIZE_SAMPLES * BITS_PER_SAMPLE / 8

#include "apart_intrinsics.h"
#include "apart_math.h"
#include "apart_tile.h"

struct game_offscreen_buffer
{
    void* memory;
    i32 height;
    i32 width;
    i32 pitch;
    i32 bytesPerPixel;
};

struct game_button_state
{
    int halfTransitionCount;
    bool32 endedDown;
    bool32 wasDown;
    bool32 started;
};

struct game_controller_input
{
    bool32 isAnalog;
    bool32 isConnected;

    r32 stickAverageX;
    r32 stickAverageY;

    bool32 started;

    bool32 scrollUpDown;
    bool32 scrollDownDown;    
    
    union
    {
	game_button_state buttons[15];
	struct
	{
	    game_button_state moveUp;
	    game_button_state moveDown;
	    game_button_state moveRight;
	    game_button_state moveLeft;

	    game_button_state actionUp;
	    game_button_state actionDown;
	    game_button_state actionLeft;
	    game_button_state actionRight;

	    game_button_state leftShoulder;
	    game_button_state rightShoulder;

	    game_button_state back;
	    game_button_state start;

	    game_button_state save;

	    game_button_state scrollUp;
	    game_button_state scrollDown;
	    
	    game_button_state terminator;
	};
    };



};

struct game_input
{
    game_button_state mouseButtons[5];
    i32 mouseX, mouseY, mouseZ;

    r32 dTime;
    game_controller_input controllers[5];
};

struct game_memory
{
    bool32 isInitialized;

    u64 permanentStorageSize;
    void* permanentStorage;

    u64 transientStorageSize;
    void* transientStorage;

    debug_platform_read_entire_file* DEBUGPlatformReadEntireFile;
    debug_platform_free_file_memory* DEBUGPlatformFreeFileMemory;
    debug_platform_write_entire_file* DEBUGPlatformWriteEntireFile;
    debug_platform_open_file* DEBUGPlatformOpenFile;
    debug_platform_close_file* DEBUGPlatformCloseFile;
    debug_platform_write_to_file* DEBUGPlatformWriteToFile;
};

struct game_sound_info
{
    bool32 bufferFilled;
    u8 buffer[AUDIO_BUFFER_SIZE_BYTES];
};

inline game_controller_input* GetController(game_input* input, int unsigned controllerIndex)
{
    game_controller_input* result = &input->controllers[controllerIndex];
    return(result);
}

#define GAME_UPDATE_AND_RENDER(name) void name(thread_context* thread, game_offscreen_buffer* buffer, game_memory* memory, game_input* input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GET_SOUND_DATA(name) void name(game_sound_info* soundInfo)
typedef GAME_GET_SOUND_DATA(game_get_sound_data);

struct memory_arena
{
    memory_index size;
    u8* base;
    memory_index used;
};

internal void
InitializeArena(memory_arena* arena, memory_index size, u8* base)
{
    arena->size = size;
    arena->base = base;
    arena->used = 0;
}

#define PushStruct(arena, type) (type*)PushSize_(arena, sizeof(type))
#define PushArray(arena, count, type) (type*)PushSize_(arena, (count)*sizeof(type))
void* PushSize_(memory_arena* arena, memory_index size)
{
    Assert((arena->used + size) <= arena->size);
    void* result = arena->base + arena->used;
    arena->used += size;

    return(result);
}


struct world
{
    tile_map* tileMap;
    tile_map tileMapItems;
};

struct loaded_bitmap
{
    i32 width;
    i32 height;
    u32* pixels;
};

struct background_bitmaps
{
    i32 alignX;
    i32 alignY;

    loaded_bitmap blueTile;
    tile_value tileValue;   
};

struct player_bitmap
{
    i32 alignX;
    i32 alignY;    

    loaded_bitmap bitmap;

};

enum e_player_bitmap_location : u8
{
    Stand,
    Launch_01,
    Launch_02,
    Launch_03,
    Air
};


struct player_anim_bitmap
{
    loaded_bitmap fullBitmap;

    e_player_bitmap_location playerBitmapLocation;

    i32 bitmapXLen;
    i32 bitmapYLen;
    
    loaded_bitmap playerAnims[5];

    i32 numOfSprites;
};

struct entity
{
    bool32 exists;

    tile_map_position p;
    u32 facingDirection;
    v2 dP;
    r32 width, height;

    bool32 canJump;
    i32 framesHeld;

    bool32 isInAir;

    loaded_bitmap entityBitmap;

};

inline bool32 IsEntityInAir(entity* entity)
{
    bool32 result = entity->dP.y != 0.0f;
    return(result);
}

inline bool32 EntityAirCheckForCollision(entity* entity)
{
    bool32 result = (entity->dP.y != 0.0f) && (entity->dP.x == 0.0f);
    return(result);    
}

struct ball_entity : public entity
{
    loaded_bitmap ballBitmap;
    entity* ballEntity;
    bool32 isActive;    
};

struct game_state
{
    i32 xOffset;
    i32 yOffset;

    r32 playerX;
    r32 playerY;

    bool32 started;

    bool32 cameraFollowingEntity;
    
    u32 cameraFollowingEntityIndex;
    tile_map_position cameraP;
    u32 cameraChunkY;
    u32 prevCameraChunkY;
    
    memory_arena worldArena;
    world* world;

    u32 playerIndexForController[ArrayCount(((game_input*)0)->controllers)];
    u32 entityCount;
    entity entities[256];

    background_bitmaps backgroundBitmaps[2];
    player_bitmap playerBitmaps[2];

    
    i32 currSelectedTileIndex;
    
    player_bitmap playerAnimations[4];
    player_bitmap* currentPlayerBitmap;

    u32 ballEntityIndex;
    
};

#define APART_H
#endif


