#include "entity.h"

inline entity*
GetEntity(game_state* gameState, u32 index)
{
    entity* entity = 0;
    if ((index > 0) && (index < ArrayCount(gameState->entities)))
    {
	entity = &gameState->entities[index];
    }
    return(entity);
}

internal u32
AddEntity(game_state* gameState)
{
    u32 entityIndex = gameState->entityCount++;
    Assert(gameState->entityCount < ArrayCount(gameState->entities));
    entity* result = &gameState->entities[entityIndex];
    return(entityIndex);
}

internal u32
AddEntity(game_state* gameState, entity* newEntity)
{
    u32 entityIndex = gameState->entityCount++;
    Assert(gameState->entityCount < ArrayCount(gameState->entities));
    gameState->entities[entityIndex] = *newEntity;
    return(entityIndex);
}

internal void
InitializePlayer(game_state* gameState, u32 entityIndex)
{
    entity* entity = GetEntity(gameState, entityIndex);
    entity->exists = true;
    entity->p.absTileX = 17/2;
    entity->p.absTileY = 3;
    entity->p.offset.x = 0.0f;
    entity->p.offset.y = 0.0f;
    entity->height = 0.5f;
    entity->width = 1.0f;
    entity->canJump = true;
    if (!GetEntity(gameState, gameState->cameraFollowingEntityIndex))
    {
	gameState->cameraFollowingEntityIndex = entityIndex;
    }
}

internal bool32
TestWall(r32 wallX, r32 relX, r32 relY, r32 playerDeltaX, r32 playerDeltaY, r32* tMin,
	 r32 minY, r32 maxY)
{
    bool32 hit = false;
    r32 epsilon = 0.0001f;

    if (playerDeltaX != 0.0f)
    {
	r32 tResult = (wallX - relX) / playerDeltaX;
	r32 y = relY + tResult * playerDeltaY;
	if ((tResult >= 0.0f) && (*tMin > tResult))
	{
	    if ((y >= minY) && (y <= maxY))
	    {
		*tMin = Maximum(0.0f, tResult - epsilon);
		hit = true;
	    }
	}
    }
    return(hit);
}

internal test_tile_dimensions
GetTestTileDimensions(tile_map_position oldP, tile_map_position newP, tile_map* tileMap, entity* entity)
{
    test_tile_dimensions result = {};
    result.minTileX = Minimum(oldP.absTileX, newP.absTileX);
    result.minTileY = Minimum(oldP.absTileY, newP.absTileY);
    result.maxTileX = Maximum(oldP.absTileX, newP.absTileX);
    result.maxTileY = Maximum(oldP.absTileY, newP.absTileY);

    u32 entityTileWidth = CeilReal32ToInt32(entity->width / tileMap->tileSideInMeters);
    u32 entityTileHeight = CeilReal32ToInt32(entity->height / tileMap->tileSideInMeters);

    result.minTileX -= entityTileWidth;
    result.minTileY -= entityTileHeight;
    result.maxTileX += entityTileWidth;
    result.maxTileY += entityTileHeight;

    return(result);
}

internal entity_movement_calculations
CalculateNewP(tile_map* tileMap, v2 ddP, entity* entity, r32 dt)
{

    entity_movement_calculations result = {};
    r32 ddPLength = LengthSq(ddP);
    if (ddPLength > 1.0f)
    {
	ddP *= 1.0f / SquareRoot(ddPLength);
    }

    r32 playerSpeed = 90.0f;
    ddP *= playerSpeed;
    
    ddP += -7.0*entity->dP;
    //ddP += -35.0f*entity->dP;

    result.oldP = entity->p;

    result.entityDelta = (0.5f * ddP * Square(dt) + entity->dP*dt);

    r32 gravity = 0.0f;
    entity->dP = ddP * dt + entity->dP;

    result.newP = Offset(tileMap, result.oldP, result.entityDelta);
    return(result);
}

//This is kind of a magic function but it is what it is
internal void
EntityCollisionRoutine(test_tile_dimensions dim, entity* entity, tile_map* tileMap, entity_movement_calculations* entityInfo, bool32 isFloor, r32* tMin, v2* wallNormal)
{
    Assert((dim.maxTileX - dim.minTileX) < 32);
    Assert((dim.maxTileY - dim.minTileY) < 32);

    
    u32 absTileZ = entity->p.absTileZ;

    for (u32 absTileY = dim.minTileY; absTileY <= dim.maxTileY; ++absTileY)
    {
	for (u32 absTileX = dim.minTileX; absTileX <= dim.maxTileX; ++absTileX)
	{
	    tile_map_position testTileP = CenteredTilePoint(absTileX, absTileY, absTileZ);
	    tile_value tileValue = GetTileValue(tileMap, testTileP);

	    if (!IsTileValueEmpty(tileValue))
	    {
		r32 diameterW = tileMap->tileSideInMeters + entity->width;
		r32 diameterH = tileMap->tileSideInMeters + entity->height;
		v2 minCorner = -0.5f*v2{diameterW, diameterH};
		v2 maxCorner = 0.5f*v2{diameterW, diameterH};

		tile_map_difference relOldPlayerP = Subtract(tileMap, &entity->p, &testTileP);
		v2 rel = relOldPlayerP.dXY;
		    
		if (TestWall(minCorner.x, rel.x, rel.y, entityInfo->entityDelta.x, entityInfo->entityDelta.y,
			     tMin, minCorner.y, maxCorner.y))
		{
		    *wallNormal = v2{-1, 0};
		}
		if (TestWall(maxCorner.x, rel.x, rel.y, entityInfo->entityDelta.x, entityInfo->entityDelta.y,
			     tMin, minCorner.y, maxCorner.y))
		{
		    *wallNormal = v2{1, 0};
		}
		if (TestWall(minCorner.y, rel.y, rel.x, entityInfo->entityDelta.y, entityInfo->entityDelta.x,
			     tMin, minCorner.x, maxCorner.x))
		{
		    *wallNormal = v2{0, -1};
		}
		if (TestWall(maxCorner.y, rel.y, rel.x, entityInfo->entityDelta.y, entityInfo->entityDelta.x,
			     tMin, minCorner.x, maxCorner.x))
		{
		    *wallNormal = v2{0, 1};
		    isFloor = true;
		}		    
	    }
	}
    }    
}

internal void
MoveBall(game_state* gameState, ball_entity* entity, r32 dt, v2 ddP)
{
    tile_map* tileMap = gameState->world->tileMap;
    entity_movement_calculations ballInfo = CalculateNewP(tileMap, ddP, entity, dt);

    test_tile_dimensions dim = GetTestTileDimensions(ballInfo.oldP, ballInfo.newP, tileMap, entity);

    r32 tRemaining = 1.0f;
    r32 tMin = 1.0f;

    bool32 isFloor = false;

    v2 vReflection = {};
    for(u32 iteration = 0; (iteration < 4) && (tRemaining > 0.0f); ++iteration)
    {
	tMin = 1.0f;
	v2 wallNormal = {};

	EntityCollisionRoutine(dim, entity, tileMap, &ballInfo, isFloor, &tMin, &wallNormal);

	entity->p = Offset(tileMap, entity->p, tMin*ballInfo.entityDelta);
	vReflection = ReflectionVector(entity->dP, wallNormal);
	
	entity->dP = vReflection;

	ballInfo.entityDelta = ballInfo.entityDelta - 1 * Inner(ballInfo.entityDelta, wallNormal)*wallNormal;
	tRemaining -= tMin;
    }
    entity->ddP = entity->dP;
}    

//NOTE: At some point it might be best to change the ddP value to an int just bc the precision is not needed and
//could be wasting space which is uneeded

internal void
MovePlayer(v2 ddP, entity* entity, game_state* gameState)
{
    tile_map* tileMap = gameState->world->tileMap;
    //Use tile_map_position which has absTile... for locations

    tile_map_position projectedLocation = entity->p;    

    projectedLocation.absTileZ = 0;
    if (ddP.x == 1.0f)
    {
	projectedLocation.absTileX++;
    }
    else if (ddP.x == -1.0f)
    {
	projectedLocation.absTileX--;
    }
    else if (ddP.y == 1.0f)
    {
	projectedLocation.absTileY++;
    }
    else if (ddP.y == -1.0f)
    {
	projectedLocation.absTileY--;
    }

    tile_map_position testTileP = CenteredTilePoint(projectedLocation.absTileX, projectedLocation.absTileY, projectedLocation.absTileZ);
    tile_value tileValue = GetTileValue(tileMap, testTileP);

    if (IsTileValueEmpty(tileValue))
    {
	entity->p = projectedLocation;
    }
}

///
internal void
MovePlayer(game_state* gameState, entity* entity, r32 dt, v2 ddP)
{
    tile_map* tileMap = gameState->world->tileMap;

    entity_movement_calculations playerInfo = CalculateNewP(tileMap, ddP, entity, dt);


    test_tile_dimensions dim = GetTestTileDimensions(playerInfo.oldP, playerInfo.newP, tileMap, entity);



    r32 tRemaining = 1.0f;
    r32 tMin = 1.0f;

    bool32 isFloor = false;
    

    for (u32 iteration = 0; (iteration < 4) && (tRemaining > 0.0f); ++iteration)
    {
	tMin = 1.0f;
	v2 wallNormal = {};

	EntityCollisionRoutine(dim, entity, tileMap, &playerInfo, isFloor, &tMin, &wallNormal);

	entity->p = Offset(tileMap, entity->p, tMin*playerInfo.entityDelta);
	entity->dP = entity->dP - 1*Inner(entity->dP, wallNormal)*wallNormal;
	//I need to be able to edit the bounciness value as well as the way the object bounces
	playerInfo.entityDelta = playerInfo.entityDelta - 1 * Inner(playerInfo.entityDelta, wallNormal)*wallNormal;
	tRemaining -= tMin;
    }


    //I don't need this for the rest of the entity types
    if ((entity->dP.x == 0) && (entity->dP.y == 0))
    {
	
    }
    else if (AbsoluteValue(entity->dP.x) > AbsoluteValue(entity->dP.y))
    {
	if (entity->dP.x > 0)
	{
	    entity->facingDirection = 0;
	}
	else
	{
	    entity->facingDirection = 2;
	}
    }
    else
    {
	if (entity->dP.x > 0)
	{
	    entity->facingDirection = 1;
	}
	else
	{
	    entity->facingDirection = 3;
	}
    }
}

