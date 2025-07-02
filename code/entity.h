#if !defined (ENTITY_H)


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

    r32 entitySpeed;
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
    v2 ddP;
};

struct entity_movement_calculations
{
    tile_map_position newP;
    tile_map_position oldP;
    v2 entityDelta;
};

struct test_tile_dimensions
{
    u32 minTileX;
    u32 maxTileX;
    u32 minTileY;
    u32 maxTileY;
};


#define ENTITY_H
#endif
