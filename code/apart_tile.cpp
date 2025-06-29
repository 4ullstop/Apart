#include "apart_tile.h"

inline void
SetTileValueUnchecked(tile_map* tileMap, tile_chunk* tileChunk, u32 tileX, u32 tileY, tile_value tileValue)
{
    //Directly set the value of the tile specified here
    Assert(tileChunk);
    Assert(tileX < tileMap->chunkDim);
    Assert(tileY < tileMap->chunkDim);

    tileChunk->tiles[tileY * tileMap->chunkDim + tileX] = tileValue;
}

inline u32
SetTileValue(tile_map* tileMap, tile_chunk* tileChunk,
	     u32 testTileX, u32 testTileY,
	     tile_value tileValue)
{
    u32 tileChunkValue = 0;

    if (tileChunk && tileChunk->tiles)
    {
	SetTileValueUnchecked(tileMap, tileChunk, testTileX, testTileY, tileValue);
    }
    return(tileChunkValue);
}

inline tile_chunk_position
GetChunkPositionFor(tile_map* tileMap, u32 absTileX, u32 absTileY, u32 absTileZ)
{
    tile_chunk_position result;
    //remember that we packed all of the information about the chunks
    //inside of one variable. This means the location of the chunks according to the world
    //are set in the higher bits and the position of the tile in the chunk is set in the lower bit
    result.tileChunkX = absTileX >> tileMap->chunkShift;
    result.tileChunkY = absTileY >> tileMap->chunkShift;
    result.tileChunkZ = absTileZ = absTileZ;
    result.relTileX = absTileX & tileMap->chunkMask;
    result.relTileY = absTileY & tileMap->chunkMask;
    return(result);
}

inline tile_chunk*
GetTileChunk(tile_map* tileMap, u32 tileChunkX, u32 tileChunkY, u32 tileChunkZ)
{
    tile_chunk* tileChunk  = 0;
    if ((tileChunkX >= 0) && (tileChunkX < tileMap->tileChunkCountX) &&
	(tileChunkY >= 0) && (tileChunkY < tileMap->tileChunkCountY) &&
	(tileChunkZ >= 0) && (tileChunkZ < tileMap->tileChunkCountZ))
    {
	//remeber the equation in this function is the getter for the rows and columns
	//this will be useful later when you setup file storage for these chunks
	tileChunk = &tileMap->tileChunks[
	    tileChunkZ * tileMap->tileChunkCountY * tileMap->tileChunkCountX +
	    tileChunkY * tileMap->tileChunkCountX +
	    tileChunkX];
    }
    return(tileChunk);
}

inline tile_value
GetTileChunkValueUnchecked(tile_map* tileMap, tile_chunk* tileChunk, u32 tileX, u32 tileY)
{
    Assert(tileChunk);
    Assert(tileX < tileMap->chunkDim);
    Assert(tileY < tileMap->chunkDim);
    
    tile_value result = tileChunk->tiles[tileY * tileMap->chunkDim + tileX];
    return(result);
}
				  
internal tile_value
GetTileValue(tile_map* tileMap, tile_chunk* tileChunk, u32 testTileX, u32 testTileY)
{
    tile_value tileChunkValue = {};

    if (tileChunk && tileChunk->tiles)
    {
	tileChunkValue = GetTileChunkValueUnchecked(tileMap, tileChunk, testTileX, testTileY);
    }

    return(tileChunkValue);
}

internal tile_value
GetTileValue(tile_map* tileMap, u32 absTileX, u32 absTileY, u32 absTileZ)
{
    tile_chunk_position chunkPos = GetChunkPositionFor(tileMap, absTileX, absTileY, absTileZ);
    tile_chunk* tileChunk = GetTileChunk(tileMap, chunkPos.tileChunkX, chunkPos.tileChunkY, chunkPos.tileChunkZ);
    tile_value tileChunkValue = GetTileValue(tileMap, tileChunk, chunkPos.relTileX, chunkPos.relTileY);

    return(tileChunkValue);
}

internal tile_value
GetTileValue(tile_map* tileMap, tile_map_position pos)
{
    tile_value tileChunkValue = GetTileValue(tileMap, pos.absTileX, pos.absTileY, pos.absTileZ);
    return(tileChunkValue);
}

internal void
SetTileValue(memory_arena* arena, tile_map* tileMap,
		u32 absTileX, u32 absTileY, u32 absTileZ,
		tile_value tileValue)
{
    //first we get the position of the chunk in the world
    tile_chunk_position chunkPos = GetChunkPositionFor(tileMap, absTileX, absTileY, absTileZ);
    tile_chunk* tileChunk = GetTileChunk(tileMap, chunkPos.tileChunkX, chunkPos.tileChunkY, chunkPos.tileChunkZ);

    Assert(tileChunk);

    //create the chunk if we can't find that it already exists
    if (!tileChunk->tiles)
    {
	u32 tileCount = tileMap->chunkDim * tileMap->chunkDim;
	tileChunk->tiles = PushArray(arena, tileCount, tile_value);
	tile_value blankTile = {};
	blankTile.collisionEnabled = false;
	blankTile.tileTexture = e_tile_texture::blueBackground;
	for (u32 tileIndex = 0; tileIndex < tileCount; ++tileIndex)
	{
	    tileChunk->tiles[tileIndex] = blankTile;
	}
    }
    SetTileValue(tileMap, tileChunk, chunkPos.relTileX, chunkPos.relTileY, tileValue);
}

inline void
RecanonicalizeCoord(tile_map* tileMap, u32* tile, r32* tileRel)
{
    i32 offset = RoundReal32ToInt32(*tileRel / tileMap->tileSideInMeters);

    *tile += offset;
    *tileRel -= offset * tileMap->tileSideInMeters;

    Assert(*tileRel >= -0.5001f*tileMap->tileSideInMeters);
    Assert(*tileRel <= 0.50001f*tileMap->tileSideInMeters);
}

inline tile_map_position
RecanonicalizePosition(tile_map* tileMap, tile_map_position pos)
{
    tile_map_position result = pos;

    RecanonicalizeCoord(tileMap, &result.absTileX, &result.offset.x);
    RecanonicalizeCoord(tileMap, &result.absTileY, &result.offset.y);

    return(result);
}

tile_map_difference
Subtract(tile_map* tileMap, tile_map_position* a, tile_map_position* b)
{
    tile_map_difference result;

    v2 dTileXY = {(r32)a->absTileX - (r32)b->absTileX,
	(r32)a->absTileY - (r32)b->absTileY};
    r32 dTileZ = (r32)a->absTileZ - (r32)b->absTileZ;

    result.dXY = tileMap->tileSideInMeters * dTileXY + (a->offset - b->offset);
    result.dZ = tileMap->tileSideInMeters * dTileZ;

    return(result);
}

inline tile_map_position
Offset(tile_map* tileMap, tile_map_position p, v2 offset)
{
    p.offset += offset;
    p = RecanonicalizePosition(tileMap, p);

    return(p);
}

inline tile_map_position
CenteredTilePoint(u32 absTileX, u32 absTileY, u32 absTileZ)
{
    tile_map_position result = {};

    result.absTileX = absTileX;
    result.absTileY = absTileY;
    result.absTileZ = absTileZ;

    return(result);
}

internal bool32
IsTileValueEmpty(tile_value tileValue)
{
    bool32 empty = !tileValue.collisionEnabled;

    return(empty);
}

internal tile_map_position
GetWorldLocationFromMouse(game_input* input, game_offscreen_buffer* buffer, tile_map* tileMap, game_state* gameState)
{
    i32 tileSideInPixels = 30;
    tile_map_position result = {};
    result.absTileX = input->mouseX;
    result.absTileY = buffer->height - input->mouseY;
    result.absTileZ = 0;

    result.absTileX /= tileSideInPixels;
    result.absTileY /= tileSideInPixels;
    result = RecanonicalizePosition(tileMap, result);
    result.absTileY += (gameState->cameraChunkY - 18);
    return(result);
}

internal void
SetTileValueFromMouse(game_input* input, game_offscreen_buffer* buffer, tile_map* tileMap, game_state* gameState, tile_value tileValue)
{
    i32 tileSideInPixels = 30;
    if ((input->mouseX <= buffer->width) && (input->mouseX >= 0) &&
	(input->mouseY <= buffer->height) && (input->mouseY >= 0))
    {
	tile_map_position mousePos = {};
#if 0	
	mousePos.absTileX = input->mouseX;
	mousePos.absTileY = buffer->height - input->mouseY;
	mousePos.absTileZ = 0;

	mousePos.absTileX /= tileSideInPixels;
	mousePos.absTileY /= tileSideInPixels;
	mousePos = RecanonicalizePosition(tileMap, mousePos);
	mousePos.absTileY += (gameState->cameraChunkY - 18);
#else
	mousePos = GetWorldLocationFromMouse(input, buffer, tileMap, gameState);
#endif	
	SetTileValue(&gameState->worldArena, tileMap, mousePos.absTileX, mousePos.absTileY, mousePos.absTileZ, tileValue);
    }    
}

